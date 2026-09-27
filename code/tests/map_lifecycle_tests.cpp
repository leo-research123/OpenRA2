#include "support/test_support.hpp"
#include "yrpp/MouseClass.h"
#include "yrpp/ScenarioClass.h"
#include <iostream>
#include <stdexcept>
#include <type_traits>

namespace {

static_assert(!std::is_abstract_v<MouseClass>);
#if defined(_WIN32) && defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(GScreenClass) == 0x10 && sizeof(MapClass) == 0x1174);
static_assert(sizeof(DisplayClass) == 0x11e8 && sizeof(RadarClass) == 0x150c);
static_assert(sizeof(PowerClass) == 0x1544 && sizeof(SidebarClass) == 0x5518);
static_assert(sizeof(TabClass) == 0x5548 && sizeof(ScrollClass) == 0x555c && sizeof(MouseClass) == 0x556c);
static_assert(offsetof(RadarClass, RadarAudio) == 0x14c0 && offsetof(MouseClass, TabData) == 0x551c);
static_assert(offsetof(MouseClass, MouseCursorIsMini) == 0x555c);
#endif
TEST(MapLifecycle, Contracts) {
    auto& map = MouseClass::Instance;
    EXPECT_TRUE((static_cast<GScreenClass*>(&map) == &GScreenClass::Instance &&
        static_cast<MapClass*>(&map) == &MapClass::Instance &&
        static_cast<DisplayClass*>(&map) == &DisplayClass::Instance &&
        static_cast<RadarClass*>(&map) == &RadarClass::Instance &&
        static_cast<PowerClass*>(&map) == &PowerClass::Instance &&
        static_cast<SidebarClass*>(&map) == &SidebarClass::Instance &&
        static_cast<TabClass*>(&map) == &TabClass::Instance &&
        static_cast<ScrollClass*>(&map) == &ScrollClass::Instance)) << "one real root shared by every original base";
    EXPECT_TRUE((!map.TryGetCellAt(CellStruct{0,0}))) << "query before allocation is unavailable";
    EXPECT_TRUE((map.MaxLevel == 13 && map.Bitfield == 2 && map.PowerOutput == -1 &&
        map.ThumbActive && map.unknown_byte_5549 == 1)) << "calibrated root defaults";
    EXPECT_TRUE((map.unknown_1258 && map.unknown_1258->BucketCount == 256 &&
        map.unknown_1258->BucketGrowthStep == 10)) << "original radar hash storage";
    EXPECT_TRUE((map.unknown_1258->BucketHashFunction({0,7,11}) == 2768)) << "original radar key hashing";
    INoticeSink* notice = &map;
    EXPECT_TRUE((!notice->INoticeSink_Unknown(0xffffffffu))) << "real secondary interface dispatch";
    ScenarioClass scenario;
    auto* previous = ScenarioClass::Instance; ScenarioClass::Instance = &scenario;
    struct Restore { ScenarioClass* old; ~Restore() { MapClass::Instance.ReleaseCellStorage(); ScenarioClass::Instance = old; } } restore{previous};
    for (const auto size : {Point2D{8,12}, Point2D{64,96}, Point2D{255,255}, Point2D{2,1}, Point2D{12,8}}) {
        const int start = scenario.UniqueID;
        EXPECT_TRUE((map.CreateEmptyCells({0,0,size.X,size.Y}, 3))) << "create original diamond map";
        int population = 0;
        for (int y = 0; y < 512; ++y) for (int x = 0; x < 512; ++x) {
            auto* cell = map.TryGetCellAt(CellStruct{static_cast<short>(x),static_cast<short>(y)});
            if (!cell) continue;
            ++population;
            EXPECT_TRUE((cell->MapCoords.X == x && cell->MapCoords.Y == y && cell->Level == 3)) << "every allocated slot owns the matching normally constructed cell";
            EXPECT_TRUE((cell->UniqueID == static_cast<DWORD>(start + population))) << "original row-major identity order";
        }
        EXPECT_TRUE((population == (2 * size.X - 1) * size.Y)) << "diamond cell population";
        EXPECT_TRUE((scenario.UniqueID == start + population + 1)) << "invalid sentinel reconstructed after map cells";
        EXPECT_TRUE((map.ValidMapCellCount == (size.X + size.Y + 1) * (size.X + size.Y + 1))) << "passability buffer capacity";
        EXPECT_TRUE((map.LevelAndPassability[map.ValidMapCellCount-1].CellPassability == 7)) << "complete passability storage initialized";
        EXPECT_TRUE((map.GetCellAt(CellStruct{-1,-1}) == &MapClass::InvalidCell &&
            MapClass::InvalidCell.MapCoords == CellStruct{-1,-1})) << "original invalid-cell fallback";
    }
    EXPECT_TRUE((!map.CreateEmptyCells({0,0,511,2},0) && !map.Cells.Items && !map.LevelAndPassability)) << "invalid dimensions do not leave half a map";
    EXPECT_TRUE((map.CreateEmptyCells({0,0,8,12},0))) << "reload after rejected map";
    auto* slots = map.Cells.Items;
    auto* passability = map.LevelAndPassability;
    map.DestructCells();
    EXPECT_TRUE((map.Cells.Items == slots && map.LevelAndPassability == passability && map.MapRect.Width == 8)) << "original DestructCells retains geometry, cell table and navigation buffers";
    for (int i = 0; i < map.Cells.Capacity; ++i) EXPECT_TRUE((!map.Cells.Items[i])) << "original destruction clears every cell slot";
    map.DestructCells();
    map.ReleaseCellStorage(); map.ReleaseCellStorage();
    EXPECT_TRUE((!map.TryGetCellAt(CellStruct{7,8}) && !map.CellIterator_NextCell && !map.MapRect.Width)) << "repeated close releases storage and geometry";
    EXPECT_TRUE((YRMemory::ConfigureFailureRecovery(nullptr,nullptr,1))) << "configure recoverable allocation failure";
    EXPECT_TRUE((!map.CreateEmptyCells({0,0,8,12},0) && !map.Cells.Items)) << "allocation failure releases the partial pointer table";
}
}

