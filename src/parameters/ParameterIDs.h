#pragma once

#include <JuceHeader.h>

// APVTS ids are saved in DAW projects and presets. Renaming one breaks old sessions.
namespace ParamID
{
// Global
constexpr const char* level   = "level";
constexpr const char* pitch   = "pitch";
constexpr const char* octave  = "octave";
constexpr const char* glide   = "glide";
constexpr const char* poly    = "poly";
constexpr const char* economy = "economy";
constexpr const char* spread  = "spread";
constexpr const char* width   = "width";

// Slot 1: spectral stage
constexpr const char* spAlgo     = "sp_algo";
constexpr const char* spDepth    = "sp_depth";
constexpr const char* spTilt     = "sp_tilt";
constexpr const char* spPartials = "sp_partials";
constexpr const char* spModDepth = "sp_moddepth";
constexpr const char* spModRate  = "sp_modrate";
constexpr const char* nzMix      = "nz_mix";
constexpr const char* nzWidth    = "nz_width";
constexpr const char* nzLow      = "nz_low";
constexpr const char* nzHigh     = "nz_high";
constexpr const char* nzDecay    = "nz_decay";
constexpr const char* sub1       = "sub1";
constexpr const char* sub2       = "sub2";

// Wave page (A and B files for Wave / Double Wave)
constexpr const char* waBalance = "w_bal";
inline juce::String wave (int w, const juce::String& what) { return juce::String (w == 0 ? "wa_" : "wb_") + what; }  // start end speed

// Slot 12: effects
constexpr const char* drOn = "dr_on", *drAmt = "dr_amt", *drMix = "dr_mix";
constexpr const char* mdOn = "md_on", *mdType = "md_type", *mdRate = "md_rate", *mdDepth = "md_depth", *mdMix = "md_mix";
constexpr const char* dlOn = "dl_on", *dlSync = "dl_sync", *dlTime = "dl_time", *dlDiv = "dl_div", *dlFb = "dl_fb", *dlMix = "dl_mix";
constexpr const char* rvOn = "rv_on", *rvSize = "rv_size", *rvDamp = "rv_damp", *rvMix = "rv_mix";

// Morph
constexpr const char* morphX    = "mx";
constexpr const char* morphY    = "my";
constexpr const char* morphMode = "mmode";
constexpr const char* morphTime = "mtime";
constexpr const char* morphSnap = "msnap";
constexpr const char* userSlot  = "muser";
inline juce::String macro (int m) { return "m" + juce::String (m); }      // offset, -0.5..0.5
inline juce::String lock (int m)  { return "lock" + juce::String (m); }   // not linked to morph

// Envelopes e1..e4: a d s r vel. LFOs l1..l3: rate sync div shape.
inline juce::String env (int i, const juce::String& what) { return "e" + juce::String (i + 1) + "_" + what; }
inline juce::String lfo (int i, const juce::String& what) { return "l" + juce::String (i + 1) + "_" + what; }

// User slots, numbered like the UI (2..11): type main p1..p4 es ea ls la ms ma.
inline juce::String slot (int i, const juce::String& what) { return "s" + juce::String (i + 2) + "_" + what; }
inline juce::String slotP (int i, int k) { return slot (i, "p" + juce::String (k + 1)); }
} // namespace ParamID
