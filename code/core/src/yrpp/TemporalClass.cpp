// Native callee failures are fatal; exceptions do not cross the weapon boundary.
// Original YR temporal module, 0x71A4E0..0x71B1B0.
#include "yrpp/TemporalClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/SpawnManagerClass.h"
#include "yrpp/CaptureManagerClass.h"
#include "yrpp/RadarEventClass.h"
#include "yrpp/VoxClass.h"
#include "yrpp/WeaponTypeClass.h"
#include <cstdlib>
#include <bit>
#include <new>
#include <cstring>
#if !defined(RA2_YRPP_GAME)
TemporalClass::TemporalClass(TechnoClass* owner) noexcept
    : AbstractClass(), Owner(owner), Target(nullptr), LifeTimer{}, unknown_pointer_38(nullptr), SourceSW(nullptr),
      NextTemporal(nullptr), PrevTemporal(nullptr), WarpRemaining(0), WarpPerStep(0) {
    LifeTimer.StartTime = Unsorted::CurrentFrame;
    LifeTimer.TimeLeft = 0;
    try {
        Array.AddItem(this);
    } catch (const std::bad_alloc&) {
        Array.IsInitialized = true;
    }
}
TemporalClass::~TemporalClass() { Array.Remove(this); }
bool TemporalClass::CanWarpTarget(TechnoClass* target) const try {
    if (!target || !target->GetTechnoType()->Warpable || target->IsIronCurtained()) return false;
    if (target->WhatAmI() == AbstractType::Unit) {
        auto* linked = target->GetNthLink(0);
        if (linked && linked->WhatAmI() == AbstractType::Building) {
            auto* building = static_cast<BuildingClass*>(linked);
            if (building->Type->WeaponsFactory &&
                MapClass::Instance.GetCellAt(target->Location)->GetBuilding() == building)
                return false;
        }
    }
    return true;
} catch (...) {
    std::abort();
}
int TemporalClass::GetWarpPerStep(int helperCount) const try {
    int helpers = 0;
    if (PrevTemporal && helperCount <= 50) helpers = PrevTemporal->GetWarpPerStep(helperCount + 1);
    WarpPerStep = Owner->GetWeapon(Owner->SelectWeapon(nullptr))->WeaponType->Damage;
    return std::bit_cast<int>(unsigned(helpers) + unsigned(WarpPerStep));
} catch (...) {
    std::abort();
}
void TemporalClass::Fire(TechnoClass* target) try {
    if (target) {
        if (target->SpawnManager) target->SpawnManager->KillNodes();
        if (target->CaptureManager) target->CaptureManager->FreeAll();
    }
    if (Owner->TemporalImUsing && Owner->TemporalImUsing->Target) Owner->TemporalImUsing->LetGo();
    if (!CanWarpTarget(target) || Owner->TemporalTargetingMe) return;
    Target = target;
    if (auto* head = Target->TemporalTargetingMe) {
        NextTemporal = head;
        PrevTemporal = head->PrevTemporal;
        head->PrevTemporal = this;
        if (PrevTemporal) PrevTemporal->NextTemporal = this;
    } else {
        Target->TemporalTargetingMe = this;
        WarpRemaining = std::bit_cast<int>(10u * unsigned(Target->GetTechnoType()->Strength));
        if (Target->WhatAmI() == AbstractType::Unit && static_cast<UnitClass*>(Target)->Type->Harvester &&
            Target->Owner == HouseClass::CurrentPlayer) {
            CoordStruct at;
            at = *Target->GetDestination(&at, nullptr);
            if (RadarEventClass::Create(RadarEventType::HarvesterAttacked, {short(at.X / 256), short(at.Y / 256)}))
                VoxClass::Play("EVA_OreMinerUnderAttack");
        }
        if (Target->WhatAmI() == AbstractType::Building && !Target->GetTechnoType()->Insignificant &&
            !Target->IsStrange())
            Target->Owner->BuildingUnderAttack(static_cast<BuildingClass*>(Target));
    }
    Target->BeingWarpedOut = true;
    if (Target->GetTechnoType()->IsGattling) Target->GattlingRateDown(1);
    if (Target->WhatAmI() == AbstractType::Building) {
        Target->Owner->RecheckPower = true;
        static_cast<BuildingClass*>(Target)->DisableTemporal();
        Target->Owner->RecheckTechTree = true;
    }
    Target->Mark(MarkType::Change);
    if (Target->TemporalImUsing && Target->TemporalImUsing->Target) Target->TemporalImUsing->LetGo();
    if (HouseClass::CurrentPlayer) Target->Deselect();
} catch (...) {
    std::abort();
}
void TemporalClass::LetGo() try {
    if (NextTemporal) {
        if (PrevTemporal) PrevTemporal->NextTemporal = NextTemporal;
        NextTemporal->PrevTemporal = PrevTemporal;
    } else if (PrevTemporal) {
        if (Target) Target->TemporalTargetingMe = PrevTemporal;
        PrevTemporal->NextTemporal = nullptr;
        PrevTemporal->WarpRemaining = WarpRemaining;
    } else if (Target) {
        Target->TemporalTargetingMe = nullptr;
        Target->BeingWarpedOut = false;
        if (Target->WhatAmI() == AbstractType::Building) {
            Target->Owner->RecheckPower = true;
            static_cast<BuildingClass*>(Target)->EnableTemporal();
            Target->Owner->RecheckTechTree = true;
        }
        Target->Mark(MarkType::Change);
    }
    Target = nullptr;
    PrevTemporal = NextTemporal = nullptr;
    SourceSW = nullptr;
    unknown_pointer_38 = nullptr;
} catch (...) {
    std::abort();
}

HRESULT YRPP_STDCALL TemporalClass::GetClassID(CLSID* output) {
    if (!output) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[4] = {0x94112424u, 0x11d3e403u, 0x50006e8eu, 0xfbb2aa04u};
    std::memcpy(output, words, sizeof(words));
    return 0;
}
#endif
