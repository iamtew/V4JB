#pragma once

#include "Theme.h"

// Edit page: the 12-slot chain. Slot 1 (spectral) and slot 12 (effects and
// master) are fixed cards on top; slots 2..11 are the module cards below.
namespace ui
{
struct SpectralCard : public juce::Component
{
    SpectralCard (V4JBAudioProcessor& p)
        : bars (p, 0), algo (p.apvts, ParamID::spAlgo)
    {
        auto& s = p.apvts;
        namespace I = ParamID;
        const std::pair<const char*, const char*> defs[] = {
            { I::spDepth, "Depth" }, { I::spTilt, "Tilt" }, { I::spPartials, "Partials" },
            { I::spModDepth, "Mod Depth" }, { I::spModRate, "Mod Rate" }, { I::sub1, "Sub -1" },
            { I::nzMix, "Noise" }, { I::nzWidth, "Width" }, { I::nzLow, "Low Cut" },
            { I::nzHigh, "High Cut" }, { I::nzDecay, "N. Decay" }, { I::sub2, "Sub -2" } };
        for (auto& d : defs)
            addAndMakeVisible (knobs.add (new Knob (s, d.first, d.second)));
        addAndMakeVisible (bars);
        addAndMakeVisible (algo);
    }
    void paint (juce::Graphics& g) override { paintCard (g, getLocalBounds(), "1  SPECTRAL"); }
    void resized() override
    {
        auto r = getLocalBounds().reduced (8, 4);
        auto head = r.removeFromTop (20);
        algo.setBounds (head.removeFromRight (160));
        r.removeFromTop (4);
        bars.setBounds (r.removeFromTop (48));
        r.removeFromTop (4);
        const int h = r.getHeight() / 2, w = r.getWidth() / 6;
        for (int i = 0; i < knobs.size(); ++i)
            knobs[i]->setBounds (r.getX() + (i % 6) * w, r.getY() + (i / 6) * h, w, h);
    }
    SpectrumBars bars;
    Combo algo;
    juce::OwnedArray<Knob> knobs;
};

struct FxCard : public juce::Component
{
    FxCard (V4JBAudioProcessor& p)
    {
        auto& s = p.apvts;
        namespace I = ParamID;
        auto knob = [&] (const char* id, const char* name) { auto* k = knobs.add (new Knob (s, id, name)); addAndMakeVisible (k); return k; };
        auto tog = [&] (const char* id, const char* name) { auto* t = toggles.add (new Toggle (s, id, name)); addAndMakeVisible (t); return t; };
        auto combo = [&] (const char* id) { auto* c = combos.add (new Combo (s, id)); addAndMakeVisible (c); return c; };

        cols[0] = { "DRIVE",  tog (I::drOn, "On"), nullptr, nullptr, { knob (I::drAmt, "Drive"), knob (I::drMix, "Mix") } };
        cols[1] = { "MOD",    tog (I::mdOn, "On"), combo (I::mdType), nullptr,
                    { knob (I::mdRate, "Rate"), knob (I::mdDepth, "Depth"), knob (I::mdMix, "Mix") } };
        cols[2] = { "DELAY",  tog (I::dlOn, "On"), combo (I::dlDiv), tog (I::dlSync, "Sync"),
                    { knob (I::dlTime, "Time"), knob (I::dlFb, "Feedback"), knob (I::dlMix, "Mix") } };
        cols[3] = { "REVERB", tog (I::rvOn, "On"), nullptr, nullptr,
                    { knob (I::rvSize, "Size"), knob (I::rvDamp, "Damping"), knob (I::rvMix, "Mix"), knob (I::rvShim, "Shimmer") } };
        cols[4] = { "MASTER", nullptr, nullptr, nullptr, { knob (I::width, "Width"), knob (I::spread, "Spread") } };
        refresh();
    }
    // Sync uses Division; free time uses the Time knob. The other control stays put, just grey.
    void refresh()
    {
        const bool sync = cols[2].extra->getToggleState();
        cols[2].knobs[0]->setEnabled (! sync);
        cols[2].knobs[0]->setAlpha (sync ? 0.4f : 1.f);
        cols[2].combo->setEnabled (sync);
        cols[2].combo->setAlpha (sync ? 1.f : 0.4f);
    }
    void paint (juce::Graphics& g) override
    {
        paintCard (g, getLocalBounds(), "12  EFFECTS & MASTER");
        g.setFont (boldFont (10.f));
        g.setColour (kFog.withAlpha (0.7f));
        for (auto& c : cols)
            g.drawText (c.title, c.titleArea, juce::Justification::centredLeft, false);
    }
    void resized() override
    {
        auto r = getLocalBounds().reduced (8, 4);
        r.removeFromTop (20);
        const int w = r.getWidth() / 5;
        for (auto& c : cols)
        {
            auto col = r.removeFromLeft (w).reduced (3, 0);
            auto head = col.removeFromTop (20);
            c.titleArea = head.removeFromLeft (46);
            if (c.on != nullptr)
                c.on->setBounds (head.removeFromLeft (34));
            auto line2 = col.removeFromTop (c.combo != nullptr || c.extra != nullptr ? 24 : 0);
            if (c.extra != nullptr)
                c.extra->setBounds (line2.removeFromLeft (44).reduced (0, 2));
            if (c.combo != nullptr)
                c.combo->setBounds (line2.reduced (2, 2));
            const int kh = juce::jmin (64, col.getHeight() / 2);
            for (size_t i = 0; i < c.knobs.size(); ++i)
                c.knobs[i]->setBounds (col.getX() + (int) (i % 2) * (col.getWidth() / 2), col.getY() + (int) (i / 2) * kh,
                                       col.getWidth() / 2, kh);
        }
    }
    struct Column
    {
        const char* title = "";
        juce::ToggleButton* on = nullptr;
        juce::ComboBox* combo = nullptr;
        juce::ToggleButton* extra = nullptr;
        std::vector<Knob*> knobs;
        juce::Rectangle<int> titleArea;
    };
    Column cols[5];
    juce::OwnedArray<Knob> knobs;
    juce::OwnedArray<Toggle> toggles;
    juce::OwnedArray<Combo> combos;
};

// One user slot. MOD flips the lower half between the module's knobs and its
// three modulation routes (envelope, LFO, MIDI -> main knob).
struct SlotCard : public juce::Component
{
    SlotCard (V4JBAudioProcessor& p, int slotIndex)
        : proc (p), index (slotIndex), bars (p, slotIndex + 1),
          type (p.apvts, ParamID::slot (slotIndex, "type")),
          main (p.apvts, ParamID::slot (slotIndex, "main"), "Main"),
          routes { { p.apvts, ParamID::slot (slotIndex, "es") },
                   { p.apvts, ParamID::slot (slotIndex, "ls") },
                   { p.apvts, ParamID::slot (slotIndex, "ms") } }
    {
        typeParam = p.apvts.getRawParameterValue (ParamID::slot (slotIndex, "type"));
        addAndMakeVisible (bars);
        addAndMakeVisible (type);
        addAndMakeVisible (main);
        for (int k = 0; k < 4; ++k)
            addAndMakeVisible (params.add (new HSlider (p.apvts, ParamID::slotP (slotIndex, k), "")));
        const char* amtKeys[3] = { "ea", "la", "ma" };
        for (int i = 0; i < 3; ++i)
        {
            auto& rt = routes[i];
            rt.amount.setSliderStyle (juce::Slider::LinearBar);
            rt.amount.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            rt.amount.setColour (juce::Slider::trackColourId, kAmber.withAlpha (0.6f));
            rt.amount.setPopupDisplayEnabled (true, false, nullptr);
            rt.att = std::make_unique<APVTS::SliderAttachment> (p.apvts, ParamID::slot (slotIndex, amtKeys[i]), rt.amount);
            rt.name.setText (kRouteNames[i], juce::dontSendNotification);
            rt.name.setFont (boldFont (11.f));
            rt.name.setJustificationType (juce::Justification::centred);
            rt.name.setInterceptsMouseClicks (false, false);
            addChildComponent (rt.name);
            addChildComponent (rt.source);
            addChildComponent (rt.amount);
        }
        modBtn.setClickingTogglesState (true);
        modBtn.setTooltip ("Routes push the main knob");
        modBtn.onClick = [this] { showRoutes (modBtn.getToggleState()); };
        addAndMakeVisible (modBtn);
        refresh();
    }

