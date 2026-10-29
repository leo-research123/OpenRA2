// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744f70235e0d5ddca107364a68f95132ce9 building.cpp:
// Visual_Character, Is_Radar_Visible, Is_Ready_To_Cloak, Should_Uncloak,
// Turret_Facing, Fire_Direction, Crew_Type and Assign_Target.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md. Calibrated to YR 1.001.
#include "yrpp/BuildingClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/Surface.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/BulletTypeClass.h"
#include "x87_integer.hpp"
#include <bit>
#include <cstring>

#if !defined(RA2_YRPP_GAME)
namespace {
int add(int a, int b) { return std::bit_cast<int>(unsigned(a) + unsigned(b)); }
bool presentation_window() { return DSurface::WindowBounds.Width > 0 && DSurface::WindowBounds.Height > 0; }
bool nearby_detector(const BuildingClass& building) {
    const auto origin = building.GetMapCoords();
    const int width = building.Type->GetFoundationWidth(), height = building.Type->GetFoundationHeight(false);
    for (int x = -1; x <= width; ++x)
        for (int y = -1; y <= height; ++y) {
            const CellStruct at{std::bit_cast<short>(static_cast<unsigned short>(unsigned(origin.X) + unsigned(x))),
                                std::bit_cast<short>(static_cast<unsigned short>(unsigned(origin.Y) + unsigned(y)))};
            auto* techno = MapClass::Instance.GetCellAt(at)->FindTechnoNearestTo(Point2D{0, 0}, false, nullptr);
            if (techno == &building) techno = MapClass::Instance.GetCellAt(at)->GetInfantry(false);
            // YR compares owners, not alliances: an allied foreign detector also counts.
            if (techno && techno->Owner != building.Owner && techno->GetTechnoType()->Sensors) return true;
        }
    return false;
}
}

int BuildingClass::Mission_AreaGuard() { return Mission_Guard(); }
int BuildingClass::GetAntiAirValue() const { return Type->AntiAirValue; }
int BuildingClass::GetAntiArmorValue() const { return Type->AntiArmorValue; }
int BuildingClass::GetAntiInfantryValue() const { return Type->AntiInfantryValue; }
ZGradient BuildingClass::GetZGradient() const { return ZGradient::Deg90; }
CellStruct BuildingClass::GetLastFlightMapCoords() const { return CellStruct::Empty; }

CoordStruct* BuildingClass::GetExitCoords(CoordStruct* output, DWORD) const {
    if (Type->ExitCoord == CoordStruct::Empty) return GetCoords(output);
    *output = {add(Location.X, Type->ExitCoord.X), add(Location.Y, Type->ExitCoord.Y),
               add(Location.Z, Type->ExitCoord.Z)};
    return output;
}

int BuildingClass::GetPipFillLevel() const {
    if (Type->PipScale != PipScale::Tiberium) return TechnoClass::GetPipFillLevel();
    if (!Type->Weeder) return 0;
    const double filled = Owner->GetWeedStoragePercentage();
    return game::x87_scaled_quotient_to_integer(filled, double(Type->GetPipMax()), 1);
}

InfantryTypeClass* BuildingClass::GetCrew() const {
    if (HasBeenCaptured || ScenarioClass::Instance->Random.RandomRanged(0, 99) >= 25 ||
        Type->Factory != AbstractType::BuildingType)
        return TechnoClass::GetCrew();
    return RulesClass::Instance->Engineer;
}

int BuildingClass::VoiceDeploy() {
    if (ArchiveTarget && ArchiveTarget->WhatAmI() == AbstractType::Cell &&
        ArchiveTarget->WhatAmI() == AbstractType::Cell &&
        static_cast<CellClass*>(ArchiveTarget)->LandType == LandType::Tiberium && Type->ResourceDestination &&
        Type->ResourceGatherer && Type->VoiceHarvest != -1) {
        QueueVoice(Type->VoiceHarvest);
        return 0;
    }
    return TechnoClass::VoiceDeploy();
}

void BuildingClass::SetTarget(AbstractClass* target) {
    if (CurrentMission == Mission::Selling || !IsPowerOnline()) {
        TechnoClass::SetTarget(nullptr);
        return;
    }
    if (!target || !GetWeapon(0)->WeaponType || GetWeapon(0)->WeaponType->Projectile->AA ||
        IsCloseEnoughToAttack(target)) {
        TechnoClass::SetTarget(target);
        return;
    }
    TechnoClass::SetTarget(nullptr);
    if ((Type->TickTank || Type->Artillary) && !Owner->IsControlledByHuman() && !IsUnderEMP()) {
        QueueMission(Mission::Selling, false);
        NextMission();
    }
}

