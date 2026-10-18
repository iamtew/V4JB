#pragma once

#include "Presets.h"
#include "dsp/Engine.h"
#include "dsp/Morph.h"
#include "parameters/ParameterLayout.h"

#include <JuceHeader.h>

// Synth shell around the additive engine. Per block: read parameters, advance
// the morph pad, render voices (split at each MIDI event), then slot 12: drive,
// chorus/phaser, delay, reverb, width, limiter, volume.
class V4JBAudioProcessor : public juce::AudioProcessor,
                           private juce::Timer
{
public:
    V4JBAudioProcessor();
    ~V4JBAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "V4JB"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 20.0; }  // 10 s max release + reverb and delay

    int getNumPrograms() override { return juce::jmax (1, factoryPresets.size()); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    v4jb::Morph morph;

    // UI spectrum taps (stage 0 = spectral slot, 1..10 = slots 2..11).
    float tap (int stage, int bar) const { return engine->taps[stage][bar].load (std::memory_order_relaxed); }

    // Message thread only. processBlock drains this. A click is dropped if the
    // queue is still full (AbstractFifo keeps one slot empty, so 31 fit).
    void pushKey (int note, bool on) noexcept;

    // ---- Message thread API for the editor ----
    // Morph: store the current macros into a corner (offsets reset, puck moves there), or jump to it.
    void storeCorner (int c);
    void recallCorner (int c);
    bool saveMorphFile (const juce::File& f, bool points);   // points = corners, else the path
    bool loadMorphFile (const juce::File& f);

    // Waves: analysis runs on a worker; getWave() returns the current data (may be null).
    // A source is an absolute path or "factory:<name>.wav" (bundled, see WaveAnalyzer::kFactoryPrefix).
    void loadWave (int w, const juce::String& source);
    void clearWave (int w);
    std::shared_ptr<v4jb::WaveData> getWave (int w) const { return waveOwned[w]; }
    juce::String getWaveName (int w) const { return waveSource[w].fromLastOccurrenceOf (":", false, false)
                                                                 .fromLastOccurrenceOf ("\\", false, false)
                                                                 .fromLastOccurrenceOf ("/", false, false); }
    std::atomic<int> waveVersion { 0 };  // bumps when a wave changes, the UI polls it

    // Presets and randomizer.
    const juce::Array<PresetEntry>& getFactoryPresets() const { return factoryPresets; }
    void loadPreset (const PresetEntry& e);
    bool saveUserPreset (const juce::String& name, const juce::String& category);
    void randomize (bool newModules);
    juce::String currentPresetName { "Init" };
    juce::String currentPresetCategory { "Init" };

    // Editor scale (drag the corner). 0.85 fits a 1440x900 screen; saved with the session.
    float uiScale = 0.85f;

private:
    void drainKeys() noexcept;
    v4jb::EngineParams readParams() const;
    void processEffects (float* L, float* R, int n, const float* macros, double bpm);
    void timerCallback() override;

    std::unique_ptr<juce::XmlElement> createStateXml();
    void applyStateXml (const juce::XmlElement& xml, bool fromPreset);
    juce::ValueTree morphTree (bool points, bool path) const;
    void applyMorphTree (const juce::ValueTree& t, bool resetMissing);
    void setParam (const juce::String& id, float realValue);
    void resetParamsToDefaults();
    void publishWave (int w, std::shared_ptr<v4jb::WaveData> data, const juce::String& source);

    // On-screen keys must not lock or allocate in processBlock. Feeding
    // MidiKeyboardState into the MIDI buffer would do both, so clicks land here.
    struct KeyEvent { juce::uint8 note = 0; juce::uint8 on = 0; };
    static constexpr int kKeyQueue = 32;
    juce::AbstractFifo keyFifo { kKeyQueue };
    KeyEvent keyEvents[kKeyQueue] {};

    std::unique_ptr<v4jb::Engine> engine = std::make_unique<v4jb::Engine>();  // ~700 KB of voices
    double sr = 48000.0;
    float lastLevel = 0.f;  // Volume ramps from here each block, so moves do not zipper
    std::vector<float> monoScratch;  // right channel when the host gives us mono out

    // Slot 12 effects.
    int fxBlock = 512;
    juce::dsp::Chorus<float> chorus;
    juce::dsp::Phaser<float> phaser;
    juce::dsp::Reverb reverb;
    juce::Reverb::Parameters reverbParams;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delay;
    juce::SmoothedValue<float> delaySamples;
    bool delayWasOn = false;
    juce::dsp::Limiter<float> limiter;

    // Waves: the message thread owns them (shared_ptr), the audio thread reads a raw
    // pointer. A replaced wave is only freed after the audio thread has finished
    // two more blocks, so it can never be read after free.
    std::shared_ptr<v4jb::WaveData> waveOwned[2];
    std::atomic<v4jb::WaveData*> waveLive[2] { nullptr, nullptr };
    juce::String waveSource[2];
    int waveGen[2] { 0, 0 };
    struct Retired { std::shared_ptr<v4jb::WaveData> data; juce::int64 block; juce::uint32 time; };
    std::vector<Retired> retired;
    std::atomic<juce::int64> blockCounter { 0 };
    juce::ThreadPool analysisPool { 1 };

    juce::Array<PresetEntry> factoryPresets = Presets::factory();
    int currentProgram = 0;

    // Cached once; reading an atomic is real-time safe, looking up by id is not.
    struct SlotPtrs { std::atomic<float>* type, *main, *p[4], *es, *ea, *ls, *la, *ms, *ma; };
    struct Ptrs
    {
        std::atomic<float>* level, *pitch, *octave, *glide, *poly, *economy, *spread, *width;
        std::atomic<float>* spAlgo, *spDepth, *spTilt, *spPartials, *spModDepth, *spModRate;
        std::atomic<float>* nzMix, *nzWidth, *nzLow, *nzHigh, *nzDecay, *sub1, *sub2;
        SlotPtrs slot[v4jb::kNumSlots];
        std::atomic<float>* env[v4jb::kNumEnvs][5];
        std::atomic<float>* lfo[v4jb::kNumLfos][4];
        std::atomic<float>* wave[2][3], *waveBal;
        std::atomic<float>* drOn, *drAmt, *drMix, *mdOn, *mdType, *mdRate, *mdDepth, *mdMix;
        std::atomic<float>* dlOn, *dlSync, *dlTime, *dlDiv, *dlFb, *dlMix, *rvOn, *rvSize, *rvDamp, *rvMix;
        std::atomic<float>* macro[v4jb::kNumMacros], *lock[v4jb::kNumMacros];
        std::atomic<float>* mx, *my, *mmode, *mtime, *msnap, *muser;
    } ptr {};

    JUCE_DECLARE_WEAK_REFERENCEABLE (V4JBAudioProcessor)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (V4JBAudioProcessor)
};
