<img src="assets/logo.png" alt="V4JB" width="160">

# V4JB

**V4JB** is an atmosphere and texture instrument. Pads, drones, air, fog. Very vibe, very mood.

VST3 and standalone. Stupid Systems LLC. [MIT](LICENSE). Site: [docs/](docs/index.html). Manual: [docs/MANUAL.md](docs/MANUAL.md).

A modular additive synth: up to 512 sine partials per voice, 8 voices. A fixed spectral stage (18 spacing algorithms, shimmer, spectral noise, sub octaves) feeds 10 slots of swappable spectral modules (filters, Hype, Modulator, Random Drops, Phrase, Wave and more), then drive, chorus/phaser, delay and reverb. A morph pad blends four macro snapshots by hand, along drawn paths, or as an envelope. Design in [docs/DSP_NOTES.md](docs/DSP_NOTES.md).

## Build

`just build-debug` (Projucer resave + MSBuild). `just check` runs the version compare test and the engine render check (`tests/engine_check.cpp`). JUCE is expected at `%USERPROFILE%\JUCE` (override with `JUCE_PATH`).

494 factory presets in 18 categories live in `presets/*.xml` (compiled in as BinaryData; list only what differs from the defaults). `just check` validates them with `tools/check_presets.ps1`. The built-in Wave page sources in `presets/waves/` are synthesised by `tools/make_waves.cpp`; regenerate them with `just waves`.

## Release

Windows, Linux, and macOS files come from GitHub Actions (`.github/workflows/dist.yml`). Run it from the Actions tab, or push a numeric tag such as `0.0.3`. The workflow does not publish a release. `<version>` is the `version` in `V4JB.jucer`.

The `dist` artifact holds:

- `V4JB-<version>-win-x64-vst3.zip`
- `V4JB-<version>-win-x64-standalone.zip`
- `V4JB-<version>-win-x64.msi`
- `V4JB-<version>-linux-x64-vst3.zip`
- `V4JB-<version>-linux-x64-standalone.zip`
- `V4JB-<version>-mac-universal-vst3.zip`
- `V4JB-<version>-mac-universal-standalone.zip`

Download that artifact into `dist/` and upload the files on a GitHub release:

```
gh run download <run-id> -n dist -D dist
```

The MSI installs the VST3, the standalone app, or both. Defaults are `Common Files\VST3` and `Program Files\Stupid Systems LLC\V4JB`; each folder can be changed. The standalone install adds a Start menu shortcut.

The Mac zips are ad-hoc signed, not notarized. Gatekeeper blocks them until you right-click Open, or run `xattr -dr com.apple.quarantine` on the `.vst3` and the `.app`. The Linux build uses the system ALSA, FreeType, fontconfig, and X11 libraries.

`just build-dist` on Windows still writes the three Windows files only. Building the MSI needs the .NET 8 SDK.
