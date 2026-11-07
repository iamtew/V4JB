# Edit

This is the sound. Slot 1 and slot 12 sit on the top row. Slots 2–11 are two rows of five: 2–6 then 7–11, left to right. Think of it as a little factory line for sine waves.

## Spectrum bars

Each card’s bar graph is a tap after that stage. Play a note and they fill. They are a picture of the last voice that started, not a live analyser of the mix, so they will not dance along with a chord. They will, however, tell you whether that Lowpass actually did anything.

## Slot 1 — Spectral

Fixed. This is the starting spectrum before any module sees it.

**Algorithm menu** (labelled Distortion in the host, because hosts have opinions): the spacing of the partials. **Depth 0** is a pure harmonic series (`n ×` the note). **Depth 1** is the full algorithm. The blend is in pitch (log-frequency), so 0.5 is halfway in octaves, not in Hertz. This is where metallic, bell, and alien sounds come from.

| Algorithm | What the partials do | Use when |
|---|---|---|
| Stiff String | Slight piano-like stretch that grows with partial number | Acoustic, inharmonic highs |
| Stretch | All ratios pulled sharp (`n^1.1`) | Brighter clang |
| Compress | All ratios pulled flat (`n^0.88`) | Crowded, nasal |
| Bar | Free-bar modes ~`(2n+1)²` | Wood, mallet, xylophone |
| Membrane | Drum-head modes | Toms, skins |
| Primes | Prime ratios (1, 1.5, 2.5, 3.5…) | Bell-ish, not quite harmonic |
| Golden | Golden-ratio spacing | Alien stack |
| Octaves | Energy piled on octaves, slight chorus | Organ, power |
| Fifths | Stacked fifths | Open, hollow |
| Root | Dense low, more harmonic high | Drones with a root |
| Log | Crowded, inharmonic | Fog, sci-fi |
| Fold | First 24 harmonic, then folds back | Clustered mid |
| Even Shift | Even partials sharp | Beating evens |
| Odd Shift | Odd partials (above the fundamental) flat | Beating odds |
| Scatter | Fixed random scatter around harmonics | Dirty, broken harmonic |
| Bell | Church-bell ratios, hum below the strike | Struck metal |
| Offset | All overtones shifted by a constant | Clang, ring-mod flavour |
| Pairs | Adjacent partials slightly sharp/flat | Slow beating pairs |

**Knobs**

- **Depth** — 0 = harmonic series, 1 = full algorithm.
- **Tilt** — starting amplitude roll-off, `n^-tilt`. 0 is a flat buzz, ~1.2 is saw-like, 2+ is dark and soft. Range 0–3.
- **Partials** — how many sines, 1–512 (Economy still caps at 128). More highs, more CPU. The usual deal.
- **Mod Depth** — each partial drifts in pitch on its own, up to about 60 cents. Shimmer.
- **Mod Rate** — how fast that wander is, roughly 0.05–20 Hz. Partials do not all move at the same speed, which is the point.
- **Noise** — crossfade from sine partials to noise partials. Each partial becomes a band of noise at the same frequency.
- **Width** (noise) — how wide that band is.
- **Low Cut / High Cut** — which partials get noise (highpass / lowpass of the noise, 20 Hz–20 kHz).
- **N. Decay** — 0 = noise stays on. Above 0 the noise is a breathy burst at note-on (chiff, breath).
- **Sub -1 / Sub -2** — clean sines one and two octaves below the note. Added **after** the module chain, so filters do not eat them. Kind of them, really.

## Slots 2–11 — module cards

Each card:

1. Slot number.
2. **Module menu**.
3. **MOD** — flips the lower half to modulation routes.
4. Spectrum bars after this slot.
5. Big **main** knob (name changes with the module).
6. Up to four extra sliders. Unused ones hide, which is nicer than a row of dummy knobs.

**Off** is a no-op: the spectrum passes through unchanged. Useful when you want a gap in the chain without rearranging everything.

**MOD** (routes push **only the main knob**, not the extra sliders):

- One **envelope** source: Off, Env 1–4. Amount bar is bipolar; centre = none, right adds, left subtracts.
- One **LFO**: Off, LFO 1–3. Same bipolar amount.
- One **MIDI** source: Off, Velocity, Mod Wheel, Aftertouch, Key, Key+Vel. Key is 0 at C2 and 1 at C7. Key+Vel is the product.

Env 1 is still the loudness of the note even if you also route it here. Two jobs, one envelope, no extra paperwork.

The three amounts can pile up; the engine then clamps the main knob to 0–1. The **Depth** morph macro scales all three amounts.

## Module catalogue

All extra knobs are 0–100% unless noted. “Use when” is a hint, not a rule. Please ignore it the moment it gets in the way.

### Basic shapes

**5-Sines** — organ register. Main **Mask** fades everything above partial 5 away. **Sine 2–5** set those partials’ levels relative to the fundamental.

**Odd/Even** — **Balance** 0 = odd only (square-ish), 0.5 = all, 1 = even only. **Keep Fund.** above 50% leaves the fundamental alone.

**Sync** — hard-sync flavour: a formant hump that sweeps up the partials. **Sync** is the slave ratio. **Resonance** is the bump height. **Bright** tilts the rest of the spectrum.

**Adder** — boost a Gaussian lump of partials. **Center** picks which, **Width** how many, **Amount** how much.

**Discrete Adder** — boost every **Step**-th partial from **Offset**, cut the rest. **Discrete** thins the boosted set (every other boosted partial drops back) for sparse metal. **Amount** is the cut/boost.

