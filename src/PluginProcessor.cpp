#include "PluginProcessor.h"
#include "PluginEditor.h"

V4JBAudioProcessor::V4JBAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
}

bool V4JBAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono())
        return false;
    return layouts.getMainInputChannelSet().isDisabled();
}

void V4JBAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    // Silence until the texture engine exists. MIDI is read here once voices do.
    buffer.clear();
    midi.clear();
}

void V4JBAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void V4JBAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Hosts can hand us anything. Only accept our own tree.
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* V4JBAudioProcessor::createEditor()
{
    return new V4JBAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new V4JBAudioProcessor();
}