    void showRoutes (bool on)
    {
        for (auto& rt : routes)
        {
            rt.name.setVisible (on);
            rt.source.setVisible (on);
            rt.amount.setVisible (on);
        }
        lastType = -1;  // re-run label visibility
        refresh();
    }

    // Timer tick: bars, and labels when the module type changed (UI or automation).
    void refresh()
    {
        bars.refresh();
        const int t = typeIndex();
        if (t == lastType)
            return;
        lastType = t;
        const auto& info = v4jb::kModuleInfo[t];
        main.label.setText (info.labels[0], juce::dontSendNotification);
        main.setVisible (t != 0);
        const bool routesOn = modBtn.getToggleState();
        for (int k = 0; k < 4; ++k)
        {
            params[k]->label.setText (info.labels[k + 1], juce::dontSendNotification);
            params[k]->setVisible (! routesOn && info.labels[k + 1][0] != 0);
        }
        resized();
        repaint();
    }

    int typeIndex() const
    {
        return juce::jlimit (0, v4jb::kNumModuleTypes - 1, (int) std::lround (typeParam->load()));
    }

    juce::String slotTitle() const
    {
        return juce::String (index + 2) + "  " + v4jb::kModuleInfo[typeIndex()].name;
    }

    void paint (juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds();
        paintCard (g, bounds, slotTitle());
        if (typeIndex() == 0)
        {
            g.setColour (kNight.withAlpha (0.5f));
            g.fillRoundedRectangle (bounds.toFloat(), 4.f);
        }
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (5, 3);
        auto head = r.removeFromTop (20);
        const int titleW = juce::roundToInt (juce::GlyphArrangement::getStringWidth (boldFont (11.f), slotTitle())) + 8;
        head.removeFromLeft (juce::jmax (18, titleW));
        modBtn.setBounds (head.removeFromRight (40));
        type.setBounds (head.reduced (2, 0));
        r.removeFromTop (2);
        bars.setBounds (r.removeFromTop (22));
        r.removeFromTop (2);
        const int mainW = juce::jmax (86, r.getWidth() * 40 / 100);
        main.setBounds (r.removeFromLeft (mainW));
        if (modBtn.getToggleState())
        {
            const int rh = r.getHeight() / 3;
            for (auto& rt : routes)
            {
                auto cell = r.removeFromTop (rh);
                rt.name.setBounds (cell.removeFromTop (12));
                const int ch = juce::jmin (22, cell.getHeight());
                auto controls = cell.removeFromTop (ch);
                rt.source.setBounds (controls.removeFromLeft (controls.getWidth() / 2).reduced (0, 1));
                rt.amount.setBounds (controls.reduced (2, 1));
            }
        }
        else
        {
            int n = 0;
            for (auto* s : params)
                if (s->isVisible())
                    ++n;
            if (n > 0)
            {
                const int ph = r.getHeight() / n;
                for (auto* s : params)
                    if (s->isVisible())
                        s->setBounds (r.removeFromTop (ph));
            }
        }
    }

