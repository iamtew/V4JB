# V4JB – Manual

## Install

Run `V4JB-<version>-win-x64.msi`. Pick VST3, Standalone, or both. Defaults are `Common Files\VST3` and `Program Files\Stupid Systems LLC\V4JB`.

Or unzip `V4JB-<version>-win-x64-vst3.zip` into your VST3 folder.

## How it makes sound

Every note is built from up to 512 sine waves (partials). Instead of 512 knobs, the sound runs through a chain of 12 slots that shape those partials:

1. **Slot 1, Spectral** (fixed): where the partials sit and how they start out.
2. **Slots 2–11**: pick any module per slot. They run in order, left to right then the second row, and the order matters.
3. **Slot 12, Effects & Master** (fixed): drive, chorus or phaser, delay, reverb, width.

The bars on each card show the first 64 partials after that slot, for the last note you played.

Play from MIDI (velocity, mod wheel, aftertouch and pitch bend ±2 all work). The Standalone window also has piano keys along the bottom (C2 to C6). Pick your MIDI keyboard under Options > Audio/MIDI Settings.

## Top bar

- **V4JB / version** (top left): click for About. "Check for update" asks GitHub for a newer tag.
- **Presets**: click the name for the list (by category), `<` `>` to step through. **Save** writes a user preset to `Documents\V4JB\Presets`.
- **Randomize**: "New sound" picks a new module chain and values; "Vary values" keeps your modules.
- **Economy**: at most 128 partials per voice. Much lighter on the CPU and usually hard to hear.
- **Voices** (1 = mono, notes glide legato), **Octave**, **Pitch** (semitones), **Glide**, **Volume**.

## Edit page

**Slot 1, Spectral**
- Menu: the spacing algorithm (Stiff String, Bell, Primes, Membrane ...). **Depth** moves from a pure harmonic series (0) to that algorithm (1). This is where metallic, bell and alien sounds come from.
- **Tilt**: brightness of the starting spectrum (low = buzzy, high = soft).
- **Partials**: how many sine waves.
- **Mod Depth / Mod Rate**: every partial drifts slightly out of tune on its own. Shimmer.
- **Noise / Width / Low Cut / High Cut / N. Decay**: turns partials into bands of noise. Width is how noisy. N. Decay above 0 makes the noise a breathy burst at the start of each note.
- **Sub -1 / Sub -2**: clean sines one and two octaves below.

**Slots 2–11**: pick a module from the menu. The big knob is the main control; the bars to its right are the module's extra controls (their names change with the module). **MOD** flips the card to its modulation routes: one envelope, one LFO and one MIDI source, each with an amount (the bar, centre = none), all pushing the main knob.

Highlights: **Lowpass/Highpass/Bandpass/Notch** (filters), **Hype** (boost or cut lows and highs: instant air or body), **Moving Filter** (sweeping notches), **Modulator** (loudness rolling up through the partials), **Random Drops** (partials flaring like rain), **Phrase** (a spectral arpeggio on the beat), **Tail** (lets partials ring on), **5-Sines** (organ), **Wave / Double Wave** (follow a sample, see the Wave page).

**Slot 12**: each effect has its own **On**. Delay **Sync** locks the time to the host tempo. **Width** is stereo width (0 mono, 2 extra wide); **Spread** pans odd and even partials apart.

## Mod page

Four envelopes (Attack, Decay, Sustain, Release, Velocity = how much velocity matters). **Env 1 is the volume of every note.** Three LFOs: Rate, Shape, and Sync to lock them to the beat.

## Morph page

Eleven macros (Character, Complexity, Tone, Punch, Length, Time FX, Depth, Speed, FX Mix, Width, User) each move a set of things at once. The bar shows each macro's current value; the slider next to it is an **offset** you can automate. **Free** takes a macro out of the morph pad. **User Macro Slot** picks which slot the User macro drives.

The XY pad blends four snapshots of the macros, one per corner (A bottom-left, B bottom-right, C top-left, D top-right):

1. Set the macro offsets to a sound you like, then press **Store A**. The offsets go back to zero and the puck jumps to A, so nothing changes.
2. Do the same for B, C and D.
3. Drag the puck around. Click a corner letter to jump to it.

**Mode**:
- **Manual**: drag the puck.
- **Path**: the puck travels a loop you draw (**Edit Path**: click to add a node, drag to move, right-click to delete). **Time** is seconds per bar. Select a node to set how long its segment lasts.
- **Path Sync**: same, but bar lengths follow the host tempo.
- **X-Axis Env**: each new note (from silence) sweeps the puck left to right over **Time** seconds, with its height read off the path. **Snap** steps it in 4–32 jumps for rhythmic morphs.

**Save Points / Save Path / Load** store the corners and the path as separate files.

## Wave page

Load an audio file into **A** and/or **B**. V4JB measures how loud each frequency is through the file. A **Wave** module (on the Edit page) then makes the partials follow it, which works like a vocoder. **Double Wave** follows both, mixed by **Balance** (the Character macro moves it too). **Start / End** set the region (End before Start plays it backwards), **Speed** sets how fast it plays (0.25x to 4x). The files are remembered by path in your session.
