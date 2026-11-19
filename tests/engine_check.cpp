// `just check`: the additive engine renders, stays bounded, fades out, and its
// building blocks (spacing, modules, morph, wave, voice stealing) do what they say. No JUCE.
#include "dsp/Engine.h"
#include "dsp/Fx.h"
#include "dsp/Morph.h"
#include "dsp/Reverb.h"

#include <algorithm>
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

    // Formant A (800 Hz F1) on note 48: partial 6 (~785 Hz) louder than partial 1 (~261 Hz).
    {
        EngineParams f = quickParams();
        f.spec.modDepth = 0.f;
        f.spec.noiseMix = 0.f;
        f.spec.tilt = 0.f;
        f.slot[0].mod.type = (int) ModuleType::Formant;
        f.slot[0].mod.main = 0.f;    // A
        f.slot[0].mod.p[0] = 0.5f;   // no shift
        f.slot[0].mod.p[1] = 0.8f;   // narrow
        f.slot[0].mod.p[2] = 1.f;    // mix
        f.slot[0].mod.p[3] = 0.5f;
        e.setParams (f, kNeutral);
        render (e, sr, 0.05);
        CHECK (std::fabs (e.voiceAmp (0, 5)) > 3.f * std::fabs (e.voiceAmp (0, 1)));
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

        Morph path;
        path.numNodes.store (2);
        path.nodeX[0].store (0.f); path.nodeY[0].store (0.f); path.nodeLen[0].store (2);
        path.nodeX[1].store (1.f); path.nodeY[1].store (1.f); path.nodeLen[1].store (2);
        path.advance (kPathFree, 0.5f, 0.5f, 1.f, 0, 0.0, 0.f, false);
        CHECK (path.outX.load() == 0.f && path.outY.load() == 0.f);
        path.advance (kPathFree, 0.5f, 0.5f, 1.f, 0, 0.0, 0.5f, false);
        CHECK (std::fabs (path.outX.load() - 0.5f) < 1.e-5f);
        CHECK (std::fabs (path.outY.load() - 0.5f) < 1.e-5f);

        Morph env;
        env.numNodes.store (2);
        env.nodeX[0].store (0.f); env.nodeY[0].store (0.2f);
        env.nodeX[1].store (1.f); env.nodeY[1].store (0.8f);
        env.advance (kXAxis, 0.f, 0.f, 1.f, 0, 0.0, 0.f, true);
        CHECK (env.outX.load() == 0.f);
        env.advance (kXAxis, 0.f, 0.f, 1.f, 0, 0.0, 0.5f, false);
        CHECK (std::fabs (env.outX.load() - 0.5f) < 1.e-5f);
        CHECK (std::fabs (env.outY.load() - 0.5f) < 1.e-5f);
    }

    // Slot 12 FDN: mix 0 is dry, an impulse rings and stays finite.
    {
        Reverb rv;
        rv.prepare (sr);
        std::vector<float> L (512, 0.f), R (512, 0.f);
        L[0] = R[0] = 1.f;
        rv.process (L.data(), R.data(), 512, 0.7f, 0.4f, 0.f);
        CHECK (L[0] == 1.f && R[0] == 1.f);
        for (int i = 1; i < 512; ++i)
            CHECK (L[i] == 0.f && R[i] == 0.f);

        rv.reset();
        std::fill (L.begin(), L.end(), 0.f);
        std::fill (R.begin(), R.end(), 0.f);
        L[0] = R[0] = 1.f;
        rv.process (L.data(), R.data(), 512, 0.7f, 0.4f, 1.f);
        float peak = 0.f;
        bool finite = true;
        for (int i = 0; i < 512; ++i)
        {
            finite = finite && std::isfinite (L[i]) && std::isfinite (R[i]);
            peak = std::fmax (peak, std::fmax (std::fabs (L[i]), std::fabs (R[i])));
        }
        CHECK (finite && peak > 0.05f && peak < 8.f);

        auto blockPeak = [&] ()
        {
            float p = 0.f;
            for (int i = 0; i < 512; ++i)
            {
                finite = finite && std::isfinite (L[i]) && std::isfinite (R[i]);
                p = std::fmax (p, std::fmax (std::fabs (L[i]), std::fabs (R[i])));
            }
            return p;
        };
        float early = 0.f, late = 0.f;
        for (int b = 0; b < 90; ++b)
        {
            std::fill (L.begin(), L.end(), 0.f);
            std::fill (R.begin(), R.end(), 0.f);
            rv.process (L.data(), R.data(), 512, 0.7f, 0.4f, 1.f);
            const float pk = blockPeak();
            if (b < 8)
                early = std::fmax (early, pk);
            if (b >= 80)
                late = std::fmax (late, pk);
        }
        CHECK (finite && early > 1.e-4f && late < early);

        rv.reset();
        L[0] = R[0] = 1.f;
        for (int i = 1; i < 512; ++i)
            L[i] = R[i] = 0.f;
        rv.process (L.data(), R.data(), 512, 0.7f, 0.4f, 1.f, 1.f);
        early = 0.f;
        late = 0.f;
        finite = true;
        for (int b = 0; b < 90; ++b)
        {
            std::fill (L.begin(), L.end(), 0.f);
            std::fill (R.begin(), R.end(), 0.f);
            rv.process (L.data(), R.data(), 512, 0.7f, 0.4f, 1.f, 1.f);
            const float pk = blockPeak();
            if (b < 8)
                early = std::fmax (early, pk);
            if (b >= 80)
                late = std::fmax (late, pk);
        }
        CHECK (finite && early > 1.e-4f && late < early);
    }

    // Limiter: a 2.0 spike comes down under the ceiling; delay ping-pong stays finite.
    {
        Limiter lim;
        lim.prepare (sr);
        std::vector<float> L (64, 0.f), R (64, 0.f);
        L[0] = R[0] = 2.f;
        lim.process (L.data(), R.data(), 64);
        float peak = 0.f;
        bool finite = true;
        for (int i = 0; i < 64; ++i)
        {
            finite = finite && std::isfinite (L[i]) && std::isfinite (R[i]);
            peak = std::fmax (peak, std::fmax (std::fabs (L[i]), std::fabs (R[i])));
        }
        CHECK (finite && peak <= Limiter::kThresh + 1.e-4f && L[0] < 2.f);

        DelayBuf d;
        d.prepare (48000);
        L.assign (256, 0.f);
        R.assign (256, 0.f);
        L[0] = 1.f;
        for (int i = 0; i < 256; ++i)
        {
            const float dl = d.pop (0, 48.f), dr = d.pop (1, 48.f);
            d.push (0, L[i] + dr * 0.4f);
            d.push (1, R[i] + dl * 0.4f);
            L[i] += 0.5f * dl;
            R[i] += 0.5f * dr;
        }
        finite = true;
        peak = 0.f;
        for (int i = 0; i < 256; ++i)
        {
            finite = finite && std::isfinite (L[i]) && std::isfinite (R[i]);
            peak = std::fmax (peak, std::fmax (std::fabs (L[i]), std::fabs (R[i])));
        }
        CHECK (finite && peak < 4.f && std::fabs (L[48]) > 0.2f);
    }

    std::printf ("rms chord %.3f  peak %.3f  | tail %.5f\n", chord.rms, chord.peak, tail.tailPeak);
    std::printf (failures == 0 ? "engine_check OK\n" : "engine_check: %d failed\n", failures);
    return failures == 0 ? 0 : 1;
}
