#pragma once

// Swirl: V4JB's texture voice. Fog at low settings, whirlwinds, pops and acid
// blurts at high settings. One mono voice; the stereo comes from the whirl pan.
// Ghosts: a small choir of formant voices that whisper or sing vowels inside the fog.
//
// Header-only and JUCE-free so tests/swirl_check.cpp can run it with plain cl.
// Real-time safe: all state is fixed-size members, render() never allocates.

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace v4jb
{
// The macro knobs, each 0..1. Defaults are the "fog" sound, ghosts silent.
struct Macros
{
    float motion  = 0.3f;  // sweep speed/depth, ups and downs, whirl speed
    float crackle = 0.f;   // density and level of cracks and pops
    float acid    = 0.f;   // blurt rate, ladder resonance, drive
    float air     = 0.7f;  // noise vs saw balance, brightness
    float ghost   = 0.f;   // ghost level and how many voices (1 centre, then L and R)
    float vowel   = 0.5f;  // u-o-a-e-i, dark to bright
    float breath  = 0.3f;  // sung (saw) to whispered (noise)
    float haunt   = 0.4f;  // vowel wander, swelling in and out, choir detune
};

class Swirl
{
public:
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        invSr = 1.f / sr;
        maxHz = 0.45f * sr;
        smooth      = onePole (20.f);
        gateAttack  = onePole (5.f);
        gateRelease = onePole (0.5f);  // ~0.32 s time constant; silent inside the 3 s tail we report
        pitchGlide  = onePole (8.f);

        // Fresh audio state; macro targets survive so the host's values stay.
        cur = target;
        rng = {};
        sweepDrift = pitchDrift = whirlDrift = {};
        sweep = pops = {};
        ladder = {};
        for (auto& g : ghosts)
            g = {};
        whirlPhase = sawPhase = blurt = gate = 0.f;
        hz = noteHz;
        gateOn = false;
        numHeld = 0;
    }

    void setMacros (const Macros& m) { target = m; }

    // Last-note priority. The held list is fixed-size so note on/off never allocates.
    void noteOn (int note)
    {
        removeHeld (note);
        held[std::min (numHeld, kMaxHeld - 1)] = note;
        numHeld = std::min (numHeld + 1, kMaxHeld);
        setNote (note);
        gateOn = true;
        fireBlurt();
    }

    void noteOff (int note)
    {
        removeHeld (note);
        if (numHeld > 0)
            setNote (held[numHeld - 1]);
        else
            gateOn = false;
    }

    void allNotesOff()
    {
        numHeld = 0;
        gateOn = false;
    }

    // Overwrites (does not add to) n samples of L and R. Pass R = nullptr for mono out.
    // Call prepare() first.
    void render (float* L, float* R, int n)
    {
        constexpr float kTwoPi = 6.2831853f;
        constexpr float kHalfPi = 1.5707963f;

        for (int i = 0; i < n; ++i)
        {
            // Macros glide (~8 ms) so knob moves never click.
            cur.motion  += smooth * (target.motion  - cur.motion);
            cur.crackle += smooth * (target.crackle - cur.crackle);
            cur.acid    += smooth * (target.acid    - cur.acid);
            cur.air     += smooth * (target.air     - cur.air);
            cur.ghost   += smooth * (target.ghost   - cur.ghost);
            cur.vowel   += smooth * (target.vowel   - cur.vowel);
            cur.breath  += smooth * (target.breath  - cur.breath);
            cur.haunt   += smooth * (target.haunt   - cur.haunt);
            const float motion = cur.motion, crackle = cur.crackle, acid = cur.acid, air = cur.air;

            // Ups and downs: three random walks. Motion makes them jump more often and glide faster.
            const float jumpProb = (0.2f + 6.f * motion) * invSr;
            const float glide = onePole (0.05f + 3.f * motion);
            const float dSweep = sweepDrift.step (rng, jumpProb, glide);
            const float dPitch = pitchDrift.step (rng, jumpProb, glide);
            const float dWhirl = whirlDrift.step (rng, jumpProb, glide);

            // Whirl: pan position circles at a drifting rate. Computed first because
            // the sweep brightens as the sound swings past, like wind going by.
            whirlPhase += (0.05f + 4.f * motion * motion) * (1.f + 0.5f * dWhirl) * invSr;
            whirlPhase -= std::floor (whirlPhase);
            const float swing = std::sin (kTwoPi * whirlPhase);

            // Sweep: white noise through a bandpass whose centre wanders by up to +-4 octaves.
            const float centre = clampHz (300.f * std::exp2 (air * 3.f + dSweep * (1.f + 3.f * motion)
                                                             + 0.5f * motion * swing));
            sweep.setCutoff (centre, 0.7f + 4.f * motion, invSr);
            const float wind = sweep.band (rng.bi()) * sweep.k;  // * k = unity gain at the peak

            // Acid blurts: random squelches, more often with Acid. Each one zaps the
            // ladder cutoff up and bends the pitch down as it decays. That is the fart.
            if (rng.next() < acid * 3.f * invSr)
                fireBlurt();
            blurt *= blurtDecay;

            hz += pitchGlide * (noteHz - hz);  // short slide between notes, 303 style
            const float pitch = std::min (hz * std::exp2 (dPitch * motion * motion + blurt * blurtDrop * acid), maxHz);
            const float dt = pitch * invSr;
            sawPhase += dt;
            sawPhase -= std::floor (sawPhase);
            const float saw = 2.f * sawPhase - 1.f - polyBlep (sawPhase, dt);

            const float ladderHz = clampHz (hz * std::exp2 (1.f + air * 1.5f + blurt * acid * 7.f + dSweep * motion));
            const float acidOut = ladder.process (saw, ladderHz * kTwoPi * invSr,
                                                  4.f * (0.2f + 0.75f * acid), 1.f + 4.f * acid);

            // Cracks and pops (Dust): rare random impulses. Each one retunes a
            // high-Q resonator so pops ping at different pitches; the raw click is the crack.
            float click = 0.f;
            if (rng.next() < crackle * crackle * 60.f * invSr)
            {
                click = rng.bi();
                pops.setCutoff (300.f * std::exp2 (rng.next() * 4.3f), 8.f, invSr);
            }
            const float pop = pops.band (click);

            float mono = 2.f * air * wind  // bandpassed noise is thin; 2x brings fog up to pad level
                       + ((1.f - air) * 0.5f + acid * blurt) * acidOut
                       + crackle * (2.f * pop + 0.5f * click);

            // Ghosts: each voice is a saw/noise excitation through three formant
            // bandpasses (F1-F3) that shape it into a vowel. Ghost fades the voices in
            // one by one (centre first, then left and right).
            float ghostL = 0.f, ghostR = 0.f;
            const float ghost = cur.ghost, vowel = cur.vowel, breath = cur.breath, haunt = cur.haunt;
            const float hauntJump = (0.1f + 2.f * haunt) * invSr;
            const float hauntGlide = onePole (0.05f + 1.5f * haunt);
            for (int v = 0; v < kNumGhosts; ++v)
            {
                const float presence = std::clamp (ghost * kNumGhosts - (float) v, 0.f, 1.f);
                if (presence <= 0.f)
                    continue;  // silent ghosts cost nothing and leave the rng alone
                Ghost& g = ghosts[v];
                const float spread = kGhostSpread[v];

                // Haunt: the vowel wanders, and the ghost swells in and out of the fog
                // (at full Haunt it can vanish completely for a moment).
                const float wander = g.vowelDrift.step (rng, hauntJump, hauntGlide);
                const float swell  = g.swellDrift.step (rng, hauntJump, hauntGlide);
                const float amp = presence * (1.f - 0.5f * haunt * (1.f - swell));

                // Sung part: a saw at the note, detuned up to +-30 cents for a choir smear.
                const float dtG = std::min (hz * std::exp2 (spread * haunt * 0.025f), maxHz) * invSr;
                g.sawPhase += dtG;
                g.sawPhase -= std::floor (g.sawPhase);
                const float sung = 2.f * g.sawPhase - 1.f - polyBlep (g.sawPhase, dtG);
                // Bandpassed noise is quieter than a bandpassed saw; 4x evens the Breath knob out.
                const float excite = (1.f - breath) * sung + breath * 4.f * rng.bi();

                // Vowel position 0..4 between table rows. Side ghosts get slightly
                // bigger/smaller "throats" so the three voices are not clones.
                const float vpos = std::clamp ((vowel + 0.35f * haunt * wander) * 4.f, 0.f, 4.f);
                const int row = std::min ((int) vpos, 3);
                const float frac = vpos - (float) row;
                const float throat = std::exp2 (spread * 0.08f * haunt);
                float voice = 0.f;
                for (int f = 0; f < 3; ++f)
                {
                    // Log-frequency blend between neighbouring vowels.
                    const float a = kVowels[row][f], b = kVowels[row + 1][f];
                    // ponytail: 9 tan() calls per sample here; update coefficients every
                    // 16-32 samples if this ever shows up in a profiler.
                    g.formant[f].setCutoff (clampHz (a * std::pow (b / a, frac) * throat), kFormantQ, invSr);
                    voice += kFormantGain[f] * g.formant[f].band (excite) * g.formant[f].k;
                }

                // Fixed pan slot (L / C / R), wider as more ghosts arrive.
                const float gPan = 0.5f + 0.5f * spread * (0.3f + 0.7f * ghost);
                ghostL += amp * voice * std::cos (gPan * kHalfPi);
                ghostR += amp * voice * std::sin (gPan * kHalfPi);
            }

            // Gate: fast-ish attack, slow fog-like release. Shared by fog and ghosts.
            const float gTarget = gateOn ? 1.f : 0.f;
            gate += (gateOn ? gateAttack : gateRelease) * (gTarget - gate);
            mono *= gate;

            // Equal-power pan; width grows with Motion. tanh is the final safety
            // limiter and adds a little grit when everything is cranked.
            const float pan = 0.5f + 0.5f * swing * (0.2f + 0.8f * motion);
            const float l = std::tanh (mono * std::cos (pan * kHalfPi) + gate * ghostL);
            const float r = std::tanh (mono * std::sin (pan * kHalfPi) + gate * ghostR);
            if (R != nullptr)
            {
                L[i] = l;
                R[i] = r;
            }
            else
                L[i] = 0.5f * (l + r);
        }
    }

private:
    // xorshift32: cheap and deterministic, so the check renders the same every run.
    struct Rng
    {
        uint32_t s = 0x9e3779b9u;
        float next()  // [0, 1)
        {
            s ^= s << 13;
            s ^= s >> 17;
            s ^= s << 5;
            return (float) (s >> 8) * (1.f / 16777216.f);
        }
        float bi() { return 2.f * next() - 1.f; }  // [-1, 1)
    };

    // Smoothed random walk in [-1, 1]: now and then picks a new target, glides toward it.
    struct Drift
    {
        float value = 0.f, goal = 0.f;
        float step (Rng& r, float jumpProb, float glide)
        {
            if (r.next() < jumpProb)
                goal = r.bi();
            value += glide * (goal - value);
            return value;
        }
    };

    // Topology-preserving (Cytomic/Zavalishin) state-variable filter. Stable under fast cutoff moves.
    struct Svf
    {
        float a1 = 0.f, a2 = 0.f, a3 = 0.f, k = 1.f, ic1 = 0.f, ic2 = 0.f;
        void setCutoff (float hz, float q, float invSr)
        {
            const float g = std::tan (3.1415927f * hz * invSr);
            k = 1.f / q;
            a1 = 1.f / (1.f + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
        }
        float band (float x)
        {
            const float v3 = x - ic2;
            const float v1 = a1 * ic1 + a2 * v3;
            const float v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.f * v1 - ic1;
            ic2 = 2.f * v2 - ic2;
            return v1;
        }
    };

    // Four one-pole lowpasses with tanh feedback. Each stage averages and tanh caps
    // the input at +-1, so it cannot blow up even at full resonance.
    struct Ladder
    {
        float s[4] {};
        float process (float in, float w, float res, float drive)
        {
            const float g = w / (1.f + w);  // one-pole coefficient, always < 1
            float x = std::tanh (drive * (in - res * s[3]));
            for (float& st : s)
                x = st += g * (x - st);
            return x * (1.f + 0.5f * res);  // resonance eats the passband; give some back
        }
    };

    // PolyBLEP: rounds off the saw's jump so high blurts do not alias as badly.
    static float polyBlep (float t, float dt)
    {
        if (t < dt)       { t /= dt;               return t + t - t * t - 1.f; }
        if (t > 1.f - dt) { t = (t - 1.f) / dt;    return t * t + t + t + 1.f; }
        return 0.f;
    }

    // One-pole smoothing coefficient for a cutoff in Hz. w / (1 + w) is close to
    // 1 - exp(-w) for slow rates and avoids an exp per sample.
    float onePole (float freq) const
    {
        const float w = 6.2831853f * freq * invSr;
        return w / (1.f + w);
    }

    float clampHz (float freq) const { return std::clamp (freq, 20.f, maxHz); }

    void setNote (int note)
    {
        noteHz = 440.f * std::exp2 ((float) (note - 69) / 12.f);
        if (gate < 1.e-4f)
            hz = noteHz;  // no slide from a note that has already faded out
    }

    void fireBlurt()
    {
        blurt = 1.f;
        blurtDrop = 1.5f * rng.next();                                    // up to 1.5 octaves
        blurtDecay = std::exp (-invSr / (0.08f + 0.22f * rng.next()));   // 80..300 ms
    }

    void removeHeld (int note)
    {
        int w = 0;
        for (int i = 0; i < numHeld; ++i)
            if (held[i] != note)
                held[w++] = held[i];
        numHeld = w;
    }

    // One choir voice: its own formant bank, saw, vowel wander and swell.
    struct Ghost
    {
        Svf formant[3];
        Drift vowelDrift, swellDrift;
        float sawPhase = 0.f;
    };

    static constexpr int kMaxHeld = 16;
    static constexpr int kNumGhosts = 3;

    // Ghost order: centre first, then left, then right. Also signs the detune and throat size.
    static constexpr float kGhostSpread[kNumGhosts] = { 0.f, -1.f, 1.f };

    // Vowel formants F1-F3 in Hz (adult averages, Peterson & Barney style), dark to bright.
    static constexpr float kVowels[5][3] = {
        { 300.f,  870.f, 2240.f },  // u
        { 450.f,  800.f, 2830.f },  // o
        { 730.f, 1090.f, 2440.f },  // a
        { 530.f, 1840.f, 2480.f },  // e
        { 270.f, 2290.f, 3010.f },  // i
    };
    static constexpr float kFormantGain[3] = { 1.f, 0.5f, 0.25f };
    static constexpr float kFormantQ = 8.f;

    float sr = 48000.f, invSr = 1.f / 48000.f, maxHz = 21600.f;
    Macros target, cur;
    float smooth = 0.f, gateAttack = 0.f, gateRelease = 0.f, pitchGlide = 0.f;

    Rng rng;
    Drift sweepDrift, pitchDrift, whirlDrift;
    Svf sweep, pops;
    Ladder ladder;
    Ghost ghosts[kNumGhosts];

    float whirlPhase = 0.f, sawPhase = 0.f;
    float noteHz = 110.f, hz = 110.f;
    float blurt = 0.f, blurtDrop = 0.f, blurtDecay = 0.f;
    float gate = 0.f;
    bool gateOn = false;
    int held[kMaxHeld] {};
    int numHeld = 0;
};
} // namespace v4jb
