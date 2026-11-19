# Playing and MIDI

Nothing fancy, just the usual polite MIDI manners.

- **Voices** 1–8 (host name **Polyphony**). When full, steal the oldest released voice, else the oldest held. Same key retriggers that voice.
- **Mono (Voices = 1)** — last note wins. Overlapping notes glide and do not restart envelopes.
- **Glide** — knob 0–2; time constant is 0.3 × that value. Poly slides from the last started note.
- **Pitch bend** — ±2 semitones.
- **Sustain (CC 64)** — held notes keep sounding until the pedal lifts.
- **All Notes Off** — envelopes go to release. **All Sound Off** (panic) cuts voices, delay, and reverb immediately.
- **Mod wheel / aftertouch / velocity / Key / Key+Vel** — only if a slot’s MOD MIDI source is set. Aftertouch is one global value (channel and poly). Until then they sit there looking decorative.
- **Economy** — 128-partial cap. Use it on big chords if the CPU is tight.
- Standalone keys: C2–C6 on screen. Computer keys play C4–E5 while the window is focused (JUCE map: A is C, W is C#, home row and the row above). Clicking a knob does not steal them. Host MIDI still works.
