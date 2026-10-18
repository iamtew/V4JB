#pragma once

#include "ParameterIDs.h"
#include "../dsp/Engine.h"
#include "../dsp/Morph.h"

#include <JuceHeader.h>

// Delay sync divisions (effects live in the processor, so their tables live here).
inline constexpr int kNumDelayDivs = 7;
inline constexpr const char* kDelayDivNames[kNumDelayDivs] = { "1/16", "1/8", "1/8 dot", "1/4", "1/4 dot", "1/2", "1 bar" };
inline constexpr float kDelayDivBeats[kNumDelayDivs] = { 0.25f, 0.5f, 0.75f, 1.f, 1.5f, 2.f, 4.f };

// Default patch: a slow, airy pad. Slot 2 Lowpass breathing on LFO 1, slot 3 a
// gentle Modulator, slot 4 Hype lifting the top. Everything else Off.
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
    auto withUnit = [] (const char* unit, int decimals)
    {
        return AudioParameterFloatAttributes().withLabel (unit).withStringFromValueFunction (
            [decimals] (float v, int) { return String (v, decimals); });
    };

    const auto unit = NormalisableRange<float> (0.f, 1.f);
    auto knob = [&] (const String& id, const String& name, float def)
    {
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, unit, def, pct (unit)));
    };
    auto ranged = [&] (const String& id, const String& name, float lo, float hi, float def, float skew,
                       const char* unitName, int decimals)
    {
        NormalisableRange<float> r (lo, hi);
        r.setSkewForCentre (lo + (hi - lo) * skew);
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def, withUnit (unitName, decimals)));
    };
    auto bipolar = [&] (const String& id, const String& name, float lo, float hi, float def)
    {
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (lo, hi), def,
                                                            withUnit ("", 2)));
    };
    auto choice = [&] (const String& id, const String& name, const char* const* names, int count, int def)
    {
        StringArray items;
        for (int i = 0; i < count; ++i)
            items.add (names[i]);
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def));
    };
    auto toggle = [&] (const String& id, const String& name, bool def)
    {
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def));
    };
    auto integer = [&] (const String& id, const String& name, int lo, int hi, int def)
    {
        layout.add (std::make_unique<AudioParameterInt> (ParameterID { id, 1 }, name, lo, hi, def));
    };

    // Global
    knob (ParamID::level, "Volume", 0.75f);
    bipolar (ParamID::pitch, "Pitch", -12.f, 12.f, 0.f);
    integer (ParamID::octave, "Octave", -3, 3, 0);
    ranged (ParamID::glide, "Glide", 0.f, 2.f, 0.f, 0.15f, "s", 2);
    integer (ParamID::poly, "Polyphony", 1, v4jb::kMaxVoices, v4jb::kMaxVoices);
    toggle (ParamID::economy, "Economy", false);
    knob (ParamID::spread, "Spread", 0.4f);
    ranged (ParamID::width, "Width", 0.f, 2.f, 1.f, 0.5f, "", 2);

    // Slot 1: spectral stage
    choice (ParamID::spAlgo, "Distortion", v4jb::kAlgoNames, v4jb::kNumAlgos, 0);
    knob (ParamID::spDepth, "Distortion Depth", 0.f);
    ranged (ParamID::spTilt, "Tilt", 0.f, 3.f, 1.2f, 0.4f, "", 2);
    integer (ParamID::spPartials, "Partials", 1, v4jb::kMaxPartials, 256);
    knob (ParamID::spModDepth, "Spectral Mod Depth", 0.2f);
    knob (ParamID::spModRate, "Spectral Mod Rate", 0.3f);
    knob (ParamID::nzMix, "Noise Mix", 0.1f);
    knob (ParamID::nzWidth, "Noise Width", 0.5f);
    knob (ParamID::nzLow, "Noise Low Cut", 0.f);
    knob (ParamID::nzHigh, "Noise High Cut", 1.f);
    knob (ParamID::nzDecay, "Noise Decay", 0.f);
    knob (ParamID::sub1, "Sub -1 Oct", 0.f);
    knob (ParamID::sub2, "Sub -2 Oct", 0.f);

    // Slots 2..11
    std::vector<const char*> moduleNames;
    for (const auto& info : v4jb::kModuleInfo)
        moduleNames.push_back (info.name);
    for (int i = 0; i < v4jb::kNumSlots; ++i)
    {
        using M = v4jb::ModuleType;
        const M defType = i == 0 ? M::Lowpass : i == 1 ? M::Modulator : i == 2 ? M::Hype : M::Off;
        const String n = "Slot " + String (i + 2) + " ";
        choice (ParamID::slot (i, "type"), n + "Module", moduleNames.data(), v4jb::kNumModuleTypes, (int) defType);
        knob (ParamID::slot (i, "main"), n + "Main", i == 0 ? 0.72f : i == 1 ? 0.3f : 0.5f);
        for (int k = 0; k < 4; ++k)
            knob (ParamID::slotP (i, k), n + "P" + String (k + 1), (i == 2 && k == 3) ? 0.65f : 0.5f);
        choice (ParamID::slot (i, "es"), n + "Env Source", v4jb::kEnvSelNames, 5, 0);
        bipolar (ParamID::slot (i, "ea"), n + "Env Amount", -1.f, 1.f, 0.f);
        choice (ParamID::slot (i, "ls"), n + "LFO Source", v4jb::kLfoSelNames, 4, i == 0 ? 1 : 0);
        bipolar (ParamID::slot (i, "la"), n + "LFO Amount", -1.f, 1.f, i == 0 ? 0.06f : 0.f);
        choice (ParamID::slot (i, "ms"), n + "MIDI Source", v4jb::kMidiSelNames, 6, 0);
        bipolar (ParamID::slot (i, "ma"), n + "MIDI Amount", -1.f, 1.f, 0.f);
    }

    // Envelopes. Env 1 is the amp envelope: slow pad by default.
    struct EnvDef { float a, d, s, r, vel; };
    const EnvDef envDefs[v4jb::kNumEnvs] = { { 0.6f, 1.5f, 0.8f, 2.5f, 0.3f }, { 0.01f, 0.8f, 0.3f, 0.8f, 0.5f },
                                             { 2.f, 3.f, 0.5f, 3.f, 0.f },     { 0.005f, 0.2f, 0.f, 0.2f, 0.5f } };
    for (int i = 0; i < v4jb::kNumEnvs; ++i)
    {
        const String n = "Env " + String (i + 1) + " ";
        ranged (ParamID::env (i, "a"), n + "Attack", 0.001f, 10.f, envDefs[i].a, 0.05f, "s", 3);
        ranged (ParamID::env (i, "d"), n + "Decay", 0.001f, 10.f, envDefs[i].d, 0.05f, "s", 3);
        knob (ParamID::env (i, "s"), n + "Sustain", envDefs[i].s);
        ranged (ParamID::env (i, "r"), n + "Release", 0.001f, 10.f, envDefs[i].r, 0.05f, "s", 3);
        knob (ParamID::env (i, "vel"), n + "Velocity", envDefs[i].vel);
    }

    const float lfoRates[v4jb::kNumLfos] = { 0.2f, 1.f, 0.05f };
    for (int i = 0; i < v4jb::kNumLfos; ++i)
    {
        const String n = "LFO " + String (i + 1) + " ";
        ranged (ParamID::lfo (i, "rate"), n + "Rate", 0.01f, 20.f, lfoRates[i], 0.05f, "Hz", 2);
        toggle (ParamID::lfo (i, "sync"), n + "Sync", false);
        choice (ParamID::lfo (i, "div"), n + "Division", v4jb::kDivNames, v4jb::kNumDivs, 4);
        choice (ParamID::lfo (i, "shape"), n + "Shape", v4jb::kLfoShapeNames, v4jb::kNumLfoShapes, 0);
    }

    // Wave page
    for (int w = 0; w < 2; ++w)
    {
        const String n = w == 0 ? "Wave A " : "Wave B ";
        knob (ParamID::wave (w, "start"), n + "Start", 0.f);
        knob (ParamID::wave (w, "end"), n + "End", 1.f);
        knob (ParamID::wave (w, "speed"), n + "Speed", 0.5f);
    }
    knob (ParamID::waBalance, "Wave Balance", 0.5f);

    // Slot 12: effects
    static constexpr const char* modTypes[] = { "Chorus", "Phaser" };
    toggle (ParamID::drOn, "Drive On", false);
    knob (ParamID::drAmt, "Drive", 0.3f);
    knob (ParamID::drMix, "Drive Mix", 0.5f);
    toggle (ParamID::mdOn, "Mod FX On", true);
    choice (ParamID::mdType, "Mod FX Type", modTypes, 2, 0);
    ranged (ParamID::mdRate, "Mod FX Rate", 0.01f, 10.f, 0.3f, 0.1f, "Hz", 2);
    knob (ParamID::mdDepth, "Mod FX Depth", 0.4f);
    knob (ParamID::mdMix, "Mod FX Mix", 0.3f);
    toggle (ParamID::dlOn, "Delay On", false);
    toggle (ParamID::dlSync, "Delay Sync", true);
    ranged (ParamID::dlTime, "Delay Time", 10.f, 2000.f, 375.f, 0.2f, "ms", 0);
    choice (ParamID::dlDiv, "Delay Division", kDelayDivNames, kNumDelayDivs, 3);
    bipolar (ParamID::dlFb, "Delay Feedback", 0.f, 0.95f, 0.4f);
    knob (ParamID::dlMix, "Delay Mix", 0.25f);
    toggle (ParamID::rvOn, "Reverb On", true);
    knob (ParamID::rvSize, "Reverb Size", 0.7f);
    knob (ParamID::rvDamp, "Reverb Damping", 0.4f);
    knob (ParamID::rvMix, "Reverb Mix", 0.3f);

    // Morph
    for (int m = 0; m < v4jb::kNumMacros; ++m)
    {
        bipolar (ParamID::macro (m), String (v4jb::kMacroNames[m]) + " Offset", -0.5f, 0.5f, 0.f);
        toggle (ParamID::lock (m), String (v4jb::kMacroNames[m]) + " Unlinked", false);
    }
    knob (ParamID::morphX, "Morph X", 0.5f);
    knob (ParamID::morphY, "Morph Y", 0.5f);
    choice (ParamID::morphMode, "Morph Mode", v4jb::kMorphModeNames, 4, 0);
    ranged (ParamID::morphTime, "Morph Time", 0.1f, 30.f, 4.f, 0.15f, "s", 2);
    choice (ParamID::morphSnap, "Morph Snap", v4jb::kSnapNames, 5, 0);
    StringArray slots;
    for (int i = 0; i < v4jb::kNumSlots; ++i)
        slots.add ("Slot " + String (i + 2));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::userSlot, 1 }, "User Macro Slot", slots, 0));

    return layout;
}
