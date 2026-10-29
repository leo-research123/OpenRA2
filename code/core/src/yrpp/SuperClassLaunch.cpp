#include "yrpp/SuperClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/InfantryTypeClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/VoxClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/BulletClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/RadarEventClass.h"
#include "yrpp/VocClass.h"
#include "x87_integer.hpp"
#include "yrpp/LocomotionClass.h"
#include "yrpp/ParasiteClass.h"
#include "yrpp/UnitClass.h"
#include <cstdlib>
#include <bit>

namespace {
// Initialized by the original 0x006CAE00 constructor, center then clockwise.
constexpr CellStruct effect_cells[]{{0, 0}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}, {0, 1}, {1, 1}};
int wrapped_add(int a, int b) { return std::bit_cast<int>(unsigned(a) + unsigned(b)); }
CoordStruct surface_coords(const CellStruct& cell) {
    auto* tile = MapClass::Instance.GetCellAt(cell);
    auto at = tile->GetCoords();
    if (tile->ContainsBridge()) at.Z = wrapped_add(at.Z, CellClass::BridgeHeight);
    return at;
}
AnimClass* invoke_animation(AnimTypeClass* type, CoordStruct at) {
    auto* memory = YRMemory::Allocate(sizeof(AnimClass));
    return memory ? ::new (memory) AnimClass(type, at, 0, 1, 0x600, 0, false) : nullptr;
}
void announce(const char* key) {
    if (!Unsorted::MuteSWLaunches) VoxClass::Play(key, -1, -1);
}
void end_player_click(bool player, const char* ready) {
    if (!player) return;
    Unsorted::CurrentSWType = -1;
    VoxClass::SilenceIndex(VoxClass::FindIndex(ready));
}

