# Morph

![Morph page](morph.png)

Eleven macros, each a “move a set of things at once.” The XY pad blends four snapshots of those macros. The teal puck is where you actually are. Drag it. Store corners. Get lost in a nice way.

## Macros

Each column:

- Group name (Sound / Dynamics / Modulation / Master / User).
- Macro name.
- **Meter** (teal) — live value after the pad and the offset. Neutral is 0.5.
- **Offset** slider — automatable, −0.5…+0.5, added on top. The DAW sees these as “Character Offset” and so on.
- **Free** — take this macro out of the pad. It sits at 0.5 plus the offset, so you can automate it without the puck fighting you.

What each macro actually moves (from the centre):

| Macro | Moves |
|---|---|
| Character | Spectral Depth, Noise mix, Double Wave / Wave-page Balance |
| Complexity | Partials count (×¼…×4), Tilt |
| Tone | Main knob of Lowpass, Highpass, Bandpass, Notch, Hype, Moving Filter, Sync; also Tilt |
| Punch | Amp attack shorter, amp decay, amp Velocity |
| Length | Amp decay, release, sustain |
| Time FX | Main knob of Phrase, Random Drops, Tail, Decay, Modulator, Blur |
| Depth | Spectral Mod Depth, and all slot route amounts (×0…×2) |
| Speed | Spectral Mod Rate, all LFO rates |
| FX Mix | Drive / Mod FX / Delay / Reverb mix |
| Width | Spread and master Width |
| User | Main knob of the slot in **User Macro Slot** (slots 2–11) |

## Corners and the pad

A is bottom-left, B bottom-right, C top-left, D top-right. The four snapshots mix bilinearly under the puck.

**Store A / B / C / D** — save what you hear into that corner, zero the offsets, jump the puck there. The sound does not jump, which is the whole point of a morph pad.

Click a corner letter to recall it (moves Morph X / Morph Y, which the host can also automate).

## Modes

- **Manual** — drag the puck.
- **Path** — the puck travels a closed loop through the nodes. **Time** is seconds per bar of path.
- **Path Sync** — same loop, but bar lengths follow the host tempo.
- **X-Axis Env** — each new note from silence sweeps the puck left to right over **Time** seconds. Height is read off the path as a curve (nodes sorted by X). **Snap** (`Off`, `4`, `8`, `16`, `32`) steps that sweep for rhythmic morphs.

## Path editing

**Edit Path** — click to add a node (up to 16), drag to move, right-click to delete. Select a node and set **segment** (`1/4 bar` … `8 bars`): how long the trip **from this node to the next** lasts.

**Clear Path** removes every node. Path modes with fewer than two nodes sit still, which is less exciting than it sounds.

## Save / load

Files are `*.v4jbmorph` under `Documents\V4JB\Morph`.

- **Save Points…** — the four corners.
- **Save Path…** — the nodes.
- **Load…** — a points file leaves the path alone, and a path file leaves the corners alone. They stay out of each other’s way.
