#pragma once

// The additive engine: up to 8 voices, each a sine bank fed by
//   slot 1 (spectral stage) -> slots 2..11 (module chain) -> amp envelope.
// Slot 12 (effects and master) lives in PluginProcessor, it needs juce::dsp.
//
// Once per control block (kCtl samples) every voice rebuilds its spectrum; in
// between, the banks just sum sines. render() never allocates: every buffer is
// a fixed-size member, and the engine itself lives on the heap inside the processor.

#include "Mod.h"
#include "Modules.h"
#include "Partials.h"
#include "Spectral.h"

#include <atomic>

namespace v4jb
{
inline constexpr const char* kEnvSelNames[5]  = { "Off", "Env 1", "Env 2", "Env 3", "Env 4" };
inline constexpr const char* kLfoSelNames[4]  = { "Off", "LFO 1", "LFO 2", "LFO 3" };
inline constexpr const char* kMidiSelNames[6] = { "Off", "Velocity", "Mod Wheel", "Aftertouch", "Key", "Key+Vel" };

// One user slot: the module and its knobs, plus three modulation routes
// (one envelope, one LFO, one MIDI source) that push the main knob around.
struct SlotParams
{
    ModuleParams mod;
    int envSel = 0;  float envAmt = 0.f;   // amounts -1..1
    int lfoSel = 0;  float lfoAmt = 0.f;
    int midiSel = 0; float midiAmt = 0.f;
};

struct WaveParams
{
    float start[2] { 0.f, 0.f }, end[2] { 1.f, 1.f };
    float speed[2] { 0.5f, 0.5f };  // 0..1 -> 0.25x..4x, 0.5 = original speed
    float balance = 0.5f;           // Double Wave: 0 = A, 1 = B
};

struct EngineParams
{
    SpectralParams spec;
    SlotParams slot[kNumSlots];
    EnvParams env[kNumEnvs];
    LfoParams lfo[kNumLfos];
    WaveParams wave;
    float pitch = 0.f;    // semitones
    int octave = 0;
    float glide = 0.f;    // seconds
    int poly = 8;         // 1 = mono legato
    bool economy = false;
    float spread = 0.4f;  // odd/even partial stereo spread
    int userSlot = 0;     // which slot's main knob the User macro moves
};

// Module types each macro reaches. The "smart" assignment: fixed by design.
namespace macro
{
inline bool isTone (int t)
{
    using M = ModuleType;
    switch ((M) t) { case M::Lowpass: case M::Highpass: case M::Bandpass: case M::Notch:
                     case M::Hype: case M::MovingFilter: case M::Sync: return true;
                     default: return false; }
}
inline bool isTime (int t)
{
    using M = ModuleType;
    switch ((M) t) { case M::Phrase: case M::RandomDrops: case M::Tail: case M::Decay:
                     case M::Modulator: case M::Blur: return true;
                     default: return false; }
}

// Macros sit at 0.5 when neutral; d = macro - 0.5 pushes each target up or down.
inline EngineParams apply (EngineParams e, const float* m)
{
    float d[kNumMacros];
    for (int i = 0; i < kNumMacros; ++i)
        d[i] = m[i] - 0.5f;

    auto& s = e.spec;
    s.depth    = clamp01 (s.depth + d[0] * 1.2f);                 // Character
    s.noiseMix = clamp01 (s.noiseMix + d[0] * 0.6f);
    e.wave.balance = clamp01 (e.wave.balance + d[0]);
    s.partials = std::clamp (s.partials * std::exp2 (d[1] * 4.f), 1.f, (float) kMaxPartials);  // Complexity
    s.tilt     = std::clamp (s.tilt - d[1] - d[2] * 0.6f, 0.f, 3.f);                         // + Tone
    s.modDepth = clamp01 (s.modDepth + d[6] * 0.8f);              // Depth
    s.modRate  = clamp01 (s.modRate + d[7]);                      // Speed

    auto& a = e.env[0];                                           // Punch, Length
    a.a   = std::clamp (a.a * std::exp2 (-d[3] * 6.f), 0.001f, 20.f);
    a.d   = std::clamp (a.d * std::exp2 (-d[3] * 2.f + d[4] * 4.f), 0.001f, 20.f);
    a.r   = std::clamp (a.r * std::exp2 (d[4] * 5.f), 0.001f, 20.f);
    a.s   = clamp01 (a.s + d[4] * 0.5f);
    a.vel = clamp01 (a.vel + d[3]);

    const float depthScale = 1.f + 2.f * d[6];
    for (int i = 0; i < kNumSlots; ++i)
    {
        auto& sl = e.slot[i];
        if (isTone (sl.mod.type)) sl.mod.main += d[2];           // Tone
        if (isTime (sl.mod.type)) sl.mod.main += d[5];           // Time FX
        if (i == e.userSlot)      sl.mod.main += d[10];          // User
        sl.mod.main = clamp01 (sl.mod.main);
        sl.envAmt *= depthScale;
        sl.lfoAmt *= depthScale;
        sl.midiAmt *= depthScale;
    }
    for (auto& l : e.lfo)
        l.rateHz *= std::exp2 (d[7] * 4.f);                       // Speed
    e.spread = clamp01 (e.spread + d[9]);                         // Width
    return e;                                                     // FX Mix: handled by the processor
}
} // namespace macro

class Engine
{
public:
    // UI spectrum taps: first kBars partial levels after each stage, for the
    // most recently started voice. Audio writes, UI reads (relaxed).
    std::atomic<float> taps[kNumStages][kBars];

