// Building animation management and native combat/factory update admission.
// Originals: 0x004509D0, 0x0043EF90, 0x00451750, 0x00451890, 0x00451E40.
#include "yrpp/BuildingClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/LightSourceClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/AirstrikeClass.h"
#include "yrpp/SuperClass.h"
#include "x87_integer.hpp"
#include "yrpp/VocClass.h"
#include "yrpp/VoxClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/TacticalClass.h"
#include "map_world.hpp"
#include "type_resources.hpp"
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <new>
namespace {
const char* animation_name(const BuildingAnimStruct& s, bool damaged, bool garrisoned) {
    return damaged ? s.Damaged : garrisoned ? s.Garrisoned : s.Anim;
}
bool usable(const char* id) { return id && *id && _strcmpi(id, "none") && _strcmpi(id, "<none>"); }
CoordStruct animation_position(const BuildingClass& building, int slot) {
    // Slot offsets are converted before querying the building render position.
    CoordStruct delta{};
    TacticalClass::Instance->PixelToCoordsAbsolute(&delta, building.Type->BuildingAnim[slot].Position);
    auto at = building.GetRenderCoords();
    at.X += delta.X;
    at.Y += delta.Y;
    at.Z += delta.Z;
    return at;
}
bool animation_power_off(const BuildingClass& building, const BuildingAnimStruct& slot) {
    // 0x00452480 handles manual shutdown; 0x004549B0/0x004547C0 handles
    // automatic power loss only for Powered buildings with positive drain.
    const bool online =
        building.Type->Powered && building.Type->PowerDrain > 0 ? building.IsPowerOnline() : building.HasPower;
    return slot.Powered && (!building.StuffEnabled || !online);
}
void update_turret_frame(BuildingClass& building) {
    if (!building.Type || !building.Type->Turret || building.Type->TurretAnimIsVoxel) return;
    auto* anim = building.Anims[9];
    if (!anim) return;
    // 0x0045125E reads PrimaryFacing at +0x388; table VA 0x007F4890.
    constexpr int frames[32] = {28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13,
                                12, 11, 10, 9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  31, 30, 29};
    const unsigned facing = ((unsigned(building.PrimaryFacing.Current().Raw) >> 10) + 1) >> 1;
    const int frame = frames[facing & 31u];
    if (anim->Animation.Value != frame) {
        anim->Animation.Value = frame;
        building.NeedsRedraw = true;
        game::map_object_changed();
    }
    // The facing frame is absolute, NOT AnimType.Start + facing, and must not
    // advance during AnimClass::Update, which runs after BuildingClass::Update.
    anim->Animation.Step = 0;
}

}
SHPStruct* BuildingClass::GetImage() const {
    if (!Type) return nullptr;
    // 0x004513D0: unplaced/construction objects use the buildup image.
    return BState != static_cast<int>(BStateType::Construction) && ActuallyPlacedOnMap ? Type->GetImage()
                                                                                       : Type->LoadBuildup();
}
int BuildingClass::GetCurrentFrame() {
    if (!Type) return 0;
    // 0x0043EF90: gates reverse their buildup; selling reverses it again.
    if (Type->LaserFence) return LaserFenceFrame;
    if (Type->FirestormWall) return FirestormWallFrame;
    if (BState == static_cast<int>(BStateType::Construction)) {
        int frame = Animation.Value;
        const auto& sequence = Type->BuildingAnimFrame[0];
        const int end = int(sequence.dwUnknown) + sequence.FrameCount - 1;
        if (Type->Gate) frame = end - frame;
        if (GetCurrentMission() == Mission::Selling) frame = end - frame;
        return frame;
    }
    if (Type->CanBeOccupied) {
        // 0x0043EF90: civilian occupiable buildings use ConditionRed; positive
        // TechLevel also enables the ordinary ConditionYellow damage threshold.
        int frame = GetOccupantCount() > 0 ? 2 : 0;
        if (IsRedHP() || (Type->TechLevel > 0 && !IsGreenHP())) ++frame;
        return Type->TechLevel == -1 && frame == 3 ? 1 : frame;
    }
    if (Type->Gate) return IsGreenHP() ? 0 : Type->GateStages + 1;
    int frame = Animation.Value;
    if (!IsGreenHP()) {
        if (BState == static_cast<int>(BStateType::Idle))
            ++frame;
        else {
            int offset = 0;
            for (int i : {1, 2, 4, 5})
                offset =
                    std::max(offset, int(Type->BuildingAnimFrame[i].dwUnknown) + Type->BuildingAnimFrame[i].FrameCount);
            frame += offset;
        }
    }
    return frame;
}
int BuildingClass::GetShapeNumber() const { return const_cast<BuildingClass*>(this)->GetCurrentFrame(); }
void BuildingClass::DestroyNthAnim(BuildingAnimSlot slot) {
    const int index = static_cast<int>(slot);
    if (slot != BuildingAnimSlot::All && (index < 0 || index >= 21)) return;
    const int first = slot == BuildingAnimSlot::All ? 0 : index;
    const int last = slot == BuildingAnimSlot::All ? 21 : index + 1;
    for (int i = first; i < last; ++i) {
        auto* anim = Anims[i];
        if (!anim) continue;
        Anims[i] = nullptr;
        delete anim;
        game::map_object_changed();
    }
}
void BuildingClass::PlayAnim(const char* name, BuildingAnimSlot slot, bool damaged, bool, int delay) {
    int i = static_cast<int>(slot);
    if (i < 0 || i >= 21 || !Type) return;
    // Damage synchronization precedes name lookup, including unknown names.
    if (IsDamaged != damaged) {
        IsDamaged = damaged;
        for (int active = 0; active < 21; ++active)
            if (Anims[active]) PlayNthAnim(static_cast<BuildingAnimSlot>(active), damaged, false, 0);
    }
    auto* t = usable(name) ? AnimTypeClass::Find(name) : nullptr;
    // 0x451890 leaves the current slot intact when the animation name is unknown.
    if (!t) return;
    // The original allocates the replacement before releasing the current slot.
    // Allocation failure is fatal there; never propagate it through this boundary.
    auto* anim = new (std::nothrow) AnimClass(t, animation_position(*this, i), delay, 1, 0x1600, 0, false);
    if (!anim) std::abort();
    const auto& config = Type->BuildingAnim[i];
    anim->ZAdjust = config.ZAdjust;
    anim->YSortAdjust = config.YSort;
    anim->IsBuildingAnim = true;
    if (IsFogged) anim->IsFogged = true;
    char translucency = Translucency;
    if (translucency == 15 && VisualCharacter(false, nullptr) == VisualType::Hidden) translucency = 16;
    for (auto* existing : Anims)
        if (existing) existing->TranslucencyLevel = translucency;
    if (auto* previous = Anims[i]) {
        anim->Animation.Value = previous->Animation.Value;
        Anims[i] = nullptr;
        delete previous;
    }
    Anims[i] = anim;
    if ((!StuffEnabled || !IsPowerOnline()) && Type->Powered && config.Powered) anim->PowerOff = true;
    if (t->ShouldUseCellDrawer) {
        anim->LightConvert = GetRemapColour();
        int intensity = GetFlashingIntensity(static_cast<short>(GetCell()->Intensity_Normal));
        if (IsIronCurtained() || (Airstrike && Airstrike->Target == this))
            intensity = GetEffectTintIntensity(intensity);
        anim->TintColor = intensity;
    }
    // The slot-9 check reads the barrel voxel flag at type offset 0x16C6.
    if (slot == BuildingAnimSlot::Turret && Type->BarrelAnimIsVoxel) anim->Invisible = true;
    if (IsBeingWarpedOut()) anim->Paused = true;
    game::map_object_changed();
}
void BuildingClass::PlayNthAnim(BuildingAnimSlot slot, bool damaged, bool garrisoned, int delay) {
    int i = static_cast<int>(slot);
    if (i < 0 || i >= 21 || !Type) return;
    const auto* name = animation_name(Type->BuildingAnim[i], damaged, garrisoned);
    // 0x451750 does not enter PlayAnim for an empty configured name.
    if (name && *name) PlayAnim(name, slot, damaged, garrisoned, delay);
}
void BuildingClass::SetAnimCoords() {
    if (!Type) return;
    const auto origin = GetRenderCoords();
    for (int i = 0; i < 21; ++i)
        if (auto* anim = Anims[i]) {
            CoordStruct delta{};
            TacticalClass::Instance->PixelToCoordsAbsolute(&delta, Type->BuildingAnim[i].Position);
            anim->SetLocation({origin.X + delta.X, origin.Y + delta.Y, origin.Z + delta.Z});
        }
}
void BuildingClass::SynchronizePowerAnimations() noexcept { SynchronizePowerAnimations(true); }
void BuildingClass::InitializePowerAnimations() noexcept { SynchronizePowerAnimations(false); }
void BuildingClass::SynchronizePowerAnimations(bool updatePowered) noexcept {
    // Preserve one visual power pass for both loading and runtime transitions.
    // ReadINI must not register centers before the first automatic power update.
    if (LightSource) {
        if (StuffEnabled && !IsDead() && IsOnMap)
            LightSource->Activate();
        else
            LightSource->Deactivate();
    }
    if (!Type) return;
    if (IsDead() || !IsOnMap) {
        DestroyNthAnim(BuildingAnimSlot::All);
        return;
    }
    ToggleDamagedAnims(!IsGreenHP());
    const bool online = IsPowerOnline();
    const bool power_loss = Type->Powered && Type->PowerDrain > 0 && !online;
    const bool low_power =
        Type->PoweredSpecial && Owner && !online && (Owner->PowerBlackoutTimer.GetTimeLeft() || Owner->IsBeingDrained);
    if (!Type->Turret || Type->TurretAnimIsVoxel) DestroyNthAnim(BuildingAnimSlot::Turret);
    for (int i : {18, 3, 4, 5, 6, 9}) {
        if (Anims[i] || (i == 9 && (!Type->Turret || Type->TurretAnimIsVoxel))) continue;
        const auto& slot = Type->BuildingAnim[i];
        if (low_power && slot.PoweredSpecial) continue;
        if (power_loss && !slot.Powered && (slot.PoweredLight || slot.PoweredEffect)) continue;
        PlayNthAnim(static_cast<BuildingAnimSlot>(i), !IsGreenHP(), GetOccupantCount() > 0);
    }
    if (updatePowered)
        UpdatePowered();
    else
        UpdatePowerAnimations();
    // Manual shutdown also applies to powered slots on non-Powered buildings.
    for (int i = 0; i < 21; ++i)
        if (auto* anim = Anims[i]; anim && Type->BuildingAnim[i].Powered)
            anim->PowerOff = animation_power_off(*this, Type->BuildingAnim[i]);
    SetAnimCoords();
    update_turret_frame(*this);
    game::map_object_changed();
}
void BuildingClass::UpdateAnimations() {
    if (!Type) return;
    const bool stage_changed = Animation.Timer.GetTimeLeft() == 0 && Animation.Rate != 0;
    Animation.HasChanged = stage_changed;
    if (stage_changed) {
        Animation.Value += Animation.Step;
        Animation.Timer.Start(Animation.Rate);
    }
    // 0x450A3C refreshes the body and all auxiliary animation colors every tick.
    UpdateAnimAppearance();
    // 0x450A86 switches repair and passenger power animations.
    if (Type->UnitRepair && GetCurrentMission() != Mission::Repair && (Anims[8] || Anims[11]) && !Type->Naval) {
        PlayNthAnim(static_cast<BuildingAnimSlot>(12), !IsGreenHP(), false);
        DestroyNthAnim(static_cast<BuildingAnimSlot>(8));
        DestroyNthAnim(static_cast<BuildingAnimSlot>(11));
    }
    if (Type->ExtraPowerBonus > 0 && Type->InfantryAbsorb && ActuallyPlacedOnMap && BState) {
        const int active = Passengers.GetTotalSize() > 0 ? 4 : 3;
        const int inactive = active == 4 ? 3 : 4;
        if (Anims[inactive]) DestroyNthAnim(static_cast<BuildingAnimSlot>(inactive));
        if (!Anims[active]) {
            const bool damaged = !IsGreenHP();
            const bool occupied = GetOccupantCount() > 0;
            PlayNthAnim(static_cast<BuildingAnimSlot>(active), damaged, occupied);
        }
    }
    if (Type->SiloDamage) {
        int frame = 0;
        if (Type->Storage > 0) {
            const int amount = game::x87_integer(Tiberium.GetTotalAmount());
            const int scaled = std::bit_cast<int>(static_cast<unsigned>(amount) * 4u);
            frame = std::clamp(game::x87_integer(double(scaled) / Type->Storage + 0.5), 0, 3);
        }
        if (frame) {
            if (!Anims[10]) PlayNthAnim(static_cast<BuildingAnimSlot>(10), !IsGreenHP(), false);
            Anims[10]->Animation.Value = frame;
        } else if (Anims[10])
            DestroyNthAnim(static_cast<BuildingAnimSlot>(10));
    }
    if (Type->Refinery) {
        const auto storage_stage = [this]() {
            if (!game::x87_integer(Tiberium.GetTotalAmount())) return 0;
            const int amount = game::x87_integer(Tiberium.GetTotalAmount());
            return std::bit_cast<int>(static_cast<unsigned>(amount) * 4u) / Type->Storage;
        };
        const int stage = storage_stage();
        if (int(StorageFilledSlots) != stage) {
            const int previous = int(StorageFilledSlots);
            if (previous >= 0) DestroyNthAnim(static_cast<BuildingAnimSlot>(3 + std::min(previous, 3)));
            StorageFilledSlots = storage_stage();
            if (int(StorageFilledSlots) >= 0)
                PlayNthAnim(static_cast<BuildingAnimSlot>(3 + std::min(int(StorageFilledSlots), 3)), !IsGreenHP(),
                            false);
        }
    }
    if (Type->SuperWeapon != -1 && GetCurrentMission() != Mission::Construction &&
        GetCurrentMission() != Mission::Selling && Type->ChargedAnimTime <= 990.0f) {
        for (int i = 0; i < Owner->Supers.Count; ++i) {
            auto* super = Owner->Supers[i];
            if (static_cast<int>(super->Type->Type) != Type->SuperWeapon) continue;
            // The factor is the exact single-precision constant at 0x7E44C0.
            if (double(super->RechargeTimer.GetTimeLeft()) * 0x1.234568p-10f > Type->ChargedAnimTime) {
                if (Anims[16]) {
                    DestroyNthAnim(static_cast<BuildingAnimSlot>(16));
                    PlayNthAnim(static_cast<BuildingAnimSlot>(17), !IsGreenHP(), false);
                }
            } else if (Anims[14]) {
                DestroyNthAnim(static_cast<BuildingAnimSlot>(14));
                PlayNthAnim(static_cast<BuildingAnimSlot>(15), !IsGreenHP(), false);
            }
        }
    }
    bool restart = false;
    if (!HasTurret() || GetCurrentMission() == Mission::Construction || GetCurrentMission() == Mission::Selling) {
        if (stage_changed && BState >= 0 && BState < 6) {
            const auto& sequence = Type->BuildingAnimFrame[BState];
            const int start = int(sequence.dwUnknown);
            if (Animation.Value == start + sequence.FrameCount - 1 ||
                (!ArchiveTarget && Type->UndeploysInto && GetCurrentMission() == Mission::Selling &&
                 Animation.Value == 23))
                IsReadyToCommence = true;
            restart = Animation.Value >= start + sequence.FrameCount;
            NeedsRedraw = true;
        } else if (BState == -1 || !Animation.Rate)
            IsReadyToCommence = true;
    }
    if (IsIronCurtained()) NeedsRedraw = true;
    if (HasTurret() && HasTurret() && Anims[9]) {
        constexpr int frames[32] = {28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13,
                                    12, 11, 10, 9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  31, 30, 29};
        const unsigned facing = ((unsigned(PrimaryFacing.Current().Raw) >> 10) + 1) >> 1;
        Anims[9]->Animation.Value = frames[facing & 31u];
        Anims[9]->Animation.Step = 0;
    }
    if (restart) {
        const auto& sequence = Type->BuildingAnimFrame[BState];
        Animation.Start(BState < 2 ? GameOptionsClass::Instance.GetAnimSpeed(sequence.FrameDuration)
                                   : sequence.FrameDuration);
        Animation.Value = int(sequence.dwUnknown);
        Mark(MarkType::Change);
    }
}
void BuildingClass::ToggleDamagedAnims(bool damaged) {
    if (IsDamaged == damaged) return;
    IsDamaged = damaged;
    for (int i = 0; i < 21; ++i)
        if (Anims[i]) PlayNthAnim(static_cast<BuildingAnimSlot>(i), damaged, false, 0);
    game::map_object_changed();
}
// Begin_Mode and body stage progression adapt OpenTS building.cpp at pinned
// 44fac744f70235e0d5ddca107364a68f95132ce9, calibrated to 0x00447780,
// 0x004509D0 and 0x0043FB20. EA/OpenTS copyrights and GPL terms retained in
// code/third_party/opents/LICENSE.md. Local adaptations: OpenRA2, 2026.
void BuildingClass::BeginMode(BStateType mode) {
    const int state = static_cast<int>(mode);
    if (!Type || state < 0 || state >= 6) return;
    QueueBState = state;
    if (BState == static_cast<int>(BStateType::None) || state == 0 || Unsorted::ScenarioInit) {
        BState = state;
        QueueBState = static_cast<int>(BStateType::None);
        // 0x447780 starts ambient slots before resetting the body stage timer.
        for (int slot : {18, 3, 4, 5, 6}) {
            if (!Unsorted::ScenarioInit) break;
            PlayNthAnim(static_cast<BuildingAnimSlot>(slot), !IsGreenHP(), GetOccupantCount() > 0);
        }
        const auto& sequence = Type->BuildingAnimFrame[state];
        int rate = sequence.FrameDuration;
        if (Type->Normalized && state != 0) rate = GameOptionsClass::Instance.GetAnimSpeed(rate);
        Animation.Start(rate);
        Animation.Value = int(sequence.dwUnknown);
        game::map_object_changed();
    }
}
bool BuildingClass::HasTurret() const {
    if (Type && Type->Turret) return true;
    for (int i = 0; i < std::min(int(UpgradeLevel), 3); ++i)
        if (Upgrades[i] && Upgrades[i]->Turret) return true;
    return false;
}
// OpenTS 44fac744 building.cpp Clicked_As_Target / Update_Anim_Appearance /
// Apparent_Brightness; YR 0x456E00 / 0x452000 / 0x456F80.
void BuildingClass::Flash(int duration) {
    if (duration) NeedsRedraw = true;
    TechnoClass::Flash(duration);
    UpdateAnimAppearance();
    game::map_object_changed();
}
int BuildingClass::GetFlashingIntensity(int intensity) const {
    if ((Flashing.DurationRemaining & 2) == 2) return intensity > 1500 ? intensity - 500 : intensity + 500;
    return intensity;
}
void BuildingClass::UpdateAnimAppearance() {
    auto* converter = GetRemapColour();
    int intensity = GetFlashingIntensity(static_cast<short>(GetCell()->Intensity_Normal));
    if (IsIronCurtained() || (Airstrike && Airstrike->Target == (WhatAmI() == AbstractType::Building ? this : nullptr)))
        intensity = GetEffectTintIntensity(intensity);
    unknown_short_700 = static_cast<short>(intensity);
    const auto translucency = [&] {
        char level = Translucency;
        if (level == 15 && VisualCharacter(false, nullptr) == VisualType::Hidden) level = 16;
        for (auto* anim : Anims)
            if (anim) anim->TranslucencyLevel = level;
    };
    for (auto* anim : Anims)
        if (anim) {
            if (anim->Type && anim->Type->ShouldUseCellDrawer) {
                anim->LightConvert = converter;
                anim->TintColor = static_cast<unsigned short>(intensity);
            }
            translucency();
        }
    translucency();
}

