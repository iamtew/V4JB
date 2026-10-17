<img src="assets/logo.png" alt="V4JB" width="160">

# V4JB

**V4JB** is an atmosphere and texture instrument. Pads, drones, air, fog. Very vibe, very mood.

VST3 and standalone. Stupid Systems LLC. [MIT](LICENSE). Site: [docs/](docs/index.html). Manual: [docs/MANUAL.md](docs/MANUAL.md).

Early scaffold: the plugin builds, loads and is silent. The texture engine is being designed in [docs/DSP_NOTES.md](docs/DSP_NOTES.md).

## Build

`just build-debug` (Projucer resave + MSBuild). `just check` runs the version compare test. JUCE is expected at `%USERPROFILE%\JUCE` (override with `JUCE_PATH`).

## Release

`just build-dist` writes `dist/V4JB-<version>-win-x64-vst3.zip`, `dist/V4JB-<version>-win-x64-standalone.zip`, and `dist/V4JB-<version>-win-x64.msi`. The MSI lets you install the VST3, the standalone app, or both. Defaults are `Common Files\VST3` and `Program Files\Stupid Systems LLC\V4JB`; each folder can be changed. Building the MSI needs the .NET 8 SDK.
