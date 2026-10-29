// Terrain constructor 0x0071BB90 and animation/update 0x0071C730.
#include "background_invalidation.hpp"
#include "yrpp/TerrainClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/FileFormats/SHP.h"
#include "map_world.hpp"
#include "target_registry.hpp"
#include <bit>
#include "yrpp/MapClass.h"
#include "yrpp/Unsorted.h"
namespace {
DynamicVectorClass<TerrainClass*> objects;
}
DynamicVectorClass<TerrainClass*>& TerrainClass::Array = objects;
TerrainClass::TerrainClass() noexcept
    : ObjectClass(), Animation{}, Type{}, IsBurning{}, IsCrumbling{}, unknown_rect_D0{} {
    Animation.Start(0);
    Array.AddItem(this);
    game::register_target_identity(*this);
}
TerrainClass::TerrainClass(TerrainTypeClass* tt, const CellStruct& coords) noexcept
    : ObjectClass(), Animation{}, Type{tt}, IsBurning{}, IsCrumbling{}, unknown_rect_D0{} {
    Animation.Start(0);
    Create_ID();
    if (tt) {
        Health = tt->Strength;
        if (coords != CellStruct::Empty) {
            const CoordStruct at{int(coords.X) * 256 + 128, int(coords.Y) * 256 + 128, 0};
            if (!Unlimbo(at, DirType::North)) UnInit();
        }
    }
    Animation.Start(0);
    Array.AddItem(this);
    game::register_target_identity(*this);
    game::invalidate_background_object(*this);
}
TerrainClass::~TerrainClass() {
    game::invalidate_background_object(*this);
    NotifyObjectExpired(true);
    Array.Remove(this);
    if (Game::IsActive && Type) {
        IsAlive = true;
        Limbo();
    }
    game::unregister_target_identity(*this);
}
void TerrainClass::PointerExpired(AbstractClass* object, bool removed) {
    ObjectClass::PointerExpired(object, removed);
    if (Type == object) {
        Type = nullptr;
        game::invalidate_background_object(*this);
    }
}
void TerrainClass::StartCrumbling() {
    if (!IsCrumbling) {
        IsCrumbling = true;
        Animation.Start(2);
        Animation.Value = 0;
        game::invalidate_background_object(*this);
    }
}

void TerrainClass::Update() {
    ObjectClass::Update();
    if (!Type) return;
    if (Type->IsAnimated && Animation.Rate == 0) {
        const int sample = ScenarioClass::Instance->Random.Random();
        // Preserve the original signed remainder, including the INT_MIN case.
        const int magnitude = sample < 0 ? std::bit_cast<int>(0u - unsigned(sample)) : sample;
        if (double(magnitude % 1000000) * 0.000001 < double(Type->AnimationProbability)) {
            Animation.Value = 0;
            Animation.Start(Type->AnimationRate);
        }
    }
    // The original Stage update tests the countdown value, including paused
    // timers. StageClass::Update treats every paused timer as expired.
    int left = Animation.Timer.TimeLeft;
    if (Animation.Timer.StartTime != -1) {
        const int elapsed = std::bit_cast<int>(unsigned(Unsorted::CurrentFrame) - unsigned(Animation.Timer.StartTime));
        left = elapsed >= left ? 0 : std::bit_cast<int>(unsigned(left) - unsigned(elapsed));
    }
    Animation.HasChanged = left == 0 && Animation.Rate != 0;
    if (!Animation.HasChanged) return;
    Animation.Value = std::bit_cast<int>(unsigned(Animation.Value) + unsigned(Animation.Step));
    Animation.Timer.Start(Animation.Rate);
    game::invalidate_background_object(*this);
    if (IsCrumbling && Animation.Value == Type->GetImage()->Frames - 1) {
        UnInit();
    } else if (Type->SpawnsTiberium && Type->IsAnimated && Animation.Value == Type->GetImage()->Frames / 2) {
        Animation.Value = 0;
        Animation.Start(0);
        MapClass::Instance.GetCellAt(Location)->SpreadTiberium(true);
    }
}