bool BuildingClass::IsReadyToCloak() const { return TechnoClass::IsReadyToCloak() && !nearby_detector(*this); }
bool BuildingClass::ShouldNotBeCloaked() const { return TechnoClass::ShouldNotBeCloaked() || nearby_detector(*this); }

VisualType BuildingClass::VisualCharacter(VARIANT_BOOL raw, HouseClass* asking) const {
    const auto level = static_cast<signed char>(Translucency);
    if (!level) return TechnoClass::VisualCharacter(raw, asking);
    if (level <= 10) return level > 5 ? VisualType::Darken : VisualType::Indistinct;
    if (static_cast<unsigned char>(raw)) {
        if (asking && MapClass::Instance.GetCellAt(Location)->Sensors_InclHouse(asking->ArrayIndex))
            return VisualType::Shadowy;
    } else if (IsOwnedByCurrentPlayer || IsSensorVisibleToPlayer() || !presentation_window() ||
               (static_cast<int>(SessionClass::Instance.GameMode) != 0 && Owner && HouseClass::CurrentPlayer &&
                HouseClass::CurrentPlayer->IsAlliedWith(Owner) && Owner->IsAlliedWith(HouseClass::CurrentPlayer)))
        return VisualType::Shadowy;
    return VisualType::Hidden;
}

bool BuildingClass::IsRadarVisible(int* detection) const {
    if (Type->Invisible || InLimbo) return false;
    if (Owner->IsControlledByCurrentPlayer()) return DiscoveredByCurrentPlayer;
    const auto center = GetCoords();
    if (!MapClass::Instance.IsWithinUsableArea(CellStruct{short(center.X / 256), short(center.Y / 256)}, true))
        return false;
    const int y = std::bit_cast<int>(unsigned(Type->GetFoundationHeight(false)) * 256u - 256u);
    const int x = std::bit_cast<int>(unsigned(Type->GetFoundationWidth()) * 256u - 256u);
    const bool shrouded =
        MapClass::Instance.IsLocationShrouded(Location) &&
        MapClass::Instance.IsLocationShrouded(CoordStruct{add(Location.X, x), add(Location.Y, y), Location.Z}) &&
        presentation_window();
    if (CloakState != ::CloakState::Cloaked && Translucency != 15 && !IsFogged && !shrouded) return true;
    if (!IsSensorVisibleToPlayer()) return false;
    if (!HouseClass::CurrentPlayer->IsAlliedWith(Owner) && !IsFogged && !shrouded) *detection = 1;
    return true;
}

DirStruct* BuildingClass::TurretFacing(DirStruct* output) const {
    if (HasTurret() || !Target) {
        *output = PrimaryFacing.Current();
        return output;
    }
    const auto target = Target->GetCoords(), origin = GetCoords();
    const double angle = Math::atan2(double(origin.Y) - target.Y, double(target.X) - origin.X);
    const auto direction =
        static_cast<unsigned short>(game::x87_integer((angle - 1.5707963267948966) * -10430.060040584269));
    // The direction occupies the low word; YR retains the coordinate's high word.
    const unsigned packed = (unsigned(origin.X) & 0xFFFF0000u) | direction;
    std::memcpy(output, &packed, sizeof(packed));
    return output;
}

DirStruct* BuildingClass::GetRealFacing(DirStruct* output) const {
    if (Target && (!HasTurret() || Type->TurretAnimIsVoxel)) {
        *output = FireAngleTo(Target);
        return output;
    }
    const auto raw = PrimaryFacing.Current().Raw;
    output->Raw = Target ? WORD(((((unsigned(raw) >> 10) + 1) >> 1) & 0x1F) << 11)
                         : WORD(((((unsigned(raw) >> 7) + 1) >> 1) & 0xFF) << 8);
    return output;
}

DamageState BuildingClass::IronCurtain(int duration, HouseClass* source, bool forceShield) {
    // This cancels planted C4, not the player's repair command.
    if (C4Applied) {
        C4Applied = false;
        C4AppliedBy = nullptr;
        C4Timer.Start(0);
    }
    return TechnoClass::IronCurtain(duration, source, forceShield);
}
#endif
