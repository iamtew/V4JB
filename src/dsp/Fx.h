#pragma once

// Slot 12 time-domain FX, JUCE-free. Drive oversampling stays in the processor.
// Allocates in prepare only.

#include "Common.h"

#include <vector>

namespace v4jb
{

inline float hermite4 (float ym1, float y0, float y1, float y2, float t)
{
    const float c1 = 0.5f * (y1 - ym1);
    const float c2 = ym1 - 2.5f * y0 + 2.f * y1 - 0.5f * y2;
    const float c3 = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);
    return ((c3 * t + c2) * t + c1) * t + y0;
}

// Stereo circular buffer, hermite reads. cap is the wrap length.
struct DelayBuf
{
    void prepare (int samples)
    {
        cap = std::max (samples, 8);
        for (int c = 0; c < 2; ++c)
        {
            buf[c].assign ((size_t) cap, 0.f);
            w[c] = 0;
        }
    }

    void reset()
    {
        for (int c = 0; c < 2; ++c)
        {
            std::fill (buf[c].begin(), buf[c].end(), 0.f);
            w[c] = 0;
        }
    }

    int maxDelay() const { return cap - 4; }

    float pop (int c, float delaySamp) const noexcept
    {
        delaySamp = std::clamp (delaySamp, 1.f, (float) maxDelay());
        float rp = (float) w[c] - delaySamp;
        const float capf = (float) cap;
        while (rp < 0.f)
            rp += capf;
        const int i0 = (int) rp;
        const float t = rp - (float) i0;
        auto at = [&] (int i) -> float
        {
            i %= cap;
            if (i < 0)
                i += cap;
            return buf[c][(size_t) i];
        };
        return hermite4 (at (i0 - 1), at (i0), at (i0 + 1), at (i0 + 2), t);
    }

    void push (int c, float x) noexcept
    {
        buf[c][(size_t) w[c]] = x;
        if (++w[c] >= cap)
            w[c] = 0;
    }

    std::vector<float> buf[2];
    int w[2] {};
    int cap = 0;
};

// Two-tap stereo chorus. Centre delay matches the old JUCE range (7..17 ms).
struct Chorus
{
    void prepare (double sampleRate)
    {
        sr = sampleRate > 1.0 ? sampleRate : 44100.0;
        d.prepare ((int) (sr * 0.05) + 8);
        reset();
    }

    void reset()
    {
        d.reset();
        phase = 0.f;
        z[0] = z[1] = 0.f;
    }

    void process (float* L, float* R, int n, float rateHz, float depth, float mix) noexcept
    {
        mix = clamp01 (mix);
        depth = clamp01 (depth);
        rateHz = std::max (0.01f, rateHz);
        const float centre = (7.f + depth * 10.f) * 0.001f * (float) sr;
        const float swing = (0.8f + depth * 3.2f) * 0.001f * (float) sr;
        const float dph = kTwoPi * rateHz / (float) sr;
        const float fb = 0.2f;
        for (int i = 0; i < n; ++i)
        {
            phase += dph;
            if (phase > kTwoPi)
                phase -= kTwoPi;
            const float lfoL = std::sin (phase);
            const float lfoR = std::sin (phase + 1.5707963f);
            const float wL = d.pop (0, centre + swing * lfoL);
            const float wR = d.pop (1, centre + swing * lfoR);
            d.push (0, L[i] + z[0] * fb);
            d.push (1, R[i] + z[1] * fb);
            z[0] = wL;
            z[1] = wR;
            L[i] += mix * (wL - L[i]);
            R[i] += mix * (wR - R[i]);
        }
    }

    DelayBuf d;
    float phase = 0.f;
    float z[2] {};
    double sr = 48000.0;
};

// Six first-order allpasses, LFO around 800 Hz, feedback 0.5.
struct Phaser
{
    void prepare (double sampleRate)
    {
        sr = sampleRate > 1.0 ? sampleRate : 44100.0;
        reset();
    }

    void reset()
    {
        phase = 0.f;
        for (int c = 0; c < 2; ++c)
        {
            fb[c] = 0.f;
            for (int s = 0; s < kStages; ++s)
                z[c][s] = 0.f;
        }
    }

    void process (float* L, float* R, int n, float rateHz, float depth, float mix) noexcept
    {
        mix = clamp01 (mix);
        depth = clamp01 (depth);
        rateHz = std::max (0.01f, rateHz);
        const float dph = kTwoPi * rateHz / (float) sr;
        const float ny = (float) sr * 0.45f;
        float* ch[2] = { L, R };
        for (int i = 0; i < n; ++i)
        {
            phase += dph;
            if (phase > kTwoPi)
                phase -= kTwoPi;
            const float lfo = 0.5f + 0.5f * std::sin (phase);
            const float fc = std::clamp (800.f * std::exp2 ((lfo - 0.5f) * depth * 3.f), 40.f, ny);
            const float t = std::tan (fc * (kTwoPi * 0.5f) / (float) sr);
            const float a = (1.f - t) / (1.f + t);
            for (int c = 0; c < 2; ++c)
            {
                float x = ch[c][i] + fb[c] * 0.5f;
                for (int s = 0; s < kStages; ++s)
                {
                    const float y = a * x + z[c][s];
                    z[c][s] = x - a * y;
                    x = y;
                }
                fb[c] = x;
                ch[c][i] += mix * (x - ch[c][i]);
            }
        }
    }

    static constexpr int kStages = 6;
    float z[2][kStages] {};
    float fb[2] {};
    float phase = 0.f;
    double sr = 48000.0;
};

// Always-on safety limiter: instant attack, 80 ms release, -0.5 dB ceiling.
struct Limiter
{
    void prepare (double sampleRate)
    {
        const float sr = (float) (sampleRate > 1.0 ? sampleRate : 44100.0);
        rel = std::exp (-1.f / (0.08f * sr));
        env = 0.f;
    }

    void reset() { env = 0.f; }

    void process (float* L, float* R, int n) noexcept
    {
        for (int i = 0; i < n; ++i)
        {
            const float p = std::max (std::fabs (L[i]), std::fabs (R[i]));
            env = p > env ? p : rel * env + (1.f - rel) * p;
            const float g = env > kThresh ? kThresh / env : 1.f;
            L[i] *= g;
            R[i] *= g;
        }
    }

    static constexpr float kThresh = 0.9440609f;  // 10^(-0.5/20)
    float env = 0.f;
    float rel = 0.f;
};

} // namespace v4jb