    Engine()
    {
        for (auto& v : voices)
            v.resetAll();
        for (auto& stage : taps)
            for (auto& t : stage)
                t.store (0.f);
    }

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        invSr = 1.f / sr;
        dt = (float) kCtl * invSr;
        ctlPos = 0;
        for (auto& v : voices)
            v.resetAll();
        numHeld = 0;
    }

    // Once per host block, before render(). macros = the 11 morph macros.
    void setParams (const EngineParams& raw, const float* macros) { p = macro::apply (raw, macros); }
    void setWaves (const WaveData* a, const WaveData* b) { wave[0] = a; wave[1] = b; }

    // bpm always; beatsNow only when the host transport is playing (else we free-run).
    void setTransport (double bpmIn, bool playing, double beatsNow)
    {
        bpm = bpmIn > 0.0 ? bpmIn : 120.0;
        if (playing)
            beats = beatsNow;
    }

    void setPitchBend (float semis) { bend = semis; }
    void setModWheel (float v) { wheel = clamp01 (v); }
    void setAftertouch (float v) { touch = clamp01 (v); }

    void noteOn (int note, float vel)
    {
        bool anyHeld = false;
        for (const auto& v : voices)
            anyHeld = anyHeld || (v.active && v.held);
        if (! anyHeld)
            noteStarted = true;

        if (p.poly <= 1)
        {
            // Mono, last-note priority. Legato notes glide without restarting envelopes.
            removeHeld (note);
            held[std::min (numHeld, kMaxHeld - 1)] = note;
            numHeld = std::min (numHeld + 1, kMaxHeld);
            Voice& v = voices[0];
            for (int i = 1; i < kMaxVoices; ++i)
                release (voices[i]);
            if (v.active && v.held)
            {
                v.note = note;
                v.targetSemis = (float) note;
                lastSemis = (float) note;
                return;
            }
            start (v, note, vel);
            return;
        }

        const int poly = std::clamp (p.poly, 1, kMaxVoices);
        Voice* pick = nullptr;
        for (int i = 0; i < poly && pick == nullptr; ++i)
            if (voices[i].active && voices[i].note == note)
                pick = &voices[i];  // same key again: retrigger that voice
        for (int i = 0; i < poly && pick == nullptr; ++i)
            if (! voices[i].active)
                pick = &voices[i];
        if (pick == nullptr)
        {
            // Steal: the oldest released voice, else the oldest held one.
            for (int pass = 0; pass < 2 && pick == nullptr; ++pass)
                for (int i = 0; i < poly; ++i)
                {
                    Voice& v = voices[i];
                    if (pass == 0 && v.held)
                        continue;
                    if (pick == nullptr || v.age < pick->age)
                        pick = &v;
                }
        }
        start (*pick, note, vel);
    }

    void noteOff (int note)
    {
        if (p.poly <= 1)
        {
            removeHeld (note);
            Voice& v = voices[0];
            if (numHeld > 0)
            {
                v.note = held[numHeld - 1];
                v.targetSemis = (float) v.note;
                lastSemis = v.targetSemis;
            }
            else if (v.note == note)
                release (v);
            return;
        }
        for (auto& v : voices)
            if (v.active && v.held && v.note == note)
                release (v);
    }

    void allNotesOff()
    {
        numHeld = 0;
        for (auto& v : voices)
            release (v);
    }