void chrono_warp(SuperClass& super, const CellStruct& destination, bool player) {
    auto& map = MapClass::Instance;
    RadarEventClass::Create(RadarEventType::SuperweaponActivated, super.ChronoMapCoords);
    RadarEventClass::Create(RadarEventType::SuperweaponActivated, destination);
    auto to = surface_coords(destination);
    auto from = surface_coords(super.ChronoMapCoords);
    if (super.Animation) {
        super.Animation->RemainingIterations = 0;
        super.Animation = nullptr;
        SuperClass::AnimExpirationListeners.Remove(&super);
    }
    if (super.AnimationGotInvalid) {
        SuperClass::AnimExpirationListeners.Remove(&super);
        super.AnimationGotInvalid = false;
    }
    to.Z += 5;
    from.Z += 5;
    invoke_animation(RulesClass::Instance->ChronoBlastDest, to);
    invoke_animation(RulesClass::Instance->ChronoBlast, from);
    for (auto offset : effect_cells) {
        const CellStruct source{short(super.ChronoMapCoords.X + offset.X), short(super.ChronoMapCoords.Y + offset.Y)};
        const CellStruct target{short(destination.X + offset.X), short(destination.Y + offset.Y)};
        for (auto* object = map.GetCellAt(source)->GetContent(); object; object = object->NextObject) {
            if (!(unsigned(object->AbstractFlags) & 4u) || object->IsInAir()) continue;
            auto* foot = static_cast<FootClass*>(object);
            if (auto* parasite = foot->ParasiteEatingMe;
                parasite && parasite->GetTechnoType()->Naval && parasite->ParasiteImUsing) {
                parasite->ParasiteImUsing->SuppressionTimer.Start(500);
                parasite->ParasiteImUsing->ExitUnit();
            }
            const bool unit = foot->WhatAmI() == AbstractType::Unit;
            bool insideFactory = false;
            if (unit && foot->RadioLinks.Capacity > 0)
                if (auto* link = foot->GetNthLink(); link && link->WhatAmI() == AbstractType::Building &&
                                                     static_cast<BuildingClass*>(link)->Type->WeaponsFactory)
                    insideFactory = map.GetCellAt(foot->Location)->GetBuilding() == link;
            auto* type = foot->GetTechnoType();
            if (type->Organic && !type->Teleporter && !foot->IsInAir()) {
                int damage = type->Strength;
                foot->ReceiveDamage(&damage, 0, RulesClass::Instance->C4Warhead, nullptr, true, false, super.Owner);
                continue;
            }
            if (foot->IsIronCurtained() || foot->IsImmobilized || foot->IsBeingWarpedOut() || foot->IsWarpingIn() ||
                insideFactory)
                continue;
            if (foot->BunkerLinkedItem && foot->BunkerLinkedItem->WhatAmI() == AbstractType::Building)
                static_cast<BuildingClass*>(foot->BunkerLinkedItem)->ClearBunker();
            ILocomotion* next = nullptr;
            IPiggyback* piggy = nullptr;
            const auto check = [](HRESULT result) {
                if (result < 0 && result != static_cast<HRESULT>(0x80004002u)) std::abort();
            };
            check(LocomotionClass::CreateInstance(&next, &LocomotionClass::CLSIDs::Teleport, nullptr, 7));
            auto position = surface_coords(target);
            if (unit) {
                if (!foot->Locomotor) std::abort();
                foot->Locomotor->Mark_All_Occupation_Bits(MarkType::Up);
                foot->Locomotor->Force_Track(-1, position);
                foot->UnmarkAllOccupationBits(position);
                foot->FrozenStill = true;
            }
            if (!next) std::abort();
            next->Link_To_Object(foot);
            check(LocomotionClass::TryPiggyback(&piggy, &next));
            if (piggy) {
                piggy->Begin_Piggyback(foot->Locomotor);
#if defined(_MSC_VER)
                foot->Locomotor = next;
#else
                if (foot->Locomotor != next) {
                    auto* old = foot->Locomotor;
                    foot->Locomotor = next;
                    next->AddRef();
                    if (old) old->Release();
                }
#endif
            }
            const auto origin = map.GetCellAt(super.ChronoMapCoords)->GetCoords();
            const auto current = foot->GetCoords();
            if (!unit) {
                const auto landing = map.GetCellAt(destination)->GetCoords();
                position = {wrapped_add(landing.X, std::bit_cast<int>(unsigned(current.X) - unsigned(origin.X))),
                            wrapped_add(landing.Y, std::bit_cast<int>(unsigned(current.Y) - unsigned(origin.Y))),
                            wrapped_add(landing.Z, std::bit_cast<int>(unsigned(current.Z) - unsigned(origin.Z)))};
                if (map.GetCellAt(position)->ContainsBridge()) position.Z += CellClass::BridgeHeight;
            }
            foot->IsImmobilized = true;
            foot->ChronoDestCoords = position;
            foot->SendToEachLink(RadioCommand(3));
            foot->ChronoWarpedByHouse = super.Owner;
            foot->SetDestination(map.GetCellAt(target), true);
            if (next) next->Release();
            if (piggy) piggy->Release();
        }
    }
    announce("EVA_ChronosphereActivated");
    end_player_click(player, "EVA_ChronosphereReady");
}
void launch_paradrop(SuperClass& super, int aircraft, CellClass* target,
                     TypeList<InfantryTypeClass*> RulesClass::* types, TypeList<int> RulesClass::* counts,
                     bool checkCounts) {
    auto* rules = RulesClass::Instance;
    if (checkCounts && (rules->*types).Count != (rules->*counts).Count) return;
    for (int i = 0; i < (rules->*types).Count; ++i) {
        const int infantry = (rules->*types)[i]->ArrayIndex;
        if (aircraft != -1 && infantry != -1) {
            super.Owner->SendParadropPlanes(aircraft, 1, Mission::ParadropApproach, target, nullptr, infantry,
                                            (rules->*counts)[i]);
            rules = RulesClass::Instance;
        }
    }
}
}

