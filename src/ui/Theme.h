#pragma once

#include "../PluginProcessor.h"

#include <JuceHeader.h>

// Shared look and small widgets for every page. "Fog at night" palette, same hex
// as bootstrap.md and docs/style.css. Header-only: only PluginEditor.cpp includes the UI.
namespace ui
{
inline const juce::Colour kNight { 0xff14121f };
inline const juce::Colour kDusk  { 0xff24233a };
inline const juce::Colour kFog   { 0xff9aa3b5 };
inline const juce::Colour kTeal  { 0xff6fc3b8 };
inline const juce::Colour kAmber { 0xffe0a458 };
inline const juce::Colour kWhite { 0xffffffff };

inline juce::Font boldFont (float h) { return juce::Font (juce::FontOptions (h).withStyle ("Bold")); }

struct PanelLnF : public juce::LookAndFeel_V4
{
    PanelLnF()
    {
        setColour (juce::ComboBox::backgroundColourId, kNight);
        setColour (juce::ComboBox::textColourId, kFog);
        setColour (juce::ComboBox::outlineColourId, kFog.withAlpha (0.3f));
        setColour (juce::ComboBox::arrowColourId, kTeal);
        setColour (juce::PopupMenu::backgroundColourId, kNight);
        setColour (juce::PopupMenu::textColourId, kFog);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, kDusk.brighter (0.2f));
        setColour (juce::PopupMenu::highlightedTextColourId, kWhite);
        setColour (juce::PopupMenu::headerTextColourId, kAmber);
        setColour (juce::Slider::trackColourId, kTeal);
        setColour (juce::Slider::backgroundColourId, kNight);
        setColour (juce::Slider::thumbColourId, kFog);
        setColour (juce::Slider::textBoxTextColourId, kFog);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId, kFog);
        setColour (juce::TabbedButtonBar::tabTextColourId, kFog);
        setColour (juce::TabbedButtonBar::frontTextColourId, kWhite);
        setColour (juce::TextButton::buttonColourId, kDusk);
        setColour (juce::TextButton::textColourOffId, kFog);
        setColour (juce::AlertWindow::backgroundColourId, kNight);
        setColour (juce::AlertWindow::textColourId, kFog);
        setColour (juce::TextEditor::backgroundColourId, kDusk);
        setColour (juce::TextEditor::textColourId, kFog);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.f);
        const float d = juce::jmin (bounds.getWidth(), bounds.getHeight());
        auto rc = bounds.withSizeKeepingCentre (d, d);
        const auto c = rc.getCentre();
        const float angle = startAngle + pos * (endAngle - startAngle);

        g.setColour (kDusk.brighter (0.15f));
        g.fillEllipse (rc);

        // Teal arc shows the value, cap sits on top.
        juce::Path arc;
        arc.addCentredArc (c.x, c.y, d * 0.44f, d * 0.44f, 0.f, startAngle, angle, true);
        g.setColour (kTeal);
        g.strokePath (arc, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        auto cap = rc.reduced (d * 0.18f);
        g.setColour (kFog);
        g.fillEllipse (cap);
        const auto tip = c.getPointOnCircumference (cap.getWidth() * 0.42f, angle);
        g.setColour (kNight);
        g.drawLine (c.x, c.y, tip.x, tip.y, 2.2f);
    }

    // Toggles look like buttons; "on" lights up teal.
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool) override
    {
        auto bounds = b.getLocalBounds().toFloat().reduced (2.f);
        const bool on = b.getToggleState();
        g.setColour (on ? kTeal.withAlpha (0.85f) : (over ? kDusk.brighter (0.25f) : kDusk.brighter (0.1f)));
        g.fillRoundedRectangle (bounds, 3.f);
        g.setFont (boldFont (11.f));
        g.setColour (on ? kNight : kFog);
        g.drawText (b.getButtonText(), bounds.toNearestInt(), juce::Justification::centred, false);
    }

    juce::Font getLabelFont (juce::Label&) override { return boldFont (11.f); }
    juce::Font getComboBoxFont (juce::ComboBox&) override { return boldFont (11.f); }
    juce::Font getPopupMenuFont() override { return boldFont (12.f); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return boldFont (11.f); }
};

using APVTS = juce::AudioProcessorValueTreeState;

