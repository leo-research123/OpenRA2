#include "yrpp/BuildingClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/CaptureManagerClass.h"
#include "yrpp/LightSourceClass.h"
#include "yrpp/VoxClass.h"
#include "type_resources.hpp"
#include <cstdio>
#include "map_world.hpp"
#include "x87_integer.hpp"
#if defined(__clang__)
#pragma STDC FENV_ACCESS ON
#elif defined(_MSC_VER)
#pragma fenv_access(on)
#endif
namespace {
void fence_activation_boundary(const BuildingClass& building) {
    // CreateEndPost has no native implementation. Never enter its original JMP.
    if (building.Type->LaserFencePost)
        std::fputs("BuildingClass: native laser-fence activation is unsupported\n", stderr);
}
void power_availability_changed(BuildingClass& building) {
    auto* owner = building.Owner;
    owner->RecheckPower = owner->RecheckRadar = owner->RecheckTechTree = true;
}
}
void BuildingClass::EnableStuff() {
    StuffEnabled = true;
    if (LightSource) LightSource->Activate();
    fence_activation_boundary(*this);
    for (int i = 0; i < 21; ++i)
        if (Type->BuildingAnim[i].Powered && Anims[i]) Anims[i]->PowerOff = false;
    game::map_object_changed();
}
void BuildingClass::DisableStuff() {
    StuffEnabled = false;
    if (LightSource) LightSource->Deactivate();
    if (Type->CloakGenerator) {
        HasCloakingData = -1;
        if (!CloakRadius) CloakRadius = Type->CloakRadiusInCells;
        NeedsRedraw = true;
    }
    if (Type->GapGenerator && GeneratingGap) DestroyGap();
    fence_activation_boundary(*this);
    for (int i = 0; i < 21; ++i)
        if (Type->BuildingAnim[i].Powered && Anims[i]) Anims[i]->PowerOff = true;
    game::map_object_changed();
}
void BuildingClass::GoOnline() {
    if (HasPower || EMPLockRemaining) return;
    HasPower = true;
    NeedsRedraw = true;
    power_availability_changed(*this);
    EnableStuff();
    if (Type->Factory != AbstractType::None) Owner->Update_FactoriesQueues(Type->Factory, Type->Naval, BuildCat(0));
    if (Owner->IsControlledByCurrentPlayer() && !game::type_resources().audio_unavailable)
        VoxClass::Play("EVA_BuildingOnLine", -1, -1);
}
void BuildingClass::GoOffline() {
    if (!HasPower || (!(Type->PowerDrain > 0) && !Type->Powered)) return;
    HasPower = false;
    NeedsRedraw = true;
    power_availability_changed(*this);
    DisableStuff();
    if (Owner->IsControlledByCurrentPlayer() && !game::type_resources().audio_unavailable)
        VoxClass::Play("EVA_BuildingOffLine", -1, -1);
    if (Type->Factory != AbstractType::None) Owner->Update_FactoriesQueues(Type->Factory, Type->Naval, BuildCat(0));
}
void BuildingClass::EnablePowerDependentAnims() noexcept {
    DestroyNthAnim(static_cast<BuildingAnimSlot>(20));
    for (int i = 0; i < 21; ++i) {
        const auto& slot = Type->BuildingAnim[i];
        if (slot.Powered) {
            if (auto* anim = Anims[i]) anim->PowerOff = false;
        } else if (slot.PoweredLight) {
            if (!Anims[i] && ActuallyPlacedOnMap && (!Type->IsAnimDelayedFire || i != 10))
                PlayNthAnim(static_cast<BuildingAnimSlot>(i), !IsGreenHP(), false);
        } else if (slot.PoweredEffect && AnimStates[i]) {
            AnimStates[i] = false;
            PlayNthAnim(static_cast<BuildingAnimSlot>(i), !IsGreenHP(), false);
        }
    }
    game::map_object_changed();
}
void BuildingClass::DisablePowerDependentAnims() noexcept {
    for (int i = 0; i < 21; ++i) {
        const auto& slot = Type->BuildingAnim[i];
        if (slot.Powered) {
            if (auto* anim = Anims[i]) anim->PowerOff = true;
        } else if (slot.PoweredLight) {
            if (Anims[i]) {
                DestroyNthAnim(static_cast<BuildingAnimSlot>(i));
                // 0x0045483B reads slot 3's Powered byte, not PoweredSpecial.
                if (i == 10 && Type->IsAnimDelayedFire && Type->BuildingAnim[3].Powered)
                    PlayNthAnim(static_cast<BuildingAnimSlot>(3), !IsGreenHP(), false);
            }
        } else if (slot.PoweredEffect && Anims[i]) {
            AnimStates[i] = true;
            DestroyNthAnim(static_cast<BuildingAnimSlot>(i));
            if (i == 16) PlayNthAnim(static_cast<BuildingAnimSlot>(20), !IsGreenHP(), false);
        }
    }
    game::map_object_changed();
}
void BuildingClass::UpdatePowerAnimations() noexcept {
    const bool online = IsPowerOnline();
    if (Type->Powered && Type->PowerDrain > 0) {
        if (online)
            EnablePowerDependentAnims();
        else
            DisablePowerDependentAnims();
    }
    if (!Type->PoweredSpecial) return;
    if (online) {
        DestroyNthAnim(static_cast<BuildingAnimSlot>(19));
        for (int i = 0; i < 21; ++i)
            if (Type->BuildingAnim[i].PoweredSpecial)
                PlayNthAnim(static_cast<BuildingAnimSlot>(i), !IsGreenHP(), false);
    } else if (Owner->PowerBlackoutTimer.GetTimeLeft() || Owner->IsBeingDrained) {
        PlayNthAnim(static_cast<BuildingAnimSlot>(19), !IsGreenHP(), false);
        for (int i = 0; i < 21; ++i)
            if (Type->BuildingAnim[i].PoweredSpecial) DestroyNthAnim(static_cast<BuildingAnimSlot>(i));
    }
}
bool BuildingClass::IsBeingDrained() const { return DrainingMe != nullptr; }
int BuildingClass::FirstActiveSWIdx() const {
    const int index = Type->SuperWeapon;
    if (index == -1) return index;
    const auto* auxiliary = SuperClass::Array[index]->Type->AuxBuilding;
    return !auxiliary || Owner->OwnedBuildingTypes.GetItemCount(auxiliary->ArrayIndex) ? index : -1;
}
int BuildingClass::SecondActiveSWIdx() const {
    const int index = Type->SuperWeapon2;
    if (index == -1) return index;
    const auto* auxiliary = SuperClass::Array[index]->Type->AuxBuilding;
    return !auxiliary || Owner->OwnedBuildingTypes.GetItemCount(auxiliary->ArrayIndex) ? index : -1;
}
// 0x004555D0: the original predicate used by defense targeting and firing.
bool BuildingClass::IsPowerOnline() const {
    if (!Type || (!HasPower && Overpowerers.Count < 2) || EMPLockRemaining > 0 || Health == 0) return false;
    if (Type->Powered && Type->PowerDrain > 0 && Owner && Owner->GetPowerPercentage() < 1.0 && Overpowerers.Count < 2)
        return false;
    if (Type->PoweredSpecial && Owner && (Owner->PowerBlackoutTimer.GetTimeLeft() || Owner->IsBeingDrained))
        return false;
    const auto mission = GetCurrentMission();
    return (!Type->NeedsEngineer || HasEngineer) && mission != Mission::Construction && mission != Mission::Selling;
}
// 0x0044E7B0 / 0x0044E880, normal building and upgrade paths.
int BuildingClass::GetPowerOutput() const {
    if (!Type || IsBeingWarpedOut()) return 0;
    DWORD power = static_cast<DWORD>(Type->PowerBonus);
    if (HasExtraPowerBonus) power += static_cast<DWORD>(Type->ExtraPowerBonus);
    if ((Type->UnitAbsorb || Type->InfantryAbsorb) && Type->ExtraPowerBonus > 0 && Passengers.NumPassengers > 0)
        power += static_cast<DWORD>(Passengers.NumPassengers) * static_cast<DWORD>(Type->ExtraPowerBonus);
    if (UpgradeLevel)
        for (auto* upgrade : Upgrades)
            if (upgrade) power += static_cast<DWORD>(upgrade->PowerBonus);
    const int signedPower = std::bit_cast<int>(power);
    return signedPower > 0 && HasPower ? game::x87_integer(GetHealthPercentage() * signedPower) : 0;
}
int BuildingClass::GetPowerDrain() const {
    if (!Type || IsBeingWarpedOut() || !HasPower) return 0;
    DWORD power = static_cast<DWORD>(Type->PowerDrain);
    if (HasExtraPowerDrain) power += static_cast<DWORD>(Type->ExtraPowerDrain);
    if (UpgradeLevel)
        for (auto* upgrade : Upgrades)
            if (upgrade) power += static_cast<DWORD>(upgrade->PowerDrain);
    return std::bit_cast<int>(power);
}

void BuildingClass::UpdatePowered() {
    // 0x004549B0: power-source registration precedes controller release and animations.
    const bool online = IsPowerOnline();
    if (Type->PowersUnit) {
        if (online && !RegisteredAsPoweredUnitSource) {
            RegisteredAsPoweredUnitSource = true;
            Owner->RegisterPoweredCenter(Type->PowersUnit);
        } else if (!online && RegisteredAsPoweredUnitSource) {
            RegisteredAsPoweredUnitSource = false;
            Owner->LostPoweredCenter(Type->PowersUnit);
        }
    }
    // 0x00454B3D..0x00454B5A: these are independent of PowersUnit.
    if (!online && CaptureManager) CaptureManager->FreeAll();
    if (!online && LocomotorTarget) ReleaseLocomotor(true);
    UpdatePowerAnimations();
}