void SuperClass::Launch(const CellStruct& cell, bool isPlayer) {
    const auto kind = Type->Type;
    // ChronoWarp is the original post-click exception to the readiness guard.
    if (kind != SuperWeaponType::ChronoWarp && !IsReady) return;
    switch (kind) {
        case SuperWeaponType::ParaDrop:
        case SuperWeaponType::AmerParaDrop: {
            const int aircraft = AircraftTypeClass::FindIndex("PDPLANE");
            auto* target = MapClass::Instance.GetCellAt(cell);
            if (target && target != &MapClass::InvalidCell) {
                if (target->Tile_Is_Water()) {
                    const CellStruct empty{0, 0};
                    CellStruct nearby;
                    MapClass::Instance.NearByLocation(nearby, cell, SpeedType::Foot, -1, MovementZone::Normal, false, 1,
                                                      1, false, false, false, true, empty, false, false);
                    if (nearby != CellStruct::Empty) {
                        auto* alternative = MapClass::Instance.GetCellAt(nearby);
                        if (alternative && alternative != &MapClass::InvalidCell && !alternative->Tile_Is_Water())
                            target = alternative;
                    }
                }
                if (kind == SuperWeaponType::AmerParaDrop)
                    launch_paradrop(*this, aircraft, target, &RulesClass::AmerParaDropInf, &RulesClass::AmerParaDropNum,
                                    true);
                else if (Owner->SideIndex == 0)
                    launch_paradrop(*this, aircraft, target, &RulesClass::AllyParaDropInf, &RulesClass::AllyParaDropNum,
                                    true);
                else if (Owner->SideIndex == 2)
                    launch_paradrop(*this, aircraft, target, &RulesClass::YuriParaDropInf, &RulesClass::YuriParaDropNum,
                                    true);
                else
                    // The original Soviet branch does not compare list lengths.
                    launch_paradrop(*this, aircraft, target, &RulesClass::SovParaDropInf, &RulesClass::SovParaDropNum,
                                    false);
            }
            if (isPlayer) {
                Unsorted::CurrentSWType = -1;
                VoxClass::SilenceIndex(VoxClass::FindIndex("EVA_ReinforcementsReady"));
            }
            break;
        }
        case SuperWeaponType::SpyPlane: {
            const int aircraft = AircraftTypeClass::FindIndex("SPYP");
            auto* target = MapClass::Instance.GetCellAt(cell);
            if (target && target != &MapClass::InvalidCell) {
                auto* rules = RulesClass::Instance;
                if (rules->AllyParaDropInf.Count == rules->AllyParaDropNum.Count)
                    for (int i = 0; i < rules->AllyParaDropInf.Count; ++i)
                        if (aircraft != -1) {
                            Owner->SendSpyPlanes(aircraft, 1, Mission::SpyplaneApproach, target, nullptr);
                            rules = RulesClass::Instance;
                        }
            }
            if (isPlayer) {
                Unsorted::CurrentSWType = -1;
                VoxClass::SilenceIndex(VoxClass::FindIndex("EVA_SpyPlaneReady"));
            }
            break;
        }
        case SuperWeaponType::Nuke: {
            if (IsOneTime && IsPresent) {
                CoordStruct at{int(cell.X) * 256 + 128, int(cell.Y) * 256 + 128, 0};
                at.Z = MapClass::Instance.GetCellFloorHeight(at);
                if (MapClass::Instance.GetCellAt(cell)->ContainsBridge()) at.Z += CellClass::BridgeHeight;
                auto* weapon = WeaponTypeClass::Find("NukePayload");
                auto* target = MapClass::Instance.GetCellAt(at);
                auto* bullet = weapon->Projectile->CreateBullet(target, nullptr, weapon->Damage, weapon->Warhead,
                                                                weapon->Speed, false);
                if (bullet) bullet->SetWeaponType(weapon);
                char warning[] = "PSIWARN";
                Owner->PsiWarn(target, bullet, warning);
                if (bullet) {
                    auto launch = at;
                    launch.Z += 20000;
                    constexpr double angle = 1.570748388432313;
                    const double sine = Math::sin(angle);
                    const double y = Math::cos(angle) * sine * -100.0;
                    const double cosine = Math::cos(angle);
                    const double x = Math::cos(angle) * cosine * -100.0;
                    bullet->MoveTo(launch, BulletVelocity{x, y, Math::sin(angle) * -100.0});
                    announce("EVA_NuclearMissileLaunched");
                    VocClass::PlayIndexAtPos(RulesClass::Instance->DigSound, at);
                }
            } else {
                for (int i = 0; i < BuildingTypeClass::Array.Count; ++i) {
                    auto* type = BuildingTypeClass::Array[i];
                    if (!type->NukeSilo ||
                        (type->SuperWeapon != Type->ArrayIndex && type->SuperWeapon2 != Type->ArrayIndex))
                        continue;
                    if (auto* silo = Owner->FindBuildingOfType(i, -1)) {
                        CoordStruct at{int(cell.X) * 256 + 128, int(cell.Y) * 256 + 128, 0};
                        at.Z = MapClass::Instance.GetCellFloorHeight(at);
                        silo->QueueMission(Mission::Missile, false);
                        silo->NextMission();
                        Owner->NukeTarget = cell;
                        silo->FiringSWType = int(Type->Type);
                        VocClass::PlayIndexAtPos(RulesClass::Instance->DigSound, at);
                        announce("EVA_NuclearMissileLaunched");
                    }
                    break;
                }
            }
            end_player_click(isPlayer, "EVA_NuclearMissileReady");
            Owner->RecheckTechTree = true;
            break;
        }
        case SuperWeaponType::IronCurtain: {
            auto at = surface_coords(cell);
            at.Z += 5;
            invoke_animation(RulesClass::Instance->IronCurtainInvokeAnim, at);
            announce("EVA_IronCurtainActivated");
            RadarEventClass::Create(RadarEventType::SuperweaponActivated, cell);
            for (auto offset : effect_cells) {
                auto* tile =
                    MapClass::Instance.GetCellAt(CellStruct{short(cell.X + offset.X), short(cell.Y + offset.Y)});
                for (auto* object = tile->GetContent(); object; object = object->NextObject)
                    if (!(unsigned(object->AbstractFlags) & 4u) || !static_cast<TechnoClass*>(object)->IsImmobilized)
                        object->IronCurtain(RulesClass::Instance->IronCurtainDuration, Owner, false);
            }
            end_player_click(isPlayer, "EVA_IronCurtainReady");
            break;
        }
        case SuperWeaponType::LightningStorm:
            LightningStorm::Start(RulesClass::Instance->LightningStormDuration,
                                  RulesClass::Instance->LightningDeferment, cell, Owner);
            announce("EVA_LightningStormCreated");
            end_player_click(isPlayer, "EVA_LightningStormReady");
            break;
        case SuperWeaponType::PsychicDominator: {
            RadarEventClass::Create(RadarEventType::SuperweaponActivated, cell);
            PsyDom::Start(Owner, cell);
            announce("EVA_PsychicDominatorActivated");
            VocClass::PlayIndexAtPos(RulesClass::Instance->PsychicDominatorActivateSound,
                                     MapClass::Instance.GetCellAt(cell)->GetCoords());
            end_player_click(isPlayer, "EVA_PsychicDominatorReady");
            break;
        }
        case SuperWeaponType::GeneticMutator: {
            const auto at = surface_coords(cell);
            auto visual = at;
            visual.Z += 5;
            invoke_animation(RulesClass::Instance->IonBlast, visual);
            announce("EVA_GeneticMutatorActivated");
            VocClass::PlayIndexAtPos(RulesClass::Instance->GeneticMutatorActivateSound, at);
            RadarEventClass::Create(RadarEventType::SuperweaponActivated, cell);
            if (RulesClass::Instance->MutateExplosion) {
                MapClass::DamageArea(at, 10000, nullptr, RulesClass::Instance->MutateExplosionWarhead, false, Owner);
            } else
                for (auto offset : effect_cells) {
                    auto* tile =
                        MapClass::Instance.GetCellAt(CellStruct{short(cell.X + offset.X), short(cell.Y + offset.Y)});
                    for (auto* object = tile->GetContent(); object;) {
                        auto* next = object->NextObject;
                        if (object->WhatAmI() == AbstractType::Infantry) {
                            int damage = static_cast<TechnoClass*>(object)->GetTechnoType()->Strength;
                            object->ReceiveDamage(&damage, 0, RulesClass::Instance->MutateWarhead, nullptr, true, false,
                                                  Owner);
                        }
                        object = next;
                    }
                }
            end_player_click(isPlayer, "EVA_GeneticMutatorReady");
            break;
        }
        case SuperWeaponType::ForceShield: {
            const auto at = surface_coords(cell);
            auto visual = at;
            visual.Z += 5;
            invoke_animation(RulesClass::Instance->ForceShieldInvokeAnim, visual);
            SpecialSoundDuration =
                RulesClass::Instance->ForceShieldDuration - RulesClass::Instance->ForceShieldPlayFadeSoundTime;
            SpecialSoundLocation = at;
            if (Type->StartSound != -1) VocClass::PlayAt(Type->StartSound, at);
            Owner->CreatePowerOutage(RulesClass::Instance->ForceShieldBlackoutDuration);
            const auto center = MapClass::Instance.GetCellAt(cell)->GetCoords();
            const int radius = std::bit_cast<int>(unsigned(RulesClass::Instance->ForceShieldRadius) << 8);
            if (center != CoordStruct::Empty && center != CoordStruct{128, 128, 0})
                for (int i = BuildingClass::Array.Count - 1; i >= 0; --i) {
                    auto* b = BuildingClass::Array[i];
                    if (!b->Owner->IsAlliedWith(Owner)) continue;
                    const auto where = b->GetCoords();
                    const double x = std::bit_cast<int>(unsigned(center.X) - unsigned(where.X));
                    const double y = std::bit_cast<int>(unsigned(center.Y) - unsigned(where.Y));
                    const double z = std::bit_cast<int>(unsigned(center.Z) - unsigned(where.Z));
                    if (game::x87_integer(Math::sqrt(x * x + y * y + z * z)) < radius)
                        b->IronCurtain(RulesClass::Instance->ForceShieldDuration, Owner, true);
                }
            end_player_click(isPlayer, "EVA_ForceShieldReady");
            break;
        }
        case SuperWeaponType::PsychicReveal: {
            auto at = MapClass::Instance.GetCellAt(cell)->GetCoords();
            MapClass::Instance.RevealArea2(&at, RulesClass::Instance->PsychicRevealRadius, Owner, 0, 0, 0, 0, 0);
            MapClass::Instance.RevealArea2(&at, RulesClass::Instance->PsychicRevealRadius, Owner, 0, 0, 0, 0, 1);
            VocClass::PlayIndexAtPos(RulesClass::Instance->PsychicRevealActivateSound,
                                     MapClass::Instance.GetCellAt(cell)->GetCoords());
            end_player_click(isPlayer, "EVA_PsychicRevealReady");
            break;
        }
        case SuperWeaponType::ChronoSphere: {
            ChronoMapCoords = cell;
            auto at = surface_coords(cell);
            at.Z += 5;
            CreateChronoAnim(at);
            if (isPlayer)
                Unsorted::CurrentSWType = 4;
            else if (Animation) {
                Animation->Invisible = true;
                Animation->StartSoundAudioController.EndLooping();
            }
            break;
        }
        case SuperWeaponType::ChronoWarp:
            chrono_warp(*this, cell, isPlayer);
            break;
        default:
            break;
    }
}
