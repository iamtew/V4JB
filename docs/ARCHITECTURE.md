# V4JB – Architecture

JUCE plugin built from `V4JB.jucer` with Projucer and MSBuild (`just build-debug`). No CMake.

## Source

- `src/PluginProcessor.*` – the audio processor. Owns the APVTS and the Swirl voice, saves and loads state as XML. `processBlock` drains Standalone key clicks from a lock-free queue, splits the block at each MIDI event, renders Swirl, then ramps Volume. Real-time: no allocations, locks or system calls.
- `src/PluginEditor.*` – the UI. Title and About box (top left), Volume (top right), eight macros in the texture well. Standalone adds a piano strip (C2–C5) under the well.
- `src/dsp/Swirl.h` – the texture voice. Header-only, no JUCE, tested by `just check`. See `docs/DSP_NOTES.md`.
- `src/parameters/` – `ParameterIDs.h` (ids are saved in DAW projects, do not rename) and `ParameterLayout.h`.
- `src/ui/VersionCompare.h` – tag compare for the update checker. Header-only, tested by `just check`.

## Update check

About → "Check for update" fetches `https://api.github.com/repos/iamtew/V4JB/tags` on a background thread, keeps tags shaped like `vN.N.N`, and compares them with the jucer `version=`.

## Release

`just build-dist` builds Release, zips VST3 and Standalone, and builds the WiX MSI from `installer/`.
