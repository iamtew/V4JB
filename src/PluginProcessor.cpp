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
    const int n = buffer.getNumSamples();
    swirl.setMacros ({ motion->load(), crackle->load(), acid->load(), air->load() });

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    // Render up to each MIDI event, apply it, carry on: notes land on their exact sample.
    int pos = 0;
    for (const auto meta : midi)
    {
        const int at = juce::jlimit (pos, n, meta.samplePosition);
        swirl.render (L + pos, R != nullptr ? R + pos : nullptr, at - pos);
        pos = at;

        const auto msg = meta.getMessage();
        if (msg.isNoteOn())
            swirl.noteOn (msg.getNoteNumber());
        else if (msg.isNoteOff())
            swirl.noteOff (msg.getNoteNumber());
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            swirl.allNotesOff();
    }
    swirl.render (L + pos, R != nullptr ? R + pos : nullptr, n - pos);
    midi.clear();

    const float lv = level->load();
    buffer.applyGainRamp (0, n, lastLevel, lv);
    lastLevel = lv;
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
