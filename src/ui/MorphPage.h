#pragma once

#include "Theme.h"

// Morph page: the XY pad with corners A-D, the path editor, and the 11 macros
// (live value meter + offset slider + "Free" = not linked to the pad).
namespace ui
{
struct MorphPad : public juce::Component
{
    explicit MorphPad (V4JBAudioProcessor& p) : proc (p)
    {
        xParam = p.apvts.getParameter (ParamID::morphX);
        yParam = p.apvts.getParameter (ParamID::morphY);
    }

    bool editPath = false;
    int selected = -1;  // selected path node, for the segment length box
    std::function<void()> onSelect;

    juce::Point<float> toScreen (float x, float y) const
    {
        auto r = area();
        return { r.getX() + x * r.getWidth(), r.getBottom() - y * r.getHeight() };
    }
    juce::Point<float> toPad (juce::Point<float> s) const
    {
        auto r = area();
        return { juce::jlimit (0.f, 1.f, (s.x - r.getX()) / r.getWidth()),
                 juce::jlimit (0.f, 1.f, (r.getBottom() - s.y) / r.getHeight()) };
    }
    juce::Rectangle<float> area() const { return getLocalBounds().toFloat().reduced (18.f); }

    int nodeAt (juce::Point<float> s) const
    {
        auto& m = proc.morph;
        for (int i = m.numNodes.load() - 1; i >= 0; --i)
            if (toScreen (m.nodeX[i].load(), m.nodeY[i].load()).getDistanceFrom (s) < 9.f)
                return i;
        return -1;
    }

    void paint (juce::Graphics& g) override
    {
        auto r = area();
        g.setColour (kNight);
        g.fillRoundedRectangle (r, 4.f);
        g.setColour (kDusk.brighter (0.15f));
        for (int i = 1; i < 4; ++i)
        {
            g.drawVerticalLine ((int) (r.getX() + r.getWidth() * (float) i / 4.f), r.getY(), r.getBottom());
            g.drawHorizontalLine ((int) (r.getY() + r.getHeight() * (float) i / 4.f), r.getX(), r.getRight());
        }

        // Corners: click a letter to jump there.
        g.setFont (boldFont (14.f));
        g.setColour (kAmber);
        const char* names[4] = { "A", "B", "C", "D" };
        for (int c = 0; c < 4; ++c)
            g.drawText (names[c], cornerBox (c), juce::Justification::centred, false);

        // Path: closed loop through the nodes (an X-Axis sweep reads it as a curve).
        auto& m = proc.morph;
        const int n = m.numNodes.load();
        if (n > 0)
        {
            juce::Path path;
            for (int i = 0; i <= n; ++i)
            {
                const int j = i % n;
                const auto pt = toScreen (m.nodeX[j].load(), m.nodeY[j].load());
                if (i == 0) path.startNewSubPath (pt); else path.lineTo (pt);
            }
            g.setColour (kFog.withAlpha (editPath ? 0.8f : 0.4f));
            g.strokePath (path, juce::PathStrokeType (1.5f));
            for (int i = 0; i < n; ++i)
            {
                const auto pt = toScreen (m.nodeX[i].load(), m.nodeY[i].load());
                g.setColour (i == selected ? kAmber : kFog);
                g.fillEllipse (juce::Rectangle<float> (8.f, 8.f).withCentre (pt));
            }
        }

        // Puck where the morph really is (follows the path while it plays).
        const auto puck = toScreen (m.outX.load(), m.outY.load());
        g.setColour (kTeal);
        g.fillEllipse (juce::Rectangle<float> (16.f, 16.f).withCentre (puck));
        g.setColour (kNight);
        g.drawEllipse (juce::Rectangle<float> (16.f, 16.f).withCentre (puck), 2.f);

        if (editPath)
        {
            g.setColour (kFog.withAlpha (0.6f));
            g.setFont (boldFont (10.f));
            g.drawText ("click: add node   drag: move   right-click: delete", r.reduced (6).removeFromBottom (14),
                        juce::Justification::centred, false);
        }
    }

