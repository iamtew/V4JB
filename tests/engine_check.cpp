// `just check`: the additive engine renders, stays bounded, fades out, and its
// building blocks (spacing, modules, morph, wave, voice stealing) do what they say. No JUCE.
#include "dsp/Engine.h"
#include "dsp/Morph.h"

#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

static int failures = 0;
#define CHECK(x) do { if (! (x)) { std::printf ("FAIL %s:%d  %s\n", __FILE__, __LINE__, #x); ++failures; } } while (0)

using namespace v4jb;

struct Stats
{
    bool finite = true;
    float peak = 0.f, rms = 0.f, tailPeak = 0.f;
};

static const float kNeutral[kNumMacros] = { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };

// Renders in 512-sample blocks like a host would. tailPeak = peak of the last 100 ms.
static Stats render (Engine& e, double sr, double seconds)
{
    const int total = (int) (sr * seconds), tailStart = total - (int) (sr * 0.1);
    std::vector<float> L (512), R (512);
    Stats st;
    double sum = 0.0;
    for (int done = 0; done < total; done += 512)
    {
        const int n = total - done < 512 ? total - done : 512;
        e.render (L.data(), R.data(), n);
        for (int i = 0; i < n; ++i)
        {
            st.finite = st.finite && std::isfinite (L[i]) && std::isfinite (R[i]);
            const float a = std::fmax (std::fabs (L[i]), std::fabs (R[i]));
            st.peak = std::fmax (st.peak, a);
            if (done + i >= tailStart)
                st.tailPeak = std::fmax (st.tailPeak, a);
            sum += 0.5 * ((double) L[i] * L[i] + (double) R[i] * R[i]);
        }
    }
    st.rms = (float) std::sqrt (sum / total);
    return st;
}

static EngineParams quickParams()
{
    EngineParams p;
    p.env[0] = { 0.01f, 0.3f, 0.8f, 0.4f, 0.f };  // 0.4 s release: idle well inside 1.5 s
    return p;
}