    // Audio thread: true once after a note started from silence (restarts X-Axis morph).
    bool takeNoteStarted()
    {
        const bool s = noteStarted;
        noteStarted = false;
        return s;
    }

    double beatClock() const { return beats; }

    // Overwrites n samples of L and R (both required).
    void render (float* L, float* R, int n)
    {
        std::fill (L, L + n, 0.f);
        std::fill (R, R + n, 0.f);
        int done = 0;
        while (done < n)
        {
            if (ctlPos == 0)
                control();
            const int m = std::min (n - done, kCtl - ctlPos);
            for (auto& v : voices)
                if (v.active)
                    v.bank.render (L + done, R + done, m, p.spread);
            ctlPos = (ctlPos + m) % kCtl;
            done += m;
        }
        // Safety saturation: transparent at normal levels, keeps 8 stacked voices
        // and boosted modules from leaving +-1. The master limiter comes later.
        for (int i = 0; i < n; ++i)
        {
            L[i] = std::tanh (L[i]);
            R[i] = std::tanh (R[i]);
        }
    }

    // Test and UI helpers.
    int voiceNote (int i) const { return voices[i].active ? voices[i].note : -1; }
    bool voiceHeld (int i) const { return voices[i].active && voices[i].held; }
    int voicePartials (int i) const { return voices[i].bank.hi; }
    float voiceFreq (int i, int k) const { return voices[i].freq[k]; }
    float voiceAmp (int i, int k) const { return voices[i].amp[k]; }

private:
    static constexpr int kMaxHeld = 16;
    static constexpr float kVoiceGain = 0.15f;

    struct Voice
    {
        bool active = false, held = false, silentBlock = false;
        int note = 60;
        float vel = 0.8f;
        uint64_t age = 0;
        float semis = 60.f, targetSemis = 60.f;
        float t = 0.f;
        float wavePos[2] { 0.f, 0.f };  // 0..1 through each wave's region
        Env env[kNumEnvs];
        PartialBank bank;
        SpectralVoice spectral;
        ModuleState mods[kNumSlots];
        float freq[kBankSize] {};
        float amp[kBankSize] {};
        float target[kBankSize] {};

        void resetAll()
        {
            active = held = silentBlock = false;
            for (auto& e : env) e = {};
            bank.reset();
            resetState();
        }
        void resetState()
        {
            spectral.reset();
            for (auto& m : mods)
                m.reset();
            t = 0.f;
            wavePos[0] = wavePos[1] = 0.f;
        }
    };

    void start (Voice& v, int note, float vel)
    {
        if (! v.active)
            v.bank.reset();  // a stolen voice keeps its bank so the levels ramp, not jump
        v.resetState();
        v.active = v.held = true;
        v.silentBlock = false;
        v.note = note;
        v.vel = clamp01 (vel);
        v.age = ++ageCounter;
        v.targetSemis = (float) note;
        v.semis = (p.glide > 0.f && lastSemis >= 0.f) ? lastSemis : (float) note;
        lastSemis = (float) note;
        for (auto& e : v.env)
            e.on();
        display = (int) (&v - voices);
    }

    static void release (Voice& v)
    {
        v.held = false;
        for (auto& e : v.env)
            e.off();
    }

    void removeHeld (int note)
    {
        int w = 0;
        for (int i = 0; i < numHeld; ++i)
            if (held[i] != note)
                held[w++] = held[i];
        numHeld = w;
    }

    float midiValue (int sel, const Voice& v) const
    {
        const float key = clamp01 ((float) (v.note - 36) / 60.f);
        switch (sel)
        {
            case 1: return v.vel;
            case 2: return wheel;
            case 3: return touch;
            case 4: return key;
            case 5: return key * v.vel;
            default: return 0.f;
        }
    }

    void tap (int stage, const float* amp, int n)
    {
        for (int b = 0; b < kBars; ++b)
            taps[stage][b].store (b < n ? std::fabs (amp[b]) : 0.f, std::memory_order_relaxed);
    }

