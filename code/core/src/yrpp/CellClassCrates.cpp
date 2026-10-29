// Campaign powerup effects reached by action 108, YR 0x00481A00.
#include "yrpp/CellClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/Powerups.h"
#include "yrpp/TagClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/VoxClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/TiberiumClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "x87_integer.hpp"
#include "x87_double.hpp"
#include <bit>
#include <cstdlib>

bool CellClass::CollectCrate(FootClass* collector) {
    if (!collector || OverlayTypeIndex == -1) return true;
    auto* overlay = OverlayTypeClass::Array.GetItemOrDefault(OverlayTypeIndex);
    if (!overlay || !overlay->Crate) return true;
    auto* owner = collector->Owner;
    auto& rules = *RulesClass::Instance;
    const bool campaign = SessionClass::Instance.GameMode == GameMode::Campaign;
    if (!campaign && owner->Type->MultiplayPassive) return true;
    if (overlay->CrateTrigger) {
        if (collector->AttachedTag) {
            collector->AttachedTag->RaiseEvent(static_cast<TriggerEvent>(49), collector, CellStruct::Empty);
            if (!collector->IsAlive) return false;
        }
        ScenarioClass::Instance->TruckCrate = true;
    }
    auto& random = ScenarioClass::Instance->Random;
    int kind = OverlayData;
    if (unsigned(kind) >= 19) {
        int total = 0;
        for (int weight : Powerups::Weights)
            total += weight;
        const int pick = random.RandomRanged(1, total);
        int cumulative = 0;
        kind = 0;
        for (; kind < 19; ++kind) {
            cumulative += Powerups::Weights[kind];
            if (pick <= cumulative) break;
        }
        if (kind >= 19) return false;
    }
    int fixedMoney = 0;
    if (campaign && !OverlayData) {
        fixedMoney = rules.SoloCrateMoney;
        if (overlay == rules.WoodCrateImg) kind = int(rules.SilverCrate);
        if (overlay == rules.CrateImg) kind = int(rules.WoodCrate);
        if (overlay == rules.WaterCrateImg) kind = int(rules.WaterCrate);
    }
    MapClass::Instance.RemoveCrate(MapCoords);
    if (kind == 6) kind = 0;
    const double argument = Powerups::Arguments[kind];
    const auto at = GetCellCoords();
    const auto sound = [&](int id) {
        if (owner->IsControlledByCurrentPlayer()) VocClass::PlayAt(id, at);
    };
    const auto animation = [&](int id, CoordStruct where) {
        if (auto* type = AnimTypeClass::Array.GetItemOrDefault(id))
            if (auto* memory = YRMemory::Allocate(sizeof(AnimClass)))
                ::new (memory) AnimClass(type, where, 0, 1, 0x600, 0, false);
    };
    if (kind == 1) {
        UnitTypeClass* type = nullptr;
        const auto choose = [&](const TypeList<UnitTypeClass*>& types) {
            const unsigned ownerMask = 1u << owner->Type->FindParentCountryIndex();
            for (auto* candidate : types)
                if (candidate->InOwners(ownerMask)) return candidate;
            return static_cast<UnitTypeClass*>(nullptr);
        };
        const auto ownsFirstTwo = [&](const auto& types) {
            for (int i = 0; i < 2 && i < types.Count; ++i)
                if (owner->CountOwnedNow(types[i]) > 0) return true;
            return false;
        };
        if (ownsFirstTwo(rules.BuildRefinery) && !ownsFirstTwo(rules.HarvesterUnit)) type = choose(rules.HarvesterUnit);
        if (rules.UnitCrateType) type = rules.UnitCrateType;
        if (!type && UnitTypeClass::Array.Count) {
            // Avoid an unbounded RNG loop when a mod disables all goodies.
            const auto goodie = [&](UnitTypeClass* item) {
                return item->CrateGoodie && (rules.BaseUnit.FindItemIndex(item) < 0 ||
                                             (SessionClass::Instance.Config.Bases && owner->IsControlledByHuman()));
            };
            bool eligible = false;
            for (auto* item : UnitTypeClass::Array)
                if (goodie(item)) {
                    eligible = true;
                    break;
                }
            if (eligible) do {
                    type = UnitTypeClass::Array[random.RandomRanged(0, UnitTypeClass::Array.Count - 1)];
                } while (!goodie(type));
        }
        if (type) {
            auto* unit = static_cast<UnitClass*>(type->CreateObject(owner));
            if (!unit) goto finish;
            if (unit->Unlimbo(at, DirType::North)) {
                sound(rules.CrateUnitSound);
                return false;
            }
            auto nearbyCell =
                MapClass::Instance.NearByLocation(MapCoords, type->SpeedType, -1, static_cast<MovementZone>(0), false,
                                                  1, 1, false, false, false, true, CellStruct::Empty, false, false);
            if (nearbyCell != CellStruct::Empty) {
                const CoordStruct position{int(nearbyCell.X) * 256 + 128, int(nearbyCell.Y) * 256 + 128, 0};
                if (unit->Unlimbo(position, DirType::North)) {
                    sound(rules.CrateUnitSound);
                    return false;
                }
            }
            GameDelete(unit);
            kind = 0;
        }
    }
    if (kind == 0) {
        const int amount = fixedMoney
                               ? fixedMoney
                               : random.RandomRanged(game::x87_integer(argument), game::x87_integer(argument) + 900);
        owner->GiveMoney(amount);
        sound(rules.CrateMoneySound);
    } else if (kind == 2) {
        sound(rules.HealCrateSound);
        for (int i = 0; i < LogicClass::Instance.Count; ++i) {
            auto* object = LogicClass::Instance[i];
            if (object && (unsigned(collector->AbstractFlags) & 1u) && object->GetOwningHouse() == owner) {
                int damage = object->Health - object->GetType()->Strength;
                object->ReceiveDamage(&damage, 0, rules.C4Warhead, nullptr, true, true, nullptr);
            }
        }
    } else if (kind == 4) {
        int damage = game::x87_integer(argument);
        collector->ReceiveDamage(&damage, 0, rules.C4Warhead, nullptr, true, false, nullptr);
        for (int i = 0; i < 5; ++i) {
            const auto position = MapClass::GetRandomCoordsNear(at, random.RandomRanged(0, 512), false);
            MapClass::DamageArea(position, damage, nullptr, rules.C4Warhead, true, nullptr);
            auto* type = MapClass::SelectDamageAnimation(damage, rules.C4Warhead, static_cast<::LandType>(0), position);
            if (auto* memory = YRMemory::Allocate(sizeof(AnimClass)))
                ::new (memory) AnimClass(type, position, 0, 1, 0x2600, -15, false);
            MapClass::FlashbangWarheadAt(damage, rules.C4Warhead, position, false, SpotlightFlags::None);
        }
    } else if (kind == 5) {
        const auto position = collector->GetCoords();
        const CoordStruct middle{(at.X + position.X) / 2, (at.Y + position.Y) / 2, (at.Z + position.Z) / 2};
        animation(0, middle);
        int damage = game::x87_integer(argument);
        collector->ReceiveDamage(&damage, 0, rules.FlameDamage, nullptr, true, false, nullptr);
        MapClass::DamageArea(at, damage, nullptr, rules.FlameDamage, true, nullptr);
    } else if (kind == 16) {
        auto* warhead = WarheadTypeClass::Find("GAS");
        const int damage = game::x87_integer(argument);
        MapClass::DamageArea(GetCoords(), damage, nullptr, warhead, false, nullptr);
        for (int i = 0; i < 8; ++i) {
            auto* cell = i < 7 ? GetNeighbourCell(static_cast<FacingType>(i)) : this;
            MapClass::DamageArea(cell->GetCoords(), damage, nullptr, warhead, false, nullptr);
        }
    } else if (kind == 17) {
        if (TiberiumClass::Array.Count) {
            int type = random.RandomRanged(0, TiberiumClass::Array.Count - 1);
            if (type == 1) type = 0;
            IncreaseTiberium(type, 1);
            const int count = random.RandomRanged(10, 20);
            for (int i = 0; i < count; ++i) {
                const auto position = MapClass::GetRandomCoordsNear(GetCoords(), random.RandomRanged(0, 768), true);
                MapClass::Instance.GetCellAt(position)->IncreaseTiberium(type, 1);
            }
        }
    } else if (kind == 7)
        MapClass::Instance.Reshroud(owner);
    else if (kind == 8) {
        MapClass::Instance.RevealShroud(owner);
        VocClass::PlayAt(rules.CrateRevealSound, at);
    } else if (kind == 3 || kind == 9 || kind == 10 || kind == 11 || kind == 14) {
        bool announce = false;
        auto& ground = MapClass::ObjectsInLayers[int(Layer::Ground)];
        for (int i = 0; i < ground.Count; ++i) {
            auto* object = ground[i];
            if (!object) continue;
            const unsigned flags = unsigned(object->AbstractFlags);
            if (!(flags & (kind == 10 ? 4u : 1u))) continue;
            if ((kind == 3 || kind == 14) && !object->IsOnMap) continue;
            const auto position = object->GetCoords();
            const auto delta = [](int a, int b) { return double(std::bit_cast<int>(unsigned(a) - unsigned(b))); };
            const double x = delta(at.X, position.X), y = delta(at.Y, position.Y), z = delta(at.Z, position.Z);
            if (game::x87_integer(Math::sqrt(x * x + y * y + z * z)) >= rules.CrateRadius) continue;
            auto* techno = static_cast<TechnoClass*>(object);
            if (kind == 3)
                techno->Cloakable = true;
            else if (kind == 9 && techno->ArmorMultiplier == 1.0) {
                techno->ArmorMultiplier = game::x87_multiply(argument, techno->ArmorMultiplier);
                if (techno->Owner->IsControlledByCurrentPlayer()) announce = true;
            } else if (kind == 11 && techno->FirepowerMultiplier == 1.0) {
                techno->FirepowerMultiplier = game::x87_multiply(argument, techno->FirepowerMultiplier);
                if (techno->Owner->IsControlledByCurrentPlayer()) announce = true;
            } else if (kind == 10) {
                auto* foot = static_cast<FootClass*>(techno);
                if (foot->SpeedMultiplier == 1.0 && foot->WhatAmI() != AbstractType::Aircraft) {
                    foot->SpeedMultiplier = game::x87_multiply(argument, foot->SpeedMultiplier);
                    if (foot->Owner->IsInPlayerControl) announce = true;
                }
            } else if (kind == 14 && techno->GetTechnoType()->Trainable) {
                for (int level = 0; double(level) < argument; ++level) {
                    if (techno->Veterancy.IsVeteran()) techno->Veterancy.SetElite();
                    if (techno->Veterancy.IsRookie()) techno->Veterancy.SetVeteran();
                    if (techno->Veterancy.IsNegative()) techno->Veterancy.Reset();
                }
            }
        }
        if (kind == 9) {
            if (announce) VoxClass::Play("EVA_UnitArmorUpgraded");
            sound(rules.CrateArmourSound);
        }
        if (kind == 10) {
            if (announce) VoxClass::Play("EVA_UnitSpeedUpgraded");
            sound(rules.CrateSpeedSound);
        }
        if (kind == 11) {
            if (announce) VoxClass::Play("EVA_UnitFirepowerUpgraded");
            sound(rules.CrateFireSound);
        }
        if (kind == 14) sound(rules.CratePromoteSound);
    } else if (kind == 12) {
        for (auto* super : owner->Supers)
            if (super->Type->Type == static_cast<SuperWeaponType>(0)) {
                super->Grant(true, false, false);
                break;
            }
    }
finish:
    auto visual = at;
    visual.Z += 200;
    animation(Powerups::Anims[kind], visual);
    return true;
}
