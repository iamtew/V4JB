#pragma once

#include "Theme.h"

// Mod page: the four envelopes (Env 1 = amp) and three LFOs that slots can route.
namespace ui
{
struct EnvCard : public juce::Component
{
    EnvCard (APVTS& s, int i) : index (i)
    {
        const std::pair<const char*, const char*> defs[] = { { "a", "Attack" }, { "d", "Decay" }, { "s", "Sustain" },
                                                             { "r", "Release" }, { "vel", "Velocity" } };
        for (auto& d : defs)
            addAndMakeVisible (knobs.add (new Knob (s, ParamID::env (i, d.first), d.second)));
    }
    void paint (juce::Graphics& g) override
    {
        paintCard (g, getLocalBounds(), "ENV " + juce::String (index + 1) + (index == 0 ? "  (AMP)" : ""));
    }
    void resized() override
    {
        auto r = getLocalBounds().reduced (8, 4);
        r.removeFromTop (20);
        const int w = r.getWidth() / knobs.size();
        for (auto* k : knobs)
            k->setBounds (r.removeFromLeft (w).withSizeKeepingCentre (w, juce::jmin (r.getHeight(), 84)));
    }
    int index;
    juce::OwnedArray<Knob> knobs;
};

struct LfoCard : public juce::Component
{
    LfoCard (APVTS& s, int i)
        : index (i), rate (s, ParamID::lfo (i, "rate"), "Rate"), shape (s, ParamID::lfo (i, "shape")),
          div (s, ParamID::lfo (i, "div")), sync (s, ParamID::lfo (i, "sync"), "Sync")
    {
        for (int slot = 0; slot < v4jb::kNumSlots; ++slot)
            route[slot] = s.getRawParameterValue (ParamID::slot (slot, "ls"));
        addAndMakeVisible (rate);
        addAndMakeVisible (shape);
        addAndMakeVisible (div);
        addAndMakeVisible (sync);
        refresh();
    }
    // A slot's LFO source is 0 = off, 1..3 = this card's index + 1.
    bool routed() const
    {
        for (auto* p : route)
            if (std::lround (p->load()) == index + 1)
                return true;
        return false;
    }
    void refresh()
    {
        const bool synced = sync.getToggleState();
        rate.setEnabled (! synced);
        rate.setAlpha (synced ? 0.4f : 1.f);
        div.setEnabled (synced);
        div.setAlpha (synced ? 1.f : 0.4f);
        repaint();
    }
    void paint (juce::Graphics& g) override
    {
        paintCard (g, getLocalBounds(), "LFO " + juce::String (index + 1));
        if (routed())
        {
            g.setColour (kTeal);
            g.fillEllipse (56.f, 9.f, 8.f, 8.f);
        }
    }
    void resized() override
    {
        auto r = getLocalBounds().reduced (8, 4);
        r.removeFromTop (20);
        rate.setBounds (r.removeFromLeft (90).withSizeKeepingCentre (90, juce::jmin (r.getHeight(), 84)));
        auto col = r.reduced (6, 0).withSizeKeepingCentre (r.getWidth() - 12, 84);
        shape.setBounds (col.removeFromTop (26).reduced (0, 2));
        col.removeFromTop (4);
        auto line = col.removeFromTop (26);
        sync.setBounds (line.removeFromLeft (54).reduced (0, 2));
        div.setBounds (line.reduced (4, 2));
    }
    int index;
    std::atomic<float>* route[v4jb::kNumSlots] {};
    Knob rate;
    Combo shape, div;
    Toggle sync;
};

struct ModPage : public juce::Component
{
    ModPage (V4JBAudioProcessor& p)
    {
        for (int i = 0; i < v4jb::kNumEnvs; ++i)
            addAndMakeVisible (envs.add (new EnvCard (p.apvts, i)));
        for (int i = 0; i < v4jb::kNumLfos; ++i)
            addAndMakeVisible (lfos.add (new LfoCard (p.apvts, i)));
    }
    void paint (juce::Graphics& g) override
    {
        g.setColour (kFog.withAlpha (0.6f));
        g.setFont (boldFont (11.f));
        g.drawText ("Each module slot can route one envelope, one LFO and one MIDI source to its main knob (MOD on the slot card).",
                    hint, juce::Justification::centred, false);
    }
    void resized() override
    {
        auto r = getLocalBounds().reduced (6);
        hint = r.removeFromBottom (24);
        auto top = r.removeFromTop (r.getHeight() / 2);
        const int ew = top.getWidth() / 2, eh = top.getHeight() / 2;
        for (int i = 0; i < envs.size(); ++i)
            envs[i]->setBounds (juce::Rectangle<int> (top.getX() + (i % 2) * ew, top.getY() + (i / 2) * eh, ew, eh).reduced (3));
        const int lw = r.getWidth() / lfos.size();
        for (auto* l : lfos)
            l->setBounds (r.removeFromLeft (lw).reduced (3));
    }
    void refresh()
    {
        for (auto* l : lfos)
            l->refresh();
    }
    juce::OwnedArray<EnvCard> envs;
    juce::OwnedArray<LfoCard> lfos;
    juce::Rectangle<int> hint;
};
} // namespace ui
