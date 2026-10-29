#include "yrpp/HouseClass.h"
#include "yrpp/InfantryClass.h"
// Display animation lifecycle; original 0x00421EA0 / 0x00422CA0.
// Debris constructor/update additions from OpenTS 44fac744 anim.cpp.
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 OpenTS contributors; third_party/opents/LICENSE.md.
#include "yrpp/AnimClass.h"
#include "yrpp/BulletClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/DisplayClass.h"
#include "map_world.hpp"
#include <algorithm>
#include <bit>
#include <cstdint>
#include "yrpp/GameOptionsClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/VocClass.h"
#include <new>
#include <cstdlib>
namespace {
DynamicVectorClass<AnimClass*> objects;
int add_coordinate(int left, int right) { return std::bit_cast<int>(unsigned(left) + unsigned(right)); }
int subtract_coordinate(int left, int right) { return std::bit_cast<int>(unsigned(left) - unsigned(right)); }
int animation_rate(const AnimTypeClass& type, bool constructing) {
    int rate = type.Rate;
    if ((type.RandomRate.Min || type.RandomRate.Max) && (!constructing || type.RandomRate.Min <= type.RandomRate.Max)) {
        if (auto* scenario = ScenarioClass::Instance)
            rate = scenario->Random.RandomRanged(type.RandomRate.Min, type.RandomRate.Max);
    }
    return type.Normalized ? GameOptionsClass::Instance.GetAnimSpeed(rate) : rate;
}
void resolve_frames(AnimTypeClass& type) {
    if (type.End == -1) {
        auto* image = type.GetImage();
        if (image) image = image->GetData();
        if (image) type.End = image->Frames / (type.Shadow ? 2 : 1);
    }
    if (type.LoopEnd == -1) type.LoopEnd = type.End;
}
}
DynamicVectorClass<AnimClass*>& AnimClass::Array = objects;
AnimClass::AnimClass(AnimTypeClass* pAnimType, const CoordStruct& location, int loopDelay, int loopCount, DWORD flags,
                     int forceZ, bool reverse) noexcept
    // CRT initializer 0x00421E60 clears COORD_NONE before the original constructor
    // reads 0x0089A178; a synthetic nonzero fixture value is not the live value.
    : ObjectClass(), Animation{}, Type{}, OwnerObject{}, unknown_D0{0xFFFFFFFFu}, LightConvert{}, LightConvertIndex{},
      PaletteName{}, TintColor{}, ZAdjust{}, YSortAdjust{}, FlamingGuyCoords{CoordStruct::Empty}, FlamingGuyRetries{},
      IsBuildingAnim{}, UnderTemporal{}, Paused{}, Unpaused{}, PausedAnimFrame{}, Reverse{}, padding_121{}, Bounce{},
      TranslucencyLevel{}, TimeToDie{}, AttachedBullet{}, Owner{}, LoopDelay{}, Accum{}, AnimFlags{}, HasExtras{},
      RemainingIterations{}, UseCellLightConvert{}, DeleteOnMapCleanup{}, IsInert{}, IsFogged{}, FlamingGuyExpire{},
      UnableToContinue{}, SkipProcessOnce{}, Invisible{}, PowerOff{}, unused_19F{}, StartSoundAudioController{},
      StopSoundAudioController{} {
    Create_ID();
    Type = pAnimType;
    LoopDelay = loopDelay;
    AnimFlags = static_cast<BlitterFlags>(flags);
    Reverse = Type && reverse;
    ZAdjust = Type ? (forceZ ? forceZ : Type->ZAdjust) : 0;
    YSortAdjust = Type ? Type->YSortAdjust : 0;
    LightConvertIndex = -1;
    TintColor = 1000;
    Accum = 1.0;
    SkipProcessOnce = true;
    RemainingIterations = 1;
    Animation.Start(0);
    if (!Array.AddItem(this)) std::abort();
    if (Type) {
        resolve_frames(*Type);
        const auto iterations = static_cast<BYTE>(unsigned(static_cast<BYTE>(std::max(loopCount, 1))) *
                                                  unsigned(static_cast<BYTE>(Type->LoopCount)));
        RemainingIterations = std::max<BYTE>(iterations, 1);
        Animation.Value = 0;
        Animation.Start(animation_rate(*Type, true));
        if (Type->Layer == Layer::Air || Type->Layer == Layer::Top)
            SetZ(RulesClass::Instance->FlightLevel);
        else
            SetHeight(0);
        if (Reverse || Type->Reverse) {
            Animation.Value = Type->LoopEnd - 1;
            Animation.Step = -Animation.Step;
        }
        // Original construction enters the display/logic lists through base
        // Unlimbo. A detached/inactive host keeps the object in limbo as it does
        // for other ObjectClass instances; it must not fabricate map presence.
        if (!Type->IsMeteor || !ScenarioClass::Instance) ObjectClass::Unlimbo(location, DirType::North);
        // OpenTS 44fac744 anim.cpp constructor; YR 0x00422541..0x00422648.
        if ((Type->Bouncer || Type->IsMeteor) && ScenarioClass::Instance) {
            auto& random = ScenarioClass::Instance->Random;
            const auto absolute = [](int value) { return value < 0 ? 0u - unsigned(value) : unsigned(value); };
            auto at = Type->IsMeteor ? location : Location;
            Vector3D<float> velocity{};
            if (Type->IsMeteor) {
                const auto y = absolute(random.Random()), x = absolute(random.Random());
                const int span = int(Type->MinZVel);
                if (span) {
                    velocity.X = float(int(x % unsigned(std::abs(span))) - Type->MaxXYVel);
                    velocity.Y = float(int(y % unsigned(std::abs(span))) - Type->MaxXYVel);
                }
                velocity.Z = float(Type->MinZVel);
                if (velocity.X < -velocity.Y) {
                    velocity.X = -velocity.X;
                    velocity.Y = -velocity.Y;
                }
                const int time = 70 - int(absolute(random.Random() % 20));
                at = {int(at.X - time * velocity.X), int(at.Y - time * velocity.Y), int(at.Z - time * velocity.Z)};
                ObjectClass::Unlimbo(at, DirType::North);
            } else {
                at.Z += 10;
                const auto z = absolute(random.Random()), y = absolute(random.Random()), x = absolute(random.Random());
                const int span = int(Type->MaxXYVel * 2),
                          vertical = int(Type->unknown_double_320 - Type->MinZVel + 1.0);
                if (span) {
                    velocity.X = float(int(x % unsigned(std::abs(span))) - Type->MaxXYVel);
                    velocity.Y = float(int(y % unsigned(std::abs(span))) - Type->MaxXYVel);
                }
                velocity.Z = float((vertical ? int(z % unsigned(std::abs(vertical))) : 0) + Type->MinZVel);
            }
            Bounce.Initialize(at, Type->Elasticity, double(1.4f), 0, velocity, 0);
            HasExtras = true;
        }
        // 0x4226F6: delayed animations start when LoopDelay reaches zero.
        if (!LoopDelay) Start();
    } else {
        // 0x004220CB..0x00422121: retire invalid animations through the
        // deferred object lifecycle, preserving the constructor's return.
        SetOwnerObject(nullptr);
        StartSoundAudioController.EndLooping();
        ObjectClass::UnInit();
    }
}
AnimClass::~AnimClass() {
    LightningStorm::CloudsPresent.Remove(this);
    LightningStorm::CloudsManifesting.Remove(this);
    LightningStorm::BoltsPresent.Remove(this);
    // 0x004228E0 broadcasts before clearing owner/slot references. In particular,
    // Building::PointerExpired restores the idle animation when slot 8 expires.
    NotifyObjectExpired(true);
    if (OwnerObject) {
        OwnerObject->AnimPointerExpired(this);
        if (OwnerObject->WhatAmI() == AbstractType::Building) {
            auto* b = static_cast<BuildingClass*>(OwnerObject);
            for (auto& a : b->Anims)
                if (a == this) a = nullptr;
        }
    }
    SetOwnerObject(nullptr);
    if (IsOnMap) Limbo();
    // Original damage fires are free animations (OwnerObject stays null).
    // Clear the building's borrowed pointer even when a finite fire expires.
    for (auto* b : BuildingClass::Array)
        for (auto& a : b->DamageFireAnims)
            if (a == this) a = nullptr;
    // 0x00422A2F/0x00422A3C detach finite sounds and end infinite loops.
    StartSoundAudioController.EndLooping();
    StopSoundAudioController.EndLooping();
    StartSoundAudioController.~AudioController();
    StopSoundAudioController.~AudioController();
    Array.Remove(this);
}
AbstractType AnimClass::WhatAmI() const { return AbsID; }
int AnimClass::Size() const { return sizeof(*this); }
ObjectTypeClass* AnimClass::GetType() const { return Type; }
Layer AnimClass::InWhichLayer() const { return OwnerObject ? Layer::Ground : Type ? Type->Layer : Layer::Air; }
int AnimClass::GetYSort() const {
    return std::bit_cast<std::int32_t>(std::uint32_t(ObjectClass::GetYSort()) + std::uint32_t(YSortAdjust));
}
CoordStruct* AnimClass::GetCoords(CoordStruct* output) const {
    *output = Location;
    if (OwnerObject) {
        const auto at = OwnerObject->GetCoords();
        output->X = add_coordinate(output->X, at.X);
        output->Y = add_coordinate(output->Y, at.Y);
        output->Z = add_coordinate(output->Z, at.Z);
    }
    return output;
}
void AnimClass::SetOwnerObject(ObjectClass* owner) {
    // OpenTS Attach_To; YR 0x00424B50. Both detach and attach reinsert in
    // display order, including a detach/reattach to the same owner.
    auto* previous = OwnerObject;
    if (previous) {
        const bool down = IsOnMap;
        if (down) DisplayClass::Remove(this);
        bool attached = false;
        for (auto* animation : Array)
            if (animation != this && animation->OwnerObject == previous) {
                attached = true;
                break;
            }
        if (!attached) {
            previous->Extinguish();
            previous->HasParachute = false;
        }
        const auto world = GetCoords();
        OwnerObject = nullptr;
        SetLocation(world);
        if (down) DisplayClass::Submit(this);
    }
    if (owner) {
        const auto world = GetCoords();
        DisplayClass::Remove(this);
        owner->HasParachute = true;
        OwnerObject = owner;
        const auto at = owner->GetCoords();
        SetLocation({subtract_coordinate(world.X, at.X), subtract_coordinate(world.Y, at.Y),
                     subtract_coordinate(world.Z, at.Z)});
        DisplayClass::Submit(this);
    }
    IsBuildingAnim = owner && owner->WhatAmI() == AbstractType::Building;
    game::map_object_changed();
}
void AnimClass::PointerExpired(AbstractClass* object, bool removed) {
    ObjectClass::PointerExpired(object, removed);
    if (OwnerObject == object && object) {
        DisplayClass::Remove(this);
        OwnerObject->AnimPointerExpired(this);
        OwnerObject = nullptr;
        // 0x00425150 removes the borrowed layer entry before notifying its owner.
        // UnableToContinue is consumed by the later animation update.
        UnableToContinue = true;
        Mark(MarkType::Change);
        game::map_object_changed();
    }
    if (Type == object) Type = nullptr;
    if (AttachedBullet == object) {
        AttachedBullet = nullptr;
        UnInit();
    }
    if (Owner == object) {
        // 0x4251CD clears AttachedBullet, not the matching Owner field.
        AttachedBullet = nullptr;
        UnInit();
    }
}
// EA REDALERT/ANIM.CPP, revision f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae.
// Copyright 2020 Electronic Arts Inc.; third_party/ea/LICENSE.TXT.
// YR calibration: 0x00423AC0 and the 0x0042435F..0x00424931 display path.
// Start/Middle own their audio and terrain effects; continuous damage remains
// outside this native update. The completion path includes MakeInfantry.
void AnimClass::Update() {
    if (!Type || TimeToDie) return;
    if (!IsInert && Type->Report != -1) {
        const auto at = GetCoords();
        VocClass::UpdatePosition(&at, &StartSoundAudioController);
    }
    if (UnableToContinue || (OwnerObject && OwnerObject->IsDead())) {
        TimeToDie = true;
        return;
    }
    // 0x00423B0B: burning victims move/select their facing before the ordinary
    // animation gates, then use Object AI for falling off bridges and landing.
    if (Type->IsFlamingGuy) {
        FlamingGuyAI();
        if (TimeToDie) return;
        ObjectClass::Update();
        NeedsRedraw = true;
        game::map_object_changed();
    }
    if (Unpaused && PausedAnimFrame == Animation.Value) Unpaused = false;
    // OpenTS anim.cpp AI, YR 0x00423BB5: physics and trailers run before
    // SkipProcessOnce/LoopDelay. An airborne bouncer loops until its first impact.
    if (HasExtras && AnimExtras() != int(BounceClass::Status::None)) {
        const auto at = Bounce.GetCoords();
        auto& map = MapClass::Instance;
        auto* cell = map.TryGetCellAt(at);
        const bool water = cell && cell->LandType == LandType::Water;
        const bool bridge = at.Z >= map.GetCellFloorHeight(at) + CellClass::BridgeHeight;
        auto* rules = RulesClass::Instance;
        if (water && !bridge) {
            if (rules) {
                if (!Type->IsMeteor && rules->Wake) new (std::nothrow) AnimClass(rules->Wake, at);
                if (rules->SplashList.Count)
                    new (std::nothrow) AnimClass(rules->SplashList[Type->IsMeteor ? rules->SplashList.Count - 1 : 0],
                                                 {at.X, at.Y, at.Z + 3});
            }
        } else {
            if (Type->ExpireAnim) {
                new (std::nothrow) AnimClass(Type->ExpireAnim, at, 0, 1, 0x2600, -30);
                if (Type->Damage > 0 && Type->Warhead) {
                    MapClass::DamageArea(at, int(Type->Damage), nullptr, Type->Warhead, true, nullptr);
                    MapClass::FlashbangWarheadAt(int(Type->Damage), Type->Warhead, at);
                }
            }
            if (Type->Spawns && Type->SpawnCount > 0 && ScenarioClass::Instance) {
                auto& random = ScenarioClass::Instance->Random;
                const int first = random.RandomRanged(0, Type->SpawnCount);
                const int count = first + random.RandomRanged(0, Type->SpawnCount);
                for (int i = 0; i < count; ++i)
                    new (std::nothrow) AnimClass(Type->Spawns, at);
            }
        }
        TimeToDie = true;
        return;
    }
    if (IsAlive && !IsInert && Type->TrailerAnim && Type->TrailerSeperation > 0 &&
        Unsorted::CurrentFrame % Type->TrailerSeperation == 0)
        new (std::nothrow) AnimClass(Type->TrailerAnim, GetCoords(), 1);
    if (SkipProcessOnce) {
        SkipProcessOnce = false;
        return;
    }
    if (LoopDelay) {
        if (!--LoopDelay) Start();
        return;
    }
    if (!IsAlive) return;
    resolve_frames(*Type);
    // 0x00424495 invalidates the displayed frame before pause/power/timer gates.
    // A static or waiting building animation must still be marked for redraw.
    Mark(MarkType::Change);
    if (PowerOff || Paused) return;
    // 0x004244BF tests remaining time, including a paused timer's stored delay.
    // StageClass::Update treats every non-ticking timer as expired instead.
    Animation.HasChanged = Animation.Timer.GetTimeLeft() == 0 && Animation.Rate != 0;
    if (!Animation.HasChanged) return;
    Animation.Value = add_coordinate(Animation.Value, Animation.Step);
    Animation.Timer.Start(Animation.Rate);
    game::map_object_changed();
    const int stage = Animation.Value;
    if (Type->MiddleFrameIndex && stage + Type->Start == Type->MiddleFrameIndex && !HasExtras) Middle();
    if (Type->PingPong) {
        const bool turn = RemainingIterations <= 1 ? (stage >= Type->End || stage == 0)
                                                   : (stage >= Type->LoopEnd - Type->Start || stage == Type->Start);
        if (turn) {
            Animation.Step = -Animation.Step;
            return;
        }
    }
    const bool finished = stage >= (RemainingIterations > 1 ? Type->LoopEnd - Type->Start : Type->End) ||
                          ((Type->Reverse || Reverse) && stage <= 0) ||
                          (Type->Shadow && !Type->Reverse && !Reverse && stage >= Type->LoopEnd - Type->Start);
    if (!finished) return;
    if (RemainingIterations && RemainingIterations != 255) --RemainingIterations;
    if (RemainingIterations) {
        Animation.Value = (Type->Reverse || Reverse) ? Type->LoopEnd : Type->LoopStart - Type->Start;
        if ((Type->RandomLoopDelay.Min || Type->RandomLoopDelay.Max) && ScenarioClass::Instance)
            LoopDelay =
                ScenarioClass::Instance->Random.RandomRanged(Type->RandomLoopDelay.Min, Type->RandomLoopDelay.Max);
        return;
    }
    if (Type->Next) {
        Type = Type->Next;
        resolve_frames(*Type);
        UnableToContinue = false;
        Accum = 0;
        RemainingIterations = static_cast<BYTE>(Type->LoopCount);
        Animation.Start(animation_rate(*Type, false));
        Animation.Value = Type->Start;
        Start();
        return;
    }
    if (Type->MakeInfantry != -1) {
        UnmarkAllOccupationBits(Location);
        if (Type->MakeInfantry >= 0 && Type->MakeInfantry < RulesClass::Instance->AnimToInfantry.Count) {
            if (!Owner || Owner->Defeated) {
                if (auto* civilian = HouseClass::FindCivilianSide()) Owner = civilian;
            }
            if (Owner) {
                auto* type = RulesClass::Instance->AnimToInfantry[Type->MakeInfantry];
                auto* infantry = static_cast<InfantryClass*>(InfantryTypeClass::Array[type->ArrayIndex]->CreateObject(Owner));
                if (!infantry || !infantry->Unlimbo(Location, DirType(96))) {
                    --Animation.Value;
                    return;
                }
                auto* cell = MapClass::Instance.GetCellAt(GetMapCoords());
                if (cell && cell->ContainsBridge() && Location.Z > cell->GetCoords().Z) {
                    infantry->Mark(MarkType::Up);
                    infantry->OnBridge = true;
                    infantry->Mark(MarkType::Down);
                }
                if (!Owner->IsHumanPlayer) infantry->QueueMission(Mission::Hunt, false);
            }
        }
    }
    TimeToDie = true;
}
