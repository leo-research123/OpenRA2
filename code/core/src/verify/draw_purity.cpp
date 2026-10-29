#include "verify/draw_purity.hpp"
#include "yrpp/AircraftClass.h"
#include "yrpp/AircraftTypeClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/InfantryTypeClass.h"
#include "yrpp/UnitTypeClass.h"
#include "yrpp/AlphaShapeClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/BulletClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/EBolt.h"
#include "yrpp/Fundamentals.h"
#include "yrpp/HouseClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/IonBlastClass.h"
#include "yrpp/LaserDrawClass.h"
#include "yrpp/LineTrail.h"
#include "yrpp/MapClass.h"
#include "yrpp/ObjectClass.h"
#include "yrpp/OverlayClass.h"
#include "yrpp/ParticleClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/RadBeam.h"
#include "yrpp/Randomizer.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/SpotlightClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/TechnoTypeClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/VoxelAnimClass.h"
#include "yrpp/WaveClass.h"
#include "yrpp/MouseClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/PlanningTokenClass.h"
#include "yrpp/BeaconManagerClass.h"
#include "yrpp/RadarEventClass.h"
#include "yrpp/MessageListClass.h"
#include "yrpp/OwnerDraw.h"
#include "yrpp/CCToolTip.h"
#include "sidebar_movie.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace game {
namespace {
struct Range {
    std::size_t offset = 0, size = 0;
};
struct Label {
    std::size_t offset = 0, size = 0;
    const char* text = "";
};
struct Item {
    const unsigned char* object = nullptr;
    std::size_t size = 0;
};
// One class of live objects (or one standalone region) as enumerated at a moment.
struct Group {
    const char* name = "";
    bool informational = false;  // render-owned state: reported, not counted as a violation
    std::vector<Item> items;
    std::vector<Range> excluded;
    std::vector<Label> labels;
};
struct GroupSnapshot {
    const char* name = "";
    bool informational = false;
    std::vector<Item> items;           // pointers and sizes at the time of the snapshot
    std::vector<std::size_t> offsets;  // item i is bytes[offsets[i], offsets[i] + items[i].size)
    std::vector<unsigned char> bytes;
    std::vector<Range> excluded;
    std::vector<Label> labels;
};

// No field is excluded any more: the drawing writes none (NeedsRedraw and AnimClass::Invisible included).
template <class T>
void add_exclusions(const T*, std::vector<Range>&) {}

template <class T>
void add_array(std::vector<Group>& out, const char* name, DynamicVectorClass<T*>& array, bool informational = false) {
    Group g;
    g.name = name;
    g.informational = informational;
    for (int i = 0; i < array.Count; ++i)
        if (T* p = array[i]) {
            if (g.excluded.empty() && g.items.empty()) add_exclusions<T>(p, g.excluded);
            g.items.push_back({reinterpret_cast<const unsigned char*>(p), sizeof(T)});
        }
    out.push_back(std::move(g));
}

void add_region(std::vector<Group>& out, const char* name, const void* p, std::size_t size, bool informational = false) {
    Group g;
    g.name = name;
    g.informational = informational;
    if (p && size) g.items.push_back({static_cast<const unsigned char*>(p), size});
    out.push_back(std::move(g));
}

std::size_t techno_type_size(const TechnoTypeClass* type) {
    switch (type->WhatAmI()) {
        case AbstractType::InfantryType:
            return sizeof(InfantryTypeClass);
        case AbstractType::UnitType:
            return sizeof(UnitTypeClass);
        case AbstractType::AircraftType:
            return sizeof(AircraftTypeClass);
        case AbstractType::BuildingType:
            return sizeof(BuildingTypeClass);
        default:
            return sizeof(TechnoTypeClass);
    }
}

// Everything the check watches, in a fixed order so a before and an after enumeration line up.
void collect(std::vector<Group>& out) {
    add_array(out, "InfantryClass", InfantryClass::Array);
    add_array(out, "UnitClass", UnitClass::Array);
    add_array(out, "AircraftClass", AircraftClass::Array);
    add_array(out, "BuildingClass", BuildingClass::Array);
    add_array(out, "AnimClass", AnimClass::Array);
    add_array(out, "BulletClass", BulletClass::Array);
    add_array(out, "TerrainClass", TerrainClass::Array);
    add_array(out, "ParticleClass", ParticleClass::Array);
    add_array(out, "ParticleSystemClass", ParticleSystemClass::Array);
    add_array(out, "VoxelAnimClass", VoxelAnimClass::Array);
    add_array(out, "OverlayClass", OverlayClass::Array);
    add_array(out, "EBolt", EBolt::Array);
    add_array(out, "LaserDrawClass", LaserDrawClass::Array);
    add_array(out, "RadBeam", RadBeam::Array);
    add_array(out, "WaveClass", WaveClass::Array);
    add_array(out, "SpotlightClass", SpotlightClass::Array);
    add_array(out, "LineTrail", LineTrail::Array);
    add_array(out, "IonBlastClass", IonBlastClass::Array);
    add_array(out, "AlphaShapeClass", AlphaShapeClass::Array);
    add_array(out, "HouseClass", HouseClass::Array);
    add_array(out, "SuperClass", SuperClass::Array);
    add_array(out, "PlanningTokenClass", PlanningTokenClass::Array);
    add_array(out, "PlanningNodeClass", PlanningNodeClass::Unknown1);
    add_region(out, "Planning.HitTestIndex", &PlanningNodeClass::Unknown2, sizeof(PlanningNodeClass::Unknown2));
    if (PlanningNodeClass::Unknown2.Count)
        add_region(out, "Planning.HitTestItems", PlanningNodeClass::Unknown2.Items,
                   sizeof(PlanningNodeClass*) * PlanningNodeClass::Unknown2.Count);
    for (auto* node : PlanningNodeClass::Unknown1) {
        for (auto* branch : node->PlanningBranches)
            add_region(out, "PlanningBranchClass", branch, sizeof(*branch));
        for (auto* member : node->PlanningMembers)
            add_region(out, "PlanningMemberClass", member, sizeof(*member));
    }
    add_region(out, "BeaconManagerClass", &BeaconManagerClass::Instance, sizeof(BeaconManagerClass));
    for (const auto& house : BeaconManagerClass::Instance.Beacons)
        for (auto* beacon : house)
            add_region(out, "BeaconClass", beacon, sizeof(BeaconClass));
    {
        Group g;
        g.name = "TechnoTypeClass";
        for (int i = 0; i < TechnoTypeClass::Array.Count; ++i)
            if (auto* type = TechnoTypeClass::Array[i])
                g.items.push_back({reinterpret_cast<const unsigned char*>(type), techno_type_size(type)});
        out.push_back(std::move(g));
    }
    {
        Group g;
        g.name = "CellClass";
        const auto& cells = MapClass::Instance.Cells;
        for (int i = 0; cells.Items && i < cells.Capacity; ++i)
            if (auto* cell = cells.Items[i]) g.items.push_back({reinterpret_cast<const unsigned char*>(cell), sizeof(CellClass)});
        out.push_back(std::move(g));
    }
    // No UI field exclusions: input reads animation, layout and redraw state.
    add_region(out, "MouseClass/UI", &MouseClass::Instance, sizeof(MouseClass));
    add_region(out, "UI.CommandButtons", &TabClass::CommandButtons, sizeof(TabClass::CommandButtons));
    add_region(out, "UI.CollapseButton", &TabClass::CollapseButton, sizeof(ShapeButtonClass));
    add_region(out, "UI.ExpandButton", &TabClass::ExpandButton, sizeof(ShapeButtonClass));
    add_region(out, "UI.TabButtons", &SidebarClass::TabButtons, sizeof(SidebarClass::TabButtons));
    add_region(out, "UI.RepairButton", &SidebarClass::ToggleRepairButton, sizeof(ShapeButtonClass));
    add_region(out, "UI.SellButton", &SidebarClass::ToggleSellButton, sizeof(ShapeButtonClass));
    add_region(out, "UI.ScrollUpButton", &SidebarClass::ScrollUpButton, sizeof(ShapeButtonClass));
    add_region(out, "UI.ScrollDownButton", &SidebarClass::ScrollDownButton, sizeof(ShapeButtonClass));
    add_region(out, "UI.DiplomacyButton", &RadarClass::DiplomacyButton, sizeof(ShapeButtonClass));
    add_region(out, "UI.OptionsButton", &RadarClass::OptionsButton, sizeof(ShapeButtonClass));
    add_region(out, "UI.RadarButton", &RadarClass::RadarButton, sizeof(RadarClass::RTacticalClass));
    add_region(out, "UI.CameoButtons", SelectClass::Array(), 240 * sizeof(SelectClass));
    add_region(out, "UI.Buttons", &GScreenClass::Buttons, sizeof(GScreenClass::Buttons));
    add_region(out, "UI.Focused", &GadgetClass::Focused, sizeof(GadgetClass::Focused));
    add_region(out, "UI.Hovered", &GadgetClass::Hovered, sizeof(GadgetClass::Hovered));
    add_region(out, "UI.StuckOn", &GadgetClass::StuckOn, sizeof(GadgetClass::StuckOn));
    add_region(out, "UI.SidebarChanged", &Game::SidebarChanged, sizeof(Game::SidebarChanged));
    add_array(out, "RadarEventClass", RadarEventClass::Array);
    auto& messages = MessageListClass::Instance;
    add_region(out, "MessageListClass", &messages, sizeof(MessageListClass));
    {
        Group labels;
        labels.name = "TextLabelClass";
        if (messages.IsEdit && messages.EditLabel)
            labels.items.push_back({reinterpret_cast<const unsigned char*>(messages.EditLabel), sizeof(TextLabelClass)});
        for (auto* label = messages.MessageList; label; label = static_cast<TextLabelClass*>(label->Next))
            labels.items.push_back({reinterpret_cast<const unsigned char*>(label), sizeof(TextLabelClass)});
        out.push_back(std::move(labels));
    }
    add_region(out, "OwnerDraw.IMEText", OwnerDraw::IMECompositionString, sizeof(OwnerDraw::IMECompositionString));
    add_region(out, "OwnerDraw.IMELength", &OwnerDraw::IMECompositionStringLength, sizeof(int));
    add_region(out, "OwnerDraw.IMECursor", &OwnerDraw::IMECompositionCursorPos, sizeof(int));
    add_region(out, "OwnerDraw.IMEComposing", &OwnerDraw::IMEComposing, sizeof(int));
    add_region(out, "OwnerDraw.SuppressCaret", &OwnerDraw::SuppressCaret, sizeof(int));
    if (auto* tips = CCToolTip::Instance) {
        add_region(out, "CCToolTip", tips, sizeof(CCToolTip));
        add_array(out, "ToolTip", tips->ToolTips);
        add_region(out, "ToolTipIndex", tips->ToolTipIndex.IndexTable,
                   std::size_t(tips->ToolTipIndex.IndexCount) * sizeof(*tips->ToolTipIndex.IndexTable));
    }
    add_region(out, "WWMovieHandle.Array", &WWMovieHandle::Array, sizeof(WWMovieHandle::Array));
    add_array(out, "WWMovieHandle", WWMovieHandle::Array);
    visit_sidebar_movie_state(&out, [](void* context, const void* bytes, unsigned size) noexcept {
        try { add_region(*static_cast<std::vector<Group>*>(context), "SidebarMovieState", bytes, size); } catch (...) {}
    });
    add_region(out, "MapClass", &MapClass::Instance, sizeof(MapClass));
    for (int i = 0; i < 5; ++i) {
        static const char* const names[5] = {"Layer[0]", "Layer[1]", "Layer[2]", "Layer[3]", "Layer[4]"};
        auto& layer = MapClass::ObjectsInLayers[i];
        add_region(out, names[i], &layer, sizeof(LayerClass));
        static const char* const lists[5] = {"Layer[0].items", "Layer[1].items", "Layer[2].items", "Layer[3].items",
                                             "Layer[4].items"};
        add_region(out, lists[i], layer.Items, std::size_t(layer.Count) * sizeof(void*));
    }
    {
        Group g;
        g.name = "ScenarioClass";
        if (auto* scenario = ScenarioClass::Instance) {
            g.items.push_back({reinterpret_cast<const unsigned char*>(scenario), sizeof(ScenarioClass)});
            g.labels.push_back({std::size_t(reinterpret_cast<const unsigned char*>(&scenario->Random) -
                                            reinterpret_cast<const unsigned char*>(scenario)),
                                sizeof(Randomizer), "ScenarioClass::Random"});
        }
        out.push_back(std::move(g));
    }
    add_region(out, "Randomizer::Global", &Randomizer::Global, sizeof(Randomizer));
    add_region(out, "Unsorted::CurrentFrame", &Unsorted::CurrentFrame, sizeof(int));
    // Render-owned state: reported for information only (selectables live in Tactical statics).
    if (TacticalClass::Instance) add_region(out, "TacticalClass(info)", TacticalClass::Instance, sizeof(TacticalClass), true);
}

void take(const std::vector<Group>& groups, std::vector<GroupSnapshot>& out) {
    out.reserve(groups.size());
    for (const auto& g : groups) {
        GroupSnapshot s;
        s.name = g.name;
        s.informational = g.informational;
        s.items = g.items;
        s.excluded = g.excluded;
        s.labels = g.labels;
        std::size_t total = 0;
        for (const auto& item : g.items) total += item.size;
        s.bytes.resize(total);
        s.offsets.reserve(g.items.size());
        std::size_t at = 0;
        for (const auto& item : g.items) {
            s.offsets.push_back(at);
            std::memcpy(s.bytes.data() + at, item.object, item.size);
            at += item.size;
        }
        out.push_back(std::move(s));
    }
}

struct Entry {
    std::string group;
    std::size_t offset = 0;
    bool informational = false;
    std::uint64_t objects = 0, renders = 0;
    std::uint32_t before = 0, after = 0;
    std::string label;
    std::uint64_t last_render = 0;
};
struct Structure {
    std::string group;
    std::uint64_t created = 0, destroyed = 0, reordered = 0, renders = 0;
};

struct Totals {
    std::uint64_t renders = 0, clean = 0;
    std::map<std::pair<std::string, std::size_t>, Entry> entries;
    std::map<std::string, Structure> structure;
};
Totals& totals() {
    static Totals value;
    return value;
}

void print_totals() noexcept {
    try {
        const auto& t = totals();
        std::fprintf(stderr, "DRAW_PURE_TOTAL renders=%llu clean=%llu\n", (unsigned long long)t.renders,
                     (unsigned long long)t.clean);
        for (const auto& [key, e] : t.entries)
            std::fprintf(stderr,
                         "DRAW_PURE_TOTAL %s group=%s offset=0x%X renders=%llu objects=%llu before=0x%08X "
                         "after=0x%08X%s%s\n",
                         e.informational ? "info" : "violation", e.group.c_str(), unsigned(e.offset),
                         (unsigned long long)e.renders, (unsigned long long)e.objects, e.before, e.after,
                         e.label.empty() ? "" : " field=", e.label.c_str());
        for (const auto& [key, s] : t.structure)
            std::fprintf(stderr,
                         "DRAW_PURE_TOTAL violation group=%s structure renders=%llu created=%llu destroyed=%llu "
                         "reordered=%llu\n",
                         s.group.c_str(), (unsigned long long)s.renders, (unsigned long long)s.created,
                         (unsigned long long)s.destroyed, (unsigned long long)s.reordered);
    } catch (...) {
    }
}

bool excluded_byte(const std::vector<Range>& excluded, std::size_t offset) {
    for (const auto& r : excluded)
        if (offset >= r.offset && offset < r.offset + r.size) return true;
    return false;
}

struct Snapshot {
    std::vector<GroupSnapshot> groups;
};
thread_local Snapshot* current = nullptr;
thread_local std::uint64_t render_ordinal = 0;

bool enabled() noexcept {
    static const bool value = [] {
        const char* text = std::getenv("RA2_VERIFY_DRAW_PURE");
        return text && text[0] && std::strcmp(text, "0") != 0;
    }();
    return value;
}

void compare(const Snapshot& before, std::vector<Group>& after_groups) {
    auto& total = totals();
    ++total.renders;
    std::map<std::pair<std::string, std::size_t>, Entry> local;
    std::map<std::string, Structure> structure;
    const int frame = Unsorted::CurrentFrame;
    for (std::size_t g = 0; g < before.groups.size() && g < after_groups.size(); ++g) {
        const auto& b = before.groups[g];
        const auto& a = after_groups[g];
        std::unordered_map<const unsigned char*, std::size_t> live;
        live.reserve(a.items.size());
        for (std::size_t i = 0; i < a.items.size(); ++i) live.emplace(a.items[i].object, i);
        std::uint64_t destroyed = 0, created = 0;
        bool same_membership = a.items.size() == b.items.size();
        for (std::size_t i = 0; i < b.items.size(); ++i) {
            const auto found = live.find(b.items[i].object);
            if (found == live.end()) {
                ++destroyed;
                same_membership = false;
                continue;
            }
            if (same_membership && a.items[i].object != b.items[i].object) same_membership = false;
            const auto size = b.items[i].size;
            const unsigned char* was = b.bytes.data() + b.offsets[i];
            const unsigned char* now = b.items[i].object;
            if (std::memcmp(was, now, size) == 0) continue;
            for (std::size_t word = 0; word < size; word += 4) {
                const std::size_t n = size - word < 4 ? size - word : 4;
                std::uint32_t was_word = 0, now_word = 0;
                bool differs = false;
                for (std::size_t k = 0; k < n; ++k) {
                    reinterpret_cast<unsigned char*>(&was_word)[k] = was[word + k];
                    reinterpret_cast<unsigned char*>(&now_word)[k] = now[word + k];
                    if (was[word + k] != now[word + k] && !excluded_byte(b.excluded, word + k)) differs = true;
                }
                if (!differs) continue;
                auto& e = local[{std::string(b.name), word}];
                if (!e.objects) {
                    e.group = b.name;
                    e.offset = word;
                    e.informational = b.informational;
                    e.before = was_word;
                    e.after = now_word;
                    for (const auto& label : b.labels)
                        if (word >= label.offset && word < label.offset + label.size) {
                            char text[96];
                            std::snprintf(text, sizeof(text), "%s+0x%X", label.text, unsigned(word - label.offset));
                            e.label = text;
                        }
                }
                ++e.objects;
            }
        }
        std::unordered_set<const unsigned char*> known;
        known.reserve(b.items.size());
        for (const auto& item : b.items) known.insert(item.object);
        for (const auto& item : a.items)
            if (!known.count(item.object)) ++created;
        if (created || destroyed || !same_membership) {
            auto& s = structure[b.name];
            s.group = b.name;
            s.created += created;
            s.destroyed += destroyed;
            if (!created && !destroyed) ++s.reordered;
        }
    }
    bool violation = false;
    for (auto& [key, e] : local) {
        if (!e.informational) violation = true;
        std::fprintf(stderr,
                     "DRAW_PURE frame=%d render=%llu %s group=%s offset=0x%X objects=%llu before=0x%08X "
                     "after=0x%08X%s%s\n",
                     frame, (unsigned long long)render_ordinal, e.informational ? "info" : "violation",
                     e.group.c_str(), unsigned(e.offset), (unsigned long long)e.objects, e.before, e.after,
                     e.label.empty() ? "" : " field=", e.label.c_str());
        auto& t = total.entries[key];
        if (!t.renders) {
            t = e;
            t.objects = 0;
        }
        ++t.renders;
        t.objects += e.objects;
        t.after = e.after;
    }
    for (auto& [name, s] : structure) {
        violation = true;
        std::fprintf(stderr, "DRAW_PURE frame=%d render=%llu violation group=%s structure created=%llu destroyed=%llu%s\n",
                     frame, (unsigned long long)render_ordinal, name.c_str(), (unsigned long long)s.created,
                     (unsigned long long)s.destroyed, s.created || s.destroyed ? "" : " reordered=1");
        auto& t = total.structure[name];
        t.group = name;
        ++t.renders;
        t.created += s.created;
        t.destroyed += s.destroyed;
        t.reordered += s.reordered;
    }
    if (!violation) ++total.clean;
}
}

DrawPurityScope::DrawPurityScope() noexcept {
    if (!enabled() || current) return;
    try {
        static const bool registered = [] {
            totals();  // constructed first, so it is destroyed after the exit handler ran
            std::atexit(print_totals);
            return true;
        }();
        (void)registered;
        auto* snapshot = new Snapshot;
        std::vector<Group> groups;
        collect(groups);
        take(groups, snapshot->groups);
        current = snapshot;
        ++render_ordinal;
        active_ = true;
    } catch (...) {
        std::fprintf(stderr, "DRAW_PURE snapshot failed; check disabled for this render\n");
    }
}

DrawPurityScope::~DrawPurityScope() {
    if (!active_) return;
    std::unique_ptr<Snapshot> snapshot(current);
    current = nullptr;
    try {
        std::vector<Group> after;
        collect(after);
        compare(*snapshot, after);
    } catch (...) {
        std::fprintf(stderr, "DRAW_PURE comparison failed\n");
    }
}
}
