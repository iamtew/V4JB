#pragma once

// Morph pad: four corner snapshots (A, B, C, D) of the 11 macros, blended by
// where the puck sits. The puck is dragged by hand (Manual), follows a drawn
// path of nodes (Path free-running or tempo-synced), or sweeps left to right
// like an envelope after each new note (X-Axis Env), reading its height off the path.
//
// The message thread edits corners and nodes, the audio thread reads them. All
// shared values are relaxed atomics: a half-updated corner for one block is
// inaudible, a lock in processBlock is not allowed.

#include "Common.h"

#include <atomic>

namespace v4jb
{
inline constexpr const char* kMacroNames[kNumMacros] = {
    "Character", "Complexity", "Tone", "Punch", "Length", "Time FX",
    "Depth", "Speed", "FX Mix", "Width", "User"
};
inline constexpr const char* kMacroGroups[kNumMacros] = {
    "Sound", "Sound", "Sound", "Dynamics", "Dynamics", "Dynamics",
    "Modulation", "Modulation", "Master", "Master", "User"
};

enum MorphMode { kManual, kPathFree, kPathSync, kXAxis };
inline constexpr const char* kMorphModeNames[4] = { "Manual", "Path", "Path Sync", "X-Axis Env" };
inline constexpr int kSnapSteps[5] = { 0, 4, 8, 16, 32 };
inline constexpr const char* kSnapNames[5] = { "Off", "4", "8", "16", "32" };

inline constexpr int kMaxNodes = 16;
inline constexpr int kNumSegLens = 6;
inline constexpr float kSegBars[kNumSegLens] = { 0.25f, 0.5f, 1.f, 2.f, 4.f, 8.f };
inline constexpr const char* kSegNames[kNumSegLens] = { "1/4 bar", "1/2 bar", "1 bar", "2 bars", "4 bars", "8 bars" };

class Morph
{
public:
    // Corner order: A bottom-left (0,0), B bottom-right (1,0), C top-left (0,1), D top-right (1,1).
    std::atomic<float> corner[4][kNumMacros];
    std::atomic<float> nodeX[kMaxNodes], nodeY[kMaxNodes];
    std::atomic<int> nodeLen[kMaxNodes];  // index into kSegBars: how long the segment from this node lasts
    std::atomic<int> numNodes { 0 };

    // Where the puck really is and the resulting macros. Audio writes, UI reads.
    std::atomic<float> outX { 0.5f }, outY { 0.5f };
    std::atomic<float> outMacro[kNumMacros];

    Morph()
    {
        for (auto& c : corner)
            for (auto& v : c)
                v.store (0.5f);
        for (int i = 0; i < kMaxNodes; ++i)
        {
            nodeX[i].store (0.5f);
            nodeY[i].store (0.5f);
            nodeLen[i].store (2);
        }
        for (auto& m : outMacro)
            m.store (0.5f);
    }

    static void bilerp (const float c[4][kNumMacros], float x, float y, float* out)
    {
        const float wa = (1.f - x) * (1.f - y), wb = x * (1.f - y), wc = (1.f - x) * y, wd = x * y;
        for (int m = 0; m < kNumMacros; ++m)
            out[m] = wa * c[0][m] + wb * c[1][m] + wc * c[2][m] + wd * c[3][m];
    }

    // Audio thread, once per block. unitSec = Time knob (seconds per bar in Path mode,
    // sweep length in X-Axis mode). beats = beat clock. noteStarted = a note began
    // while none were held (restarts the X-Axis sweep).
    void advance (int mode, float manX, float manY, float unitSec, int snapIdx, double beats,
                  float dt, bool noteStarted)
    {
        const int n = std::clamp (numNodes.load (std::memory_order_relaxed), 0, kMaxNodes);
        float x = manX, y = manY;

        if ((mode == kPathFree || mode == kPathSync) && n >= 2)
        {
            // Closed loop through the nodes. Lengths in bars; Free mode plays one bar per unitSec.
            float total = 0.f;
            for (int i = 0; i < n; ++i)
                total += kSegBars[std::clamp (nodeLen[i].load (std::memory_order_relaxed), 0, kNumSegLens - 1)];
            if (mode == kPathSync)
                pathPos = std::fmod (beats / 4.0, (double) total);
            else
                pathPos = std::fmod (pathPos + dt / std::max (unitSec, 0.01f), (double) total);

            float u = (float) pathPos;
            for (int i = 0; i < n; ++i)
            {
                const float len = kSegBars[std::clamp (nodeLen[i].load (std::memory_order_relaxed), 0, kNumSegLens - 1)];
                if (u <= len || i == n - 1)
                {
                    const int j = (i + 1) % n;
                    const float t = std::clamp (u / len, 0.f, 1.f);
                    x = lerp (nodeX[i].load (std::memory_order_relaxed), nodeX[j].load (std::memory_order_relaxed), t);
                    y = lerp (nodeY[i].load (std::memory_order_relaxed), nodeY[j].load (std::memory_order_relaxed), t);
                    break;
                }
                u -= len;
            }
        }
        else if (mode == kXAxis)
        {
            if (noteStarted)
                envT = 0.f;
            envT += dt;
            x = std::min (1.f, envT / std::max (unitSec, 0.01f));
            const int steps = kSnapSteps[std::clamp (snapIdx, 0, 4)];
            if (steps > 0)
                x = std::floor (x * (float) steps) / (float) steps;  // stepped, rhythmic sweep
            if (n > 0)
                y = pathHeightAt (x, n);
        }

        outX.store (clamp01 (x), std::memory_order_relaxed);
        outY.store (clamp01 (y), std::memory_order_relaxed);
    }

    // Macros at the current puck. Locked macros ignore the pad (sit at 0.5);
    // offsets ride on top either way, so the host can automate them.
    void macros (const float* offset, const bool* locked, float* out)
    {
        float c[4][kNumMacros];
        for (int i = 0; i < 4; ++i)
            for (int m = 0; m < kNumMacros; ++m)
                c[i][m] = corner[i][m].load (std::memory_order_relaxed);
        bilerp (c, outX.load (std::memory_order_relaxed), outY.load (std::memory_order_relaxed), out);
        for (int m = 0; m < kNumMacros; ++m)
        {
            out[m] = clamp01 ((locked[m] ? 0.5f : out[m]) + offset[m]);
            outMacro[m].store (out[m], std::memory_order_relaxed);
        }
    }

private:
    // The path read as a curve y(x): nodes sorted by x, linear between them.
    float pathHeightAt (float x, int n) const
    {
        float xs[kMaxNodes], ys[kMaxNodes];
        for (int i = 0; i < n; ++i)
        {
            xs[i] = nodeX[i].load (std::memory_order_relaxed);
            ys[i] = nodeY[i].load (std::memory_order_relaxed);
            for (int j = i; j > 0 && xs[j] < xs[j - 1]; --j)  // insertion sort, n <= 16
            {
                std::swap (xs[j], xs[j - 1]);
                std::swap (ys[j], ys[j - 1]);
            }
        }
        if (x <= xs[0]) return ys[0];
        for (int i = 1; i < n; ++i)
            if (x <= xs[i])
                return lerp (ys[i - 1], ys[i], (x - xs[i - 1]) / std::max (xs[i] - xs[i - 1], 1.e-6f));
        return ys[n - 1];
    }

    double pathPos = 0.0;
    float envT = 0.f;
};
} // namespace v4jb