    juce::Rectangle<float> cornerBox (int c) const
    {
        auto b = getLocalBounds().toFloat();
        const float s = 18.f;
        return { (c == 1 || c == 3) ? b.getRight() - s : b.getX(), c >= 2 ? b.getY() : b.getBottom() - s, s, s };
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        for (int c = 0; c < 4; ++c)
            if (cornerBox (c).contains (e.position))
            {
                proc.recallCorner (c);
                return;
            }
        auto& m = proc.morph;
        if (editPath)
        {
            dragNode = nodeAt (e.position);
            if (e.mods.isPopupMenu())
            {
                if (dragNode >= 0)
                    deleteNode (dragNode);
                dragNode = -1;
                return;
            }
            if (dragNode < 0 && m.numNodes.load() < v4jb::kMaxNodes)
            {
                // New node: write its data first, then publish it by bumping the count.
                const int i = m.numNodes.load();
                const auto pt = toPad (e.position);
                m.nodeX[i].store (pt.x);
                m.nodeY[i].store (pt.y);
                m.nodeLen[i].store (2);
                m.numNodes.store (i + 1);
                dragNode = i;
            }
            select (dragNode);
            repaint();
            return;
        }
        xParam->beginChangeGesture();
        yParam->beginChangeGesture();
        dragging = true;
        mouseDrag (e);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        const auto pt = toPad (e.position);
        if (editPath)
        {
            if (dragNode >= 0)
            {
                proc.morph.nodeX[dragNode].store (pt.x);
                proc.morph.nodeY[dragNode].store (pt.y);
                repaint();
            }
            return;
        }
        if (dragging)
        {
            xParam->setValueNotifyingHost (pt.x);
            yParam->setValueNotifyingHost (pt.y);
        }
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (dragging)
        {
            xParam->endChangeGesture();
            yParam->endChangeGesture();
        }
        dragging = false;
        dragNode = -1;
    }

    void deleteNode (int i)
    {
        auto& m = proc.morph;
        const int n = m.numNodes.load();
        m.numNodes.store (0);  // hide the path while it shifts
        for (int j = i; j < n - 1; ++j)
        {
            m.nodeX[j].store (m.nodeX[j + 1].load());
            m.nodeY[j].store (m.nodeY[j + 1].load());
            m.nodeLen[j].store (m.nodeLen[j + 1].load());
        }
        m.numNodes.store (n - 1);
        select (-1);
        repaint();
    }

    void select (int i)
    {
        selected = i;
        if (onSelect)
            onSelect();
    }

    V4JBAudioProcessor& proc;
    juce::RangedAudioParameter* xParam = nullptr;
    juce::RangedAudioParameter* yParam = nullptr;
    bool dragging = false;
    int dragNode = -1;
};

// One macro: name, live value meter beside the offset slider, Free toggle.
struct MacroColumn : public juce::Component
{
    MacroColumn (V4JBAudioProcessor& p, int m)
        : proc (p), index (m), free (p.apvts, ParamID::lock (m), "Free")
    {
        offset.setSliderStyle (juce::Slider::LinearVertical);
        offset.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        offset.setPopupDisplayEnabled (true, false, nullptr);
        offset.setTooltip ("Offset (automatable): added on top of the morph value");
        att = std::make_unique<APVTS::SliderAttachment> (p.apvts, ParamID::macro (m), offset);
        addAndMakeVisible (offset);
        addAndMakeVisible (free);
    }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds();
        g.setFont (boldFont (9.f));
        g.setColour (kFog.withAlpha (0.6f));
        g.drawText (v4jb::kMacroGroups[index], r.removeFromTop (12), juce::Justification::centred, false);
        g.setFont (boldFont (10.f));
        g.setColour (kFog);
        g.drawText (v4jb::kMacroNames[index], r.removeFromTop (14), juce::Justification::centred, false);
        g.setColour (kNight);
        g.fillRoundedRectangle (meter.toFloat(), 2.f);
        const float v = proc.morph.outMacro[index].load();
        g.setColour (kTeal);
        g.fillRect (meter.toFloat().withTrimmedTop ((1.f - v) * (float) meter.getHeight()).reduced (1.f, 0.f));
    }
    void resized() override
    {
        auto r = getLocalBounds();
        r.removeFromTop (28);
        free.setBounds (r.removeFromBottom (22).reduced (2, 1));
        auto body = r.reduced (2, 4);
        meter = body.removeFromLeft (body.getWidth() / 2).reduced (4, 0);
        offset.setBounds (body);
    }
    V4JBAudioProcessor& proc;
    int index;
    juce::Slider offset;
    std::unique_ptr<APVTS::SliderAttachment> att;
    Toggle free;
    juce::Rectangle<int> meter;
};

