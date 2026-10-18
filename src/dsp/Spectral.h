#pragma once

// Slot 1, the fixed spectral stage. Builds a voice's starting spectrum before
// the module chain sees it:
//   - Spectral Distortion: where the partials sit. Harmonic series n * f0, or one
//     of 18 spacing algorithms, blended by Depth (in log-frequency, so Depth 0.5
//     is halfway in pitch, not in Hz).
//   - Tilt: starting amplitudes n^-tilt (1 = saw-like, 2 = soft, 0 = flat buzz).
//   - Spectral Modulation: every partial gets its own slow random pitch wander.
//   - Spectral Noise: each partial's amplitude is multiplied by its own
//     band-limited random signal, which smears it into a band of noise on the
//     same frequency. Mix crossfades sine partials to noise partials.
// The two sub-octave sines are added by the engine after the module chain.

#include "Common.h"

namespace v4jb
{
inline constexpr int kNumAlgos = 18;
inline constexpr const char* kAlgoNames[kNumAlgos] = {
    "Stiff String", "Stretch", "Compress", "Bar", "Membrane", "Primes",
    "Golden", "Octaves", "Fifths", "Root", "Log", "Fold",
    "Even Shift", "Odd Shift", "Scatter", "Bell", "Offset", "Pairs"
};

struct SpectralParams
{
    int algo = 0;
    float depth = 0.f;        // 0 = pure harmonic series, 1 = full algorithm
    float tilt = 1.2f;        // amplitude roll-off exponent
    float partials = 256.f;   // how many partials (1..512)
    float modDepth = 0.2f;    // spectral modulation, 0..1 -> 0..60 cents
    float modRate = 0.3f;     // 0..1 -> 0.05..20 Hz average wander rate
    float noiseMix = 0.1f;    // 0 = sines, 1 = noise partials
    float noiseWidth = 0.5f;  // noise bandwidth around each partial
    float noiseLow = 0.f;     // noise highpass, 0..1 -> 20 Hz..20 kHz
    float noiseHigh = 1.f;    // noise lowpass, same mapping
    float noiseDecay = 0.f;   // 0 = steady noise, else noise fades after note-on (breath, chiff)
    float sub1 = 0.f;         // sub 1 octave down, level
    float sub2 = 0.f;         // sub 2 octaves down, level
};

// Ratio tables, built once on the message thread (the constructor allocates nothing,
// but 36 KB of maths is not audio-thread work either).
struct SpectralTables
{
    float log2n[kMaxPartials];                 // log2(n)
    float logAlg[kNumAlgos][kMaxPartials];     // log2(algorithmRatio / n)

    SpectralTables()
    {
        // Primes for the "Primes" algorithm. The 512th prime is 3671.
        static constexpr int kSieve = 3700;
        bool composite[kSieve] = {};
        int primes[kMaxPartials] = {};
        int np = 0;
        for (int i = 2; i < kSieve && np < kMaxPartials; ++i)
        {
            if (composite[i])
                continue;
            primes[np++] = i;
            for (int j = i * 2; j < kSieve; j += i)
                composite[j] = true;
        }

        Rng scatter;
        for (int k = 0; k < kMaxPartials; ++k)
        {
            const float n = (float) (k + 1);
            log2n[k] = std::log2 (n);
            const float rnd = scatter.next();
            for (int a = 0; a < kNumAlgos; ++a)
                logAlg[a][k] = std::log2 (std::max (ratio (a, k + 1, primes, rnd), 0.01f) / n);
        }
    }

