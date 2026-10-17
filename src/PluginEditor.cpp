#include "PluginEditor.h"
#include "ui/VersionCompare.h"

#include <thread>
#include <vector>

namespace
{
constexpr int kEditorW = 720;
constexpr int kEditorH = 460;
constexpr int kPad = 8;
constexpr int kTitleH = 40;
constexpr int kTitleHitW = 150;
constexpr int kLabelH = 12;
constexpr int kKnobW = 72;
constexpr int kMacroW = 130;
constexpr int kMacroH = 150;

// "Fog at night" palette, same hex as bootstrap.md and docs/style.css.
const juce::Colour kNight { 0xff14121f };
const juce::Colour kDusk  { 0xff24233a };
const juce::Colour kFog   { 0xff9aa3b5 };
const juce::Colour kTeal  { 0xff6fc3b8 };
const juce::Colour kAmber { 0xffe0a458 };
const juce::Colour kWhite { 0xffffffff };

juce::Font boldFont (float h)
{
    return juce::Font (juce::FontOptions (h).withStyle ("Bold"));
}
} // namespace

V4JBAudioProcessorEditor::Knob::Knob (juce::AudioProcessorValueTreeState& st,
                                      const juce::String& id, const juce::String& name)
{
    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, kFog);
    label.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (label);

    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 56, 12);
    slider.setColour (juce::Slider::textBoxTextColourId, kFog);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (slider);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (st, id, slider);
}

void V4JBAudioProcessorEditor::Knob::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (kLabelH));
    slider.setBounds (r);
}

void V4JBAudioProcessorEditor::PanelLnF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                                           float pos, float startAngle, float endAngle,
                                                           juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.f);
    const float d = juce::jmin (bounds.getWidth(), bounds.getHeight());
    auto rc = bounds.withSizeKeepingCentre (d, d);
    const auto c = rc.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);

    g.setColour (kDusk);
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

void V4JBAudioProcessorEditor::PanelLnF::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool)
{
    auto bounds = b.getLocalBounds().toFloat().reduced (2.f);
    g.setColour (over ? kDusk.brighter (0.2f) : kDusk);
    g.fillRoundedRectangle (bounds, 3.f);
    g.setFont (boldFont (11.f));
    g.setColour (kFog);
    g.drawText (b.getButtonText(), bounds.toNearestInt(), juce::Justification::centred, false);
}

juce::Font V4JBAudioProcessorEditor::PanelLnF::getLabelFont (juce::Label&)
{
    return boldFont (11.f);
}

V4JBAudioProcessorEditor::AboutBox::AboutBox()
{
    status.setJustificationType (juce::Justification::centred);
    status.setColour (juce::Label::textColourId, kFog);
    status.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (status);

    // ToggleButton is only used for the LnF look here, so it must not latch.
    checkBtn.setClickingTogglesState (false);
    closeBtn.setClickingTogglesState (false);
    checkBtn.onClick = [this] { checkForUpdate(); };
    closeBtn.onClick = [this] { setVisible (false); };
    addAndMakeVisible (checkBtn);
    addAndMakeVisible (closeBtn);

    releasesBtn.setColour (juce::HyperlinkButton::textColourId, kAmber);
    releasesBtn.setVisible (false);
    addAndMakeVisible (releasesBtn);
}

void V4JBAudioProcessorEditor::AboutBox::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.55f));
    auto card = cardBounds().toFloat();
    g.setColour (kNight);
    g.fillRoundedRectangle (card, 6.f);
    g.setColour (kFog.withAlpha (0.35f));
    g.drawRoundedRectangle (card.reduced (0.5f), 6.f, 1.f);

    auto r = cardBounds().reduced (16, 14);
    g.setColour (kFog);
    g.setFont (boldFont (20.f));
    g.drawText ("V4JB", r.removeFromTop (26), juce::Justification::centred, false);
    g.setFont (boldFont (12.f));
    g.drawText ("v" JucePlugin_VersionString, r.removeFromTop (18), juce::Justification::centred, false);
    g.setFont (boldFont (11.f));
    g.setColour (kFog.withAlpha (0.8f));
    g.drawText ("Stupid Systems LLC", r.removeFromTop (16), juce::Justification::centred, false);
    g.drawText ("MIT", r.removeFromTop (16), juce::Justification::centred, false);
}

void V4JBAudioProcessorEditor::AboutBox::resized()
{
    auto r = cardBounds().reduced (16, 14);
    r.removeFromTop (26 + 18 + 16 + 16 + 8);
    checkBtn.setBounds (r.removeFromTop (22).withSizeKeepingCentre (160, 22));
    r.removeFromTop (6);
    status.setBounds (r.removeFromTop (22));
    r.removeFromTop (4);
    releasesBtn.setBounds (r.removeFromTop (20).withSizeKeepingCentre (140, 20));
    closeBtn.setBounds (cardBounds().removeFromBottom (36).reduced (90, 8));
}

void V4JBAudioProcessorEditor::AboutBox::mouseUp (const juce::MouseEvent& e)
{
    if (e.mouseWasClicked() && ! cardBounds().contains (e.getPosition()))
        setVisible (false);
}

