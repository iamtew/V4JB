# Wave

![Wave page](wave.png)

Two files, A and B. A Wave module follows one of them; Double Wave follows both. **Nothing here makes sound by itself** — put a Wave or Double Wave module in a slot on Edit, or this page is just a very pretty spectrogram.

Each panel:

- Spectrogram: time left to right, frequency bottom to top, brightness = level. Amber lines are **Start** and **End**.
- **Load…** — built-ins first (**bells**, **chords**, **rain**, **vowels**), then **File…** (`wav`, `aif`/`aiff`, `flac`, `ogg`, `mp3`). Analysis runs on a worker thread (mono mix, up to 120 s), so the audio thread does not have to think about your holiday recordings.
- **Clear**.
- **Start / End** — region in the file. **End before Start plays backwards.** Yes, on purpose.
- **Speed** — 0.25× to 4×. 50% is original speed. The file loops in that region.

**Balance** under the two panels is the A↔B mix for Double Wave (Character also moves it).

Built-in waves are remembered by name in presets. Your own files are remembered by absolute path; if you move the file, load it again. Factory presets in the Waves category bring the built-ins they need.

On Edit, Wave **Freeze** holds the Start frame. Double Wave **Balance** is this page’s Balance plus the module’s own Balance plus Character. Three cooks, one soup; it usually tastes fine.
