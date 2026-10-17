# V4JB – DSP notes

Where the texture engine gets designed. Nothing below is built yet.

## Ideas on the table

- **Filtered noise**: noise through a resonant filter bank. Wind, air, tonal hiss.
- **Drifting pad**: detuned unison or wavetable voices with slow random drift LFOs.
- **Granular cloud**: grains read from an internal oscillator buffer.
- **Big space**: FDN reverb with a freeze or infinite mode as the glue.
- **Macros**: a few knobs (Mood, Motion, Air, Depth) mapped across the engines.

## Rules

- Real-time safe: allocate in `prepareToPlay`, never in `processBlock`.
- Every non-trivial DSP block gets one runnable check in `tests/`.
