#pragma once
#include <string>

class TechnoTypeClass;

namespace game {
// Lookup dependencies of the original INI helpers. Objects and registries are
// owned by the host's existing game classes; this API owns no substitute model.
enum class IniTypeKind {
    color_scheme, super_weapon, vox, house_type, house, side, movie, theme,
    infantry, unit, aircraft, building, techno
};
struct IniTypeResult {
    int index = -1;
    TechnoTypeClass* techno = nullptr;
};
struct IniRuntimeServices {
    void* context = nullptr;
    // A miss leaves the output unchanged. Names are borrowed for the operation.
    bool (*name)(void*, IniTypeKind, int, const char*&) = nullptr;
    // allocate=true is used only for HouseType and Side. Preserve original
    // registry insertion and constructor semantics; never allocate proxy types.
    // Country lookup includes UIName/ID and Random=-2; House lookup is case-sensitive.
    // ColorScheme lookup selects the matching scheme with ShadeCount != 1.
    bool (*find)(void*, IniTypeKind, const char*, bool allocate, IniTypeResult&) = nullptr;
    // Return the original StringTable result, including its MISSING: text.
    bool (*stringtable)(void*, const char*, const wchar_t*&) = nullptr;
};

// Install borrowed original-game dependencies for a synchronous operation.
// Nested scopes restore the previous services, including on exception. All
// callbacks are required. The host serializes mutations of its game registries.
// Basic text, values, enums, UUBlock and digest need no runtime services.
bool with_ini_runtime(const IniRuntimeServices& services, void (*operation)(void*),
    void* context, std::string& error);
}
