#pragma once

// Analysed audio for the Wave and Double Wave modules: a spectral envelope per
// frame on a log-frequency grid. Built off the audio thread (src/WaveAnalyzer.h),
// only read here. The modules look up the gain at each partial's frequency, so a
// partial is loud where the sample was loud. That is the vocoder effect.

#include "Common.h"

#include <vector>

namespace v4jb
{
struct WaveData
{
    static constexpr int kBands = 256;
    static constexpr float kLoHz = 20.f;
    static constexpr float kOctaves = 10.f;  // 20 Hz .. 20.48 kHz

    int numFrames = 0;
    float seconds = 0.f;       // length of the source file
    std::vector<float> gain;   // numFrames * kBands, 0..1 amplitude, peak-normalised

    // Fractional band for a frequency. Public so the analyser uses the same grid.
    static float bandOf (float hz)
    {
        const float b = std::log2 (std::max (hz, 1.f) / kLoHz) / kOctaves * (float) (kBands - 1);
        return std::clamp (b, 0.f, (float) (kBands - 1));
    }

    // pos01 = position in the file (0 start, 1 end). Bilinear in time and frequency.
    float at (float pos01, float hz) const
    {
        if (numFrames <= 0)
            return 1.f;
        const float fpos = clamp01 (pos01) * (float) (numFrames - 1);
        const int f0 = (int) fpos;
        const int f1 = std::min (f0 + 1, numFrames - 1);
        const float ft = fpos - (float) f0;
        const float bpos = bandOf (hz);
        const int b0 = (int) bpos;
        const int b1 = std::min (b0 + 1, kBands - 1);
        const float bt = bpos - (float) b0;
        const float* r0 = gain.data() + (size_t) f0 * kBands;
        const float* r1 = gain.data() + (size_t) f1 * kBands;
        return lerp (lerp (r0[b0], r0[b1], bt), lerp (r1[b0], r1[b1], bt), ft);
    }
};
} // namespace v4jb
