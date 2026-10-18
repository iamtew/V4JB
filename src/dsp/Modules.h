#pragma once

// The module pool for slots 2..11. A module never touches audio: it edits a
// voice's partial amplitudes (and sometimes frequencies) once per control
// block, and the sine bank turns the result into sound. Modules run in series,
// so order matters (a Lowpass before Random Drops is not the same as after).
//
// One switch instead of a class per module: every module is a handful of lines,
// shares one state struct, and the whole pool stays readable in one file.
// All knobs arrive as 0..1. Labels below are what the UI shows; "" = unused.

#include "Common.h"
#include "Wave.h"

namespace v4jb
{
enum class ModuleType : int
{
    Off, FiveSines, OddEven, Sync, Adder, DiscreteAdder, EveryNth,
    Lowpass, Highpass, Bandpass, Notch, MovingFilter, Decay, Hype,
    Modulator, Detuned, Blur, Noise, Octaver, SecondTone,
    Phrase, RandomDrops, Tail, Wave, DoubleWave,
    Clip, HarmonicAdd, RangeEnhance,
    Count
};
inline constexpr int kNumModuleTypes = (int) ModuleType::Count;

struct ModuleInfo
{
    const char* name;
    const char* labels[5];  // main knob, then p1..p4
};

inline constexpr ModuleInfo kModuleInfo[kNumModuleTypes] = {
    { "Off",            { "", "", "", "", "" } },
    { "5-Sines",        { "Mask", "Sine 2", "Sine 3", "Sine 4", "Sine 5" } },
    { "Odd/Even",       { "Balance", "Keep Fund.", "", "", "" } },
    { "Sync",           { "Sync", "Resonance", "Bright", "", "" } },
    { "Adder",          { "Center", "Width", "Amount", "", "" } },
    { "Discrete Adder", { "Step", "Amount", "Discrete", "Offset", "" } },
    { "Every Nth",      { "N", "Depth", "Offset", "", "" } },
    { "Lowpass",        { "Cutoff", "Slope", "Resonance", "Key Track", "" } },
    { "Highpass",       { "Cutoff", "Slope", "Resonance", "Key Track", "" } },
    { "Bandpass",       { "Center", "Width", "Resonance", "Key Track", "" } },
    { "Notch",          { "Center", "Width", "Depth", "Key Track", "" } },
    { "Moving Filter",  { "Position", "Density", "Rate", "Depth", "" } },
    { "Decay",          { "Rate", "Tilt", "Floor", "", "" } },
    { "Hype",           { "Amount", "Low Freq", "Low Gain", "High Freq", "High Gain" } },
    { "Modulator",      { "Depth", "Rate", "Period", "Sharp", "" } },
    { "Detuned",        { "Detune", "Style", "Beat", "", "" } },
    { "Blur",           { "Amount", "Speed", "", "", "" } },
    { "Noise",          { "Amount", "Color", "", "", "" } },
    { "Octaver",        { "Up 1 Oct", "Up 2 Oct", "Dry", "", "" } },
    { "Second Tone",    { "Interval", "Level", "Detune", "", "" } },
    { "Phrase",         { "Pattern", "Rate", "Depth", "Partials", "Smooth" } },
    { "Random Drops",   { "Density", "Decay", "Floor", "Range", "Low Bias" } },
    { "Tail",           { "Amount", "Time", "Damping", "", "" } },
    { "Wave",           { "Amount", "Shift", "Contrast", "Source B", "Freeze" } },
    { "Double Wave",    { "Amount", "Balance", "Shift", "Contrast", "Freeze" } },
    { "Clip",           { "Threshold", "Makeup", "", "", "" } },
    { "Harmonic Add",   { "Base", "Level 1", "Level 2", "Level 3", "Level 4" } },
    { "Range Enhance",  { "Center", "Width", "Amount", "", "" } },
};

// Second Tone intervals in semitones (first one is a detuned unison).
inline constexpr float kIntervals[8] = { 0.15f, 3.f, 4.f, 5.f, 7.f, 12.f, 19.f, 24.f };

// Phrase patterns: which of the low partials lights up on each of 8 steps,
// as a position 0..1 across the "Partials" range.
inline constexpr float kPhrase[8][8] = {
    { 0.f, 1.f / 7, 2.f / 7, 3.f / 7, 4.f / 7, 5.f / 7, 6.f / 7, 1.f },  // up
    { 1.f, 6.f / 7, 5.f / 7, 4.f / 7, 3.f / 7, 2.f / 7, 1.f / 7, 0.f },  // down
    { 0.f, 2.f / 7, 4.f / 7, 6.f / 7, 1.f, 5.f / 7, 3.f / 7, 1.f / 7 },  // up and down
    { 0.f, 0.6f, 0.2f, 1.f, 0.4f, 0.8f, 0.1f, 0.5f },                    // scattered
    { 0.f, 0.f, 0.5f, 0.f, 1.f, 0.f, 0.5f, 0.f },                        // pulse
    { 0.f, 1.f / 3, 2.f / 3, 1.f / 3, 0.f, 2.f / 3, 1.f, 2.f / 3 },      // chord
    { 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f },                          // trill
    { 0.f, 0.15f, 0.3f, 0.15f, 0.6f, 0.45f, 0.9f, 0.75f },               // climb
};

struct ModuleParams
{
    int type = 0;
    float main = 0.5f;
    float p[4] { 0.5f, 0.5f, 0.5f, 0.5f };
};

// Per voice, per slot. Each module uses what it needs; reset on note-on.
struct ModuleState
{
    float a[kMaxPartials] {};
    float b[kMaxPartials] {};
    float c[kMaxPartials] {};
    float phase = 0.f;

