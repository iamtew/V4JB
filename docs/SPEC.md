# V4JB spec

This file is the product spec.

Atmosphere and texture instrument. Pads, drones, air, fog. Sister plugin to TEW03 and K1X. Stupid Audio. MIT.

- Modular additive synth: up to 512 sine partials per voice, shaped by a chain of spectral modules, brought to life by a morph pad
- Clean, glassy, digital, always moving
- Not an effect on incoming audio, not a sample library / ROMpler

## Product

- VST3 + Standalone
- Instrument (MIDI in); does not process input audio
- About + GitHub tag check (title-bar name + version)
- Window scale 40–150%, saved with the session
- Factory presets in the plugin; user presets in `Documents\V4JB\Presets` as `Name.xml`
- Jucer `version=` is the binary, About dialog, git tag, and `just build-dist` name. Tags match that string (no `v` prefix).

Tabs: Edit, Mod, Morph, Wave. Standalone adds a key strip (C2–C6).

## Engine

- Additive: 8 voices, 512 partials (128 in Economy), 18 spacing algorithms, spectral modulation, spectral noise, two sub octaves
- 1 is mono (last-note priority, legato glide); 2–8 are polyphonic
- 12 slots: Slot 1 Spectral (fixed), slots 2–11 pick any module, Slot 12 Effects & Master (fixed, summed audio)
- 4 envelopes, 3 tempo-syncable LFOs
- MIDI: velocity, mod wheel, aftertouch, pitch bend ±2 semitones

## Morph and Wave

- Morph pad: 4 corners, drawn paths (free or synced), X-axis envelope mode with snap
- 11 fixed-assignment macros + offsets + one User macro
- Wave / Double Wave: shape partials from analysed audio files
- Smart randomizer: new module chain, or vary values and keep modules

## Effects

Drive, chorus/phaser, ping-pong delay, reverb, width, limiter — Slot 12 only.

## Do not

- Effect on incoming audio (it is an instrument)
- Sample library / ROMpler
- Copying commercial plugins: their published feature lists are a reference, but code and presets are our own
- CMake (Projucer only)
- Allocations, locks, or system calls in `processBlock`

## Stack

- JUCE from `%USERPROFILE%\JUCE`, Projucer workflow only
- C++17, APVTS for all parameters
- Just + justfile
- Header-only JUCE-free engine in `dsp/` (tested by `just check`)

## UI

Fog at night. Do not invent another palette.

- `#14121f` night
- `#24233a` dusk
- `#9aa3b5` fog
- `#6fc3b8` teal
- `#e0a458` amber
- `#ffffff` white

`assets/logo.af` is the Affinity source. `assets/logo.png` is the export. Never overwrite the PNG. `just icon` derives the Standalone ICO and `docs/logo.png`.

## Layout

```
src/
  PluginProcessor.h/.cpp
  PluginEditor.h/.cpp
  Presets.h WaveAnalyzer.h
  dsp/
  parameters/ParameterIDs.h ParameterLayout.h
  ui/Theme.h EditPage.h ModPage.h MorphPage.h WavePage.h VersionCompare.h
presets/*.xml
presets/waves/*.wav
docs/
justfile
V4JB.jucer
```

README stays small. LICENSE is MIT. `docs/` is the gh-pages site.
