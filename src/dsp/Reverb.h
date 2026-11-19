#pragma once

#include "Common.h"

#include <vector>

namespace v4jb
{

// 8-line Householder FDN for slot 12. Size stretches the lines and the decay;
// Damp is a one-pole lowpass in the feedback. Mix 0 is dry (same dry/wet split
// the old JUCE Freeverb used, so factory presets do not jump). Allocates in
// prepare only.
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
    }

    void reset()
    {
        for (int i = 0; i < kN; ++i)
        {
            std::fill (buf[i].begin(), buf[i].end(), 0.f);
            w[i] = 0;
            lp[i] = 0.f;
        }
    }

    void process (float* L, float* R, int n, float size, float damp, float mix) noexcept
    {
        if (n <= 0 || capN <= 2)
            return;
        size = clamp01 (size);
        damp = clamp01 (damp);
        mix = clamp01 (mix);
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
                const float inj = ((i & 1) != 0 ? inR : inL) * 0.25f;
                lp[i] += lpCoef * ((y[i] - house) * g - lp[i]);
                write (i, inj + lp[i]);
                if ((i & 1) != 0)
                    outR += y[i];
                else
                    outL += y[i];
            }
            L[s] = dry * inL + mix * outL * 0.25f;
            R[s] = dry * inR + mix * outR * 0.25f;
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

    std::vector<float> buf[kN];
    int w[kN] {};
    float lp[kN] {};
    int capN = 0;
    double sr = 48000.0;
};

} // namespace v4jb
