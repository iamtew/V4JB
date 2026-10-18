#pragma once

#include "Theme.h"
#include "../WaveAnalyzer.h"

// Wave page: load the two audio files the Wave and Double Wave modules follow,
// set the playback region and speed, and see the analysed spectra.
namespace ui
{
struct WavePanel : public juce::Component
{
    WavePanel (V4JBAudioProcessor& p, int w)
        : proc (p), index (w),
          start (p.apvts, ParamID::wave (w, "start"), "Start"),
          end (p.apvts, ParamID::wave (w, "end"), "End"),
          speed (p.apvts, ParamID::wave (w, "speed"), "Speed")
    {
        for (auto* k : { &start, &end, &speed })
            addAndMakeVisible (k);
        loadBtn.onClick = [this] { choose(); };
        clearBtn.onClick = [this] { proc.clearWave (index); };
        addAndMakeVisible (loadBtn);
        addAndMakeVisible (clearBtn);
        startParam = p.apvts.getRawParameterValue (ParamID::wave (w, "start"));
        endParam = p.apvts.getRawParameterValue (ParamID::wave (w, "end"));
    }

    // Built-in waves first, then a file browser.
    void choose()
    {
        const auto builtIn = WaveAnalyzer::factoryWaves();
        juce::PopupMenu menu;
        menu.addSectionHeader ("Built-in");
        for (int i = 0; i < builtIn.size(); ++i)
            menu.addItem (i + 1, builtIn[i].upToLastOccurrenceOf (".", false, false));
        menu.addSeparator();
        menu.addItem (-1, "File...");
        juce::Component::SafePointer<WavePanel> safe (this);
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&loadBtn), [safe, builtIn] (int id)
        {
            if (safe == nullptr || id == 0)
                return;
            if (id == -1)
                safe->browse();
            else
                safe->proc.loadWave (safe->index, WaveAnalyzer::kFactoryPrefix + builtIn[id - 1]);
        });
    }

    void browse()
    {
        chooser = std::make_unique<juce::FileChooser> ("Load audio for Wave " + juce::String (index == 0 ? "A" : "B"),
                                                       juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
        juce::Component::SafePointer<WavePanel> safe (this);
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [safe] (const juce::FileChooser& fc)
                              {
                                  if (safe != nullptr && fc.getResult().existsAsFile())
                                      safe->proc.loadWave (safe->index, fc.getResult().getFullPathName());
                              });
    }

    // Rebuild the spectrogram image when the wave changes: time left to right,
    // frequency bottom to top, brightness = level.
    void rebuild()
    {
        data = proc.getWave (index);
        image = {};
        if (data == nullptr || data->numFrames == 0)
            return;
        const int w = juce::jmin (data->numFrames, 1024), h = v4jb::WaveData::kBands;
        image = juce::Image (juce::Image::RGB, w, h, true);
        for (int x = 0; x < w; ++x)
        {
            const float* row = data->gain.data() + (size_t) (x * (data->numFrames - 1) / juce::jmax (1, w - 1)) * h;
            for (int b = 0; b < h; ++b)
            {
                const float level = juce::jlimit (0.f, 1.f, (20.f * std::log10 (juce::jmax (row[b], 1.e-6f)) + 60.f) / 60.f);
                image.setPixelAt (x, h - 1 - b, kNight.interpolatedWith (kTeal, level));
            }
        }
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        paintCard (g, getLocalBounds(), index == 0 ? "WAVE A" : "WAVE B");
        g.setColour (kNight);
        g.fillRoundedRectangle (view.toFloat(), 3.f);
        g.setFont (boldFont (11.f));
        if (image.isValid())
        {
            g.drawImage (image, view.toFloat(), juce::RectanglePlacement::stretchToFit);
            // Playback region.
            const float x0 = (float) view.getX() + startParam->load() * (float) view.getWidth();
            const float x1 = (float) view.getX() + endParam->load() * (float) view.getWidth();
            g.setColour (kAmber);
            g.drawVerticalLine ((int) x0, (float) view.getY(), (float) view.getBottom());
            g.drawVerticalLine ((int) x1, (float) view.getY(), (float) view.getBottom());
            g.setColour (kFog);
            g.drawText (proc.getWaveName (index) + "  (" + juce::String (data->seconds, 1) + " s)",
                        nameArea, juce::Justification::centredLeft, true);
        }
        else
        {
            g.setColour (kFog.withAlpha (0.5f));
            g.drawText ("No file loaded", view, juce::Justification::centred, false);
        }
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (8, 4);
        auto head = r.removeFromTop (22);
        head.removeFromLeft (60);
        clearBtn.setBounds (head.removeFromRight (60).reduced (2, 1));
        loadBtn.setBounds (head.removeFromRight (80).reduced (2, 1));
        nameArea = head.reduced (4, 0);
        r.removeFromTop (4);
        auto knobs = r.removeFromBottom (80);
        view = r.reduced (0, 4);
        row (knobs.withSizeKeepingCentre (240, 80), { &start, &end, &speed });
    }

    V4JBAudioProcessor& proc;
    int index;
    Knob start, end, speed;
    juce::TextButton loadBtn { "Load..." }, clearBtn { "Clear" };
    std::unique_ptr<juce::FileChooser> chooser;
    std::shared_ptr<v4jb::WaveData> data;
    juce::Image image;
    juce::Rectangle<int> view, nameArea;
    std::atomic<float>* startParam = nullptr, *endParam = nullptr;
};

struct WavePage : public juce::Component
{
    WavePage (V4JBAudioProcessor& p) : proc (p), a (p, 0), b (p, 1), balance (p.apvts, ParamID::waBalance, "Balance")
    {
        addAndMakeVisible (a);
        addAndMakeVisible (b);
        addAndMakeVisible (balance);
    }
    void refresh()
    {
        const int v = proc.waveVersion.load();
        if (v != seenVersion)
        {
            seenVersion = v;
            a.rebuild();
            b.rebuild();
        }
        a.repaint();  // region lines follow the knobs
        b.repaint();
    }
    void paint (juce::Graphics& g) override
    {
        g.setColour (kFog.withAlpha (0.6f));
        g.setFont (boldFont (11.f));
        g.drawText ("Put a Wave module (follows A, or B with Source B) or a Double Wave module (A and B, mixed by Balance) "
                    "in a slot on the Edit page. End before Start plays backwards.",
                    hint, juce::Justification::centred, true);
    }
    void resized() override
    {
        auto r = getLocalBounds().reduced (6);
        hint = r.removeFromBottom (36);
        auto bal = r.removeFromBottom (90);
        balance.setBounds (bal.withSizeKeepingCentre (90, 86));
        a.setBounds (r.removeFromLeft (r.getWidth() / 2).reduced (3));
        b.setBounds (r.reduced (3));
    }
    V4JBAudioProcessor& proc;
    WavePanel a, b;
    Knob balance;
    juce::Rectangle<int> hint;
    int seenVersion = -1;
};
} // namespace ui
