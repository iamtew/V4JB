# V4JB – Cursor Rules

Atmosphere and texture instrument, VST3 + Standalone. Pads, drones, air, fog. Sister plugin to TEW03 and K1X.

## Stack
- JUCE (Projucer workflow only – no CMake)
- C++17, real-time safe: no allocations, no locks, no system calls in processBlock
- APVTS for all parameters
- Just + justfile

## Layout
src/
 PluginProcessor.h/.cpp   engine + effects + state, presets, waves
 PluginEditor.h/.cpp      top bar + tabs
 Presets.h WaveAnalyzer.h
 dsp/                     header-only, JUCE-free engine (tested by `just check`)
 parameters/ParameterIDs.h ParameterLayout.h
 ui/Theme.h EditPage.h ModPage.h MorphPage.h WavePage.h VersionCompare.h
presets/*.xml             factory presets (BinaryData)
presets/waves/*.wav       built-in Wave sources (BinaryData, made by `just waves`)
tests/version_check.cpp engine_check.cpp
tools/make_waves.cpp      synthesises presets/waves/
tools/check_presets.ps1   validates presets/*.xml (run by `just check`)
docs/
installer/
justfile
V4JB.jucer

## Version
- Jucer `version=` is what the binary and About dialog show.
- Git tags must match that string (`0.0.2`).
- `just build-dist` names the zips and MSI from the jucer version.

## Do NOT
- Introduce CMake
- Allocate on the audio thread
- Copy commercial plugin code
