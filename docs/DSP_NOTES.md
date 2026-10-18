# V4JB – DSP notes

How the additive engine works. Functional model taken from public descriptions of modular additive synths (Loom II style); every algorithm and preset here is our own.

## Signal flow

```
MIDI -> voice allocator (8 voices, oldest stolen)
     -> per voice, every 64 samples (control block):
          slot 1   spectral stage   -> partial frequencies + amplitudes
          slots 2..11 module chain  -> edits those arrays in series
          amp envelope (Env 1)      -> target levels for the sine bank
     -> sine bank sums partials every sample, levels ramp across the block
     -> tanh safety
slot 12 (processor): drive, chorus/phaser, ping-pong delay, reverb, M/S width, limiter, volume
```

All engine code is header-only and JUCE-free in `src/dsp/`; `tests/engine_check.cpp` renders it. Effects use `juce::dsp`.

## Sine bank (`Partials.h`)

Up to 512 partials plus two sub sines. Each partial is a complex rotator `(re, im) *= (cos w, sin w)`: four multiplies per sample, no `sin()`. One Newton step per block keeps the rotators on the unit circle. Start phases are fixed and scattered so big harmonic stacks do not spike. Partials at or above 0.48 x sample rate are muted, and silent partials are skipped. **Spread** pans odd harmonics left and even right; the fundamental and subs stay centred. **Economy** caps the count at 128.

## Slot 1: spectral stage (`Spectral.h`)

- **Distortion**: 18 spacing algorithms (Stiff String, Stretch, Compress, Bar, Membrane, Primes, Golden, Octaves, Fifths, Root, Log, Fold, Even Shift, Odd Shift, Scatter, Bell, Offset, Pairs). **Depth** blends from n x f0 to the algorithm in log-frequency. Tables are built once.
- **Tilt**: start amplitudes n^-tilt (1 = saw-like).
- **Spectral modulation**: each partial does its own random walk in pitch (up to 60 cents), at a rate around **Mod Rate**. The partials wander at slightly different speeds.
- **Spectral noise**: each partial's amplitude is multiplied by its own smoothed random signal, which turns the sine into a band of noise at the same frequency. **Width** sets the smoothing (bandwidth up to about ±375 Hz at 48 kHz), and the result is normalised to RMS 1. **Low/High Cut** shape which partials get noise. **Decay** makes the noise a burst after note-on (breath, chiff). **Mix** crossfades sines to noise.
- **Subs**: sines at f0/2 and f0/4, added after the module chain.

## Module pool (`Modules.h`)

One `processModule()` switch, per-voice state per slot. Knobs are all 0..1. A slot's **main** knob can be pushed by one envelope, one LFO, and one MIDI source (velocity, mod wheel, aftertouch, key, key x velocity).

| Group | Modules |
|------|---------|
| Basic | 5-Sines, Odd/Even, Sync (hump at the slave pitch), Adder, Discrete Adder (every Nth boosted, Discrete thins), Every Nth, Harmonic Add, Range Enhance, Clip |
| Filter | Lowpass / Highpass (6..48 dB/oct + resonant bump, key track), Bandpass, Notch, Moving Filter (notches on a sqrt-frequency scale, so they spread apart as they rise), Decay (high partials die faster), Hype (two shelves, ±18 dB at Amount 0.5) |
| Effect | Modulator (a travelling loudness wave up the partials), Detuned (alternating / random / growing detune plus beating), Blur (random level wander), Noise (fresh random level each block), Octaver (copies to 2n and 4n), Second Tone (half the partials become a second tone at an interval) |
| Rhythm | Phrase (8-step spectral arpeggio over the low partials, on the beat clock), Random Drops (random partials flare and fade), Tail (per-partial peak hold that rings down) |
| Wave | Wave (follow file A or B), Double Wave (A and B mixed by Balance) |

**Level guard**: if total spectral power after the chain is above 4, the voice is scaled down, so stacked boosts cannot blow up.

## Envelopes and LFOs (`Mod.h`)

Four ADSRs per voice: linear attack, exponential decay and release (down 60 dB after Release). Env 1 is the amp. The velocity setting decides how much velocity scales each envelope. Three global LFOs (sine, triangle, saw, square, S&H) run free or lock to the beat clock: host position while playing, otherwise free-running at host tempo (120 if none).

## Morph (`Morph.h`) and macros (`Engine.h`, `macro::apply`)

11 macros. Each one sits at 0.5 when neutral and pushes a fixed set of targets:

| Macro | Moves |
|------|------|
| Character | distortion depth, noise mix, Double Wave balance |
| Complexity | partial count (x1/4..x4), tilt |
| Tone | main knob of filter-type modules, tilt |
| Punch | amp attack (shorter), decay, velocity |
| Length | amp decay, release, sustain |
| Time FX | main knob of Phrase, Random Drops, Tail, Decay, Modulator, Blur |
| Depth | all route amounts (x0..x2), spectral mod depth |
| Speed | LFO rates, spectral mod rate |
| FX Mix | every effect mix (processor) |
| Width | partial spread, master width |
| User | main knob of the slot picked in "User Macro Slot" |

Macro value = morph-pad blend of the four corners (bilinear), or 0.5 if the macro is set to **Free**, plus the automatable **offset**. The puck moves by hand (**Manual**), around a closed node path (**Path**, one bar per Time seconds; **Path Sync**, bar lengths on the beat clock), or as an envelope (**X-Axis Env**: X sweeps 0 to 1 over Time after a note starts from silence, Y is read off the path as a curve, Snap steps it).

## Wave analysis (`WaveAnalyzer.h`, `Wave.h`)

On a worker thread: mono mixdown (max 120 s), Hann 2048-point FFT every 1024 samples, loudest bin per band into 256 log bands (20 Hz..20 kHz), peak-normalised, at most 2048 frames. The audio thread gets a raw pointer through an atomic. The replaced data is freed on the message thread after two more audio blocks. Per voice, each wave plays through its Start..End region at Speed (0.25x..4x) and loops.

## Cost

The sine sum dominates. At 48 kHz a 130 Hz note has about 176 audible partials, and a 520 Hz note about 44. Spectral work runs about 750 times a second per voice. Use Economy (128 partials) on big chords if CPU is tight.