juce::Rectangle<int> V4JBAudioProcessorEditor::AboutBox::cardBounds() const
{
    return getLocalBounds().withSizeKeepingCentre (320, 240);
}

void V4JBAudioProcessorEditor::AboutBox::applyResult (const juce::String& text, bool hasUpdate)
{
    fetching.store (false);
    status.setText (text, juce::dontSendNotification);
    releasesBtn.setVisible (hasUpdate);
}

void V4JBAudioProcessorEditor::AboutBox::checkForUpdate()
{
    if (fetching.exchange (true))
        return;
    status.setText ("Checking...", juce::dontSendNotification);
    releasesBtn.setVisible (false);
    // Network on a detached thread; the result hops back to the message thread
    // and is dropped if the editor closed meanwhile (SafePointer goes null).
    juce::Component::SafePointer<AboutBox> safe (this);
    std::thread ([safe]
    {
        juce::String text;
        bool hasUpdate = false;
        juce::URL url ("https://api.github.com/repos/iamtew/V4JB/tags");
        const auto opts = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                              .withExtraHeaders ("User-Agent: V4JB\nAccept: application/vnd.github+json")
                              .withConnectionTimeoutMs (8000);
        if (auto stream = url.createInputStream (opts))
        {
            const auto parsed = juce::JSON::parse (stream->readEntireStreamAsString());
            if (auto* arr = parsed.getArray())
            {
                std::vector<std::string> names;
                names.reserve ((size_t) arr->size());
                for (const auto& v : *arr)
                    if (auto* obj = v.getDynamicObject())
                        names.push_back (obj->getProperty ("name").toString().toStdString());
                const auto newest = v4jb::newestNewerTag (names, JucePlugin_VersionString);
                if (newest.empty())
                    text = "You are up to date.";
                else
                {
                    text = "Newer version available: " + juce::String (newest);
                    hasUpdate = true;
                }
            }
            else
                text = "Could not reach GitHub.";
        }
        else
            text = "Could not reach GitHub.";

        juce::MessageManager::callAsync ([safe, text, hasUpdate]
        {
            if (safe != nullptr)
                safe->applyResult (text, hasUpdate);
        });
    }).detach();
}

V4JBAudioProcessorEditor::V4JBAudioProcessorEditor (V4JBAudioProcessor& p)
    : juce::AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&panelLnF);
    setOpaque (true);

    logo = juce::ImageCache::getFromMemory (BinaryData::logo_png, BinaryData::logo_pngSize);

    for (auto* k : { &volume, &motion, &crackle, &acid, &air })
        addAndMakeVisible (k);
    addChildComponent (aboutBox);

    setSize (kEditorW, kEditorH);
}

V4JBAudioProcessorEditor::~V4JBAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void V4JBAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (kNight);

    auto r = getLocalBounds().reduced (kPad);
    auto title = r.removeFromTop (kTitleH);
    if (logo.isValid())
    {
        auto logoR = title.removeFromLeft (kTitleH).reduced (2);
        g.drawImage (logo, logoR.toFloat(), juce::RectanglePlacement::centred);
        title.removeFromLeft (6);
    }
    auto nameCol = title.removeFromLeft (90);
    g.setColour (kFog);
    g.setFont (boldFont (16.f));
    g.drawText ("V4JB", nameCol.removeFromTop (22), juce::Justification::centredLeft, false);
    g.setFont (boldFont (10.f));
    g.drawText ("v" JucePlugin_VersionString, nameCol, juce::Justification::centredLeft, false);

    // Texture well: the Swirl macros sit in here (see resized()).
    auto well = textureArea();
    g.setColour (kDusk);
    g.fillRoundedRectangle (well.toFloat(), 4.f);
    g.setColour (kFog.withAlpha (0.4f));
    g.setFont (boldFont (11.f));
    g.drawText ("Hold a MIDI note to play", well.removeFromBottom (28), juce::Justification::centred, false);

    g.setColour (kNight.brighter (0.08f));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.5f), 2.f, 2.f);
}

void V4JBAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (kPad);
    auto title = r.removeFromTop (kTitleH);
    volume.setBounds (title.removeFromRight (kKnobW).withHeight (kTitleH + 40));

    // Four big macro knobs, one row, centred in the well.
    auto row = textureArea().withSizeKeepingCentre (4 * kMacroW, kMacroH);
    for (auto* k : { &motion, &crackle, &acid, &air })
        k->setBounds (row.removeFromLeft (kMacroW).reduced (kPad, 0));

    aboutBox.setBounds (getLocalBounds());
}

juce::Rectangle<int> V4JBAudioProcessorEditor::titleHitBounds() const
{
    return { kPad, kPad, kTitleHitW, kTitleH };
}

juce::Rectangle<int> V4JBAudioProcessorEditor::textureArea() const
{
    auto r = getLocalBounds().reduced (kPad);
    r.removeFromTop (kTitleH + 44);
    return r;
}

void V4JBAudioProcessorEditor::mouseUp (const juce::MouseEvent& e)
{
    if (e.mouseWasClicked() && titleHitBounds().contains (e.getPosition()))
    {
        aboutBox.setVisible (true);
        aboutBox.toFront (true);
    }
}
