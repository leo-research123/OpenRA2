// YR 0x4D9E70 / 0x4D9F70 / 0x4D98C0.
#include "yrpp/FootClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/VoxClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/RadarEventClass.h"
#include "x87_integer.hpp"
#if !defined(RA2_YRPP_GAME)
void FootClass::Destroyed(ObjectClass*) {
    if (Owner->IsControlledByCurrentPlayer() && !GetTechnoType()->Spawned) {
        const auto cell = GetMapCoords();
        if (RadarEventClass::Create(RadarEventType::UnitLost, cell)) VoxClass::Play("EVA_UnitLost", -1, -1);
    }
}
bool FootClass::CanBeSold() const {
    const auto kind = WhatAmI();
    if (int(kind) > 0 && int(kind) <= 2 && HasAnyLink()) {
        auto* link = GetNthLink();
        if (link->WhatAmI() == AbstractType::Building && static_cast<BuildingClass*>(link)->Type->UnitRepair) {
            const auto there = link->GetCoords(), here = GetCoords();
            const double x = here.X - there.X, y = here.Y - there.Y, z = here.Z - there.Z;
            if (game::x87_integer(Math::sqrt(x * x + y * y + z * z)) < 128) return true;
        }
    }
    return ObjectClass::CanBeSold();
}
void FootClass::Sell(int control) {
    if (!control) return;
    if (Owner->IsControlledByCurrentPlayer()) {
        VoxClass::Play("EVA_UnitSold", -1, -1);
        VocClass::PlayGlobal(RulesClass::Instance->SellSound, 0x2000, 1.0f, nullptr);
    }
    Owner->GiveMoney(GetRefund());
    Stun();
    Limbo();
    UnInit();
}
#endif
