#include "PluginEditor.h"
#include "ui/VersionCompare.h"

#include <thread>
#include <vector>

using namespace ui;

namespace
{
constexpr int kEditorW = 1220;
constexpr int kEditorH = 800;
constexpr int kPad = 8;
constexpr int kTopH = 64;
constexpr int kKeyboardH = 72;
constexpr int kKeyLow = 36;   // C2
constexpr int kKeyHigh = 84;  // C6
constexpr int kTitleHitW = 140;

int whiteKeysInRange (int low, int high)
{
    int n = 0;
    for (int note = low; note <= high; ++note)
        if (! juce::MidiMessage::isMidiNoteBlack (note))
            ++n;
    return juce::jmax (1, n);
}
} // namespace

// ---------------------------------------------------------------- About

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

// ---------------------------------------------------------------- Editor

V4JBAudioProcessorEditor::V4JBAudioProcessorEditor (V4JBAudioProcessor& p)
    : juce::AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&panelLnF);
    setOpaque (true);
    logo = juce::ImageCache::getFromMemory (BinaryData::logo_png, BinaryData::logo_pngSize);

    presetBtn.onClick = [this] { showPresetMenu(); };
    presetBtn.setTooltip ("Presets");
    prevBtn.onClick = [this] { stepPreset (-1); };
    nextBtn.onClick = [this] { stepPreset (1); };
    saveBtn.onClick = [this] { savePresetDialog(); };
    randBtn.onClick = [this] { showRandomMenu(); };
    randBtn.setTooltip ("Smart randomizer: new values, or a whole new module chain");
    economy.setTooltip ("Economy: at most 128 partials per voice, much lighter on CPU");
    for (juce::Component* c : { (juce::Component*) &presetBtn, (juce::Component*) &prevBtn, (juce::Component*) &nextBtn,
                                (juce::Component*) &saveBtn, (juce::Component*) &randBtn, (juce::Component*) &economy,
                                (juce::Component*) &poly, (juce::Component*) &octave, (juce::Component*) &pitch,
                                (juce::Component*) &glide, (juce::Component*) &volume })
        content.addAndMakeVisible (c);

    const auto tabColour = kNight.brighter (0.05f);
    tabs.addTab ("Edit", tabColour, &editPage, false);
    tabs.addTab ("Mod", tabColour, &modPage, false);
    tabs.addTab ("Morph", tabColour, &morphPage, false);
    tabs.addTab ("Wave", tabColour, &wavePage, false);
    tabs.setTabBarDepth (28);
    tabs.setOutline (0);
    content.addAndMakeVisible (tabs);
    content.addChildComponent (aboutBox);

    // VST3 stays keyboard-free. The host already has keys; this strip is for Standalone.
    if (proc.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
    {
        keyboard = std::make_unique<juce::MidiKeyboardComponent> (
            keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard);
        keyboard->setAvailableRange (kKeyLow, kKeyHigh);
        keyboard->setScrollButtonsVisible (false);
        // Mouse only. The component's QWERTY map stays off while it never takes focus.
        keyboard->setWantsKeyboardFocus (false);
        keyboard->setMouseClickGrabsKeyboardFocus (false);
        keyboard->setColour (juce::MidiKeyboardComponent::whiteNoteColourId, kFog);
        keyboard->setColour (juce::MidiKeyboardComponent::blackNoteColourId, kNight);
        keyboard->setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, kDusk);
        keyboard->setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, kTeal.withAlpha (0.35f));
        keyboard->setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, kTeal.withAlpha (0.65f));
        keyboard->setColour (juce::MidiKeyboardComponent::textLabelColourId, kNight);
        keyboard->setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);
        keyboardState.addListener (this);
        content.addAndMakeVisible (*keyboard);
    }

    baseW = kEditorW;
    baseH = kEditorH + (keyboard != nullptr ? kKeyboardH : 0);
    content.setSize (baseW, baseH);
    addAndMakeVisible (content);

    // Drag the corner to scale (40%..150%), aspect ratio locked. The scale is saved with the session.
    // Read it first: setResizeLimits clamps the still-empty editor and resized() overwrites uiScale.
    const float s = juce::jlimit (0.4f, 1.5f, proc.uiScale);
    setResizable (true, true);
    setResizeLimits (baseW * 2 / 5, baseH * 2 / 5, baseW * 3 / 2, baseH * 3 / 2);
    getConstrainer()->setFixedAspectRatio ((double) baseW / (double) baseH);
    setSize (juce::roundToInt ((float) baseW * s), juce::roundToInt ((float) baseH * s));
    startTimerHz (30);
}