    struct Route
    {
        Route (APVTS& st, const juce::String& id) : source (st, id) {}
        juce::Label name;
        Combo source;
        juce::Slider amount;
        std::unique_ptr<APVTS::SliderAttachment> att;
    };
    static constexpr const char* kRouteNames[3] = { "Env", "LFO", "MIDI" };

    V4JBAudioProcessor& proc;
    int index;
    SpectrumBars bars;
    Combo type;
    Knob main;
    juce::OwnedArray<HSlider> params;
    Route routes[3];
    juce::ToggleButton modBtn { "MOD" };
    std::atomic<float>* typeParam = nullptr;
    int lastType = -1;
};

struct EditPage : public juce::Component
{
    EditPage (V4JBAudioProcessor& p) : spectral (p), fx (p)
    {
        addAndMakeVisible (spectral);
        addAndMakeVisible (fx);
        for (int i = 0; i < v4jb::kNumSlots; ++i)
            addAndMakeVisible (slots.add (new SlotCard (p, i)));
    }
    void refresh()
    {
        spectral.bars.refresh();
        fx.refresh();
        for (auto* s : slots)
            s->refresh();
    }
    void resized() override
    {
        auto r = getLocalBounds().reduced (6);
        auto top = r.removeFromTop (r.getHeight() * 36 / 100);
        spectral.setBounds (top.removeFromLeft (top.getWidth() * 46 / 100).reduced (3));
        fx.setBounds (top.reduced (3));
        const int h = r.getHeight() / 2, w = r.getWidth() / 5;
        for (int i = 0; i < slots.size(); ++i)
            slots[i]->setBounds (juce::Rectangle<int> (r.getX() + (i % 5) * w, r.getY() + (i / 5) * h, w, h).reduced (3));
    }
    SpectralCard spectral;
    FxCard fx;
    juce::OwnedArray<SlotCard> slots;
};
} // namespace ui
