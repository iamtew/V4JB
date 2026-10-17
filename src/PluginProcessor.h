#pragma once

#include "parameters/ParameterLayout.h"

#include <JuceHeader.h>

// Scaffold: a silent synth with one Volume param. The texture engine plugs in here.
class V4JBAudioProcessor : public juce::AudioProcessor
{
public:
    V4JBAudioProcessor();
    ~V4JBAudioProcessor() override = default;

    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "V4JB"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (V4JBAudioProcessor)
};