V4JBAudioProcessorEditor::~V4JBAudioProcessorEditor()
{
    stopTimer();
    if (keyboard != nullptr)
    {
        keyboardState.allNotesOff (1);  // note-offs reach the engine before the keys are destroyed
        keyboardState.removeListener (this);
    }
    setLookAndFeel (nullptr);
}

void V4JBAudioProcessorEditor::handleNoteOn (juce::MidiKeyboardState*, int, int note, float)
{
    proc.pushKey (note, true);
}

void V4JBAudioProcessorEditor::handleNoteOff (juce::MidiKeyboardState*, int, int note, float)
{
    proc.pushKey (note, false);
}

// 30 Hz: only the visible page pulls spectra and meters.
void V4JBAudioProcessorEditor::timerCallback()
{
    if (proc.currentPresetCategory.isNotEmpty())
        presetBtn.setButtonText (proc.currentPresetCategory + "  ·  " + proc.currentPresetName);
    else
        presetBtn.setButtonText (proc.currentPresetName);
    presetBtn.setTooltip (proc.currentPresetCategory.isNotEmpty()
                              ? proc.currentPresetCategory + " / " + proc.currentPresetName
                              : proc.currentPresetName);
    switch (tabs.getCurrentTabIndex())
    {
        case 0: editPage.refresh(); break;
        case 2: morphPage.refresh(); break;
        case 3: wavePage.refresh(); break;
        default: break;
    }
}

void V4JBAudioProcessorEditor::refreshPresetList()
{
    presetList = proc.getFactoryPresets();
    presetList.addArray (Presets::user());
}

void V4JBAudioProcessorEditor::showPresetMenu()
{
    refreshPresetList();

    // Group by category, factory first; menu ids are presetList index + 1.
    juce::PopupMenu menu;
    juce::StringArray categories;
    for (auto& e : presetList)
        categories.addIfNotAlreadyThere (e.category);
    for (auto& cat : categories)
    {
        juce::PopupMenu sub;
        for (int i = 0; i < presetList.size(); ++i)
            if (presetList[i].category == cat)
                sub.addItem (i + 1, presetList[i].name, true, presetList[i].name == proc.currentPresetName);
        menu.addSubMenu (cat, sub);
    }
    menu.addSeparator();
    menu.addItem (-1, "Save preset...");
    menu.addItem (-2, "Open user preset folder");

    juce::Component::SafePointer<V4JBAudioProcessorEditor> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetBtn), [safe] (int id)
    {
        if (safe == nullptr || id == 0)
            return;
        if (id == -1)
            safe->savePresetDialog();
        else if (id == -2)
        {
            Presets::userDir().createDirectory();
            Presets::userDir().startAsProcess();
        }
        else if (juce::isPositiveAndBelow (id - 1, safe->presetList.size()))
        {
            safe->presetIndex = id - 1;
            safe->proc.loadPreset (safe->presetList[id - 1]);
        }
    });
}

void V4JBAudioProcessorEditor::stepPreset (int delta)
{
    refreshPresetList();
    if (presetList.isEmpty())
        return;

    juce::Array<int> group;
    for (int i = 0; i < presetList.size(); ++i)
        if (presetList[i].category == proc.currentPresetCategory)
            group.add (i);
    if (group.isEmpty())  // Random / unknown name: walk the whole list
        for (int i = 0; i < presetList.size(); ++i)
            group.add (i);

    int pos = 0;
    for (int g = 0; g < group.size(); ++g)
        if (presetList[group[g]].name == proc.currentPresetName)
        {
            pos = g;
            break;
        }
    pos = (pos + delta + group.size()) % group.size();
    presetIndex = group[pos];
    proc.loadPreset (presetList[presetIndex]);
}

