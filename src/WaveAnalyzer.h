#pragma once

#include "dsp/Wave.h"

#include <JuceHeader.h>

// Turns an audio file into a v4jb::WaveData: a 2048-point FFT every 1024
// samples, magnitudes gathered into 256 log-spaced bands (20 Hz..20 kHz), then
// peak-normalised. Runs on a worker thread, never the audio thread.
// Returns nullptr if the source cannot be read.
namespace WaveAnalyzer
{
// Sources: an absolute path, or kFactoryPrefix + a bundled file name (presets/waves/*.wav, BinaryData).
inline const juce::String kFactoryPrefix { "factory:" };

// Original file names of the bundled .wav resources, e.g. "vowels.wav".
inline juce::StringArray factoryWaves()
{
    juce::StringArray names;
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        const juce::String file = BinaryData::getNamedResourceOriginalFilename (BinaryData::namedResourceList[i]);
        if (file.endsWithIgnoreCase (".wav"))
            names.add (file);
    }
    names.sort (true);
    return names;
}

inline std::unique_ptr<juce::AudioFormatReader> openReader (juce::AudioFormatManager& formats, const juce::String& source)
{
    if (! source.startsWith (kFactoryPrefix))
        return std::unique_ptr<juce::AudioFormatReader> (formats.createReaderFor (juce::File (source)));
    const auto name = source.fromFirstOccurrenceOf (kFactoryPrefix, false, false);
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
        if (name == BinaryData::getNamedResourceOriginalFilename (BinaryData::namedResourceList[i]))
        {
            int size = 0;
            const char* data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], size);
            return std::unique_ptr<juce::AudioFormatReader> (
                formats.createReaderFor (std::make_unique<juce::MemoryInputStream> (data, (size_t) size, false)));
        }
    return nullptr;
}

inline std::shared_ptr<v4jb::WaveData> analyse (const juce::String& source)
{
    using v4jb::WaveData;
    constexpr int kOrder = 11, kSize = 1 << kOrder, kHop = 1024, kMaxFrames = 2048;
    constexpr double kMaxSeconds = 120.0;

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    auto reader = openReader (formats, source);
    if (reader == nullptr || reader->lengthInSamples <= 0 || reader->sampleRate <= 0.0)
        return nullptr;

    const int len = (int) std::min<juce::int64> (reader->lengthInSamples, (juce::int64) (reader->sampleRate * kMaxSeconds));
    juce::AudioBuffer<float> buf ((int) reader->numChannels, len);
    reader->read (&buf, 0, len, 0, true, true);

    // Mono mixdown.
    std::vector<float> mono ((size_t) len, 0.f);
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        juce::FloatVectorOperations::add (mono.data(), buf.getReadPointer (ch), len);

    // Frames: one per hop, thinned evenly if the file is long.
    const int rawFrames = std::max (1, (len - kSize) / kHop + 1);
    const int frames = std::min (rawFrames, kMaxFrames);

    auto out = std::make_shared<WaveData>();
    out->numFrames = frames;
    out->seconds = (float) (len / reader->sampleRate);
    out->gain.assign ((size_t) frames * WaveData::kBands, 0.f);

    juce::dsp::FFT fft (kOrder);
    juce::dsp::WindowingFunction<float> window (kSize, juce::dsp::WindowingFunction<float>::hann);
    std::vector<float> work ((size_t) kSize * 2);
    std::vector<int> binBand (kSize / 2);
    for (int b = 0; b < kSize / 2; ++b)
        binBand[(size_t) b] = (int) WaveData::bandOf ((float) (b * reader->sampleRate / kSize));

    float peak = 0.f;
    for (int f = 0; f < frames; ++f)
    {
        const int start = (int) ((juce::int64) f * (rawFrames - 1) / std::max (1, frames - 1)) * kHop;
        std::fill (work.begin(), work.end(), 0.f);
        for (int i = 0; i < kSize && start + i < len; ++i)
            work[(size_t) i] = mono[(size_t) (start + i)];
        window.multiplyWithWindowingTable (work.data(), kSize);
        fft.performFrequencyOnlyForwardTransform (work.data());

        // Each band keeps its loudest bin; empty low bands borrow from the nearest filled one below.
        float* row = out->gain.data() + (size_t) f * WaveData::kBands;
        for (int b = 1; b < kSize / 2; ++b)
            row[binBand[(size_t) b]] = std::max (row[binBand[(size_t) b]], work[(size_t) b]);
        for (int b = 1; b < WaveData::kBands; ++b)
            if (row[b] == 0.f)
                row[b] = row[b - 1];
        for (int b = 0; b < WaveData::kBands; ++b)
            peak = std::max (peak, row[b]);
    }
    if (peak > 0.f)
        juce::FloatVectorOperations::multiply (out->gain.data(), 1.f / peak, (int) out->gain.size());
    return out;
}
} // namespace WaveAnalyzer
