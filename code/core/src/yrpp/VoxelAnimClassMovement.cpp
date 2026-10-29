// YR 0x00749F30: trailer creation precedes Bounce movement and contact effects.
#include "yrpp/VoxelAnimClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/TiberiumClass.h"
#include "yrpp/OverlayClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/RadarClass.h"
#include "map_world.hpp"
#include "RulesClassReaders.hpp"

namespace {
void animation(AnimTypeClass* type, const CoordStruct& at, int delay = 0, unsigned flags = 0x600, int z = 0) {
    if (type)
        if (auto* memory = YRMemory::Allocate(sizeof(AnimClass)))
            ::new (memory) AnimClass(type, at, delay, 1, flags, z, false);
}
}
void VoxelAnimClass::Update() {
    if (!Type || TimeToDie) {
        UnInit();
        return;
    }
    if (Type->StartSound != -1) {
        const auto at = GetCoords();
        VocClass::UpdatePosition(&at, &Audio3);
    }
    if (Duration) --Duration;
    auto& map = MapClass::Instance;
    auto* cell = map.GetCellAt(Location);
    const bool water = cell->LandType == LandType::Water;
    const bool aboveDeck = Location.Z >= map.GetCellFloorHeight(Location) + CellClass::BridgeHeight;
    if (Duration <= 0) {
        auto at = Bounce.GetCoords();
        if (!water || aboveDeck) {
            if (Type->ExpireAnim) {
                animation(Type->ExpireAnim, at, 0, 0x2600, -30);
                MapClass::DamageArea(at, Type->Damage, nullptr, Type->Warhead, true, nullptr);
                MapClass::FlashbangWarheadAt(Type->Damage, Type->Warhead, at, false, SpotlightFlags::None);
            }
            if (Type->IsMeteor && Type->Spawns && Type->SpawnCount > 0) {
                const int count = ScenarioClass::Instance->Random.RandomRanged(0, Type->SpawnCount) +
                                  ScenarioClass::Instance->Random.RandomRanged(0, Type->SpawnCount);
                for (int i = 0; i < count; ++i)
                    if (auto* memory = YRMemory::Allocate(sizeof(VoxelAnimClass)))
                        ::new (memory) VoxelAnimClass(Type->Spawns, &at, nullptr);
            }
            // The original seeds an empty overlay and queues growth, rather than
            // thickening a patch through IncreaseTiberium.
            if (Type->IsTiberium && !aboveDeck) {
                constexpr CellStruct offsets[] = {{0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}};
                const int count = Type->IsMeteor ? 8 : 1;
                for (int i = 0; i < count; ++i) {
                    auto target = CellClass::Coord2Cell(at);
                    if (Type->IsMeteor) {
                        target.X += offsets[i].X;
                        target.Y += offsets[i].Y;
                    }
                    auto* occupied = map.TryGetCellAt(target);
                    auto* tib = TiberiumClass::Array.GetItemOrDefault(TiberiumClass::FindIndex(127));
                    if (!occupied || !occupied->CanTiberiumGerminate(nullptr) || !tib || !tib->Image) continue;
                    int index = tib->Image->ArrayIndex;
                    auto& random = ScenarioClass::Instance->Random;
                    if (occupied->SlopeIndex)
                        index += 2 * occupied->SlopeIndex + random.RandomRanged(0, 1) + tib->NumImages - 2;
                    else
                        index += random.RandomRanged(0, 11);
                    if (auto* overlay = OverlayTypeClass::Array.GetItemOrDefault(index))
                        if (auto* memory = YRMemory::Allocate(sizeof(OverlayClass)))
                            ::new (memory) OverlayClass(overlay, target, -1);
                    tib->RegisterForGrowth(&target);
                    occupied->OverlayData = 0;
                    occupied->MarkForRedraw();
                    RadarClass::Instance.MarkTerrainCellDirty(target);
                    game::map_resource_changed(*occupied);
                }
            }
        } else if (Type->IsMeteor) {
            auto where = Location;
            where.Z += 5;
            auto& list = RulesClass::Instance->SplashList;
            if (list.Count) animation(list[list.Count - 1], where);
        } else {
            animation(RulesClass::Instance->Wake, Location);
            auto where = Location;
            where.Z += 10;
            if (RulesClass::Instance->SplashList.Count) animation(RulesClass::Instance->SplashList[0], where);
        }
        UnInit();
        return;
    }
    if (Type->TrailerAnim && !(Unsorted::CurrentFrame % 2)) {
        // 0x0074A768 uses the voxel cell diagonal and historical world-to-client scale.
        const auto& p = Bounce.Coords;
        const CoordStruct at{rule_integer(p.X), rule_integer(p.Y),
                             rule_integer(double(p.Z) * Math::sqrt(256.0 * 256.0 * 2.0) * 0.01924500897298839 + 0.5)};
        animation(Type->TrailerAnim, at, 1);
    }
    const auto status = Bounce.Update();
    if (Type->IsMeteor) Bounce.Velocity.Z = float(double(Bounce.Velocity.Z) + Bounce.Gravity);
    if (status == BounceClass::Status::Impact)
        Duration = 0;
    else if (status == BounceClass::Status::Bounce) {
        if (water)
            Duration = 0;
        else {
            animation(Type->BounceAnim, Location);
            const auto at = Bounce.GetCoords();
            for (auto* object = map.GetCellAt(at)->FirstObject; object;) {
                auto* next = object->NextObject;
                const int distance = std::abs(at.X - object->Location.X) + std::abs(at.Y - object->Location.Y);
                if (distance <= Type->DamageRadius)
                    object->ReceiveDamage(&Type->Damage, TacticalClass::AdjustForZ(distance), Type->Warhead, nullptr,
                                          false, false, nullptr);
                object = next;
            }
        }
    }
    SetLocation(Bounce.GetCoords());
}
