// YR 0x70FD70: establish the original reciprocal drain and animation ownership.
#include "yrpp/TechnoClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/CaptureManagerClass.h"
#include "yrpp/AnimClass.h"
#include <cstdlib>
#include "yrpp/FootClass.h"
#include "yrpp/TeamClass.h"
#if !defined(RA2_YRPP_GAME)
void TechnoClass::StartDrain(TechnoClass* target) try {
    if (!target || GetCell()->GetBuilding() != target) return;
    target->DrainingMe = this;
    DrainTarget = target;
    target->Owner->RecheckPower = true;
    if (target->CaptureManager) target->CaptureManager->FreeAll();
    auto* memory = YRMemory::Allocate(sizeof(AnimClass));
    CoordStruct coords;
    DrainAnim = memory ? ::new (memory) AnimClass(RulesClass::Instance->DrainAnimationType, *GetCoords(&coords), 0, 1,
                                                  0x600, 0, false)
                       : nullptr;
    if (DrainAnim) DrainAnim->SetOwnerObject(this);
    if ((AbstractFlags & ::AbstractFlags::Foot) != ::AbstractFlags::None) {
        auto* foot = static_cast<FootClass*>(this);
        if (foot->Team) foot->Team->LiberateMember(foot, -1, true);
    }
} catch (...) {
    std::abort();
}

void TechnoClass::StopDrain() try {
    if (DrainAnim) {
        DrainAnim->UnInit();
        DrainAnim = nullptr;
    }
    if (DrainTarget) {
        DrainTarget->DrainingMe = nullptr;
        if (DrainTarget->Owner) DrainTarget->Owner->RecheckPower = true;
        DrainTarget = nullptr;
    }
} catch (...) {
    std::abort();
}

#endif
