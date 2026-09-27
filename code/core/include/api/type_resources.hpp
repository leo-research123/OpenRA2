#pragma once
#include <cstddef>
#include "yrpp/Theater.h"
class CCINIClass;
#include "yrpp/GeneralStructures.h"

namespace game {
// Borrowed original data only. No image device, shadow type registry or STL ABI.
// Null art uses CCINIClass::INI_Art. An explicit theater overrides Scenario;
// otherwise the real Scenario is queried, never a fabricated world object.
struct TypeResourceServices {
    void* context = nullptr;
    CCINIClass* art = nullptr;
    const TheaterType* theater = nullptr;
    // Required only for the Arctic name-format branch at 005F9070. Bytes of
    // target off_832AE8 were not supplied; the core does not guess that format.
    bool (*arctic_image_name)(void*, const char*, char*, std::size_t) noexcept = nullptr;
    // Borrowed exact foundation entries from 0x00B0EDC0, lifetime >= TerrainType.
    // Missing table data is not synthesized from the Foundation name.
    bool (*terrain_foundation)(void*, int, CellStruct*&) noexcept = nullptr;
    // Explicit capability: map-only sessions omit unresolved sound references.
    bool audio_unavailable = false;
    // Preview-only hosts must not arm units before their original firing,
    // damage and target-query dependencies are installed. Normal/game-backed
    // loads keep this false and load the original weapon/deployment fields.
    bool combat_unavailable = false;
};
enum class TypeResourceStatus { complete, unavailable, invalid_argument, failure };
// Synchronous, nestable scope. Callbacks and the operation may not propagate
// exceptions across the ABI. Caught local exceptions become failure.
TypeResourceStatus with_type_resources(const TypeResourceServices&,
    void (*operation)(void*), void* argument) noexcept;
TypeResourceStatus last_type_resource_status() noexcept;
}
