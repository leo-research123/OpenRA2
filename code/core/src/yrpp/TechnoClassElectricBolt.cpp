// YR 0x6FD460 / 0x6FD570: effect creation after normal weapon damage.
#include "yrpp/TechnoClass.h"
#include "yrpp/EBolt.h"
#include "yrpp/UnitClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/TacticalClass.h"
#include <algorithm>
#include <bit>

EBolt* TechnoClass::CreateElectricBolt(AbstractClass* target, WeaponTypeClass* weapon, const CoordStruct& source) {
    void* memory = YRMemory::Allocate(sizeof(EBolt));
    if (!memory) return nullptr;
    auto* bolt = ::new (memory) EBolt;
    int depth = 0;
    if (WhatAmI() == AbstractType::Building) {
        const auto firing = TacticalClass::CoordsToScreen(source);
        CoordStruct rendered;
        GetRenderCoords(&rendered);
        const auto base = TacticalClass::CoordsToScreen(rendered);
        depth = std::min(0, std::bit_cast<int>(unsigned(firing.Y) - unsigned(base.Y)));
    }
    CoordStruct impact;
    if (target && (target->AbstractFlags & ::AbstractFlags::Object) != ::AbstractFlags::None)
        static_cast<ObjectClass*>(target)->GetTargetCoords(&impact);
    else
        target->GetCenterCoords(&impact);
    if (weapon) bolt->AlternateColor = weapon->IsAlternateColor;
    bolt->Fire(source, impact, static_cast<DWORD>(depth));
    return bolt;
}
void TechnoClass::FireElectricBolt(AbstractClass* target) {
    const int slot = SelectWeapon(target);
    auto* weapon = GetWeapon(slot)->WeaponType;
    CoordStruct source;
    GetFLH(&source, slot, CoordStruct(0, 0, 0));
    auto* bolt = CreateElectricBolt(target, weapon, source);
    if (bolt && WhatAmI() == AbstractType::Unit && IsAlive) {
        auto* unit = static_cast<UnitClass*>(this);
        if (!unit->ElectricBolt) {
            unit->ElectricBolt = bolt;
            bolt->SetOwner(unit, slot);
        }
    }
}
