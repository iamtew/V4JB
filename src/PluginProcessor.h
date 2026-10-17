#pragma once

#include "dsp/Swirl.h"
#include "parameters/ParameterLayout.h"

#include <JuceHeader.h>

// Synth shell around the Swirl texture voice: MIDI in, eight macros, Volume.
class V4JBAudioProcessor : public juce::AudioProcessor
{
public:
    V4JBAudioProcessor();
    ~V4JBAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int) override { swirl.prepare (sampleRate); }
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "V4JB"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }  // Swirl's release fade

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    v4jb::Swirl swirl;
    float lastLevel = 0.f;  // Volume ramps from here each block, so moves do not zipper

    // Cached once; reading an atomic is real-time safe, looking up by id is not.
    std::atomic<float>* level   = apvts.getRawParameterValue (ParamID::level);
    std::atomic<float>* motion  = apvts.getRawParameterValue (ParamID::motion);
    std::atomic<float>* crackle = apvts.getRawParameterValue (ParamID::crackle);
    std::atomic<float>* acid    = apvts.getRawParameterValue (ParamID::acid);
    std::atomic<float>* air     = apvts.getRawParameterValue (ParamID::air);
    std::atomic<float>* ghost   = apvts.getRawParameterValue (ParamID::ghost);
    std::atomic<float>* vowel   = apvts.getRawParameterValue (ParamID::vowel);
    std::atomic<float>* breath  = apvts.getRawParameterValue (ParamID::breath);
    std::atomic<float>* haunt   = apvts.getRawParameterValue (ParamID::haunt);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (V4JBAudioProcessor)
};
