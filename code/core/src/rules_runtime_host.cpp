#include "yrpp/InfantryTypeClass.h"
#include "yrpp/UnitTypeClass.h"
#include "yrpp/AircraftTypeClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/VoxelAnimTypeClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/ParticleTypeClass.h"
#include "yrpp/ParticleSystemTypeClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/SuperWeaponTypeClass.h"
#include "yrpp/HouseTypeClass.h"
#include "yrpp/CampaignClass.h"
#include "yrpp/TeamTypeClass.h"
#include "yrpp/AITriggerTypeClass.h"
#include "yrpp/TriggerTypeClass.h"
#include "yrpp/TagTypeClass.h"
#include "yrpp/TiberiumClass.h"
#include "rules_runtime.hpp"
#include "yrpp/ScriptTypeClass.h"
#include "yrpp/SideClass.h"
#include "yrpp/TaskForceClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/SmudgeTypeClass.h"
#include "yrpp/TerrainTypeClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/ColorScheme.h"
#include <cfenv>
#include <new>

namespace game {
namespace {
template<class T, class Operation> bool visit(Operation& operation) { return operation(T::Array); }
template<class Operation> bool visit_registry(AbstractType kind, Operation operation) {
    switch (kind) {
    case AbstractType::ScriptType: return visit<ScriptTypeClass>(operation);
    case AbstractType::Side: return visit<SideClass>(operation);
    case AbstractType::TaskForce: return visit<TaskForceClass>(operation);
    case AbstractType::OverlayType: return visit<OverlayTypeClass>(operation);
    case AbstractType::SmudgeType: return visit<SmudgeTypeClass>(operation);
    case AbstractType::TerrainType: return visit<TerrainTypeClass>(operation);
    case AbstractType::IsotileType: return visit<IsometricTileTypeClass>(operation);
    case InfantryTypeClass::AbsID: return visit<InfantryTypeClass>(operation);
    case UnitTypeClass::AbsID: return visit<UnitTypeClass>(operation);
    case AircraftTypeClass::AbsID: return visit<AircraftTypeClass>(operation);
    case BuildingTypeClass::AbsID: return visit<BuildingTypeClass>(operation);
    case AnimTypeClass::AbsID: return visit<AnimTypeClass>(operation);
    case VoxelAnimTypeClass::AbsID: return visit<VoxelAnimTypeClass>(operation);
    case BulletTypeClass::AbsID: return visit<BulletTypeClass>(operation);
    case ParticleTypeClass::AbsID: return visit<ParticleTypeClass>(operation);
    case ParticleSystemTypeClass::AbsID: return visit<ParticleSystemTypeClass>(operation);
    case WeaponTypeClass::AbsID: return visit<WeaponTypeClass>(operation);
    case WarheadTypeClass::AbsID: return visit<WarheadTypeClass>(operation);
    case SuperWeaponTypeClass::AbsID: return visit<SuperWeaponTypeClass>(operation);
    case HouseTypeClass::AbsID: return visit<HouseTypeClass>(operation);
    case CampaignClass::AbsID: return visit<CampaignClass>(operation);
    case TeamTypeClass::AbsID: return visit<TeamTypeClass>(operation);
    case AITriggerTypeClass::AbsID: return visit<AITriggerTypeClass>(operation);
    case TriggerTypeClass::AbsID: return visit<TriggerTypeClass>(operation);
    case TagTypeClass::AbsID: return visit<TagTypeClass>(operation);
    case TiberiumClass::AbsID: return visit<TiberiumClass>(operation);
    default: return false;
    }
}
bool count(void*, AbstractType kind, int& output) {
    return visit_registry(kind, [&](auto& array) { output = array.Count; return true; });
}
bool at(void*, AbstractType kind, int index, AbstractTypeClass*& output) {
    return visit_registry(kind, [&](auto& array) {
        if (index < 0 || index >= array.Count) return false;
        output = array[index]; return true;
    });
}
template<class T> bool allocate(const char*, AbstractTypeClass*&, bool);
bool find(void*, AbstractType kind, const char* name, int& output) {
    if (!name) return false;
    if (kind == AbstractType::ParticleSystemType) {
        // 0x00644630 is From_Name, unlike pointer FindOrAllocate: literal
        // "none" is a name, only "<none>" and empty produce index -1.
        if (!*name || !_strcmpi(name,"<none>")) { output=-1; return true; }
        try {
            AbstractTypeClass* object=nullptr;
            if (!allocate<ParticleSystemTypeClass>(name,object,false)) return false;
            for (int i=0;i<ParticleSystemTypeClass::Array.Count;++i)
                if (ParticleSystemTypeClass::Array[i]==object) { output=i; return true; }
            output=-1;
            return true;
        } catch (...) { return false; }
    }
    return visit_registry(kind, [&](auto& array) {
        for (int i = 0; i < array.Count; ++i) {
            if (array[i] && _strcmpi(array[i]->ID, name) == 0) { output = i; return true; }
        }
        output = -1; return true;
    });
}
template<class T> bool allocate(const char* name, AbstractTypeClass*& output, bool sentinel) {
    // Side creation follows the existing original Rules binding (no sentinel).
    if (sentinel && (_strcmpi(name, "none") == 0 || _strcmpi(name, "<none>") == 0)) {
        output = nullptr; return true;
    }
    if (auto* existing = T::Find(name)) { output = existing; return true; }
    auto* storage = YRMemory::Allocate(sizeof(T));
    if (!storage) { output = nullptr; return true; }
    try { output = new (storage) T(name); }
    catch (...) { YRMemory::Deallocate(storage); throw; }
    return true; // Target FindOrAllocate returns null on allocation failure.
}
bool resolve(void*, AbstractType kind, const char* name, AbstractTypeClass*& output) {
    if (!name) return false;
    try {
        switch (kind) {
        case AbstractType::ScriptType: output = ScriptTypeClass::FindOrAllocate(name); return true;
        case AbstractType::Side: return allocate<SideClass>(name, output, false);
        case AbstractType::OverlayType: output = OverlayTypeClass::FindOrAllocate(name); return true;
        case AbstractType::SmudgeType: output = SmudgeTypeClass::FindOrAllocate(name); return true;
        case AbstractType::TerrainType: output = TerrainTypeClass::FindOrAllocate(name); return true;
        case InfantryTypeClass::AbsID: return allocate<InfantryTypeClass>(name, output, true);
        case UnitTypeClass::AbsID: return allocate<UnitTypeClass>(name, output, true);
        case AircraftTypeClass::AbsID: return allocate<AircraftTypeClass>(name, output, true);
        case BuildingTypeClass::AbsID: { auto* existing = BuildingTypeClass::Find(name); if (!existing) return false; output = existing; return true; }
        case AnimTypeClass::AbsID: return allocate<AnimTypeClass>(name, output, true);
        case VoxelAnimTypeClass::AbsID: return allocate<VoxelAnimTypeClass>(name, output, true);
        case BulletTypeClass::AbsID: return allocate<BulletTypeClass>(name, output, true);
        case ParticleTypeClass::AbsID: return allocate<ParticleTypeClass>(name, output, true);
        case ParticleSystemTypeClass::AbsID: return allocate<ParticleSystemTypeClass>(name, output, true);
        case WeaponTypeClass::AbsID: return allocate<WeaponTypeClass>(name, output, true);
        case WarheadTypeClass::AbsID: return allocate<WarheadTypeClass>(name, output, true);
        case SuperWeaponTypeClass::AbsID: return allocate<SuperWeaponTypeClass>(name, output, true);
        case HouseTypeClass::AbsID: return allocate<HouseTypeClass>(name, output, true);
        case CampaignClass::AbsID: { auto* existing = CampaignClass::Find(name); if (!existing) return false; output = existing; return true; }
        case TeamTypeClass::AbsID: return allocate<TeamTypeClass>(name, output, true);
        case AITriggerTypeClass::AbsID: return allocate<AITriggerTypeClass>(name, output, true);
        case TriggerTypeClass::AbsID: return allocate<TriggerTypeClass>(name, output, true);
        case TagTypeClass::AbsID: return allocate<TagTypeClass>(name, output, true);
        case TiberiumClass::AbsID: return allocate<TiberiumClass>(name, output, true);
        case AbstractType::TaskForce: return allocate<TaskForceClass>(name, output, true);
        default: return false; // TMP needs its explicit multi-argument constructor.
        }
    } catch (...) { return false; }
}
const RulesRuntimeServices services = [] {
    RulesRuntimeServices value;
    value.color_schemes=&ColorScheme::Array;
    value.resolve = resolve; value.type_count = count; value.type_at = at; value.find_index = find;
    value.finalize_weapon = [](void*, AbstractTypeClass* type) {
        auto* weapon = dynamic_cast<WeaponTypeClass*>(type);
        if (!weapon || (weapon->Projectile && weapon->Projectile->ROT == 0 && !RulesClass::Instance))
            return false;
        weapon->CalculateSpeed();
        return true;
    };
    return value;
}();
}
const RulesRuntimeServices& native_rules_runtime() noexcept { return services; }
const RulesRuntimeServices* default_rules_runtime() noexcept { return &services; }
void rules_integer_rounding() noexcept { std::fesetround(FE_TOWARDZERO); }
}
