#pragma once

// Shared bits for the additive engine: sizes, the random generator, small math.
// Everything under src/dsp is header-only and JUCE-free so tests/engine_check.cpp
// builds with plain cl. Real-time rule: nothing here allocates.

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace v4jb
{
constexpr int kMaxPartials = 512;
constexpr int kEcoPartials = 128;                // Economy mode cap
constexpr int kBankSize    = kMaxPartials + 2;   // + the two sub-harmonic sines at the top
constexpr int kSub1        = kMaxPartials;       // 1 octave below the fundamental
constexpr int kSub2        = kMaxPartials + 1;   // 2 octaves below
constexpr int kMaxVoices   = 8;
constexpr int kNumSlots    = 10;                 // user module slots (UI calls them slot 2..11)
constexpr int kCtl         = 64;                 // samples per control block: spectra update this often
constexpr int kBars        = 64;                 // spectrum display = first 64 partials
constexpr int kNumStages   = kNumSlots + 1;      // spectral slot + 10 module slots, one tap each
constexpr int kNumMacros   = 11;
constexpr int kNumEnvs     = 4;
constexpr int kNumLfos     = 3;
constexpr float kTwoPi     = 6.2831853f;

// xorshift32: cheap and deterministic, so the check renders the same every run.
struct Rng
{
    uint32_t s = 0x9e3779b9u;
    float next()  // [0, 1)
    {
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        return (float) (s >> 8) * (1.f / 16777216.f);
    }
    float bi() { return 2.f * next() - 1.f; }  // [-1, 1)
};

inline float clamp01 (float x) { return std::clamp (x, 0.f, 1.f); }
inline float lerp (float a, float b, float t) { return a + (b - a) * t; }
inline float sq (float x) { return x * x; }

// 0..1 knob position onto a log range lo..hi (both > 0).
inline float logMap (float x, float lo, float hi) { return lo * std::pow (hi / lo, x); }

// Same mapping the filters use for "cutoff" style knobs: 0..1 -> 20 Hz..20 kHz.
inline float knobHz (float x) { return logMap (clamp01 (x), 20.f, 20000.f); }
} // namespace v4jb
