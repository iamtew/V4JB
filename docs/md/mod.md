# Mod

![Mod page](mod.png)

Four envelopes on top, three LFOs below. The footer is the rule, and it is a short one: **each module slot can route one envelope, one LFO and one MIDI source to its main knob** (MOD on the slot card). That is the whole matrix. No secret fifth bus.

## Envelopes

Per voice. **Env 1 (AMP)** is always the volume of the note. Env 2–4 exist to be routed, which is a nicer job than it sounds.

- **Attack** — 1 ms–10 s, linear, from the current level (a retrigger never clicks).
- **Decay** — 1 ms–10 s, exponential toward Sustain.
- **Sustain** — level held while the key is down.
- **Release** — 1 ms–10 s, exponential; about 60 dB down after this time, then idle.
- **Velocity** — how much MIDI velocity scales this envelope. 0 = ignore velocity.

## LFOs

Global, shared by all voices. Bipolar (−1…+1), so they can push a knob up *and* down, which is half the fun.

- **Rate** — 0.01–20 Hz when Sync is off.
- **Shape** — Sine, Triangle, Saw, Square, S&H (new random value once per cycle).
- **Sync** — lock to the beat clock instead of Rate.
- **Division** when synced: `1/16`, `1/8`, `1/4`, `1/2`, `1 bar`, `2 bars`, `4 bars`, `8 bars`.

While the host plays, the clock is the transport. Otherwise it free-runs at the host tempo (120 BPM if the host does not send one). Phrase on the Edit page uses the same clock, so they can stay friends.