    void reset()
    {
        std::fill (std::begin (a), std::end (a), 0.f);
        std::fill (std::begin (b), std::end (b), 0.f);
        std::fill (std::begin (c), std::end (c), 0.f);
        phase = 0.f;
    }
};

struct ModuleContext
{
    float f0 = 110.f;        // voice fundamental, Hz
    float dt = 0.f;          // control period, seconds
    float t = 0.f;           // seconds since note-on
    double beats = 0.0;      // beat clock (host when playing, free-running otherwise)
    int n = 0;               // partials in use
    Rng* rng = nullptr;
    const WaveData* wave[2] { nullptr, nullptr };
    float wavePos[2] { 0.f, 0.f };    // current playback position in each file, 0..1
    float waveStart[2] { 0.f, 0.f };  // region start, used by Freeze
    float waveBalance = 0.5f;         // Double Wave: 0 = A, 1 = B
};

namespace detail
{
// Cheap stable hash 0..1 per partial index, for "random but fixed" choices.
inline float hash01 (int k)
{
    uint32_t x = (uint32_t) k * 2654435761u;
    x ^= x >> 15;
    x *= 2246822519u;
    x ^= x >> 13;
    return (float) (x & 0xffffff) / 16777216.f;
}

inline float keyTracked (float knob, float keyTrack, float f0)
{
    return knobHz (knob) * std::pow (f0 / 261.63f, keyTrack);  // key track 1 = cutoff follows the note
}

inline float wavePos (const ModuleContext& c, int w, bool freeze)
{
    return freeze ? c.waveStart[w] : c.wavePos[w];
}
} // namespace detail

inline void processModule (const ModuleParams& m, ModuleState& st, const ModuleContext& c, float* freq, float* amp)
{
    using detail::hash01;
    const int n = c.n;
    const float main = clamp01 (m.main);
    const float* p = m.p;

    switch ((ModuleType) m.type)
    {
        case ModuleType::FiveSines:
            // Mask 1 = only partials 1..5 survive; partials 2..5 take their slider level
            // (relative to the fundamental).
            for (int k = 1; k < n; ++k)
                amp[k] = k < 5 ? lerp (amp[k], amp[0] * p[k - 1], main) : amp[k] * (1.f - main);
            break;

        case ModuleType::OddEven:
        {
            // Balance 0 = odd harmonics only (square-ish), 0.5 = all, 1 = even only.
            const float evenG = std::min (1.f, 2.f * main), oddG = std::min (1.f, 2.f * (1.f - main));
            for (int k = 0; k < n; ++k)
            {
                if (k == 0 && p[0] > 0.5f)
                    continue;
                amp[k] *= ((k + 1) % 2 == 0) ? evenG : oddG;
            }
            break;
        }

        case ModuleType::Sync:
        {
            // Hard sync's signature: a formant-like hump at the slave pitch on top of
            // the master's harmonics. Sync sweeps the hump up through the partials.
            const float R = 1.f + main * 15.f, w = 0.6f + R * 0.12f;
            const float bright = (p[1] - 0.5f) * 1.5f;
            for (int k = 0; k < n; ++k)
            {
                const float nn = (float) (k + 1);
                amp[k] *= (1.f + p[0] * 6.f * std::exp (-sq ((nn - R) / w))) * std::pow (nn, bright);
            }
            break;
        }

        case ModuleType::Adder:
        {
            const float centre = 1.f + main * 63.f, w = 0.5f + p[0] * 16.f;
            for (int k = 0; k < n; ++k)
                amp[k] *= 1.f + p[1] * 5.f * std::exp (-sq (((float) (k + 1) - centre) / w));
            break;
        }

        case ModuleType::DiscreteAdder:
        {
            // Boost every Step-th partial from Offset, cut the rest. Discrete thins the
            // boosted set (every other one drops back) for sparse metallic spectra.
            const int stepN = 2 + (int) (main * 7.99f);
            const int off = (int) (p[2] * (float) (stepN - 1) + 0.5f);
            for (int k = 0; k < n; ++k)
            {
                const bool hit = k >= off && (k - off) % stepN == 0;
                if (hit)
                {
                    const bool thinned = ((k - off) / stepN) % 2 == 1;
                    amp[k] *= 1.f + p[0] * 4.f * (thinned ? 1.f - p[1] : 1.f);
                }
                else
                    amp[k] *= 1.f - p[0] * 0.95f;
            }
            break;
        }

        case ModuleType::EveryNth:
        {
            const int N = 2 + (int) (main * 14.99f);
            const int off = (int) (p[1] * (float) (N - 1) + 0.5f);
            for (int k = 0; k < n; ++k)
                if (! (k >= off && (k - off) % N == 0))
                    amp[k] *= 1.f - p[0];
            break;
        }

        case ModuleType::Lowpass:
        case ModuleType::Highpass:
        {
            // Magnitude of an n-th order Butterworth-ish slope (6..48 dB/oct) plus a
            // resonant bump at the cutoff.
            const float fc = detail::keyTracked (main, p[2], c.f0);
            const float order2 = 2.f * (1.f + p[0] * 7.f);
            const bool low = (ModuleType) m.type == ModuleType::Lowpass;
            for (int k = 0; k < n; ++k)
            {
                const float x = low ? freq[k] / fc : fc / freq[k];
                const float res = 1.f + p[1] * 6.f * std::exp (-sq (std::log2 (freq[k] / fc)) * 20.f);
                amp[k] *= res / std::sqrt (1.f + std::pow (x, order2));
            }
            break;
        }

        case ModuleType::Bandpass:
        {
            const float fc = detail::keyTracked (main, p[2], c.f0);
            const float wOct = 0.2f + p[0] * 4.f;
            for (int k = 0; k < n; ++k)
            {
                const float d = std::log2 (freq[k] / fc);
                amp[k] *= (1.f + p[1] * 4.f * std::exp (-d * d * 30.f)) / (1.f + sq (sq (d / wOct)));
            }
            break;
        }

        case ModuleType::Notch:
        {
            const float fc = detail::keyTracked (main, p[2], c.f0);
            const float wOct = 0.05f + p[0] * 2.f;
            for (int k = 0; k < n; ++k)
                amp[k] *= 1.f - p[1] * std::exp (-sq (std::log2 (freq[k] / fc) / wOct));
            break;
        }

        case ModuleType::MovingFilter:
        {
            // A comb of notches on a sqrt(frequency) scale, so the notches spread
            // further apart as frequency rises (a phaser does the opposite in Hz).
            st.phase += c.dt * logMap (p[1], 0.02f, 8.f);
            st.phase -= std::floor (st.phase);
            const float density = 0.5f + p[0] * 4.f;
            for (int k = 0; k < n; ++k)
            {
                const float x = std::sqrt (freq[k] / c.f0) * density + main + st.phase;
                amp[k] *= 1.f - p[2] * (0.5f + 0.5f * std::cos (kTwoPi * x));
            }
            break;
        }

        case ModuleType::Decay:
        {
            // Higher partials die faster, like a plucked string or struck bar.
            const float rate = logMap (main, 0.05f, 20.f) * 0.1f;
            const float expo = 0.3f + p[0] * 1.7f, floor = p[1] * 0.5f;
            for (int k = 0; k < n; ++k)
                amp[k] *= floor + (1.f - floor) * std::exp (-c.t * rate * std::pow ((float) (k + 1), expo));
            break;
        }

        case ModuleType::Hype:
        {
            // Two shelves, +-18 dB each at Amount 0.5 (up to +-36 at 1).
            const float scale = main * 2.f;
            const float fl = logMap (p[0], 40.f, 1000.f), fh = logMap (p[2], 1000.f, 16000.f);
            const float lowDb = (p[1] - 0.5f) * 36.f * scale, highDb = (p[3] - 0.5f) * 36.f * scale;
            for (int k = 0; k < n; ++k)
            {
                const float f = freq[k];
                const float db = lowDb / (1.f + sq (f / fl)) + highDb / (1.f + sq (fh / f));
                amp[k] *= std::exp2 (db * (1.f / 6.0206f));
            }
            break;
        }

        case ModuleType::Modulator:
        {
            // A travelling wave of loudness that rolls up through the partials.
            st.phase += c.dt * logMap (p[0], 0.02f, 10.f);
            st.phase -= std::floor (st.phase);
            const float period = 2.f + p[1] * 30.f, sharp = 1.f + p[2] * 8.f;
            for (int k = 0; k < n; ++k)
            {
                const float wv = 0.5f + 0.5f * std::cos (kTwoPi * ((float) (k + 1) / period - st.phase));
                amp[k] *= 1.f - main * (1.f - std::pow (wv, sharp));
            }
            break;
        }

        case ModuleType::Detuned:
        {
            // Unison emulation: partials detuned (style: alternating, random, or growing
            // with n) plus the slow beating you would hear between two detuned voices.
            const float cents = main * 30.f;
            const int style = std::min ((int) (p[0] * 3.f), 2);
            for (int k = 0; k < n; ++k)
            {
                float d = style == 0 ? (k % 2 ? 1.f : -1.f)
                        : style == 1 ? hash01 (k) * 2.f - 1.f
                                     : std::min (1.f, (float) (k + 1) / 32.f);
                const float ratio = std::exp2 (d * cents / 1200.f);
                freq[k] *= ratio;
                const float beatHz = std::min (std::fabs (freq[k] * (ratio - 1.f)) * 2.f, 20.f);
                amp[k] *= 1.f - p[1] * 0.5f * (1.f - std::cos (kTwoPi * beatHz * c.t + (float) k * 1.7f));
            }
            break;
        }

        case ModuleType::Blur:
        {
            // Every partial's level wanders randomly: fuzzy, out-of-focus spectra.
            const float speed = logMap (p[0], 0.2f, 30.f);
            const float jump = speed * c.dt * 4.f, glide = std::min (1.f, speed * c.dt * 3.f);
            for (int k = 0; k < n; ++k)
            {
                if (c.rng->next() < jump)
                    st.b[k] = c.rng->bi();
                st.a[k] += (st.b[k] - st.a[k]) * glide;
                amp[k] *= 1.f - main * (0.5f + 0.5f * st.a[k]);
            }
            break;
        }

        case ModuleType::Noise:
        {
            // Fresh signed random level per partial every block: grainy, hissy partials.
            const float colour = (p[0] - 0.5f) * 1.5f;
            for (int k = 0; k < n; ++k)
            {
                const float w = std::min (4.f, std::pow ((float) (k + 1), colour));
                amp[k] *= (1.f - main) + main * c.rng->bi() * 1.7f * w;
            }
            break;
        }

        case ModuleType::Octaver:
        {
            // Copies the spectrum up one and two octaves (partial n feeds 2n and 4n).
            std::copy (amp, amp + n, st.c);
            for (int k = 0; k < n; ++k)
            {
                const int nn = k + 1;
                float add = 0.f;
                if (nn % 2 == 0) add += main * st.c[nn / 2 - 1];
                if (nn % 4 == 0) add += p[0] * st.c[nn / 4 - 1];
                amp[k] = st.c[k] * p[1] * 2.f + add;
            }
            break;
        }

        case ModuleType::SecondTone:
        {
            // Splits the bank: even slots keep the first tone's partials, odd slots
            // become a second tone at the interval. Each tone gets half the partials.
            const float semis = kIntervals[std::min ((int) (main * 8.f), 7)] + (p[1] - 0.5f) * 0.5f;
            const float q = std::exp2 (semis / 12.f);
            std::copy (freq, freq + n, st.b);
            std::copy (amp, amp + n, st.c);
            for (int k = 0; k < n; ++k)
            {
                const int h = k / 2;
                freq[k] = k % 2 ? st.b[h] * q : st.b[h];
                amp[k] = k % 2 ? st.c[h] * p[0] * 2.f : st.c[h];
            }
            break;
        }

        case ModuleType::Phrase:
        {
            // Spectral arpeggiator over the lowest partials, locked to the beat clock.
            static constexpr float kStepsPerBeat[4] = { 1.f, 2.f, 4.f, 8.f };
            const int pattern = std::min ((int) (main * 8.f), 7);
            const float spb = kStepsPerBeat[std::min ((int) (p[0] * 4.f), 3)];
            const int step = (int) ((int64_t) std::floor (c.beats * spb) & 7);
            const int N = std::min (2 + (int) (p[2] * 14.f), n);
            const int lit = (int) std::lround (kPhrase[pattern][step] * (float) (N - 1));
            const float coef = 1.f - std::exp (-c.dt / logMap (p[3], 0.002f, 0.3f));
            for (int k = 0; k < N; ++k)
            {
                st.a[k] += ((k == lit ? 1.f : 1.f - p[1]) - st.a[k]) * coef;
                amp[k] *= st.a[k];
            }
            break;
        }

        case ModuleType::RandomDrops:
        {
            // Random partials flare up and fade: rain on a tin roof.
            const float rate = main * main * 60.f;
            const float expected = rate * c.dt;
            int count = (int) expected + (c.rng->next() < expected - std::floor (expected) ? 1 : 0);
            count = std::min (count, 8);
            const int range = std::max (1, (int) ((0.05f + 0.95f * p[2]) * (float) n));
            for (int i = 0; i < count; ++i)
            {
                const float u = std::pow (c.rng->next(), 1.f + p[3] * 3.f);  // Low Bias favours low partials
                st.a[std::min ((int) (u * (float) range), n - 1)] = 1.f;
            }
            const float decay = std::exp (-c.dt / logMap (p[0], 0.02f, 3.f));
            const float floor = p[1];
            for (int k = 0; k < range; ++k)
            {
                st.a[k] *= decay;
                amp[k] *= floor + (1.6f - floor) * st.a[k];
            }
            break;
        }

        case ModuleType::Tail:
        {
            // Each partial holds its peak and lets it ring down: a spectral reverb that
            // smooths over gaps left by rhythmic modules. Damping shortens high partials.
            const float T = logMap (p[0], 0.1f, 10.f);
            for (int k = 0; k < n; ++k)
            {
                const float tk = T / (1.f + p[1] * 8.f * (float) (k + 1) / (float) n);
                st.a[k] = std::max (std::fabs (amp[k]), st.a[k] * std::exp (-c.dt / tk));
                amp[k] = amp[k] * (1.f - main) + st.a[k] * main * (amp[k] < 0.f ? -1.f : 1.f);
            }
            break;
        }

        case ModuleType::Wave:
        {
            const int w = p[2] > 0.5f ? 1 : 0;
            const WaveData* wd = c.wave[w];
            if (wd == nullptr || wd->numFrames == 0)
                break;
            const float pos = detail::wavePos (c, w, p[3] > 0.5f);
            const float shift = std::exp2 ((p[0] - 0.5f) * 2.f), contrast = std::exp2 ((p[1] - 0.5f) * 3.f);
            for (int k = 0; k < n; ++k)
                amp[k] *= lerp (1.f, std::pow (std::max (wd->at (pos, freq[k] * shift), 1.e-4f), contrast), main);
            break;
        }

        case ModuleType::DoubleWave:
        {
            const WaveData* A = c.wave[0];
            const WaveData* B = c.wave[1];
            const bool hasA = A != nullptr && A->numFrames > 0, hasB = B != nullptr && B->numFrames > 0;
            if (! hasA && ! hasB)
                break;
            const bool freeze = p[3] > 0.5f;
            const float pa = detail::wavePos (c, 0, freeze), pb = detail::wavePos (c, 1, freeze);
            const float bal = clamp01 (c.waveBalance + (p[0] - 0.5f) * 2.f);
            const float shift = std::exp2 ((p[1] - 0.5f) * 2.f), contrast = std::exp2 ((p[2] - 0.5f) * 3.f);
            for (int k = 0; k < n; ++k)
            {
                const float f = freq[k] * shift;
                const float g = lerp (hasA ? A->at (pa, f) : 1.f, hasB ? B->at (pb, f) : 1.f, bal);
                amp[k] *= lerp (1.f, std::pow (std::max (g, 1.e-4f), contrast), main);
            }
            break;
        }

        case ModuleType::Clip:
        {
            // Flattens the spectrum: no partial may be louder than Threshold below the peak.
            float peak = 0.f;
            for (int k = 0; k < n; ++k)
                peak = std::max (peak, std::fabs (amp[k]));
            if (peak <= 0.f)
                break;
            const float thr = peak * std::exp2 (-main * 10.f);
            const float makeup = std::pow (peak / thr, p[0] * 0.5f);
            for (int k = 0; k < n; ++k)
                amp[k] = std::clamp (amp[k], -thr, thr) * makeup;
            break;
        }

        case ModuleType::HarmonicAdd:
        {
            // Adds four harmonics of a chosen base partial: base, 2x, 3x, 4x.
            const int base = 1 + (int) (main * 15.99f);
            for (int j = 1; j <= 4; ++j)
            {
                const int idx = base * j - 1;
                if (idx < n)
                    amp[idx] += sq (p[j - 1]);
            }
            break;
        }

        case ModuleType::RangeEnhance:
        {
            const float centre = std::exp2 (main * 9.f);  // partial 1..512
            const float wOct = 0.2f + p[0] * 4.f, db = (p[1] - 0.5f) * 48.f;
            for (int k = 0; k < n; ++k)
                amp[k] *= std::exp2 (db * std::exp (-sq (std::log2 ((float) (k + 1) / centre) / wOct)) / 6.0206f);
            break;
        }

        case ModuleType::Off:
        case ModuleType::Count:
            break;
    }
}
} // namespace v4jb
