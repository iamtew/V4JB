# Playing and MIDI

Nothing fancy, just the usual polite MIDI manners.

- **Voices** 1–8 (host name **Polyphony**). When full, steal the oldest released voice, else the oldest held. Same key retriggers that voice.
- **Mono (Voices = 1)** — last note wins. Overlapping notes glide and do not restart envelopes.
- **Glide** — knob 0–2; time constant is 0.3 × that value. Poly slides from the last started note.
- **Pitch bend** — ±2 semitones.
- **Mod wheel / aftertouch / velocity / Key / Key+Vel** — only if a slot’s MOD MIDI source is set. Aftertouch is one global value (channel and poly). Until then they sit there looking decorative.
- **Economy** — 128-partial cap. Use it on big chords if the CPU is tight.
- Standalone keys: C2–C6, mouse only (no QWERTY map). Host MIDI still works, so you do not have to click a tiny piano like it is 1998.
