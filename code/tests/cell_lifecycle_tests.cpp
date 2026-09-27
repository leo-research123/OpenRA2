#include "support/test_support.hpp"
#include "yrpp/CellClass.h"
#include "yrpp/ScenarioClass.h"
#include "map_runtime.hpp" // Explicit test access to owned-effect destruction.
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {

struct FogVector final : DynamicVectorClass<FoggedObjectClass*> {
    int& destroyed;
    explicit FogVector(int& value) : destroyed(value) {}
    ~FogVector() override { ++destroyed; }
};
TEST(CellLifecycle, Contracts) {
    ScenarioClass scenario;
    auto* previous = ScenarioClass::Instance;
    ScenarioClass::Instance = &scenario;
    struct Restore { ScenarioClass* value; ~Restore() { ScenarioClass::Instance = value; } } restore{previous};
    scenario.UniqueID = 73;
    int vectors_destroyed = 0, effects_destroyed = 0;
    const double scale = 0.25;
    const bool count_references = true;
    game::MapRuntimeServices runtime{&scale, nullptr, &count_references,
        [](PixelFXClass* effect) noexcept { ++*reinterpret_cast<int*>(effect); }};
    struct Context { int* vectors; int* effects; } context{&vectors_destroyed, &effects_destroyed};
    EXPECT_TRUE((game::with_map_runtime(runtime, [](void* pointer) {
        auto& context = *static_cast<Context*>(pointer);
        for (unsigned index = 0; index < 64; ++index) {
            std::unique_ptr<CellClass, GameDeleter> cell(CellClass::Create());
            EXPECT_TRUE((bool(cell))) << "normal original cell allocation";
            EXPECT_TRUE((cell->UniqueID == 74 + index)) << "one Scenario ID per live cell";
            AbstractClass* base = cell.get();
            EXPECT_TRUE((base->WhatAmI() == AbstractType::Cell && base->Size() == sizeof(CellClass))) << "native Cell virtual table";
            EXPECT_TRUE((cell->MapCoords == CellStruct{0,0} && cell->IsoTileTypeIndex == 0xffff &&
                cell->OverlayTypeIndex == -1 && cell->SmudgeTypeIndex == -1)) << "original empty terrain sentinels";
            EXPECT_TRUE((cell->Visibility == -2 && cell->Foggedness == -2 && cell->ShroudCounter == 1 &&
                cell->TubeIndex == -1 && !cell->SlopeIndex && !cell->Level)) << "original visibility and geometry defaults";
            EXPECT_TRUE((cell->Intensity == 0x10000 && cell->Intensity_Normal == 1000 &&
                cell->Color2_Blue == 1000)) << "original light defaults";
            cell->MapCoords = {3,4}; cell->Level = 2;
            CoordStruct coords{};
            EXPECT_TRUE((base->GetCoords(&coords) == &coords && coords.X == 896 && coords.Y == 1152 &&
                coords.Z == 208)) << "normal cell participates in geometry";
            cell->FoggedObjects = new FogVector(*context.vectors);
            EXPECT_TRUE((cell->FoggedObjects->AddItem(nullptr))) << "owned vector allocation";
            cell->PixelFX = reinterpret_cast<PixelFXClass*>(context.effects);
        }
    }, &context))) << "cell lifetime service scope";
    EXPECT_TRUE((vectors_destroyed == 64 && effects_destroyed == 64)) << "owned resources destroyed exactly once";
    EXPECT_TRUE((scenario.UniqueID == 137)) << "scenario identity state preserved after release";
    // Factory failure returns null, creates no ID, and crosses no exception.
    EXPECT_TRUE((YRMemory::ConfigureFailureRecovery(nullptr, nullptr, 1))) << "configure terminal allocation limit";
    EXPECT_TRUE((!CellClass::Create() && scenario.UniqueID == 137)) << "allocation failure leaves scenario unchanged";
}
}

