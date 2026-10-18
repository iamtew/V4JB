#pragma once

#include <JuceHeader.h>

// Preset files are the plugin state XML (<PARAMS>, same as a DAW session) plus
// presetName / category attributes. Factory presets are compiled in from
// presets/*.xml and may list only the parameters they change; the rest load at
// their defaults. User presets are full snapshots in Documents/V4JB/Presets.
struct PresetEntry
{
    juce::String name, category;
    juce::String resource;  // factory: BinaryData resource name
    juce::File file;        // user: file on disk
};

namespace Presets
{
inline juce::File userDir()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("V4JB").getChildFile ("Presets");
}

inline std::unique_ptr<juce::XmlElement> parse (const PresetEntry& e)
{
    if (e.resource.isNotEmpty())
    {
        int size = 0;
        if (const char* data = BinaryData::getNamedResource (e.resource.toRawUTF8(), size))
            return juce::parseXML (juce::String::fromUTF8 (data, size));
        return nullptr;
    }
    return juce::parseXML (e.file);
}

inline PresetEntry entryFrom (const juce::XmlElement& xml, const juce::String& fallbackName)
{
    PresetEntry e;
    e.name = xml.getStringAttribute ("presetName", fallbackName);
    e.category = xml.getStringAttribute ("category", "User");
    return e;
}

// Factory presets in file-name order (files are numbered, Init first; natural
// order so 100_x comes after 99_x).
inline juce::Array<PresetEntry> factory()
{
    struct Item { juce::String file; PresetEntry entry; };
    std::vector<Item> items;
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        const juce::String res = BinaryData::namedResourceList[i];
        const juce::String file = BinaryData::getNamedResourceOriginalFilename (res.toRawUTF8());
        if (! file.endsWithIgnoreCase (".xml"))
            continue;
        PresetEntry probe;
        probe.resource = res;
        if (auto xml = parse (probe))
        {
            auto e = entryFrom (*xml, file.upToLastOccurrenceOf (".", false, false));
            e.resource = res;
            items.push_back ({ file, e });
        }
    }
    std::sort (items.begin(), items.end(), [] (const Item& a, const Item& b) { return a.file.compareNatural (b.file) < 0; });
    juce::Array<PresetEntry> out;
    for (auto& it : items)
        out.add (it.entry);
    return out;
}

inline juce::Array<PresetEntry> user()
{
    juce::Array<PresetEntry> out;
    for (const auto& f : userDir().findChildFiles (juce::File::findFiles, false, "*.xml"))
        if (auto xml = juce::parseXML (f))
        {
            auto e = entryFrom (*xml, f.getFileNameWithoutExtension());
            e.file = f;
            out.add (e);
        }
    return out;
}
} // namespace Presets
