#pragma once

#include "PluginProcessor.h"
#include "ui/EditPage.h"
#include "ui/ModPage.h"
#include "ui/MorphPage.h"
#include "ui/WavePage.h"

#include <atomic>

// Top bar (About, presets, randomizer, voice controls, volume) over four tabs:
// Edit (the 12-slot chain), Mod (envelopes, LFOs), Morph, Wave.
class V4JBAudioProcessorEditor : public juce::AudioProcessorEditor,
                                 private juce::MidiKeyboardState::Listener,
                                 private juce::Timer
{
public:
    explicit V4JBAudioProcessorEditor (V4JBAudioProcessor&);
    ~V4JBAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // Everything lives in `content`, laid out at a fixed base size and scaled as
    // one picture. Dragging the corner changes only the scale, so nothing reflows.
    struct Content : public juce::Component
    {
        explicit Content (V4JBAudioProcessorEditor& e) : ed (e) {}
        void paint (juce::Graphics& g) override { ed.paintContent (g); }
        void resized() override { ed.layoutContent(); }
        void mouseUp (const juce::MouseEvent& e) override { ed.contentMouseUp (e); }
        V4JBAudioProcessorEditor& ed;
    };
    void paintContent (juce::Graphics&);
    void layoutContent();
    void contentMouseUp (const juce::MouseEvent&);

    void handleNoteOn (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void timerCallback() override;
    juce::Rectangle<int> titleHitBounds() const;

    void showPresetMenu();
    void refreshPresetList();
    void stepPreset (int delta);
    void savePresetDialog();
    void showRandomMenu();

    V4JBAudioProcessor& proc;
    ui::PanelLnF panelLnF;
    Content content { *this };  // declared before its children so it is destroyed after them
    int baseW = 0, baseH = 0;   // unscaled size (taller in Standalone: piano keys)

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

    // Top bar.
    juce::TextButton presetBtn, prevBtn { "<" }, nextBtn { ">" }, saveBtn { "Save" }, randBtn { "Randomize" };
    ui::Toggle economy { proc.apvts, ParamID::economy, "Economy" };
    ui::Knob poly   { proc.apvts, ParamID::poly,   "Voices" };
    ui::Knob octave { proc.apvts, ParamID::octave, "Octave" };
    ui::Knob pitch  { proc.apvts, ParamID::pitch,  "Pitch" };
    ui::Knob glide  { proc.apvts, ParamID::glide,  "Glide" };
    ui::Knob volume { proc.apvts, ParamID::level,  "Volume" };

    // Pages.
    ui::EditPage editPage { proc };
    ui::ModPage modPage { proc };
    ui::MorphPage morphPage { proc };
    ui::WavePage wavePage { proc };
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };

    AboutBox aboutBox;
    juce::TooltipWindow tooltips { this };
    juce::Image logo;
    juce::Array<PresetEntry> presetList;  // factory then user, rebuilt for the menu and arrows
    int presetIndex = 0;

    // Standalone only. Declared after the state so the component dies first.
    juce::MidiKeyboardState keyboardState;
    std::unique_ptr<juce::MidiKeyboardComponent> keyboard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (V4JBAudioProcessorEditor)
};