    // Once per kCtl samples: advance clocks and LFOs, rebuild every voice's spectrum.
    void control()
    {
        beats += (double) dt * bpm / 60.0;
        float lfo[kNumLfos];
        for (int i = 0; i < kNumLfos; ++i)
            lfo[i] = lfos[i].step (p.lfo[i], dt, beats, rng);

        const float glideCoef = p.glide <= 0.f ? 1.f : 1.f - std::exp (-dt / (p.glide * 0.3f));
        const int n = partialCount (p.spec, p.economy);

        for (int vi = 0; vi < kMaxVoices; ++vi)
        {
            Voice& v = voices[vi];
            if (! v.active)
                continue;

            float env[kNumEnvs];
            for (int i = 0; i < kNumEnvs; ++i)
                env[i] = v.env[i].step (p.env[i], dt) * (1.f - p.env[i].vel * (1.f - v.vel));

            // Amp envelope finished: ramp the bank to silence for one block, then free the voice.
            if (v.env[0].stage == Env::Idle)
            {
                if (v.silentBlock)
                {
                    v.active = false;
                    continue;
                }
                v.silentBlock = true;
            }

            v.semis += glideCoef * (v.targetSemis - v.semis);
            const float f0 = 440.f * std::exp2 ((v.semis + p.pitch + 12.f * (float) p.octave + bend - 69.f) / 12.f);
            v.t += dt;

            runSpectral (tables, p.spec, v.spectral, rng, f0, dt, v.t, n, v.freq, v.amp);
            const bool show = vi == display;
            if (show)
                tap (0, v.amp, n);

            ModuleContext ctx;
            ctx.f0 = f0;
            ctx.dt = dt;
            ctx.t = v.t;
            ctx.beats = beats;
            ctx.n = n;
            ctx.rng = &rng;
            ctx.waveBalance = p.wave.balance;
            for (int w = 0; w < 2; ++w)
            {
                ctx.wave[w] = wave[w];
                const float st = p.wave.start[w], en = p.wave.end[w];
                if (wave[w] != nullptr && wave[w]->seconds > 0.f)
                {
                    // Loop through the region; End before Start plays it backwards.
                    const float regionSec = std::max (std::fabs (en - st) * wave[w]->seconds, 0.01f);
                    v.wavePos[w] += dt * std::exp2 ((p.wave.speed[w] - 0.5f) * 4.f) / regionSec;
                    v.wavePos[w] -= std::floor (v.wavePos[w]);
                }
                ctx.wavePos[w] = lerp (st, en, v.wavePos[w]);
                ctx.waveStart[w] = st;
            }

            for (int s = 0; s < kNumSlots; ++s)
            {
                const SlotParams& sp = p.slot[s];
                if (sp.mod.type != (int) ModuleType::Off)
                {
                    ModuleParams mp = sp.mod;
                    if (sp.envSel > 0)  mp.main += sp.envAmt * env[sp.envSel - 1];
                    if (sp.lfoSel > 0)  mp.main += sp.lfoAmt * lfo[sp.lfoSel - 1];
                    if (sp.midiSel > 0) mp.main += sp.midiAmt * midiValue (sp.midiSel, v);
                    mp.main = clamp01 (mp.main);
                    processModule (mp, v.mods[s], ctx, v.freq, v.amp);
                }
                if (show)
                    tap (s + 1, v.amp, n);
            }

            // Level guard: boosting modules can pile up; cap the spectrum's total power.
            float power = 0.f;
            for (int k = 0; k < n; ++k)
                power += v.amp[k] * v.amp[k];
            const float guard = power > 4.f ? std::sqrt (4.f / power) : 1.f;

            const float gain = v.silentBlock ? 0.f : env[0] * kVoiceGain * guard;
            for (int k = 0; k < n; ++k)
                v.target[k] = v.amp[k] * gain;
            const float subGain = v.silentBlock ? 0.f : env[0] * kVoiceGain;
            v.freq[kSub1] = f0 * 0.5f;
            v.freq[kSub2] = f0 * 0.25f;
            v.target[kSub1] = clamp01 (p.spec.sub1) * subGain;
            v.target[kSub2] = clamp01 (p.spec.sub2) * subGain;
            v.bank.setTargets (v.freq, v.target, n, invSr);
        }
    }

    EngineParams p;
    SpectralTables tables;
    Voice voices[kMaxVoices];
    Lfo lfos[kNumLfos];
    Rng rng;
    const WaveData* wave[2] { nullptr, nullptr };

    float sr = 48000.f, invSr = 1.f / 48000.f, dt = (float) kCtl / 48000.f;
    int ctlPos = 0;
    double bpm = 120.0, beats = 0.0;
    float bend = 0.f, wheel = 0.f, touch = 0.f;
    float lastSemis = -1.f;
    uint64_t ageCounter = 0;
    int display = 0;
    bool noteStarted = false;
    int held[kMaxHeld] {};
    int numHeld = 0;
};
} // namespace v4jb
