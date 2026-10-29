#include "map_selection.hpp"
#include "yrpp/ScenarioClass.h"
#include "yrpp/HouseTypeClass.h"
#include "yrpp/SideClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/CCFileClass.h"
#include <memory>
#include <string>
#include <vector>
#include <cstdio>

namespace game {
class MapChoices {
public:
    struct Entry {
        std::string section, filename;
        std::vector<std::string> links;
    };
    std::vector<Entry> entries;
};
namespace {
bool equal_name(const char* a, const char* b) noexcept {
    if (!a || !b) return false;
    do {
        const auto upper = [](unsigned char c) { return c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c; };
        if (upper(*a) != upper(*b)) return false;
    } while (*a++ && *b++);
    return true;
}
bool read_choices(const char* file_name, const char* side, MapChoices& result) {
    CCFileClass file(file_name);
    if (!file.Exists()) return false;
    CCINIClass ini;
    if (ini.ReadCCFile(&file, false, false) <= 0) return false;
    // 0x005CEEF0 stops at the first missing numbered side entry, up to 100.
    for (int n = 1; n <= 100; ++n) {
        char key[12], section[512]{}, text[512]{};
        std::snprintf(key, sizeof(key), "%d", n);
        if (!ini.ReadString(side, key, "", section, sizeof(section))) break;
        MapChoices::Entry entry;
        entry.section = section;
        ini.ReadString(section, "Scenario", "", text, sizeof(text));
        entry.filename = text;
        // 0x005CFF81 enumerates numeric keys in ascending order, including gaps.
        for (int link = 0; link < 256; ++link) {
            std::snprintf(key, sizeof(key), "%d", link);
            if (ini.ReadString(section, key, "", text, sizeof(text))) entry.links.emplace_back(text);
        }
        result.entries.push_back(std::move(entry));
    }
    return !result.entries.empty();
}
}
int find_map_choice(const MapChoices& choices, const char* filename) {
    if (!filename || !*filename) return -1;
    for (unsigned i = 0; i < choices.entries.size(); ++i)
        if (equal_name(choices.entries[i].filename.c_str(), filename)) return static_cast<int>(i);
    return -1;
}
bool open_map_choices(const ScenarioClass& scenario, MapChoices*& output) {
    if (output) return false;
    auto* country = HouseTypeClass::Array.GetItemOrDefault(scenario.HumanPlayerHouseTypeIndex);
    auto* side = country ? SideClass::Array.GetItemOrDefault(country->SideIndex) : nullptr;
    if (!side) return false;
    // YR uses MAPSELMD. The standalone host can also launch classic maps:
    // select their resource graph by membership, never by filename numbering.
    auto choices = std::make_unique<MapChoices>();
    if (read_choices("MAPSELMD.INI", side->ID, *choices) &&
        find_map_choice(*choices, scenario.FileName) >= 0) {
        output = choices.release();
        return true;
    }
    auto classic = std::make_unique<MapChoices>();
    if (read_choices("MAPSEL.INI", side->ID, *classic) &&
        find_map_choice(*classic, scenario.FileName) >= 0) {
        output = classic.release();
        return true;
    }
    // Preserve the original default graph for explicit-stage validation.
    if (choices->entries.empty()) return false;
    output = choices.release();
    return true;
}
void close_map_choices(MapChoices* choices) noexcept { delete choices; }
int map_choice_count(const MapChoices& choices) noexcept { return static_cast<int>(choices.entries.size()); }
const char* map_choice_filename(const MapChoices& choices, int index) noexcept {
    return index >= 0 && index < map_choice_count(choices) ? choices.entries[index].filename.c_str() : nullptr;
}
int map_choice_link_count(const MapChoices& choices, int index) noexcept {
    return index >= 0 && index < map_choice_count(choices) ? static_cast<int>(choices.entries[index].links.size()) : 0;
}
int map_choice_link(const MapChoices& choices, int index, int link) noexcept {
    if (link < 0 || link >= map_choice_link_count(choices, index)) return -1;
    const auto& name = choices.entries[index].links[link];
    // MSChoice looks up section identifiers with its string comparison.
    for (unsigned i = 0; i < choices.entries.size(); ++i)
        if (equal_name(choices.entries[i].section.c_str(), name.c_str())) return static_cast<int>(i);
    return -1;
}
}
