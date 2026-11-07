# Hello, fog

V4JB is an atmosphere instrument. Pads, drones, air, fog. Very vibe, very mood. You play a note, and a bank of sine waves (partials) starts singing. Up to 512 of them, plus two optional sub sines if you want the floor to rumble a little.

This is the user book. Internals live under Spec, Architecture, and DSP notes in the sidebar, if you enjoy reading the wiring.

VST3 and standalone. Windows, Linux, and Mac.

## Install

The Windows installer is the easy path: `V4JB-{version}-win-x64-installer.msi` (older 0.0.5 builds shipped as `V4JB-{version}-win-x64.msi`). Pick VST3, Standalone, or both. Defaults are `Common Files\VST3` and `Program Files\Stupid Audio\V4JB`; each folder can be changed.

Or unzip `V4JB-{version}-win-x64-vst3.zip` into your VST3 folder, and/or unzip the standalone zip wherever you like. Same idea on Linux and Mac, minus the installer.

In a DAW, scan for VST3 and load **V4JB**. In Standalone, pick your audio device and MIDI keyboard under **Options > Audio/MIDI Settings**.

Mac builds are ad-hoc signed, so Gatekeeper will sulk until you right-click Open the first time (or `xattr -dr com.apple.quarantine` on the `.vst3` and the `.app`). Linux wants the usual system ALSA, FreeType, fontconfig, and X11 libraries.

## How it makes sound

Instead of 512 knobs, the sound runs through a chain of 12 slots that shape those partials:

1. **Slot 1, Spectral** (fixed): where the partials sit and how they start out — spacing, brightness, shimmer, noise.
2. **Slots 2–11**: pick any module per slot. They run in order, left to right, then the second row. Order matters: a Lowpass before Random Drops is not the same as after.
3. **Slot 12, Effects & Master** (fixed): drive, chorus or phaser, ping-pong delay, reverb, stereo width. This is the only stage that works on the summed audio instead of the partial list.

The teal bars on each Edit card show the first 64 partials **after that slot**, for the last note you played. Empty bars mean no note has sounded yet. The plugin is not broken. It is shy.

Play from MIDI. Velocity, mod wheel, aftertouch, and pitch bend ±2 semitones all work. The Standalone window also has piano keys along the bottom (C2 to C6). Up to 8 voices; when they run out, the oldest is stolen. **Economy** caps each voice at 128 partials, which is much lighter on the CPU and usually hard to hear.

## The window

![Edit page](edit.png)

The top bar is always there. Under it sit four tabs:

| Tab | What it is |
|---|---|
| **Edit** | The 12-slot chain: spectral stage, modules, effects |
| **Mod** | Four envelopes and three LFOs that slots can route |
| **Morph** | XY pad, 11 macros, path and corners |
| **Wave** | Two audio files that Wave / Double Wave modules follow |

Standalone adds the key strip. VST3 does not: the host already has keys. Drag the window corner to scale (40%–150%); the scale is saved with the session, so you do not have to rediscover your favourite size every Tuesday.
