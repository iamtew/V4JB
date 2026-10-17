#pragma once

#include "ParameterIDs.h"

#include <JuceHeader.h>

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;

    AudioProcessorValueTreeState::ParameterLayout layout;

    auto pct = [] (NormalisableRange<float> range)
    {
        return AudioParameterFloatAttributes()
            .withStringFromValueFunction ([range] (float v, int)
            {
                return String (roundToInt (range.convertTo0to1 (v) * 100.f)) + "%";
            })
            .withValueFromStringFunction ([range] (const String& t)
            {
                return range.convertFrom0to1 (jlimit (0.f, 1.f, t.getFloatValue() / 100.f));
            });
    };

    auto unit = NormalisableRange<float> (0.f, 1.f);
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::level, 1 },
                                                        "Volume", unit, 0.75f, pct (unit)));

    return layout;
}
