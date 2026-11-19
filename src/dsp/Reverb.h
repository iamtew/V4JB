#pragma once

#include "Common.h"

#include <vector>

namespace v4jb
{

// 8-line Householder FDN for slot 12. Size stretches the lines and the decay;
// Damp is a one-pole lowpass in the feedback. Shimmer is an octave-up delay
// fed back into the FDN (capped so it blooms then dies). Mix 0 is dry (same
// dry/wet split the old JUCE Freeverb used, so factory presets do not jump).
// Allocates in prepare only.
struct Reverb
{
    void prepare (double sampleRate)
    {
        sr = sampleRate > 1.0 ? sampleRate : 44100.0;
        // Longest base tap is 97 ms; size can stretch it a little under 0.22 s.
        capN = (int) (sr * 0.22) + 4;
        for (int i = 0; i < kN; ++i)
        {
            buf[i].assign ((size_t) capN, 0.f);
            w[i] = 0;
            lp[i] = 0.f;
        }
        shCap = (int) (sr * 0.09) + 8;
        for (int c = 0; c < 2; ++c)
        {
            shBuf[c].assign ((size_t) shCap, 0.f);
            shW[c] = 0;
            shOff[c] = (float) (shCap / 2);
            zSh[c] = 0.f;
        }
    }

    void reset()
    {
        for (int i = 0; i < kN; ++i)
        {
            std::fill (buf[i].begin(), buf[i].end(), 0.f);
            w[i] = 0;
            lp[i] = 0.f;
        }
        for (int c = 0; c < 2; ++c)
        {
            std::fill (shBuf[c].begin(), shBuf[c].end(), 0.f);
            shW[c] = 0;
            shOff[c] = (float) (shCap / 2);
            zSh[c] = 0.f;
        }
    }

    void process (float* L, float* R, int n, float size, float damp, float mix, float shimmer = 0.f) noexcept
    {
        if (n <= 0 || capN <= 2)
            return;
        size = clamp01 (size);
        damp = clamp01 (damp);
        mix = clamp01 (mix);
        const float shAmt = clamp01 (shimmer) * 0.45f;  // cap loop gain so the bloom decays
        const float scale = 0.38f + 0.62f * size;
        const float rt60 = 0.4f + 7.6f * size;
        const float lpCoef = 0.08f + 0.84f * (1.f - damp);
        const float dry = 1.f - 0.5f * mix;
        const float maxD = (float) capN - 2.f;

        for (int s = 0; s < n; ++s)
        {
            float y[kN];
            float sum = 0.f;
            for (int i = 0; i < kN; ++i)
            {
                y[i] = read (i, std::clamp (kMs[i] * 0.001f * scale * (float) sr, 1.f, maxD));
                sum += y[i];
            }
            const float house = sum * (2.f / (float) kN);
            const float inL = L[s], inR = R[s];
            float outL = 0.f, outR = 0.f;
            for (int i = 0; i < kN; ++i)
            {
                const float delaySec = kMs[i] * 0.001f * scale;
                const float g = std::pow (0.001f, delaySec / rt60);
                const float inj = ((i & 1) != 0 ? inR : inL) * 0.25f
                                + ((i & 1) != 0 ? zSh[1] : zSh[0]) * shAmt;
                lp[i] += lpCoef * ((y[i] - house) * g - lp[i]);
                write (i, inj + lp[i]);
                if ((i & 1) != 0)
                    outR += y[i];
                else
                    outL += y[i];
            }
            const float wetL = outL * 0.25f, wetR = outR * 0.25f;
            L[s] = dry * inL + mix * wetL;
            R[s] = dry * inR + mix * wetR;
            if (shAmt > 0.f && shCap > 4)
            {
                // Read head runs at 2x write speed: octave up. Grain is the character.
                const float shLp = 0.15f + 0.7f * (1.f - damp);
                zSh[0] = shLp * zSh[0] + (1.f - shLp) * octaveUp (0, wetL);
                zSh[1] = shLp * zSh[1] + (1.f - shLp) * octaveUp (1, wetR);
            }
        }
    }

private:
    static constexpr int kN = 8;
    static constexpr float kMs[kN] = { 29.f, 37.f, 43.f, 53.f, 61.f, 73.f, 83.f, 97.f };

    float read (int i, float delaySamp) const noexcept
    {
        float rp = (float) w[i] - delaySamp;
        while (rp < 0.f)
            rp += (float) capN;
        const int i0 = (int) rp;
        const int i1 = i0 + 1 < capN ? i0 + 1 : 0;
        const float t = rp - (float) i0;
        return buf[i][(size_t) i0] * (1.f - t) + buf[i][(size_t) i1] * t;
    }

    void write (int i, float x) noexcept
    {
        buf[i][(size_t) w[i]] = x;
        if (++w[i] >= capN)
            w[i] = 0;
    }

    float octaveUp (int c, float x) noexcept
    {
        shBuf[c][(size_t) shW[c]] = x;
        if (++shW[c] >= shCap)
            shW[c] = 0;
        shOff[c] -= 1.f;
        if (shOff[c] < 1.f)
            shOff[c] += (float) shCap;
        float rp = (float) shW[c] - shOff[c];
        while (rp < 0.f)
            rp += (float) shCap;
        const int i0 = (int) rp;
        const int i1 = i0 + 1 < shCap ? i0 + 1 : 0;
        const float t = rp - (float) i0;
        return shBuf[c][(size_t) i0] * (1.f - t) + shBuf[c][(size_t) i1] * t;
    }

    std::vector<float> buf[kN];
    int w[kN] {};
    float lp[kN] {};
    int capN = 0;
    std::vector<float> shBuf[2];
    int shW[2] {};
    float shOff[2] {};
    float zSh[2] {};
    int shCap = 0;
    double sr = 48000.0;
};

} // namespace v4jb
