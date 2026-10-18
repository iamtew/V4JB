// Generates the built-in Wave page sources into presets/waves/ (run: `just waves`).
// Everything is synthesised here, so the bundled audio is ours to ship.
// 16-bit mono 44.1 kHz. Deterministic: the same code always writes the same files.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace
{
constexpr int kSr = 44100;
constexpr double kPi = 3.14159265358979323846;

struct Rng
{
    uint32_t s = 0x12345678u;
    double next() { s = s * 1664525u + 1013904223u; return (s >> 8) * (1.0 / 16777216.0); }  // 0..1
};

bool writeWav (const char* path, std::vector<double> x)
{
    double peak = 1e-9;
    for (double v : x)
        peak = std::max (peak, std::abs (v));
    std::vector<int16_t> pcm (x.size());
    for (size_t i = 0; i < x.size(); ++i)
        pcm[i] = (int16_t) std::lround (x[i] / peak * 0.9 * 32767.0);

    FILE* f = std::fopen (path, "wb");
    if (f == nullptr)
        return false;
    auto u32 = [f] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto u16 = [f] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    const uint32_t bytes = (uint32_t) (pcm.size() * 2);
    std::fwrite ("RIFF", 1, 4, f); u32 (36 + bytes); std::fwrite ("WAVE", 1, 4, f);
    std::fwrite ("fmt ", 1, 4, f); u32 (16); u16 (1); u16 (1); u32 (kSr); u32 (kSr * 2); u16 (2); u16 (16);
    std::fwrite ("data", 1, 4, f); u32 (bytes);
    std::fwrite (pcm.data(), 2, pcm.size(), f);
    std::fclose (f);
    std::printf ("wrote %s (%.1f s)\n", path, (double) x.size() / kSr);
    return true;
}

// Bright 110 Hz saw (additive) whose spectrum passes through the formants of A, E, I, O, U.
std::vector<double> vowels()
{
    const double F[5][3] = { { 800, 1150, 2900 }, { 400, 1700, 2600 }, { 300, 2200, 3000 },
                             { 450, 800, 2830 }, { 325, 700, 2530 } };
    const double f0 = 110.0, seconds = 5.0;
    const int n = (int) (seconds * kSr), harmonics = (int) (16000.0 / f0);
    std::vector<double> out ((size_t) n), gain ((size_t) harmonics + 1), phase ((size_t) harmonics + 1, 0.0);
    for (int i = 0; i < n; ++i)
    {
        if (i % 64 == 0)  // formant gains at control rate
        {
            const double pos = std::min (3.999, (double) i / n * 4.0);  // A..U across the file
            const int v = (int) pos;
            const double t = pos - v, s = t * t * (3 - 2 * t);
            for (int k = 1; k <= harmonics; ++k)
            {
                double g = 0.02;
                for (int j = 0; j < 3; ++j)
                {
                    const double fc = F[v][j] + (F[v + 1][j] - F[v][j]) * s, bw = 60.0 + fc * 0.06;
                    const double d = (k * f0 - fc) / bw;
                    g += (j == 0 ? 1.0 : 0.6) / (1.0 + d * d);
                }
                gain[(size_t) k] = g / k;
            }
        }
        const double vib = 1.0 + 0.004 * std::sin (2 * kPi * 5.0 * i / kSr);
        double y = 0;
        for (int k = 1; k <= harmonics; ++k)
        {
            phase[(size_t) k] += 2 * kPi * k * f0 * vib / kSr;
            y += gain[(size_t) k] * std::sin (phase[(size_t) k]);
        }
        out[(size_t) i] = y * std::min (1.0, std::min (i, n - i) / (0.05 * kSr));
    }
    return out;
}

// Four inharmonic bell strikes; upper partials die first.
std::vector<double> bells()
{
    const double ratio[] = { 0.5, 1.0, 1.19, 1.56, 2.0, 2.51, 2.66, 3.01, 4.1, 5.4 };
    const struct { double t, f; } hits[] = { { 0.0, 220 }, { 1.0, 330 }, { 2.0, 262 }, { 2.75, 196 } };
    const int n = 4 * kSr;
    std::vector<double> out ((size_t) n, 0.0);
    for (const auto& h : hits)
        for (int p = 0; p < 10; ++p)
        {
            const double f = h.f * ratio[p], decay = 0.4 + 2.5 / (1 + p), amp = 1.0 / (1 + p * 0.5);
            for (int i = (int) (h.t * kSr); i < n && f < 18000; ++i)
            {
                const double t = (double) i / kSr - h.t;
                out[(size_t) i] += amp * std::exp (-t / decay) * std::sin (2 * kPi * f * t);
            }
        }
    return out;
}

// Sparse high drops (short chirps) over a low lowpassed-noise rumble.
std::vector<double> rain()
{
    const int n = 4 * kSr;
    std::vector<double> out ((size_t) n, 0.0);
    Rng rng;
    double lp = 0;
    for (int i = 0; i < n; ++i)
    {
        lp += 0.01 * ((rng.next() * 2 - 1) - lp);
        out[(size_t) i] = lp * 2.0;
    }
    for (int d = 0; d < 240; ++d)  // ~60 drops per second
    {
        const int start = (int) (rng.next() * (n - kSr / 20));
        const double f = 2000 + rng.next() * 4000, amp = 0.1 + rng.next() * 0.4;
        for (int i = 0; i < kSr / 20; ++i)
        {
            const double t = (double) i / kSr;
            out[(size_t) (start + i)] += amp * std::exp (-t / 0.006) * std::sin (2 * kPi * f * (1 + 8 * t) * t);
        }
    }
    return out;
}

// Am, F, C, G: four notes each, sines with three harmonics, crossfaded.
std::vector<double> chords()
{
    const double notes[4][4] = { { 220.0, 261.63, 329.63, 440.0 }, { 174.61, 220.0, 261.63, 349.23 },
                                 { 130.81, 196.0, 261.63, 329.63 }, { 196.0, 246.94, 293.66, 392.0 } };
    const double chordLen = 1.25;
    const int n = (int) (4 * chordLen * kSr);
    std::vector<double> out ((size_t) n, 0.0);
    for (int c = 0; c < 4; ++c)
    {
        const double t0 = c * chordLen - 0.15, t1 = (c + 1) * chordLen + 0.15;
        for (int i = std::max (0, (int) (t0 * kSr)); i < std::min (n, (int) (t1 * kSr)); ++i)
        {
            const double t = (double) i / kSr, env = std::sin (kPi * (t - t0) / (t1 - t0));
            double y = 0;
            for (double f : notes[c])
                for (int h = 1; h <= 3; ++h)
                    y += std::sin (2 * kPi * f * h * t) / (h * h);
            out[(size_t) i] += env * y;
        }
    }
    return out;
}
} // namespace

int main()
{
    bool ok = writeWav ("presets/waves/vowels.wav", vowels());
    ok = writeWav ("presets/waves/bells.wav", bells()) && ok;
    ok = writeWav ("presets/waves/rain.wav", rain()) && ok;
    ok = writeWav ("presets/waves/chords.wav", chords()) && ok;
    return ok ? 0 : 1;
}