**Every Nth** — keep every **N**th partial from **Offset**; **Depth** is how hard the others are cut.

**Harmonic Add** — adds four harmonics of a chosen **Base** partial (base, 2×, 3×, 4×). **Level 1–4** are those four.

**Range Enhance** — a bell EQ across partial numbers. **Center** is which partial (1–512), **Width** in octaves, **Amount** ±24 dB at centre.

**Clip** — no partial may be louder than **Threshold** below the peak (flattens the spectrum). **Makeup** restores level.

### Filters and tone

**Lowpass / Highpass** — Butterworth-ish slope plus a resonant bump. **Cutoff** is the main knob (key-trackable). **Slope** 6–48 dB/oct. **Resonance**. **Key Track** 1 = cutoff follows the note (referenced to C4).

**Bandpass** — **Center**, **Width**, **Resonance**, **Key Track**.

**Notch** — **Center**, **Width**, **Depth** (how deep the hole), **Key Track**.

**Moving Filter** — a comb of notches on a sqrt-frequency scale, so they spread apart as they rise (a phaser does the opposite in Hz). **Position** slides the comb. **Density** how many. **Rate** how fast it crawls. **Depth** how deep.

**Decay** — higher partials die faster, like a plucked string. **Rate**, **Tilt** (how much faster the highs go), **Floor** (what remains).

**Hype** — two shelves, low and high. **Amount** scales both (at 50% you get ±18 dB, at 100% ±36 dB). **Low Freq / Low Gain**, **High Freq / High Gain**. Instant air or body, depending on your mood and the mix.

### Motion and doubling

**Modulator** — a travelling wave of loudness that rolls up through the partials. **Depth**, **Rate**, **Period** (how many partials per bump), **Sharp**.

**Detuned** — unison emulation. **Detune** up to 30 cents. **Style**: alternating sharp/flat, random per partial, or growing with partial number. **Beat** adds the slow amplitude beating you would hear between two detuned voices.

**Blur** — every partial’s level wanders. Fuzzy, out-of-focus. **Amount**, **Speed**.

**Noise** — a fresh random level per partial every control block. Grainy, hissy. **Amount**, **Color** (tilts the grain toward low or high partials).

**Octaver** — copies the spectrum up one and two octaves (partial `n` feeds `2n` and `4n`). **Up 1 Oct** is the main knob, **Up 2 Oct**, **Dry**.

**Second Tone** — splits the bank: even slots keep the first tone, odd slots become a second tone. **Interval** steps through a unison-ish detune, minor 3rd, major 3rd, 4th, 5th, octave, octave+5th, two octaves. **Level** of the second tone. **Detune** a few extra cents.

### Rhythm

**Phrase** — spectral arpeggio on the beat clock (host position while playing, otherwise free-running at host tempo, 120 if none). Lights one of the low partials each step.

- **Pattern**: up, down, up-and-down, scattered, pulse, chord, trill, climb (8 steps).
- **Rate**: 1, 2, 4 or 8 steps per beat.
- **Depth**: how far unlit partials duck.
- **Partials**: how many of the lowest partials are in the game.
- **Smooth**: how fast it crossfades between steps.

**Random Drops** — random partials flare and fade (rain on tin). **Density**, **Decay**, **Floor**, **Range** (how high the drops reach), **Low Bias** (favour low partials).

**Tail** — each partial holds its peak and rings down: a spectral reverb that fills gaps left by Phrase or Drops. **Amount**, **Time**, **Damping** (highs die faster).

### Wave followers

These need files on the [Wave page](wave.md). With nothing loaded they do nothing, which is honest of them.

**Wave** — each partial’s level follows how loud that frequency is in the file.

- **Amount** — dry spectrum vs followed.
- **Shift** — slide the file’s spectrum up or down in octaves against the partials.
- **Contrast** — exaggerate or flatten the file’s peaks.
- **Source B** — above 50% follow Wave B instead of A.
- **Freeze** — above 50% hold the **Start** frame instead of playing.

**Double Wave** — same, but A and B mixed.

- **Amount**, **Balance** (0 = A, 1 = B; the Wave page Balance and the Character macro also move this), **Shift**, **Contrast**, **Freeze**.

## Slot 12 — Effects & Master

Audio after the sine bank. Each effect has its own **On**. Mix knobs are also pushed by the **FX Mix** morph macro; **Width** is pushed by the **Width** macro.

**Drive** — tanh saturator. **Drive** is gain into the curve. **Mix**.

**Mod** — **Chorus** or **Phaser**. **Rate** 0.01–10 Hz, **Depth**, **Mix**.

**Delay** — ping-pong (each side’s echo feeds the other). **Sync** uses **Division** (`1/16`, `1/8`, `1/8 dot`, `1/4`, `1/4 dot`, `1/2`, `1 bar`) against host tempo. Off uses **Time** 10–2000 ms. **Feedback** 0–0.95. **Mix**. Turning it off and on again clears stale echoes, which is the polite way to stop a delay that has been thinking too hard.

**Reverb** — **Size**, **Damping**, **Mix**.

**Master** — **Width** 0 = mono, 1 = natural, 2 = extra wide (M/S). **Spread** pans odd partials one way and even the other (fundamental and subs stay centred). This Spread is applied in the sine bank, not here, but the knob lives on this card because that is where you go looking for “make it wider.”

A safety limiter sits after this chain, then Volume.
