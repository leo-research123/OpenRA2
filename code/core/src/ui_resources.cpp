#include "ui_resources.hpp"
#include "yrpp/MouseClass.h"
#include "yrpp/MixFileClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/Drawing.h"
#include "yrpp/BeaconManagerClass.h"
#include "rules_runtime.hpp"
#include <cstdio>
#include <atomic>

namespace game {
namespace {
UiResources* beacon_art_owner = nullptr;
}
void UiResources::clear() noexcept {
    static std::atomic<std::uint64_t> next_revision{1};
    revision_ = next_revision.fetch_add(1, std::memory_order_relaxed);
    if (WWMouseClass::Instance == &mouse_) {
        WWMouseClass::Instance = nullptr;
        MouseClass::CursorShape = nullptr;
        MouseClass::CursorInitialized = false;
        MouseClass::CursorTimer.Stop();
        FileSystem::MOUSE_PAL = nullptr;
        mouse_.Image = nullptr;
        mouse_.field_10 = false;
    }
    cursor_palette_.reset();
    cursor_.reset();
    if (beacon_art_owner == this) {
        BeaconManagerClass::ReleaseArt();
        beacon_art_owner = nullptr;
    }
    // Retire borrowed original art before its owner frees the SHP references.
    const auto* diplomacy = image(UiImage::briefing);
    const auto* options = image(UiImage::options);
    if ((diplomacy && RadarClass::DiplomacyShape == diplomacy) || (options && RadarClass::OptionsShape == options)) {
        RadarClass::Instance.SetRadarButtonsVisible(false);
        for (auto* button : {&RadarClass::DiplomacyButton, &RadarClass::OptionsButton}) {
            if (GadgetClass::StuckOn == button) GadgetClass::StuckOn = nullptr;
            if (GadgetClass::Hovered == button) GadgetClass::Hovered = nullptr;
            button->IsPressed = false;
        }
        RadarClass::DiplomacyButton.SetShape(nullptr, 0, 0);
        RadarClass::OptionsButton.SetShape(nullptr, 0, 0);
        RadarClass::Instance.DisposeOfArt();
    }
    // Sidebar command controls borrow the same per-side art. Release both
    // their device capture and their pointer before a faction reload frees it.
    ShapeButtonClass* commands[]{&SidebarClass::ToggleRepairButton, &SidebarClass::ToggleSellButton};
    const UiImage kinds[]{UiImage::repair, UiImage::sell};
    for (int i = 0; i < 2; ++i)
        if (image(kinds[i]) && commands[i]->ShapeData == image(kinds[i])) {
            GScreenClass::Instance.RemoveButton(commands[i]);
            commands[i]->IsPressed = false;
            commands[i]->SetShape(nullptr, 0, 0);
        }
    if (image(UiImage::radar) && RadarClass::RadarAnim == image(UiImage::radar)) RadarClass::RadarAnim = nullptr;
    for (auto& image : images_)
        image.reset();
    if (BitFont::Instance == font_.get()) BitFont::Instance = nullptr;
    font_.reset();
    side_ = -1;
}
bool UiResources::load(int side) noexcept {
    if (side_ == side) return true;
    clear();
    error_[0] = 0;
    try {
        // MOUSE.SHA belongs to the common CONQMD/CONQUER archives, even when
        // the host initializes UI resources before loading terrain.
        if (!MixFileClass::LoadTerrainMixes()) {
            std::snprintf(error_, sizeof(error_), "Could not mount common cursor archives");
            return false;
        }
        if (!MixFileClass::LoadSidebarMixes(side)) {
            std::snprintf(error_, sizeof(error_), "Could not mount sidebar archives for side %d", side);
            return false;
        }
        font_ = std::make_unique<BitFont>("GAME.FNT");
        if (!font_->InternalPTR) {
            std::snprintf(error_, sizeof(error_), "Could not read original GAME.FNT");
            clear();
            return false;
        }
        CCFileClass palette_file("SIDEBAR.PAL");
        if (palette_file.ReadBytes(&palette_, sizeof(palette_)) != sizeof(palette_)) {
            std::snprintf(error_, sizeof(error_), "Could not read SIDEBAR.PAL");
            return false;
        }
        for (auto& color : palette_.Entries) {
            color.R <<= 2;
            color.G <<= 2;
            color.B <<= 2;
        }
        CCFileClass cameo_file("CAMEO.PAL");
        if (cameo_file.ReadBytes(&cameo_palette_, sizeof(cameo_palette_)) != sizeof(cameo_palette_)) {
            std::snprintf(error_, sizeof(error_), "Could not read CAMEO.PAL");
            return false;
        }
        for (auto& color : cameo_palette_.Entries) {
            color.R <<= 2;
            color.G <<= 2;
            color.B <<= 2;
        }
        BytePalette mouse_palette{}, waypoint_palette{};
        CCFileClass mouse_file("MOUSEPAL.PAL"), waypoint_file("WAYPOINT.PAL");
        if (mouse_file.ReadBytes(&mouse_palette, sizeof(mouse_palette)) != sizeof(mouse_palette) ||
            waypoint_file.ReadBytes(&waypoint_palette, sizeof(waypoint_palette)) != sizeof(waypoint_palette)) {
            std::snprintf(error_, sizeof(error_), "Could not read MOUSEPAL.PAL or WAYPOINT.PAL");
            clear();
            return false;
        }
        for (auto* palette : {&mouse_palette, &waypoint_palette})
            for (auto& color : palette->Entries) {
                color.R <<= 2;
                color.G <<= 2;
                color.B <<= 2;
            }
        cursor_ = std::make_unique<SHPReference>("MOUSE.SHA");
        cursor_->Load();
        auto* shape = cursor_->Data;
        if (!shape || shape->Width <= 0 || shape->Height <= 0 || shape->Frames < 517) {
            std::snprintf(error_, sizeof(error_), "Invalid original cursor image: MOUSE.SHA");
            clear();
            return false;
        }
        cursor_palette_ = std::make_unique<ConvertClass>(mouse_palette);
        FileSystem::MOUSE_PAL = cursor_palette_.get();
        FileSystem::WAYPOINT_PAL = waypoint_palette;
        MouseClass::CursorShape = shape;
        MouseClass::CursorInitialized = false;
        WWMouseClass::Instance = &mouse_;
        mouse_.RefCount = 0;
        mouse_.CaptureMouse();
        auto& mouse = MouseClass::Instance;
        mouse.WaypointColorRed = mouse.WaypointColorGreen = mouse.WaypointColorBlue = 0;
        mouse.SetCursor(MouseCursorType::Default, false);
        const char* names[]{"CREDITS.SHP", "TOP.SHP",      side == 2 ? "RADARY.SHP" : "RADAR.SHP",
                            "SIDE1.SHP",   "SIDE2.SHP",    "SIDE2B.SHP",
                            "SIDE3.SHP",   "ADDON.SHP",    "LSPACER.SHP",
                            "LENDCAP.SHP", "BTTNBKGD.SHP", "RENDCAP.SHP",
                            "REPAIR.SHP",  "SELL.SHP",     "R-DN.SHP",
                            "R-UP.SHP",    "DIPLOBTN.SHP", "OPTBTN.SHP",
                            "TAB00.SHP",   "TAB01.SHP",    "TAB02.SHP",
                            "TAB03.SHP",   "POWERP.SHP",   "GCLOCK2.SHP",
                            "DARKEN.SHP"};
        for (unsigned i = 0; i < images_.size(); ++i) {
            char button[32];
            const char* name;
            if (i < static_cast<unsigned>(UiImage::button0))
                name = names[i];
            else {
                std::snprintf(button, sizeof(button), "Button%02u.SHP", i - static_cast<unsigned>(UiImage::button0));
                name = button;
            }
            CCFileClass probe(name);
            if (!probe.Exists()) {
                if (i >= static_cast<unsigned>(UiImage::button0))
                    continue;  // Unassigned command slots may have no shape.
                std::snprintf(error_, sizeof(error_), "Missing original UI image: %s", name);
                clear();
                return false;
            }
            auto image = std::make_unique<SHPReference>(name);
            image->Load();
            auto* data = image->GetData();
            if (!data || data->Width <= 0 || data->Height <= 0 || data->Frames <= 0) {
                std::snprintf(error_, sizeof(error_), "Invalid original UI image: %s", name);
                clear();
                return false;
            }
            images_[i] = std::move(image);
        }
        CCFileClass ui_file("UIMD.INI");
        CCINIClass ini;
        if (ini.ReadCCFile(&ui_file, false, false) <= 0) {
            std::snprintf(error_, sizeof(error_), "Could not read UIMD.INI");
            clear();
            return false;
        }
        const auto* defaults = default_rules_runtime();
        auto runtime = defaults ? *defaults : RulesRuntimeServices{};
        bind_command_bar_runtime(runtime);
        bool parsed = false;
        struct Read {
            CCINIClass& ini;
            bool& parsed;
        } read{ini, parsed};
        if (!with_rules_runtime(
                runtime,
                [](void* pointer) {
                    auto& r = *static_cast<Read*>(pointer);
                    r.parsed = RulesClass::Read_AdvancedCommandBar(&r.ini, false);
                },
                &read) ||
            !parsed) {
            std::snprintf(error_, sizeof(error_), "Could not read original advanced command bar");
            clear();
            return false;
        }
        Drawing::SetTooltipColorForSide(side);
        BeaconManagerClass::Instance.LoadArt();
        beacon_art_owner = this;
        BitFont::Instance = font_.get();
        side_ = side;
        return true;
    } catch (...) {
        std::snprintf(error_, sizeof(error_), "Original UI resource loading failed");
        clear();
        return false;
    }
}
}
