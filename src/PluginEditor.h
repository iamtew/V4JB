#pragma once

#include "PluginProcessor.h"

#include <atomic>

class V4JBAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit V4JBAudioProcessorEditor (V4JBAudioProcessor&);
    ~V4JBAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::Rectangle<int> titleHitBounds() const;
    juce::Rectangle<int> textureArea() const;

    V4JBAudioProcessor& proc;

    struct PanelLnF : public juce::LookAndFeel_V4
    {
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                               float startAngle, float endAngle, juce::Slider&) override;
        void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
        juce::Font getLabelFont (juce::Label&) override;
    };

    // Click the name/version in the top left. "Check for update" asks GitHub for tags.
    struct AboutBox : public juce::Component
    {
        AboutBox();
        void paint (juce::Graphics& g) override;
        void resized() override;
        void mouseUp (const juce::MouseEvent& e) override;
        void checkForUpdate();
        void applyResult (const juce::String& text, bool hasUpdate);
        juce::Rectangle<int> cardBounds() const;

        juce::Label status;
        juce::ToggleButton checkBtn { "Check for update" };
        juce::HyperlinkButton releasesBtn {
            "Open releases",
            juce::URL ("https://github.com/iamtew/V4JB/releases/latest")
        };
        juce::ToggleButton closeBtn { "Close" };
        std::atomic<bool> fetching { false };
    };

    struct Knob : public juce::Component
    {
        Knob (juce::AudioProcessorValueTreeState& st, const juce::String& id, const juce::String& name);
        void resized() override;

        juce::Label label;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    PanelLnF panelLnF;
    Knob volume { proc.apvts, ParamID::level, "Volume" };
    Knob motion  { proc.apvts, ParamID::motion,  "Motion" };
    Knob crackle { proc.apvts, ParamID::crackle, "Crackle" };
    Knob acid    { proc.apvts, ParamID::acid,    "Acid" };
    Knob air     { proc.apvts, ParamID::air,     "Air" };
    AboutBox aboutBox;
    juce::Image logo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (V4JBAudioProcessorEditor)
};
