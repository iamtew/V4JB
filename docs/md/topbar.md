# Top bar

Left to right, because that is how the window is built and also how most of us read.

- **Logo + V4JB / version** — click for [About](about.md).
- **Presets** — the current name (`category · name`). Click it for a popup with a name filter on top. Nothing typed: grouped by category, factory first. Typing keeps names that contain the text, shown as `category · name`. **Save preset…** and **Open user preset folder** sit at the bottom. `<` and `>` still step the current category, filter or not.
- **Save** — writes a user preset to `Documents\V4JB\Presets` as `Name.xml`. You get a name and a category (default **User**). Factory presets stay in the plugin; user presets show up in the menu next time.
- **Randomize** — two choices:
  - **New sound (new module chain)** — picks a short chain from a curated pool (Wave / Double Wave only if a file is loaded; at most one Double Wave) and new values.
  - **Vary values (keep modules)** — same slots, new numbers, still inside ranges that tend to sound like music rather than a fax machine.
- **Economy** — at most 128 partials per voice. Use it on big chords if the CPU is tight.
- **Voices** — 1 to 8 (host name **Polyphony**). **1 is mono**: last-note priority, legato notes glide without restarting envelopes. 2–8 are polyphonic.
- **Octave** — −3 to +3 octaves on the whole instrument.
- **Pitch** — −12 to +12 semitones (fine transpose, on top of MIDI).
- **Glide** — knob 0–2. The slide’s time constant is 0.3 × that value (about 0.6 s at the top), not a 2-second ramp. In mono, held notes slide from the last pitch. In poly it slides from the last started note.
- **Volume** — output level after the limiter.