void V4JBAudioProcessorEditor::savePresetDialog()
{
    auto* w = new juce::AlertWindow ("Save preset", "Saved to Documents/V4JB/Presets", juce::MessageBoxIconType::NoIcon);
    w->addTextEditor ("name", proc.currentPresetName, "Name");
    w->addTextEditor ("category", "User", "Category");
    w->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<V4JBAudioProcessorEditor> safe (this);
    // deleteWhenDismissed: the window is deleted after this callback runs.
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w] (int result)
    {
        if (safe != nullptr && result == 1)
            safe->proc.saveUserPreset (w->getTextEditorContents ("name"), w->getTextEditorContents ("category"));
    }), true);
}

void V4JBAudioProcessorEditor::showRandomMenu()
{
    juce::PopupMenu menu;
    menu.addItem (1, "New sound (new module chain)");
    menu.addItem (2, "Vary values (keep modules)");
    juce::Component::SafePointer<V4JBAudioProcessorEditor> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&randBtn), [safe] (int id)
    {
        if (safe != nullptr && id > 0)
            safe->proc.randomize (id == 1);
    });
}

void V4JBAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (kNight);  // covers the sub-pixel edge the scaled content may leave
}

void V4JBAudioProcessorEditor::resized()
{
    if (baseW == 0)
        return;  // called before the constructor finished
    const float s = (float) getWidth() / (float) baseW;
    content.setTransform (juce::AffineTransform::scale (s));
    proc.uiScale = s;
}

void V4JBAudioProcessorEditor::paintContent (juce::Graphics& g)
{
    g.fillAll (kNight);

    auto title = content.getLocalBounds().reduced (kPad).removeFromTop (kTopH - kPad);
    if (logo.isValid())
    {
        auto logoR = title.removeFromLeft (44).reduced (2);
        g.drawImage (logo, logoR.toFloat(), juce::RectanglePlacement::centred);
        title.removeFromLeft (6);
    }
    auto nameCol = title.removeFromLeft (90).withSizeKeepingCentre (90, 40);
    g.setColour (kFog);
    g.setFont (boldFont (16.f));
    g.drawText ("V4JB", nameCol.removeFromTop (22), juce::Justification::centredLeft, false);
    g.setFont (boldFont (10.f));
    g.drawText ("v" JucePlugin_VersionString, nameCol, juce::Justification::centredLeft, false);

    g.setColour (kNight.brighter (0.08f));
    g.drawRoundedRectangle (content.getLocalBounds().toFloat().reduced (1.5f), 2.f, 2.f);
}

void V4JBAudioProcessorEditor::layoutContent()
{
    auto r = content.getLocalBounds().reduced (kPad);
    auto top = r.removeFromTop (kTopH - kPad);
    top.removeFromLeft (44 + 6 + 90 + 10);

    volume.setBounds (top.removeFromRight (70));
    for (auto* k : { &glide, &pitch, &octave, &poly })
        k->setBounds (top.removeFromRight (62));
    economy.setBounds (top.removeFromRight (84).withSizeKeepingCentre (76, 26));

    auto presets = top.withSizeKeepingCentre (top.getWidth(), 28);
    prevBtn.setBounds (presets.removeFromLeft (28).reduced (1));
    presetBtn.setBounds (presets.removeFromLeft (260).reduced (1));
    nextBtn.setBounds (presets.removeFromLeft (28).reduced (1));
    presets.removeFromLeft (8);
    saveBtn.setBounds (presets.removeFromLeft (60).reduced (1));
    presets.removeFromLeft (8);
    randBtn.setBounds (presets.removeFromLeft (100).reduced (1));

    if (keyboard != nullptr)
    {
        auto keys = r.removeFromBottom (kKeyboardH);
        keyboard->setBounds (keys);
        keyboard->setKeyWidth ((float) keys.getWidth() / (float) whiteKeysInRange (kKeyLow, kKeyHigh));
        r.removeFromBottom (kPad);
    }
    tabs.setBounds (r);
    aboutBox.setBounds (content.getLocalBounds());
}

juce::Rectangle<int> V4JBAudioProcessorEditor::titleHitBounds() const
{
    return { kPad, kPad, kTitleHitW, kTopH - kPad };
}

void V4JBAudioProcessorEditor::contentMouseUp (const juce::MouseEvent& e)
{
    if (e.mouseWasClicked() && titleHitBounds().contains (e.getPosition()))
    {
        aboutBox.setVisible (true);
        aboutBox.toFront (true);
    }
}