// Rotary knob with a name above and the value below.
struct Knob : public juce::Component
{
    Knob (APVTS& st, const juce::String& id, const juce::String& name)
    {
        label.setText (name, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (label);
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 12);
        // Set here, not only in PanelLnF: the text box is built before the editor installs the LnF.
        slider.setColour (juce::Slider::textBoxTextColourId, kFog);
        slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (slider);
        att = std::make_unique<APVTS::SliderAttachment> (st, id, slider);
    }
    void resized() override
    {
        auto r = getLocalBounds();
        label.setBounds (r.removeFromTop (12));
        slider.setBounds (r);
    }
    juce::Label label;
    juce::Slider slider;
    std::unique_ptr<APVTS::SliderAttachment> att;
};

// Horizontal slider with a name on the left. Used for module params and route amounts.
struct HSlider : public juce::Component
{
    HSlider (APVTS& st, const juce::String& id, const juce::String& name)
    {
        label.setText (name, juce::dontSendNotification);
        label.setFont (boldFont (10.f));
        addAndMakeVisible (label);
        slider.setSliderStyle (juce::Slider::LinearBar);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setColour (juce::Slider::trackColourId, kTeal.withAlpha (0.6f));
        slider.setColour (juce::Slider::backgroundColourId, kNight);
        slider.setPopupDisplayEnabled (true, false, nullptr);
        addAndMakeVisible (slider);
        att = std::make_unique<APVTS::SliderAttachment> (st, id, slider);
    }
    void resized() override
    {
        auto r = getLocalBounds();
        label.setBounds (r.removeFromLeft (labelW));
        slider.setBounds (r.reduced (0, 3));
    }
    int labelW = 62;
    juce::Label label;
    juce::Slider slider;
    std::unique_ptr<APVTS::SliderAttachment> att;
};

// ComboBox filled from the choice parameter's own item list.
struct Combo : public juce::ComboBox
{
    Combo (APVTS& st, const juce::String& id)
    {
        if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (st.getParameter (id)))
            addItemList (c->choices, 1);
        att = std::make_unique<APVTS::ComboBoxAttachment> (st, id, *this);
    }
    std::unique_ptr<APVTS::ComboBoxAttachment> att;
};

struct Toggle : public juce::ToggleButton
{
    Toggle (APVTS& st, const juce::String& id, const juce::String& text) : juce::ToggleButton (text)
    {
        att = std::make_unique<APVTS::ButtonAttachment> (st, id, *this);
    }
    std::unique_ptr<APVTS::ButtonAttachment> att;
};

// Bar graph of the first 64 partial levels after one stage, in dB (-60..0).
// refresh() is called by the editor's timer; bars fall back slowly so they read well.
struct SpectrumBars : public juce::Component
{
    SpectrumBars (V4JBAudioProcessor& p, int stageIndex) : proc (p), stage (stageIndex) {}

    void refresh()
    {
        bool changed = false;
        for (int b = 0; b < v4jb::kBars; ++b)
        {
            const float a = proc.tap (stage, b);
            const float h = a > 1.e-6f ? juce::jlimit (0.f, 1.f, (20.f * std::log10 (a) + 60.f) / 60.f) : 0.f;
            const float next = juce::jmax (h, shown[b] - 0.04f);
            changed = changed || std::abs (next - shown[b]) > 1.e-3f;
            shown[b] = next;
        }
        if (changed)
            repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (kNight);
        g.fillRoundedRectangle (r, 3.f);
        const float bw = r.getWidth() / (float) v4jb::kBars;
        g.setColour (kTeal.withAlpha (0.85f));
        for (int b = 0; b < v4jb::kBars; ++b)
        {
            const float h = shown[b] * (r.getHeight() - 2.f);
            g.fillRect (r.getX() + (float) b * bw, r.getBottom() - 1.f - h, juce::jmax (1.f, bw - 1.f), h);
        }
    }

    V4JBAudioProcessor& proc;
    int stage;
    float shown[v4jb::kBars] {};
};

// Rounded card with a title in the top-left; children lay out below kTitleH.
inline void paintCard (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title)
{
    g.setColour (kDusk);
    g.fillRoundedRectangle (r.toFloat(), 4.f);
    g.setColour (kAmber);
    g.setFont (boldFont (11.f));
    g.drawText (title, r.reduced (8, 4).removeFromTop (14), juce::Justification::centredLeft, false);
}

// Lays out components left to right in equal cells.
inline void row (juce::Rectangle<int> r, std::initializer_list<juce::Component*> items)
{
    if (items.size() == 0)
        return;
    const int w = r.getWidth() / (int) items.size();
    for (auto* c : items)
        c->setBounds (r.removeFromLeft (w).reduced (2, 0));
}
} // namespace ui
