// Original live-array destruction, 0x4FC6D0..0x4FC980.
#include "yrpp/HouseClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/CaptureManagerClass.h"
#include "yrpp/TemporalClass.h"
#include <cstdlib>

namespace {
void destroy_objects(HouseClass* house, int kind) noexcept {
    try {
        TechnoClass* previous = nullptr;
        int i = 0;
        while (i < TechnoClass::Array.Count) {
            auto* object = TechnoClass::Array[i];
            auto* original = object->GetOriginalOwner();
            if (kind == 0 && original != object->Owner &&
                (original != house || (object->MindControlledBy &&
                                       object->MindControlledBy->CaptureManager->SetOriginalOwnerToCivilian(object)))) {
                ++i;
                continue;
            }
            if (original != house) {
                ++i;
                continue;
            }
            if (kind != 0) {
                const bool building = object->WhatAmI() == AbstractType::Building;
                if (building != (kind == 1) || object->InLimbo) {
                    ++i;
                    continue;
                }
                if (kind >= 2 && object->GetTechnoType()->Naval != (kind == 3)) {
                    ++i;
                    continue;
                }
            }
            if (object == previous) {
                ++i;
                continue;
            }
            int damage = object->Health;
            if (object->TemporalTargetingMe) object->TemporalTargetingMe->JustLetGo();
            object->ReceiveDamage(&damage, 0, RulesClass::Instance->C4Warhead, nullptr, true, true, nullptr);
            // Damage can remove the current entry. Re-read this live index;
            // a surviving object is guarded against a second damage call.
            previous = object;
        }
    } catch (...) {
        std::abort();
    }
}
}
void HouseClass::DestroyAll() noexcept { destroy_objects(this, 0); }
void HouseClass::DestroyAllBuildings() noexcept { destroy_objects(this, 1); }
void HouseClass::DestroyAllNonBuildingsNonNaval() noexcept { destroy_objects(this, 2); }
void HouseClass::DestroyAllNonBuildingsNaval() noexcept { destroy_objects(this, 3); }