int main()
{
    const double sr = 48000.0;
    // ~700 KB of voice state: heap, not stack.
    auto engine = std::make_unique<Engine>();
    Engine& e = *engine;
    e.prepare (sr);
    EngineParams p = quickParams();
    e.setParams (p, kNeutral);

    // No note held: silence.
    auto quiet = render (e, sr, 0.3);
    CHECK (quiet.finite && quiet.peak == 0.f);

    // A held chord: sound, finite, bounded.
    for (int note : { 48, 55, 60, 64 })
        e.noteOn (note, 0.8f);
    auto chord = render (e, sr, 1.5);
    CHECK (chord.finite && chord.rms > 0.01f && chord.peak <= 1.f);

    // Harmonic series: Depth 0 and no spectral modulation give exactly n * f0.
    {
        EngineParams h = quickParams();
        h.spec.modDepth = 0.f;
        h.spec.noiseMix = 0.f;
        e.setParams (h, kNeutral);
        render (e, sr, 0.05);
        const int v = 0;  // first voice holds note 48
        const float f0 = e.voiceFreq (v, 0);
        CHECK (std::fabs (f0 - 440.f * std::exp2 ((48.f - 69.f) / 12.f)) < 0.01f);
        bool harmonic = true;
        for (int k = 1; k < 64; ++k)
            harmonic = harmonic && std::fabs (e.voiceFreq (v, k) - f0 * (float) (k + 1)) < 1.e-3f * f0 * (float) (k + 1);
        CHECK (harmonic);

        // Nothing renders at or above Nyquist: note 48 = 130.8 Hz, so at most 0.48 * 48000 / 130.8 = 176 partials.
        CHECK (e.voicePartials (v) <= 176);

        // Economy caps the partial count, even for a low note.
        h.economy = true;
        e.setParams (h, kNeutral);
        e.noteOn (24, 0.8f);
        render (e, sr, 0.05);
        int lowVoice = -1;
        for (int i = 0; i < kMaxVoices; ++i)
            if (e.voiceNote (i) == 24)
                lowVoice = i;
        CHECK (lowVoice >= 0 && e.voicePartials (lowVoice) <= kEcoPartials);
        e.noteOff (24);
    }

    // Every spacing algorithm, at full depth with noise and subs: finite and bounded.
    for (int a = 0; a < kNumAlgos; ++a)
    {
        EngineParams s = quickParams();
        s.spec.algo = a;
        s.spec.depth = 1.f;
        s.spec.noiseMix = 0.5f;
        s.spec.sub1 = s.spec.sub2 = 1.f;
        e.setParams (s, kNeutral);
        auto st = render (e, sr, 0.1);
        CHECK (st.finite && st.peak <= 1.f);
    }

    // Every module, in every slot at once, knobs at both extremes: finite and bounded.
    WaveData wd;
    wd.numFrames = 4;
    wd.seconds = 1.f;
    wd.gain.assign ((size_t) wd.numFrames * WaveData::kBands, 0.05f);
    e.setWaves (&wd, &wd);
    for (int t = 1; t < kNumModuleTypes; ++t)
        for (float k : { 0.f, 1.f })
        {
            EngineParams m = quickParams();
            for (auto& sl : m.slot)
            {
                sl.mod.type = t;
                sl.mod.main = k;
                for (auto& q : sl.mod.p) q = k;
                sl.lfoSel = 1;
                sl.lfoAmt = 1.f;
            }
            e.setParams (m, kNeutral);
            auto st = render (e, sr, 0.05);
            if (! (st.finite && st.peak <= 1.f))
                std::printf ("  module %s at %.0f misbehaves\n", kModuleInfo[t].name, k);
            CHECK (st.finite && st.peak <= 1.f);
        }

    // Wave: one hot band (around 1 kHz) makes the partials there louder than elsewhere.
    {
        WaveData hot;
        hot.numFrames = 1;
        hot.seconds = 1.f;
        hot.gain.assign (WaveData::kBands, 0.01f);
        const int b = (int) WaveData::bandOf (1000.f);
        for (int i = b - 3; i <= b + 3; ++i)
            hot.gain[(size_t) i] = 1.f;
        e.setWaves (&hot, nullptr);
        EngineParams w = quickParams();
        w.spec.modDepth = 0.f;
        w.spec.noiseMix = 0.f;
        w.spec.tilt = 0.f;  // flat start so only the wave shapes it
        w.slot[0].mod.type = (int) ModuleType::Wave;
        w.slot[0].mod.main = 1.f;
        w.slot[0].mod.p[0] = 0.5f;  // no shift
        w.slot[0].mod.p[1] = 0.5f;  // contrast 1
        w.slot[0].mod.p[2] = 0.f;   // source A
        e.setParams (w, kNeutral);
        render (e, sr, 0.05);
        // Voice 0 plays note 48 (130.8 Hz): partial 8 is ~1046 Hz, partial 2 is ~262 Hz.
        CHECK (std::fabs (e.voiceAmp (0, 7)) > 10.f * std::fabs (e.voiceAmp (0, 1)));
        e.setWaves (nullptr, nullptr);
    }

    // Release: everything fades to near silence.
    e.setParams (quickParams(), kNeutral);
    e.allNotesOff();
    auto tail = render (e, sr, 1.5);
    CHECK (tail.finite && tail.tailPeak < 1.e-3f);

    // Voice stealing: 9 notes on 8 voices; the first (oldest) note is the one that goes.
    {
        auto fresh = std::make_unique<Engine>();
        fresh->prepare (sr);
        fresh->setParams (quickParams(), kNeutral);
        for (int i = 0; i < 9; ++i)
        {
            fresh->noteOn (40 + i, 0.8f);
            render (*fresh, sr, 0.01);
        }
        bool oldestGone = true, newestThere = false;
        for (int i = 0; i < kMaxVoices; ++i)
        {
            oldestGone = oldestGone && fresh->voiceNote (i) != 40;
            newestThere = newestThere || fresh->voiceNote (i) == 48;
        }
        CHECK (oldestGone && newestThere);
    }

    // Morph: each corner returns its own snapshot, the centre returns the average.
    {
        float c[4][kNumMacros];
        for (int i = 0; i < 4; ++i)
            for (int m = 0; m < kNumMacros; ++m)
                c[i][m] = 0.1f * (float) i + 0.01f * (float) m;
        const float xy[4][2] = { { 0.f, 0.f }, { 1.f, 0.f }, { 0.f, 1.f }, { 1.f, 1.f } };
        float out[kNumMacros];
        for (int i = 0; i < 4; ++i)
        {
            Morph::bilerp (c, xy[i][0], xy[i][1], out);
            bool same = true;
            for (int m = 0; m < kNumMacros; ++m)
                same = same && out[m] == c[i][m];
            CHECK (same);
        }
        Morph::bilerp (c, 0.5f, 0.5f, out);
        bool avg = true;
        for (int m = 0; m < kNumMacros; ++m)
            avg = avg && std::fabs (out[m] - 0.25f * (c[0][m] + c[1][m] + c[2][m] + c[3][m])) < 1.e-6f;
        CHECK (avg);
    }

    std::printf ("rms chord %.3f  peak %.3f  | tail %.5f\n", chord.rms, chord.peak, tail.tailPeak);
    std::printf (failures == 0 ? "engine_check OK\n" : "engine_check: %d failed\n", failures);
    return failures == 0 ? 0 : 1;
}
