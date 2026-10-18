#pragma once

// Modulation sources: four ADSR envelopes per voice (Env 1 is always the amp
// envelope) and three global LFOs. All run at control rate (once per kCtl samples).

#include "Common.h"

namespace v4jb
{
struct EnvParams
{
    float a = 0.01f, d = 0.5f, s = 0.7f, r = 0.5f;  // seconds, seconds, level, seconds
    float vel = 0.5f;                               // velocity sensitivity 0..1
};

struct Env
{
    enum Stage { Idle, Attack, Decay, Release };
    Stage stage = Idle;
    float level = 0.f;

    void on() { stage = Attack; }  // attacks from the current level: a retrigger never clicks
    void off() { if (stage != Idle) stage = Release; }

    // Linear attack, exponential decay and release. Decay is ~99% there after d
    // seconds; release is down 60 dB after r seconds and idle at ~-100 dB.
    float step (const EnvParams& p, float dt)
    {
        switch (stage)
        {
            case Attack:
                level += dt / std::max (p.a, 1.e-3f);
                if (level >= 1.f) { level = 1.f; stage = Decay; }
                break;
            case Decay:
                level += (p.s - level) * (1.f - std::exp (-dt * 5.f / std::max (p.d, 1.e-3f)));
                break;
            case Release:
                level *= std::exp (-dt * 7.f / std::max (p.r, 1.e-3f));
                if (level < 1.e-5f) { level = 0.f; stage = Idle; }
                break;
            case Idle:
                break;
        }
        return level;
    }
};

inline constexpr int kNumLfoShapes = 5;
inline constexpr const char* kLfoShapeNames[kNumLfoShapes] = { "Sine", "Triangle", "Saw", "Square", "S&H" };
inline constexpr int kNumDivs = 8;
inline constexpr const char* kDivNames[kNumDivs] = { "1/16", "1/8", "1/4", "1/2", "1 bar", "2 bars", "4 bars", "8 bars" };
inline constexpr float kDivBeats[kNumDivs] = { 0.25f, 0.5f, 1.f, 2.f, 4.f, 8.f, 16.f, 32.f };

struct LfoParams
{
    float rateHz = 1.f;
    bool sync = false;  // lock to the beat clock instead of rateHz
    int div = 4;
    int shape = 0;
};

// Bipolar output -1..1.
struct Lfo
{
    float phase = 0.f, held = 0.f;

    float step (const LfoParams& p, float dt, double beats, Rng& rng)
    {
        const float prev = phase;
        if (p.sync)
        {
            const double cycles = beats / (double) kDivBeats[std::clamp (p.div, 0, kNumDivs - 1)];
            phase = (float) (cycles - std::floor (cycles));
        }
        else
        {
            phase += p.rateHz * dt;
            phase -= std::floor (phase);
        }
        if (phase < prev)
            held = rng.bi();  // new S&H value once per cycle

        switch (p.shape)
        {
            case 1:  return 1.f - 4.f * std::fabs (phase - 0.5f);
            case 2:  return 2.f * phase - 1.f;
            case 3:  return phase < 0.5f ? 1.f : -1.f;
            case 4:  return held;
            default: return std::sin (kTwoPi * phase);
        }
    }
};
} // namespace v4jb
