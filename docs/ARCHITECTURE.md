# V4JB – Architecture

JUCE plugin built from `V4JB.jucer` with Projucer and MSBuild (`just build-debug`). No CMake.

## Source

- `src/PluginProcessor.*` – the audio processor. Owns the APVTS, saves and loads state as XML. `processBlock` is real-time: no allocations, locks or system calls.
- `src/PluginEditor.*` – the UI. Title and About box (top left), Volume (top right), texture area (body).
- `src/parameters/` – `ParameterIDs.h` (ids are saved in DAW projects, do not rename) and `ParameterLayout.h`.
- `src/ui/VersionCompare.h` – tag compare for the update checker. Header-only, tested by `just check`.

## Update check

About → "Check for update" fetches `https://api.github.com/repos/iamtew/V4JB/tags` on a background thread, keeps tags shaped like `vN.N.N`, and compares them with the jucer `version=`.

## Release

`just build-dist` builds Release, zips VST3 and Standalone, and builds the WiX MSI from `installer/`.
