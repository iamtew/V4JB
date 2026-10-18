#pragma once

// The sine bank: one voice's partials, summed sample by sample.
//
// Each partial is a complex rotator (re, im) stepped by (cos w, sin w): four
// multiplies per sample instead of a sin() call. Struct-of-arrays so the inner
// loop walks contiguous memory. Frequencies and target amplitudes arrive once per
// control block (kCtl samples); amplitudes ramp linearly across the block so
// spectral changes never click.
//
// ponytail: plain scalar loop. Measured cost is roughly 3-4 cycles per partial
// per sample; at 8 voices x 512 partials that is under a fifth of one core.
// Hand SIMD only if a profiler says so.

#include "Common.h"

namespace v4jb
{
struct PartialBank
{
    float re[kBankSize];
    float im[kBankSize];
    float cr[kBankSize];
    float ci[kBankSize];
    float amp[kBankSize];
    float step[kBankSize];
    float tgt[kBankSize];
    int hi = 0;  // partials [0, hi) are rendered, plus the two subs

    // Fixed scattered start phases: all-zero phases stack into a spiky peak,
    // scattered ones keep the crest factor low for big pads.
    void reset()
    {
        Rng r;
        for (int k = 0; k < kBankSize; ++k)
        {
            const float ph = kTwoPi * r.next();
            re[k] = std::cos (ph);
            im[k] = std::sin (ph);
            cr[k] = 1.f;
            ci[k] = 0.f;
            amp[k] = step[k] = tgt[k] = 0.f;
        }
        hi = 0;
    }

    // freq in Hz and target amplitude for every bank index. Partials >= n (below
    // the subs) and anything near Nyquist are faded to zero.
    void setTargets (const float* freq, const float* target, int n, float invSr)
    {
        int top = 0;
        for (int k = 0; k < kBankSize; ++k)
        {
            amp[k] = tgt[k];  // last ramp landed here; resetting kills float drift
            float t = (k < n || k >= kMaxPartials) ? target[k] : 0.f;
            const float w = kTwoPi * freq[k] * invSr;
            if (! (w > 0.f && w < kTwoPi * 0.48f))
                t = 0.f;
            tgt[k] = t;
            step[k] = (t - amp[k]) * (1.f / (float) kCtl);
            if (t == 0.f && amp[k] == 0.f)
                continue;  // silent: skip the trig and the render

            cr[k] = std::cos (w);
            ci[k] = std::sin (w);
            // Rotators drift off the unit circle slowly; one Newton step per block pulls them back.
            const float g = 1.5f - 0.5f * (re[k] * re[k] + im[k] * im[k]);
            re[k] *= g;
            im[k] *= g;
            if (k < kMaxPartials)
                top = k + 1;
        }
        hi = top;
    }

    // Adds n samples (n <= kCtl) into L and R. spread pans odd partials left and
    // even partials right (0 = mono); the fundamental and subs stay centred.
    void render (float* L, float* R, int n, float spread)
    {
        for (int s = 0; s < n; ++s)
        {
            float c = 0.f, a = 0.f, b = 0.f;
            if (hi > 0)
                c += tick (0);
            int k = 1;
            for (; k + 1 < hi; k += 2)
            {
                a += tick (k);
                b += tick (k + 1);
            }
            if (k < hi)
                a += tick (k);
            if (amp[kSub1] != 0.f || step[kSub1] != 0.f) c += tick (kSub1);
            if (amp[kSub2] != 0.f || step[kSub2] != 0.f) c += tick (kSub2);

            // k odd = partials 2, 4, 6 (even harmonics) go right; k even = 3, 5, 7 go left.
            L[s] += c + b * (1.f + spread) + a * (1.f - spread);
            R[s] += c + b * (1.f - spread) + a * (1.f + spread);
        }
    }

private:
    float tick (int k)
    {
        const float nr = re[k] * cr[k] - im[k] * ci[k];
        const float ni = re[k] * ci[k] + im[k] * cr[k];
        re[k] = nr;
        im[k] = ni;
        amp[k] += step[k];
        return amp[k] * ni;
    }
};
} // namespace v4jb