    // Partial n (1-based) as a multiple of f0 for algorithm a.
    static float ratio (int a, int ni, const int* primes, float rnd)
    {
        static constexpr float kMembrane[16] = { 1.f, 1.594f, 2.136f, 2.296f, 2.653f, 2.918f, 3.156f, 3.501f,
                                                 3.600f, 3.652f, 4.060f, 4.154f, 4.601f, 4.832f, 4.903f, 5.131f };
        static constexpr float kBell[12] = { 0.5f, 1.f, 1.2f, 1.5f, 2.f, 2.5f, 2.67f, 3.f, 4.f, 5.33f, 6.67f, 8.f };
        const float n = (float) ni;
        switch (a)
        {
            case 0:  return n * std::sqrt (1.f + 0.0005f * n * n);           // stiff string: piano-like stretch that grows with n
            case 1:  return std::pow (n, 1.1f);                              // stretch
            case 2:  return std::pow (n, 0.88f);                             // compress
            case 3:  return sq (2.f * n + 1.f) / 9.f;                        // free bar modes ~ (2n+1)^2
            case 4:  return ni <= 16 ? kMembrane[ni - 1] : 5.131f + 0.3f * (n - 16.f);  // drum head
            case 5:  return (float) primes[ni - 1] * 0.5f;                   // 1, 1.5, 2.5, 3.5, 5.5 ...
            case 6:  return 1.f + (n - 1.f) * 1.618034f;                     // golden-ratio spacing
            case 7:                                                          // octave stack, slight chorus
            {
                const float p = std::exp2 (std::floor (std::log2 (n)));
                return p * (1.f + 0.003f * (n - p));
            }
            case 8:                                                          // stacked fifths
            {
                const float p = std::pow (1.5f, std::floor (std::log (n) / std::log (1.5f)));
                return p * (1.f + 0.003f * (n - p));
            }
            case 9:  return std::sqrt (n * (n + 1.f) * 0.5f);                // dense low, harmonic-ish high
            case 10: return 1.f + 3.f * std::log (n);                        // log spacing: crowded, alien
            case 11: return ni <= 24 ? n : 1.f + std::fmod (n - 1.f, 24.f) + 0.13f * std::floor ((n - 1.f) / 24.f);
            case 12: return ni % 2 == 0 ? n * 1.06f : n;                     // even partials sharp
            case 13: return (ni % 2 == 1 && ni > 1) ? n * 0.95f : n;         // odd partials flat
            case 14: return ni == 1 ? 1.f : n + (rnd - 0.5f) * 0.9f;         // fixed random scatter
            case 15: return ni <= 12 ? kBell[ni - 1] : 8.f + 1.1f * (n - 12.f);  // church bell (hum below)
            case 16: return ni == 1 ? 1.f : n + 0.37f;                       // all overtones shifted: clangy
            case 17: return ni == 1 ? 1.f : n * (ni % 2 ? 1.012f : 0.988f);  // beating pairs
            default: return n;
        }
    }
};

// Per-voice state for the spectral stage.
struct SpectralVoice
{
    float drift[kMaxPartials] {};
    float goal[kMaxPartials] {};
    float noise[kMaxPartials] {};

    void reset()
    {
        std::fill (std::begin (drift), std::end (drift), 0.f);
        std::fill (std::begin (goal), std::end (goal), 0.f);
        std::fill (std::begin (noise), std::end (noise), 0.f);
    }
};

inline int partialCount (const SpectralParams& p, bool economy)
{
    const int n = std::clamp ((int) p.partials, 1, kMaxPartials);
    return economy ? std::min (n, kEcoPartials) : n;
}

// Fills freq (Hz) and amp for partials [0, n). t = seconds since note-on, dt = control period.
inline void runSpectral (const SpectralTables& T, const SpectralParams& p, SpectralVoice& v, Rng& rng,
                         float f0, float dt, float t, int n, float* freq, float* amp)
{
    const float* la = T.logAlg[std::clamp (p.algo, 0, kNumAlgos - 1)];
    const float depth = clamp01 (p.depth);

    const float rateHz = logMap (clamp01 (p.modRate), 0.05f, 20.f);
    const float jump = rateHz * dt;                       // chance per block of a new wander target
    const float glide = 1.f - std::exp (-dt * rateHz * 3.f);
    const float octs = clamp01 (p.modDepth) * (60.f / 1200.f);

    for (int k = 0; k < n; ++k)
    {
        if (octs > 0.f)
        {
            // Each partial picks a new random target now and then and glides there.
            // Rate varies per partial (0.5x..1.5x) so they do not move in lockstep.
            if (rng.next() < jump * (0.5f + (float) (k % 7) / 6.f))
                v.goal[k] = rng.bi();
            v.drift[k] += glide * (v.goal[k] - v.drift[k]);
        }
        freq[k] = f0 * (float) (k + 1) * std::exp2 (depth * la[k] + octs * v.drift[k]);
        amp[k] = std::exp2 (-p.tilt * T.log2n[k]);
    }

    const float mix = clamp01 (p.noiseMix);
    if (mix <= 0.f)
        return;

    // Each partial's noise is a smoothed random walk: c = 1 means a fresh random
    // value every block (widest band, about +-375 Hz at 48 kHz), small c is a
    // slow wobble (narrow band). norm brings its RMS back to 1 whatever c is.
    const float c = lerp (0.03f, 1.f, sq (clamp01 (p.noiseWidth)));
    const float norm = std::sqrt (3.f * (2.f - c) / c);
    const float lo = knobHz (p.noiseLow), hiHz = knobHz (p.noiseHigh);
    const float burst = p.noiseDecay <= 0.f ? 1.f
                                            : std::exp (-t / logMap (1.f - clamp01 (p.noiseDecay), 0.05f, 8.f));
    for (int k = 0; k < n; ++k)
    {
        v.noise[k] += c * (rng.bi() - v.noise[k]);
        const float f = freq[k];
        const float shape = 1.f / ((1.f + sq (lo / f)) * (1.f + sq (f / hiHz)));  // 12 dB/oct HP and LP
        const float nz = std::clamp (v.noise[k] * norm, -3.f, 3.f);
        amp[k] *= (1.f - mix) + mix * burst * shape * nz;
    }
}
} // namespace v4jb
