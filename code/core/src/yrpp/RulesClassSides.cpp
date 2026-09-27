// Original Side/HouseType objects and cross-references, calibrated at 672440.
// YR's side grouping has no counterpart in pinned RA1's fixed country model.
#include "yrpp/RulesClass.h"
#include "yrpp/SideClass.h"
#include "yrpp/HouseTypeClass.h"
#include "RulesClassReaders.hpp"

namespace {
bool find_index(AbstractType kind, const char* name, int& index) {
    const auto& runtime = game::rules_runtime();
    return runtime.find_index && runtime.find_index(runtime.context, kind, name, index);
}
void append_country(TypeList<int>& list, int index) {
    if (!list.AddItem(index)) throw std::bad_alloc();
}
}
bool YRPP_STDCALL RulesClass::Read_Sides(CCINIClass* ini) {
    if (!ini) return false;
    const int count = ini->GetKeyCount("Sides");
    for (int index = 0; index < count; ++index) {
        const char* name = ini->GetKeyName("Sides", index);
        AbstractTypeClass* result = nullptr;
        if (!game::rules_resolve_type(AbstractType::Side, name, result) || !result)
            throw std::runtime_error("RulesClass side allocation failed");
        auto* side = static_cast<SideClass*>(result);
        char text[128];
        if (ini->ReadString("Sides", name, "", text, sizeof(text))) {
            TypeList<int> countries;
            char* cursor = text;
            while (char* token = next_rule_token(cursor)) {
                int country = -1;
                if (!find_index(AbstractType::HouseType, token, country))
                    throw std::runtime_error("RulesClass country index dependency failed");
                if (country != -1) append_country(countries, country);
                else {
                    int source_index = -1;
                    if (!find_index(AbstractType::Side, token, source_index))
                        throw std::runtime_error("RulesClass side index dependency failed");
                    if (source_index != -1) {
                        AbstractTypeClass* source_type = nullptr;
                        if (!game::rules_type_at(AbstractType::Side, source_index, source_type))
                            throw std::runtime_error("RulesClass side enumeration dependency failed");
                        auto* source = static_cast<SideClass*>(source_type);
                        for (int i = 0; i < source->HouseTypes.Count; ++i)
                            append_country(countries, source->HouseTypes[i]);
                    }
                }
            }
            side->HouseTypes = countries;
        } else side->HouseTypes = TypeList<int>(side->HouseTypes);
        for (int i = 0; i < side->HouseTypes.Count; ++i) {
            int side_count = 0, side_index = -1;
            if (!game::rules_type_count(AbstractType::Side, side_count))
                throw std::runtime_error("RulesClass side count dependency failed");
            for (int j = 0; j < side_count; ++j) {
                AbstractTypeClass* candidate = nullptr;
                if (!game::rules_type_at(AbstractType::Side, j, candidate))
                    throw std::runtime_error("RulesClass side enumeration dependency failed");
                if (candidate == side) { side_index = j; break; }
            }
            AbstractTypeClass* country_type = nullptr;
            if (!game::rules_type_at(AbstractType::HouseType, side->HouseTypes[i], country_type))
                throw std::runtime_error("RulesClass country enumeration dependency failed");
            auto* country = static_cast<HouseTypeClass*>(country_type);
            country->SideIndex = side_index;
        }
    }
    return count > 0;
}

bool YRPP_STDCALL RulesClass::Read_Teams_Obsolete(CCINIClass* ini) {
    if (!ini) return false;
    const int count = ini->GetKeyCount("Teams");
    for (int index = 0; index < count; ++index) {
        char name[128];
        if (!ini->ReadString("Teams", ini->GetKeyName("Teams", index), "", name, sizeof(name)))
            return false; // Target dereferences null for an empty entry.
        AbstractTypeClass* team = nullptr;
        if (!game::rules_resolve_type(AbstractType::TeamType, name, team) || !team)
            throw std::runtime_error("RulesClass team allocation failed");
        team->LoadFromINI(ini);
    }
    return count > 0;
}
