# V4JB – DSP notes

Where the texture engine gets designed.

## Built: Swirl (`src/dsp/Swirl.h`)

One mono voice, played by holding a MIDI note (last note wins, short 303-style slide between notes). Stereo comes from the whirl pan. Header-only and JUCE-free; `tests/swirl_check.cpp` renders it.

- **Wind**: white noise through a TPT state-variable bandpass. The centre wanders by random walks (the "ups and downs") and brightens as the whirl swings past.
- **Acid**: PolyBLEP saw at the note pitch into a 4-pole ladder with tanh feedback. Random **blurts** zap the cutoff up and bend the pitch down as they decay (80–300 ms each).
- **Cracks and pops**: Dust. Rare random impulses; each one is a click (crack) and retunes a high-Q resonator (pop).
- **Whirl**: equal-power pan circling at a drifting rate, width grows with Motion.
- **Ghosts**: three choir voices (centre, left, right), each with its own PolyBLEP saw at the note (detuned up to +-30 cents) mixed with noise, into three parallel TPT bandpasses at the vowel's formants F1-F3 (Q 8, gains 1 / 0.5 / 0.25). The vowel table (u-o-a-e-i, adult averages) is blended in log-frequency. Side voices get a slightly bigger or smaller "throat" (formants shifted up to +-0.08 octave). Each voice has its own random walks for vowel wander and amplitude swell. Ghosts that are faded out by the Ghost knob are skipped entirely.
- **Gate**: one-pole attack (~30 ms) and release (~0.32 s time constant, silent inside the 3 s tail). Shared by fog and ghosts.
- **Limiter**: tanh on the output. Transparent at fog levels, grit when cranked.

Macros, all 0–1:

| Knob | Does |
|------|------|
| Motion | sweep speed and depth, pitch ups and downs, whirl speed and width |
| Crackle | pop density (up to ~60/s) and level |
| Acid | blurt rate (up to ~3/s), ladder resonance and drive |
| Air | noise vs saw balance, brightness |
| Ghost | ghost level; voices fade in one by one (centre, then L and R), pan spread widens |
| Vowel | u-o-a-e-i, dark to bright |
| Breath | ghost excitation: sung saw at 0, whispered noise at 1 |
| Haunt | vowel wander depth and speed, swell depth (to silence at 1), choir detune, throat spread |

Defaults (Motion 0.3, Air 0.7, Ghost 0, rest of the fog knobs 0) are the fog. Vowel 0.5, Breath 0.3 and Haunt 0.4 are waiting for Ghost to come up.

## Ideas still on the table

- **Granular cloud**: grains read from an internal oscillator buffer.
- **Big space**: FDN reverb with a freeze or infinite mode as the glue.
- **Polyphony**: Swirl is mono today.

## Rules

- Real-time safe: allocate in `prepareToPlay`, never in `processBlock`.
- Every non-trivial DSP block gets one runnable check in `tests/`.