struct MorphPage : public juce::Component
{
    MorphPage (V4JBAudioProcessor& p)
        : proc (p), pad (p), mode (p.apvts, ParamID::morphMode), snap (p.apvts, ParamID::morphSnap),
          userSlot (p.apvts, ParamID::userSlot), time (p.apvts, ParamID::morphTime, "Time")
    {
        addAndMakeVisible (pad);
        for (int m = 0; m < v4jb::kNumMacros; ++m)
            addAndMakeVisible (macros.add (new MacroColumn (p, m)));
        const char* names[4] = { "Store A", "Store B", "Store C", "Store D" };
        for (int c = 0; c < 4; ++c)
        {
            auto* b = store.add (new juce::TextButton (names[c]));
            b->setTooltip ("Save the current macro values into this corner");
            b->onClick = [this, c] { proc.storeCorner (c); };
            addAndMakeVisible (b);
        }
        for (auto* c : { &mode, &snap, &userSlot })
            addAndMakeVisible (c);
        addAndMakeVisible (time);

        editBtn.setClickingTogglesState (true);
        editBtn.onClick = [this] { pad.editPath = editBtn.getToggleState(); pad.select (-1); pad.repaint(); };
        addAndMakeVisible (editBtn);
        clearBtn.onClick = [this] { proc.morph.numNodes.store (0); pad.select (-1); pad.repaint(); };
        addAndMakeVisible (clearBtn);

        for (int i = 0; i < v4jb::kNumSegLens; ++i)
            segLen.addItem (v4jb::kSegNames[i], i + 1);
        segLen.setTextWhenNothingSelected ("segment");
        segLen.onChange = [this]
        {
            if (pad.selected >= 0 && segLen.getSelectedId() > 0)
                proc.morph.nodeLen[pad.selected].store (segLen.getSelectedId() - 1);
        };
        addAndMakeVisible (segLen);
        pad.onSelect = [this]
        {
            segLen.setEnabled (pad.selected >= 0);
            if (pad.selected >= 0)
                segLen.setSelectedId (proc.morph.nodeLen[pad.selected].load() + 1, juce::dontSendNotification);
            else
                segLen.setSelectedId (0, juce::dontSendNotification);
        };
        pad.onSelect();

        auto file = [this] (juce::TextButton& b, bool save, bool points)
        {
            b.onClick = [this, save, points] { fileAction (save, points); };
            addAndMakeVisible (b);
        };
        file (savePoints, true, true);
        file (savePath, true, false);
        file (loadBtn, false, false);
    }

    void fileAction (bool save, bool points)
    {
        const auto dir = Presets::userDir().getParentDirectory().getChildFile ("Morph");
        dir.createDirectory();
        const juce::String pattern = "*.v4jbmorph";
        chooser = std::make_unique<juce::FileChooser> (save ? (points ? "Save morph points" : "Save morph path") : "Load points or path",
                                                       dir, pattern);
        juce::Component::SafePointer<MorphPage> safe (this);
        const int chooserFlags = save ? juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting
                                      : juce::FileBrowserComponent::openMode;
        chooser->launchAsync (chooserFlags | juce::FileBrowserComponent::canSelectFiles, [safe, save, points] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (safe == nullptr || f == juce::File())
                return;
            if (save)
                safe->proc.saveMorphFile (f.withFileExtension (".v4jbmorph"), points);
            else
                safe->proc.loadMorphFile (f);
            safe->pad.repaint();
        });
    }

    void refresh()
    {
        pad.repaint();
        for (auto* m : macros)
            m->repaint();
    }

    void paint (juce::Graphics& g) override
    {
        paintCard (g, macroArea.expanded (4), "MACROS");
        paintCard (g, ctlArea.expanded (4), "PATH & CORNERS");
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (10);
        const int padSize = juce::jmin (r.getHeight(), r.getWidth() / 2 - 20);
        pad.setBounds (r.removeFromLeft (padSize).withSizeKeepingCentre (padSize, padSize));
        r.removeFromLeft (16);

        ctlArea = r.removeFromBottom (170).reduced (4);
        r.removeFromBottom (12);
        macroArea = r.reduced (4);
        auto mr = macroArea.withTrimmedTop (18);
        const int w = mr.getWidth() / macros.size();
        for (auto* m : macros)
            m->setBounds (mr.removeFromLeft (w));

        auto c = ctlArea.withTrimmedTop (20).reduced (4, 0);
        auto line1 = c.removeFromTop (26);
        for (auto* b : store)
            b->setBounds (line1.removeFromLeft (90).reduced (2));
        line1.removeFromLeft (10);
        userSlot.setBounds (line1.removeFromLeft (110).reduced (2));
        c.removeFromTop (8);
        time.setBounds (c.removeFromRight (80));
        auto line2 = c.removeFromTop (26);
        mode.setBounds (line2.removeFromLeft (120).reduced (2));
        snap.setBounds (line2.removeFromLeft (70).reduced (2));
        editBtn.setBounds (line2.removeFromLeft (90).reduced (2));
        segLen.setBounds (line2.removeFromLeft (100).reduced (2));
        clearBtn.setBounds (line2.removeFromLeft (90).reduced (2));
        c.removeFromTop (8);
        auto line3 = c.removeFromTop (26);
        savePoints.setBounds (line3.removeFromLeft (110).reduced (2));
        savePath.setBounds (line3.removeFromLeft (110).reduced (2));
        loadBtn.setBounds (line3.removeFromLeft (110).reduced (2));
        hintArea = c;
    }

    V4JBAudioProcessor& proc;
    MorphPad pad;
    juce::OwnedArray<MacroColumn> macros;
    juce::OwnedArray<juce::TextButton> store;
    Combo mode, snap, userSlot;
    Knob time;
    juce::ToggleButton editBtn { "Edit Path" };
    juce::TextButton clearBtn { "Clear Path" };
    juce::ComboBox segLen;
    juce::TextButton savePoints { "Save Points..." }, savePath { "Save Path..." }, loadBtn { "Load..." };
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Rectangle<int> macroArea, ctlArea, hintArea;
};
} // namespace ui
