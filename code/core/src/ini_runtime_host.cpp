#include "ini_runtime.hpp"
#include "rules_runtime.hpp"
#include "type_registry.hpp"
#include "yrpp/HouseTypeClass.h"
#include "yrpp/SideClass.h"
#include "yrpp/SuperWeaponTypeClass.h"
#include "yrpp/InfantryTypeClass.h"
#include "yrpp/UnitTypeClass.h"
#include "yrpp/AircraftTypeClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/StringTable.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/Unsorted.h"
#include <cstring>
namespace game {
namespace {
template<class T> bool name_in(int index, const char*& result) {
    if (index < 0 || index >= T::Array.Count || !T::Array[index]) return false;
    result = T::Array[index]->ID; return true;
}
bool name(void*, IniTypeKind kind, int index, const char*& result) {
    switch (kind) {
    case IniTypeKind::movie:
        if(index<0||index>=MovieInfo::Array.Count)return false;
        result=MovieInfo::Array[index];return result!=nullptr;
    case IniTypeKind::color_scheme: {
        const auto* colors=rules_runtime().color_schemes;
        if(!colors || index<0 || index>=colors->Count || !(*colors)[index])return false;
        result=(*colors)[index]->ID;return result!=nullptr;
    }
    case IniTypeKind::house_type: return name_in<HouseTypeClass>(index, result);
    case IniTypeKind::side: return name_in<SideClass>(index, result);
    case IniTypeKind::super_weapon: return name_in<SuperWeaponTypeClass>(index, result);
    case IniTypeKind::infantry: return name_in<InfantryTypeClass>(index, result);
    case IniTypeKind::unit: return name_in<UnitTypeClass>(index, result);
    case IniTypeKind::aircraft: return name_in<AircraftTypeClass>(index, result);
    case IniTypeKind::building: return name_in<BuildingTypeClass>(index, result);
    case IniTypeKind::techno: return name_in<TechnoTypeClass>(index, result);
    default: return false;
    }
}
template<class T> bool find_in(const char* text, IniTypeResult& output) {
    const int index = T::FindIndex(text);
    if (index < 0) return false;
    IniTypeResult result; result.index = index;
    if constexpr (std::is_base_of_v<TechnoTypeClass, T>) result.techno = T::Array[index];
    output = result; return true;
}
bool find(void*, IniTypeKind kind, const char* text, bool allocate, IniTypeResult& output) {
    if (!text) return false;
    try {
        switch (kind) {
        case IniTypeKind::movie: {
            const int index=MovieInfo::FindIndex(text);
            if(index<0)return false;
            output=IniTypeResult{index,nullptr};return true;
        }
        case IniTypeKind::color_scheme: {
            const auto* colors=rules_runtime().color_schemes;
            if(!colors)return false;
            for(int i=0;i<colors->Count;++i) {
                const auto* color=(*colors)[i];
                // 0x00474A90 excludes the single-shade companion entries.
                if(color && color->ID && color->ShadeCount!=1 && !_strcmpi(color->ID,text)) {
                    output=IniTypeResult{i,nullptr};return true;
                }
            }
            return false;
        }
        case IniTypeKind::house_type:
            if (!_strcmpi(text, "Random")) { output = IniTypeResult{-2, nullptr}; return true; }
            if (const int country = HouseTypeClass::FindIndexOfName(text); country >= 0) {
                output = IniTypeResult{country, nullptr}; return true;
            }
            if (allocate && game::allocate_type<HouseTypeClass>(text)) return find_in<HouseTypeClass>(text, output);
            return false;
        case IniTypeKind::side:
            if (find_in<SideClass>(text, output)) return true;
            if (allocate && game::allocate_type<SideClass>(text)) return find_in<SideClass>(text, output);
            return false;
        case IniTypeKind::super_weapon: return find_in<SuperWeaponTypeClass>(text, output);
        case IniTypeKind::infantry: return find_in<InfantryTypeClass>(text, output);
        case IniTypeKind::unit: return find_in<UnitTypeClass>(text, output);
        case IniTypeKind::aircraft: return find_in<AircraftTypeClass>(text, output);
        case IniTypeKind::building: return find_in<BuildingTypeClass>(text, output);
        case IniTypeKind::techno: return find_in<TechnoTypeClass>(text, output);
        case IniTypeKind::vox: {
            const auto& services = rules_runtime();
            int index = -1;
            if (!services.sound_index || !services.sound_index(services.context, text, index) || index < 0) return false;
            output = IniTypeResult{index, nullptr}; return true;
        }
        default: return false;
        }
    } catch (...) { return false; }
}
bool stringtable(void*, const char* label, const wchar_t*& output) {
    if (!label) return false;
    const auto* value = StringTable::LoadString(label);
    if (!value) return false;
    output = value; return true;
}
const IniRuntimeServices services{nullptr, name, find, stringtable};
}
const IniRuntimeServices* default_ini_runtime() noexcept { return &services; }
}
