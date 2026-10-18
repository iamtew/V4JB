# V4JB – Architecture

JUCE plugin built from `V4JB.jucer` with Projucer and MSBuild (`just build-debug`). No CMake. Every new file needs a `<FILE>` entry in the jucer, then `just resave`.

## Source

- `src/PluginProcessor.*` – the audio processor. Owns the APVTS, the engine, the morph pad, the effects (slot 12) and the wave data. Per block: read parameters, advance the morph, split the block at each MIDI event and render the engine, run the effects, ramp Volume. Real-time: no allocations, locks or system calls. Also handles state save/load, presets, the randomizer, and wave loading on a worker thread (`juce::ThreadPool`), message thread only.
- `src/PluginEditor.*` – the shell: top bar (About, presets, Randomize, Economy, Voices, Octave, Pitch, Glide, Volume) and four tabs. Standalone adds a piano strip (C2–C6).
- `src/ui/` – header-only pages: `Theme.h` (palette, look and feel, Knob/HSlider/Combo/Toggle, spectrum bars), `EditPage.h` (12-slot chain), `ModPage.h` (envelopes, LFOs), `MorphPage.h` (XY pad, path editor, macros), `WavePage.h`. `VersionCompare.h` compares tags for the update checker.
- `src/dsp/` – the engine, header-only, no JUCE, tested by `just check`: `Common.h`, `Partials.h` (sine bank), `Spectral.h` (slot 1), `Modules.h` (module pool), `Mod.h` (envelopes, LFOs), `Morph.h`, `Wave.h`, `Engine.h` (voices, macro mapping, spectrum taps). See `docs/DSP_NOTES.md`.
- `src/WaveAnalyzer.h` – audio to `WaveData` (FFT, worker thread). A wave source is an absolute path or `factory:<name>.wav`, which reads a bundled `presets/waves/*.wav` from BinaryData. `tools/make_waves.cpp` (`just waves`) synthesises those files.
- `src/Presets.h` – factory presets (BinaryData from `presets/*.xml`) and user presets (`Documents/V4JB/Presets`).
- `src/parameters/` – `ParameterIDs.h` (ids are saved in DAW projects and presets, do not rename) and `ParameterLayout.h` (about 225 parameters, generated in loops).

## Threads and sharing

| Data | Writer | Reader | How |
|------|--------|--------|-----|
| Parameters | host / UI | audio | APVTS atomics, cached pointers |
| Standalone key clicks | UI | audio | `AbstractFifo` |
| Morph corners, path nodes | UI | audio | relaxed atomics in `v4jb::Morph` |
| Puck, live macros, spectrum taps | audio | UI | relaxed atomics, UI polls at 30 Hz |
| Wave data | worker -> message thread | audio | `atomic<WaveData*>`, old data freed after 2 audio blocks |

## State

The APVTS tree (`<PARAMS>`) plus a `<MORPH c0..c3 nodes>` child, `waveA`/`waveB` sources (file path or `factory:` name) and `presetName`. Factory presets may set `waveA`/`waveB`; presets without them keep the loaded waves. User presets are the same XML with `presetName` and `category`. Factory presets list only what differs from the defaults. Morph points and paths can also be saved on their own (`.v4jbmorph`).

## Update check

About → "Check for update" fetches `https://api.github.com/repos/iamtew/V4JB/tags` on a background thread, keeps tags shaped like `vN.N.N`, and compares them with the jucer `version=`.

## Release

`just build-dist` builds Release, zips VST3 and Standalone, and builds the WiX MSI from `installer/`.
