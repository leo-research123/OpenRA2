// Original YR BombClass, 0x004385D0..0x00438B30.
#include "yrpp/BombClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include <bit>
#include <cstdlib>
#include <cstring>

namespace {
DynamicVectorClass<BombClass*> bombs;
}
DynamicVectorClass<BombClass*>& BombClass::Array = bombs;

BombClass::BombClass() noexcept
    : AbstractClass(), Owner(nullptr), OwnerHouse(nullptr), Target(nullptr), DeathBomb(false), PlantingFrame(0),
      DetonationFrame(0), Audio(), TickSound(RulesClass::Instance->BombTickingSound), ShouldPlayTickingSound(false),
      Harmless(false) {
    if (!Array.AddItem(this)) std::abort();
}

BombClass::~BombClass() {
    // Audio is the only nontrivial member. Its teardown precedes registry removal.
    Audio.~AudioController();
    Array.Remove(this);
}

HRESULT YRPP_STDCALL BombClass::GetClassID(CLSID* output) {
    if (!output) return static_cast<HRESULT>(0x80004003u);
    const DWORD words[]{108652931u, 299085213u, 268441278u, 1822515787u};
    std::memcpy(output, words, sizeof(words));
    return 0;
}

void BombClass::Detonate() {
    if (!Target || Harmless) return;
    Target->AttachedBomb = nullptr;
    Target->BombVisible = false;
    Harmless = true;
    if (!Target->InLimbo) {
        const auto location = Target->Location;
        auto& rules = *RulesClass::Instance;
        MapClass::DamageArea(location, rules.IvanDamage, Owner, rules.IvanWarhead, true, nullptr);
        if (auto* storage = YRMemory::Allocate(sizeof(AnimClass))) {
            const auto land = MapClass::Instance.GetCellAt(CellClass::Coord2Cell(location))->LandType;
            auto* type = MapClass::SelectDamageAnimation(rules.IvanDamage, rules.IvanWarhead, land, location);
            // The original 0x0048ACE0 always returns -15.
            new (storage) AnimClass(type, location, 0, 1, 0x2600, -15, false);
        }
        if (Target && Target->WhatAmI() == AbstractType::Building &&
            static_cast<BuildingClass*>(Target)->Type->BridgeRepairHut) {
            bool wood = false;
            for (int y = -2; y < 3; ++y)
                for (int x = -2; x < 3; ++x) {
                    auto base = Target->GetMapCoords();
                    const int tile = MapClass::Instance.GetCellAt(CellStruct{short(base.X + x), short(base.Y + y)})
                                         ->IsoTileTypeIndex;
                    base = Target->GetMapCoords();
                    const int overlay = MapClass::Instance.GetCellAt(CellStruct{short(base.X + x), short(base.Y + y)})
                                            ->OverlayTypeIndex;
                    if ((tile >= IsometricTileTypeClass::WoodBridgeSet &&
                         tile < IsometricTileTypeClass::WoodBridgeSet + 16) ||
                        (overlay >= 74 && overlay <= 101))
                        wood = true;
                }
            const auto cell = Target->GetMapCoords();
            if (wood)
                MapClass::Instance.DestroyWoodBridgeAt(cell);
            else
                MapClass::Instance.DestroyConcreteBridgeAt(cell);
        }
    }
    Owner = nullptr;
    Target = nullptr;
    OwnerHouse = nullptr;
    Audio.End();
    ShouldPlayTickingSound = false;
}

void BombClass::Disarm() {
    if (Target) {
        Target->AttachedBomb = nullptr;
        Target->BombVisible = false;
    }
    Harmless = true;
    Owner = nullptr;
    Target = nullptr;
    OwnerHouse = nullptr;
    Audio.End();
    ShouldPlayTickingSound = false;
}

bool BombClass::TimeToExplode() const { return !DeathBomb && Unsorted::CurrentFrame > DetonationFrame && !Harmless; }

int BombClass::GetCurrentFlickerFrame() const {
    if (DeathBomb == 1) return 12;
    const auto& rules = *RulesClass::Instance;
    const int elapsed = std::bit_cast<int>(unsigned(Unsorted::CurrentFrame) - unsigned(PlantingFrame));
    int frame = 2 * (elapsed / (rules.IvanTimedDelay / 6));
    if (Unsorted::CurrentFrame % (2 * rules.IvanIconFlickerRate) >= rules.IvanIconFlickerRate) ++frame;
    return frame < 12 ? frame : 11;
}
