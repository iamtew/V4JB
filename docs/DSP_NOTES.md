# V4JB – DSP notes

Where the texture engine gets designed.

## Built: Swirl (`src/dsp/Swirl.h`)

One mono voice, played by holding a MIDI note (last note wins, short 303-style slide between notes). Stereo comes from the whirl pan. Header-only and JUCE-free; `tests/swirl_check.cpp` renders it.

- **Wind**: white noise through a TPT state-variable bandpass. The centre wanders by random walks (the "ups and downs") and brightens as the whirl swings past.
- **Acid**: PolyBLEP saw at the note pitch into a 4-pole ladder with tanh feedback. Random **blurts** zap the cutoff up and bend the pitch down as they decay (80–300 ms each).
- **Cracks and pops**: Dust. Rare random impulses; each one is a click (crack) and retunes a high-Q resonator (pop).
- **Whirl**: equal-power pan circling at a drifting rate, width grows with Motion.
- **Gate**: one-pole attack (~30 ms) and release (~0.32 s time constant, silent inside the 3 s tail).
- **Limiter**: tanh on the output. Transparent at fog levels, grit when cranked.

Macros, all 0–1:

| Knob | Does |
|------|------|
| Motion | sweep speed and depth, pitch ups and downs, whirl speed and width |
| Crackle | pop density (up to ~60/s) and level |
| Acid | blurt rate (up to ~3/s), ladder resonance and drive |
| Air | noise vs saw balance, brightness |

Defaults (Motion 0.3, Air 0.7, rest 0) are the fog.

## Ideas still on the table

- **Granular cloud**: grains read from an internal oscillator buffer.
- **Big space**: FDN reverb with a freeze or infinite mode as the glue.
- **Polyphony**: Swirl is mono today.

## Rules

- Real-time safe: allocate in `prepareToPlay`, never in `processBlock`.
- Every non-trivial DSP block gets one runnable check in `tests/`.
