#include "type_resources.hpp"
#include "rules_runtime.hpp"
#include "yrpp/ScenarioClass.h"
#include "yrpp/CCFileClass.h"
#include <cstdio>
#include <cstring>
#include <cctype>
namespace game {
namespace {
thread_local const TypeResourceServices* active = nullptr;
thread_local TypeResourceStatus status = TypeResourceStatus::complete;
const TypeResourceServices defaults{};
}
const TypeResourceServices& type_resources() noexcept { return active ? *active : defaults; }
void type_resource_result(TypeResourceStatus value) noexcept {
    if (status == TypeResourceStatus::complete) status = value;
}
TypeResourceStatus last_type_resource_status() noexcept { return status; }
TypeResourceStatus with_type_resources(const TypeResourceServices& services,
        void (*operation)(void*), void* argument) noexcept {
    if (!operation) return TypeResourceStatus::invalid_argument;
    auto* previous = active;
    const auto old_status = status;
    active = &services;
    status = TypeResourceStatus::complete;
    try { operation(argument); }
    catch (...) { type_resource_result(TypeResourceStatus::failure); }
    auto result = status;
    active = previous;
    // Preserve nested failure in the enclosing operation; no dangling context.
    status = previous && old_status != TypeResourceStatus::complete ? old_status : result;
    return result;
}
CCINIClass& type_art_ini() noexcept {
    return type_resources().art ? *type_resources().art : CCINIClass::INI_Art;
}
TheaterType type_theater() noexcept {
    if (auto* value = type_resources().theater) return *value;
    return ScenarioClass::Instance ? ScenarioClass::Instance->Theater : TheaterType::None;
}
bool type_image_filename(const ObjectTypeClass& type, char* out, std::size_t size, bool new_theater) {
    if (!out || size == 0) return false;
    const auto theater = type_theater();
    const int theater_index = static_cast<int>(theater);
    const auto* table = theater_index >= 0 && theater_index < 6 ? Theater::Get(theater) : nullptr;
    if (type.Theater && !table) {
        type_resource_result(TypeResourceStatus::unavailable); return false;
    }
    const char* extension = type.Theater ? table->Extension : "shp";
    const int count = std::snprintf(out, size, "%s.%s", type.ImageFile, extension);
    if (count < 0 || std::size_t(count) >= size) {
        type_resource_result(TypeResourceStatus::invalid_argument); return false;
    }
    if (!type.Theater && type.NewTheater && new_theater && count >= 2) {
        const int first = std::tolower(static_cast<unsigned char>(out[0]));
        const int second = std::tolower(static_cast<unsigned char>(out[1]));
        if ((first == 'g' || first == 'n' || first == 'c' || first == 'y') &&
            (second == 'a' || second == 't')) {
            if (!table) { type_resource_result(TypeResourceStatus::unavailable); return false; }
            out[1] = table->Letter[0];
        }
    }
    return true;
}
bool load_owned_type_shape(const char* filename, SHPStruct*& output) {
    // Type-owned storage must not borrow an SHPReference from the global name
    // cache: Smudge/Terrain's original destructors unconditionally free Image.
    // Use the existing original CCFile allocation path (also used by demand
    // loading and theater reload). No software surface or converter is needed.
    output = nullptr;
    CCFileClass file(filename);
    if (!file.Exists(false)) return true;
    if (file.GetFileSize() < 8) { type_resource_result(TypeResourceStatus::failure); return false; }
    output = static_cast<SHPStruct*>(file.ReadWholeFile());
    if (!output) { type_resource_result(TypeResourceStatus::failure); return false; }
    return true;
}
bool building_shape_filename(const char* base, bool theater_extension,
        char* output, std::size_t capacity) {
    if (!base || !output || !capacity) return false;
    const int index = static_cast<int>(type_theater());
    if (index < -1 || index >= 6) {
        type_resource_result(TypeResourceStatus::unavailable); return false;
    }
    const auto* theater = index >= 0 ? Theater::Get(type_theater()) : nullptr;
    const char* extension = theater_extension && theater ? theater->Extension : "shp";
    const int length = std::snprintf(output, capacity, "%s.%s", base, extension);
    if (length < 0 || std::size_t(length) >= capacity) {
        type_resource_result(TypeResourceStatus::invalid_argument); return false;
    }
    // 0x005F96B0 is called for ALL building parts, not only NewTheater=yes.
    if (theater && std::strlen(base) >= 2) {
        const int first = std::tolower(static_cast<unsigned char>(output[0]));
        const int second = std::tolower(static_cast<unsigned char>(output[1]));
        if ((first == 'g' || first == 'n' || first == 'c' || first == 'y') &&
                (second == 'a' || second == 't')) output[1] = theater->Letter[0];
    }
    return true;
}
bool load_building_owned_shape(const char* base, SHPStruct*& image, bool& owned) {
    if (owned) YRMemory::Deallocate(image);
    image = nullptr; owned = false;
    if (!base || !*base || INIClass::IsBlankValue(base)) return true;
    char filename[260]{};
    if (!building_shape_filename(base, false, filename, sizeof(filename)) ||
            !load_owned_type_shape(filename, image)) return false;
    if (!image) {
        // 0x005F9710 unconditionally replaces the second filename character.
        filename[1] = 'G';
        if (!load_owned_type_shape(filename, image)) return false;
    }
    owned = image != nullptr;
    return true;
}
bool load_building_cached_shape(const char* base, bool theater_extension,
        SHPStruct*& image, char* resolved, std::size_t capacity) {
    image = nullptr;
    if (resolved && capacity) *resolved = 0;
    if (!base || !*base || INIClass::IsBlankValue(base)) return true;
    char filename[260]{};
    if (!building_shape_filename(base, theater_extension, filename, sizeof(filename))) return false;
    image = static_cast<SHPStruct*>(FileSystem::LoadFile(filename, false));
    if (!image) {
        filename[1] = 'G';
        image = static_cast<SHPStruct*>(FileSystem::LoadFile(filename, false));
    }
    if (resolved && capacity) std::snprintf(resolved, capacity, "%s", filename);
    return true;
}
bool read_type_sound(CCINIClass& ini, const char* section, const char* key, int& value) {
    char name[512]{};
    if (!ini.ReadString(section, key, "", name, sizeof(name))) return true;
    if (type_resources().audio_unavailable) { value = -1; return true; }
    const auto& runtime = rules_runtime();
    int found = -1;
    if (!runtime.sound_index || !runtime.sound_index(runtime.context, name, found)) {
        type_resource_result(TypeResourceStatus::unavailable); return false;
    }
    if (found != -1) value = found;
    return true;
}
}
