#include "map_configuration.hpp"
#include "audio_session.hpp"
#include "map_world_internal.hpp"
#include "map_view.hpp"
#include "rules_runtime.hpp"
#include "yrpp/FileSystem.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TiberiumClass.h"

namespace game {
void initialize_campaign_superweapon_option(const RulesClass& rules, SessionClass& session) noexcept {
    if (session.GameMode == GameMode::Campaign) session.Config.SWAllowed = rules.SuperWeaponsAllowed;
}

bool with_map_rules_runtime(MapWorld& world, void (*operation)(void*), void* argument) {
    auto services = native_rules_runtime();
    services.context = &world;
    services.defer_sound_references = !native_audio_ready();
    services.retain_sound_reference = [](void* p, const CCINIClass*, const char* section, const char* key) noexcept {
        try {
            static_cast<MapWorld*>(p)->impl->deferred_sound_fields.emplace_back(section, key);
            return true;
        } catch (...) {
            return false;
        }
    };
    services.shape = [](void*, const char* name, SHPStruct*& result) {
        result = static_cast<SHPStruct*>(FileSystem::LoadFile(name, false));
        return true;  // Missing optional art is the original null result.
    };
    // Named colors feed the original custom-palette cache separately from Array.
    services.register_color = [](void*, const char* name, const ColorStruct& color) {
        const int previous = ColorScheme::GetNumberOfSchemes();
        ColorScheme::AddPaletteColor(name, color);
        return ColorScheme::GetNumberOfSchemes() == previous + 1;
    };
    services.clear_color_tables = [](void*) {
        ColorScheme::ClearPaletteColors();
        return true;
    };
    // 0x0068C9C0 with the theater unit palette (0x00886380) and TEMPERAT.PAL.
    services.create_color_scheme = [](void* p, const char* name, const ColorStruct& color, int shades) {
        ColorScheme::FindByName(name, color, static_cast<MapWorld*>(p)->impl->presentation.unit_palette, FileSystem::TEMPERAT_PAL,
                                shades);
        return true;
    };
    services.destroy_color_scheme = [](void*, ColorScheme* scheme) {
        delete scheme;
        return true;
    };
    // RulesClass::Init (0x668BF0) also retires the Houses; their registry is
    // not a type registry.
    services.type_count = [](void*, AbstractType kind, int& count) {
        if (kind == AbstractType::House) {
            count = HouseClass::Array.Count;
            return true;
        }
        return native_rules_runtime().type_count(nullptr, kind, count);
    };
    services.destroy_first_type = [](void*, AbstractType kind) {
        if (kind == AbstractType::House) {
            if (!HouseClass::Array.Count || !HouseClass::Array[0]) return false;
            delete HouseClass::Array[0];
            return true;
        }
        AbstractTypeClass* type = nullptr;
        if (!native_rules_runtime().type_at(nullptr, kind, 0, type) || !type) return false;
        delete type;
        return true;
    };
    services.read_tiberiums = [](void*, CCINIClass* ini) { return TiberiumClass::LoadFromINIList(ini); };
    bind_command_bar_runtime(services);
    return with_rules_runtime(services, operation, argument);
}
}