void BuildingClass::Update() {
    // 0x0043FB20..0x0043FBEF: power transitions precede animation/mission AI;
    // the previous state is still visible while the transition is dispatched.
    if (Type && IsAlive && IsOnMap) {
        const bool online = IsPowerOnline();
        if (WasOnline != online) {
            UpdatePowered();
            WasOnline = online;
            game::map_object_changed();
        }
    }
    UpdateDamageFires();
    if (!Type || !IsAlive || !IsOnMap) return;
    // 0x0043FD08..0x0043FDD6: restart before checking ownership and online state.
    if (!IsBeingWarpedOut() && !IsWarpingIn() && CashProductionTimer.GetTimeLeft() == 1) {
        CashProductionTimer.Start(Type->ProduceCashDelay);
        if (Owner && !Owner->Type->MultiplayPassive && IsPowerOnline()) {
            const int amount = Type->ProduceCashAmount;
            if (amount > 0)
                Owner->GiveMoney(amount);
            else
                Owner->TakeMoney(std::bit_cast<int>(0u - static_cast<unsigned>(amount)));
        }
    }
    // 0x0044003F..0x0044006C compares Health with the building's own cached
    // health at 0x544, independently of the generic EstimatedHealth field.
    if (unknown_544 != Health) {
        if (Owner) Owner->RecheckPower = Owner->RecheckRadar = true;
        unknown_544 = Health;
    }
    // 0x43FE72: the victim owns temporal progress, including unarmed buildings.
    if (TemporalTargetingMe) TemporalTargetingMe->Update();
    if (InLimbo || !IsAlive) return;
    if (Health <= 0) {
        if (!C4Timer.GetTimeLeft()) {
            Limbo();
            LeaveRubble();
            IsAlive = false;
            ActuallyPlacedOnMap = false;
            game::map_object_changed();
            delete this;
        }
        return;
    }
    UpdateAnimations();
    if (Animation.HasChanged) game::map_object_changed();
    // 0x43FB20: every building runs the inherited update, including
    // unarmed buildings. The base owns flashing, cloak and mission dispatch.
    if (ReadyToNextMission() && BState && NextMission()) IsReadyToCommence = false;
    TechnoClass::Update();
    if (!IsAlive) return;

    ToggleDamagedAnims(!IsGreenHP());
    // 0x43FFA3: the post-mission transition also admits BState == 0.
    if (IsReadyToCommence && NextMission()) IsReadyToCommence = false;
    if (QueueBState >= 0 && QueueBState < 6) {
        if (BState != QueueBState) {
            BState = QueueBState;
            const auto& sequence = Type->BuildingAnimFrame[BState];
            Animation.Start(BState < 2 ? GameOptionsClass::Instance.GetAnimSpeed(sequence.FrameDuration)
                                       : sequence.FrameDuration);
            Animation.Value = int(sequence.dwUnknown);
            NeedsRedraw = true;
            game::map_object_changed();
        }
        QueueBState = -1;
    }
    if (UnloadTimer.State1) {
        if (UnloadTimer.IsTimerFinished()) UnloadTimer.SetToDone();
        NeedsRedraw = true;
        game::map_object_changed();
    }
    // 0x4400F4..0x4401D2: inherited mission AI runs before delayed fire,
    // overpower, garrison and repair. Repair must observe this frame's mission
    // transition (including Selling), not the state before TechnoClass::Update.
    UpdateDelayedFire();
    UpdateOverpowerers();
    if (Type->CanBeOccupied) UpdateGarrison();
    UpdateRepair();
    if (Type->Factory != AbstractType::None) FactoryAI();
    // 0x4401D2..0x440378: delayed warhead deaths use the existing C4 timer.
    // Bridge repair huts have a separate bridge-destruction branch.
    if (C4Applied && !C4Timer.GetTimeLeft() && !Type->BridgeRepairHut) {
        int damage = Health;
        ReceiveDamage(&damage, 0, RulesClass::Instance->C4Warhead, C4AppliedBy, true, false, nullptr);
        if (!IsAlive) return;
        Mark(MarkType::Change);
    }
    EstimatedHealth = Health;
}
