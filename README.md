<img src="assets/logo.png" alt="V4JB" width="160">

# V4JB

**V4JB** is an atmosphere and texture instrument. Pads, drones, air, fog. Very vibe, very mood.

VST3 and standalone. Stupid Systems LLC. [MIT](LICENSE). Site: [docs/](docs/index.html). Manual: [docs/MANUAL.md](docs/MANUAL.md).

A modular additive synth: up to 512 sine partials per voice, 8 voices. A fixed spectral stage (18 spacing algorithms, shimmer, spectral noise, sub octaves) feeds 10 slots of swappable spectral modules (filters, Hype, Modulator, Random Drops, Phrase, Wave and more), then drive, chorus/phaser, delay and reverb. A morph pad blends four macro snapshots by hand, along drawn paths, or as an envelope. Design in [docs/DSP_NOTES.md](docs/DSP_NOTES.md).

## Build

`just build-debug` (Projucer resave + MSBuild). `just check` runs the version compare test and the engine render check (`tests/engine_check.cpp`). JUCE is expected at `%USERPROFILE%\JUCE` (override with `JUCE_PATH`).

394 factory presets in 16 categories live in `presets/*.xml` (compiled in as BinaryData; list only what differs from the defaults). `just check` validates them with `tools/check_presets.ps1`. The built-in Wave page sources in `presets/waves/` are synthesised by `tools/make_waves.cpp`; regenerate them with `just waves`.

## Release

`just build-dist` writes `dist/V4JB-<version>-win-x64-vst3.zip`, `dist/V4JB-<version>-win-x64-standalone.zip`, and `dist/V4JB-<version>-win-x64.msi`. The MSI lets you install the VST3, the standalone app, or both. Defaults are `Common Files\VST3` and `Program Files\Stupid Systems LLC\V4JB`; each folder can be changed. Building the MSI needs the .NET 8 SDK.
