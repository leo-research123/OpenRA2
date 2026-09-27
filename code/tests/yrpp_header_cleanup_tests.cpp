#include "support/test_support.hpp"
// Exercise the relocated definitions through their public declarations.
#include "yrpp/FileClass.h"
#include "yrpp/RawFileClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/HouseTypeClass.h"
#include "yrpp/FileSystem.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/Surface.h"
#include "yrpp/CCINIClass.h"

namespace {

struct DisplayProbe final : DisplayClass {
    unsigned cursor_calls=0;
    bool SetCursor(MouseCursorType, bool) override { ++cursor_calls;return false; }
    bool UpdateCursor(MouseCursorType, bool) override { return false; }
    bool RestoreCursor() override { return false; }
    void UpdateCursorMinimapState(bool) override {}
    MouseCursorType GetLastMouseCursor() override { return static_cast<MouseCursorType>(0); }
};
}


TEST(YrppHeaderCleanup, PublicContracts) {
    DisplayProbe display;

    RawFileClass file;
    EXPECT_TRUE((!file.SkipCDCheck && !file.HasHandle() && file.GetFileName() == nullptr)) << "RawFile default construction and accessors";
    EXPECT_TRUE((file.FileClass::GetFileTime() == 0 && !file.FileClass::SetFileTime(0))) << "FileClass default time methods";

    Surface surface;
    surface.Width = 320;
    surface.Height = 200;
    const auto bounds = surface.GetRect();
    EXPECT_TRUE((bounds.X == 0 && bounds.Y == 0 && bounds.Width == 320 && bounds.Height == 200)) << "Surface rectangle convenience overload";
    EXPECT_TRUE((!surface.CopyFromWhole(nullptr, false, false) && surface.GetPitch() == 0)) << "Surface legacy default results preserved";

    const CellStruct foundation[]{{-1, -2}, {2, 3}, {0x7fff, 0x7fff}};
    const auto size = display.FoundationBoundsSize(foundation);
    EXPECT_TRUE((size.X == 4 && size.Y == 6)) << "Display foundation convenience overload";
    const auto empty = display.FoundationBoundsSize(nullptr);
    EXPECT_TRUE((empty.X == 0 && empty.Y == 0)) << "Display null foundation";

    const auto x = TacticalClass::AdjustForZShapeMove(256, 0);
    const auto y = TacticalClass::AdjustForZShapeMove(0, 256);
    EXPECT_TRUE((x.X == 30 && x.Y == 15 && y.X == -30 && y.Y == 15)) << "Tactical projection helper";

    INIClass ini;
    EXPECT_TRUE((ini.WriteInteger("cleanup", "integer", 37))) << "INI setup";
    int integer = 0;
    ini.GetInteger("cleanup", "integer", integer);
    EXPECT_TRUE((integer == 37)) << "INI GetInteger wrapper";
    double fraction = 0.25;
    ini.GetDouble("missing", "value", fraction);
    EXPECT_TRUE((fraction == 0.25)) << "INI GetDouble default preservation";
    EXPECT_TRUE((ini.WriteRate("cleanup", "rate", 900) && ini.ReadRate("cleanup", "rate", 0) == 900)) << "INI rate wrappers";
}

TEST(YrppHeaderCleanup, DisplayLoadRequiresStream) {
    DisplayProbe display;
    // 0x004AE6F0 delegates to the five Layer loads. Native null streams
    // return E_POINTER; they no longer enter an unavailable-entry abort.
    EXPECT_EQ(display.DisplayClass::Load(nullptr),static_cast<HRESULT>(0x80004003u));
}
TEST(YrppHeaderCleanup, DisplaySaveRequiresStream) {
    DisplayProbe display;
    EXPECT_EQ(display.DisplayClass::Save(nullptr),static_cast<HRESULT>(0x80004003u));
}
TEST(YrppHeaderCleanup, DisplayActionRequiresPalette) {
    HouseTypeClass country("DISPLAY_CURSOR");HouseClass house(&country);
    DisplayProbe display;
    auto* previous_player=HouseClass::CurrentPlayer;
    auto* previous_palette=FileSystem::MOUSE_PAL;
    const auto restore=ra2::test::scope_exit([&] {
        HouseClass::CurrentPlayer=previous_player;FileSystem::MOUSE_PAL=previous_palette;
    });
    HouseClass::CurrentPlayer=&house;FileSystem::MOUSE_PAL=nullptr;
    // 0x004AAE90 queries the current player's waypoints before cursor art.
    // The native missing-palette boundary returns false without device calls.
    EXPECT_FALSE(display.DisplayClass::ConvertAction({0,0},false,nullptr,Action::Move,false));
    EXPECT_EQ(display.cursor_calls,0u);
}
