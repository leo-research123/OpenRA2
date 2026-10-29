#include "type_resources.hpp"
#include "rules_runtime.hpp"
#include "yrpp/ScenarioClass.h"
#include "yrpp/CCFileClass.h"
#include <cstdio>
#include <cstring>
namespace game {
namespace {
thread_local const TypeResourceServices* active = nullptr;
thread_local TypeResourceStatus status = TypeResourceStatus::complete;
thread_local char failed_resource[64]{};
const TypeResourceServices defaults{};
}
const TypeResourceServices& type_resources() noexcept { return active ? *active : defaults; }
void type_resource_result(TypeResourceStatus value) noexcept {
    if (status == TypeResourceStatus::complete) status = value;
}
TypeResourceStatus last_type_resource_status() noexcept { return status; }
void type_resource_failure(const char* name) noexcept {
    if (name && !failed_resource[0]) std::snprintf(failed_resource, sizeof(failed_resource), "%s", name);
    type_resource_result(TypeResourceStatus::failure);
}
const char* last_type_resource_failure() noexcept { return failed_resource; }
TypeResourceStatus with_type_resources(const TypeResourceServices& services, void (*operation)(void*),
                                       void* argument) noexcept {
    if (!operation) return TypeResourceStatus::invalid_argument;
    auto* previous = active;
    const auto old_status = status;
    active = &services;
    status = TypeResourceStatus::complete;
    // A nested scope keeps the name an enclosing scope already failed on.
    if (!previous || old_status == TypeResourceStatus::complete) failed_resource[0] = 0;
    try {
        operation(argument);
    } catch (...) {
        type_resource_result(TypeResourceStatus::failure);
    }
    auto result = status;
    active = previous;
    // Preserve nested failure in the enclosing operation; no dangling context.
    status = previous && old_status != TypeResourceStatus::complete ? old_status : result;
    return result;
}
CCINIClass& type_art_ini() noexcept { return type_resources().art ? *type_resources().art : CCINIClass::INI_Art; }
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
        type_resource_result(TypeResourceStatus::unavailable);
        return false;
    }
    const char* extension = type.Theater ? table->Extension : "SHP";
    const int count = std::snprintf(out, size, "%s.%s", type.ImageFile, extension);
    if (count < 0 || std::size_t(count) >= size) {
        type_resource_result(TypeResourceStatus::invalid_argument);
        return false;
    }
    if (!type.Theater && type.NewTheater && new_theater && table && count >= 2) {
        const auto lower = [](unsigned char c) { return c >= 'A' && c <= 'Z' ? c + 32 : int(c); };
        const int first = lower(static_cast<unsigned char>(out[0]));
        const int second = lower(static_cast<unsigned char>(out[1]));
        if ((first == 'g' || first == 'n' || first == 'c' || first == 'y') && (second == 'a' || second == 't')) {
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
    if (file.GetFileSize() < 8) {
        type_resource_failure(filename);
        return false;
    }
    output = static_cast<SHPStruct*>(file.ReadWholeFile());
    if (!output) {
        type_resource_failure(filename);
        return false;
    }
    return true;
}
bool building_shape_filename(const char* base, bool theater_extension, char* output, std::size_t capacity) {
    return building_shape_filename(base, theater_extension, type_theater(), output, capacity);
}
bool building_shape_filename(const char* base, bool theater_extension, TheaterType selected_theater, char* output,
                             std::size_t capacity) {
    if (!base || !output || !capacity) return false;
    const int index = static_cast<int>(selected_theater);
    if (index < -1 || index >= 6) {
        type_resource_result(TypeResourceStatus::unavailable);
        return false;
    }
    const auto* theater = index >= 0 ? Theater::Get(selected_theater) : nullptr;
    const char* extension = theater_extension && theater ? theater->Extension : "SHP";
    const int length = std::snprintf(output, capacity, "%s.%s", base, extension);
    if (length < 0 || std::size_t(length) >= capacity) {
        type_resource_result(TypeResourceStatus::invalid_argument);
        return false;
    }
    // 0x005F96B0 is called for ALL building parts, not only NewTheater=yes.
    if (theater && std::strlen(base) >= 2) {
        const auto lower = [](unsigned char c) { return c >= 'A' && c <= 'Z' ? c + 32 : int(c); };
        const int first = lower(static_cast<unsigned char>(output[0]));
        const int second = lower(static_cast<unsigned char>(output[1]));
        if ((first == 'g' || first == 'n' || first == 'c' || first == 'y') && (second == 'a' || second == 't'))
            output[1] = theater->Letter[0];
    }
    return true;
}
bool read_type_sound(CCINIClass& ini, const char* section, const char* key, int& value, std::size_t capacity) {
    char name[512]{};
    if (!capacity || capacity > sizeof(name)) return false;
    if (!ini.ReadString(section, key, "", name, capacity)) return true;
    if (type_resources().audio_unavailable) {
        value = -1;
        return true;
    }
    const auto& runtime = rules_runtime();
    int found = -1;
    if (!runtime.sound_index || !runtime.sound_index(runtime.context, name, found)) {
        type_resource_result(TypeResourceStatus::unavailable);
        return false;
    }
    if (found != -1) value = found;
    return true;
}
}
