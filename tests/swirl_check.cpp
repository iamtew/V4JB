// `just check`: the Swirl texture voice renders sound, stays bounded, and fades out. No JUCE.
#include "dsp/Swirl.h"

#include <cmath>
#include <cstdio>
#include <vector>

static int failures = 0;
#define CHECK(x) do { if (! (x)) { std::printf ("FAIL %s:%d  %s\n", __FILE__, __LINE__, #x); ++failures; } } while (0)

struct Stats
{
    bool finite = true;
    float peak = 0.f, rms = 0.f, tailPeak = 0.f, sideRms = 0.f;
};

// Renders in 512-sample blocks like a host would. tailPeak = peak of the last 100 ms.
static Stats render (v4jb::Swirl& s, double sr, double seconds)
{
    const int total = (int) (sr * seconds), tailStart = total - (int) (sr * 0.1);
    std::vector<float> L (512), R (512);
    Stats st;
    double sum = 0.0, side = 0.0;
    for (int done = 0; done < total; done += 512)
    {
        const int n = total - done < 512 ? total - done : 512;
        s.render (L.data(), R.data(), n);
        for (int i = 0; i < n; ++i)
        {
            st.finite = st.finite && std::isfinite (L[i]) && std::isfinite (R[i]);
            const float a = std::fmax (std::fabs (L[i]), std::fabs (R[i]));
            st.peak = std::fmax (st.peak, a);
            if (done + i >= tailStart)
                st.tailPeak = std::fmax (st.tailPeak, a);
            sum += 0.5 * ((double) L[i] * L[i] + (double) R[i] * R[i]);
            side += 0.25 * (double) (L[i] - R[i]) * (L[i] - R[i]);
        }
    }
    st.rms = (float) std::sqrt (sum / total);
    st.sideRms = (float) std::sqrt (side / total);
    return st;
}

int main()
{
    const double sr = 48000.0;
    v4jb::Swirl s;
    s.prepare (sr);

    // No note held: silence.
    auto quiet = render (s, sr, 0.5);
    CHECK (quiet.finite && quiet.peak == 0.f);

    // Fog: default macros, one note held.
    s.noteOn (48);
    auto fog = render (s, sr, 3.0);
    CHECK (fog.finite);
    CHECK (fog.rms > 0.01f);
    CHECK (fog.peak <= 1.f);

    // Everything cranked: whirlwind, pops, acid blurts. Must stay finite and limited.
    s.setMacros ({ 1.f, 1.f, 1.f, 1.f });
    auto chaos = render (s, sr, 5.0);
    CHECK (chaos.finite);
    CHECK (chaos.rms > 0.01f);
    CHECK (chaos.peak <= 1.f);
    CHECK (chaos.sideRms > 0.01f);  // the whirl actually moves it between L and R

    s.setMacros ({ 1.f, 1.f, 1.f, 0.f });  // all saw, max acid
    auto acid = render (s, sr, 5.0);
    CHECK (acid.finite && acid.rms > 0.01f && acid.peak <= 1.f);

    // Release: the fog fades to near silence within the 3 s tail the plugin reports.
    s.noteOff (48);
    auto tail = render (s, sr, 3.0);
    CHECK (tail.finite);
    CHECK (tail.tailPeak < 1.e-3f);

    std::printf ("rms  fog %.3f  chaos %.3f  acid %.3f  | chaos peak %.3f  tail %.5f\n",
                 fog.rms, chaos.rms, acid.rms, chaos.peak, tail.tailPeak);
    std::printf (failures == 0 ? "swirl_check OK\n" : "swirl_check: %d failed\n", failures);
    return failures == 0 ? 0 : 1;
}
