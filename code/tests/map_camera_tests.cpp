#include "support/test_support.hpp"
#include "yrpp/TacticalClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/Unsorted.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {

}

TEST(MapCamera, Contracts) {
    TacticalClass tactical({}, {}, 0,1,0,1,1);
    std::ifstream input(RA2_CAMERA_FIXTURE);
    EXPECT_TRUE((bool(input))) << "open original camera fixture";
    unsigned count=0; char kind;
    while (input>>kind) {
        if (kind=='C') {
            int width,changed; RectangleStruct visible,viewport{}; Point2D point,expected,minimum,maximum;
            input>>width>>visible.X>>visible.Y>>visible.Width>>visible.Height>>viewport.Width>>viewport.Height
                >>point.X>>point.Y>>expected.X>>expected.Y>>changed;
            TacticalClass::CameraCenterBounds(width,visible,viewport,minimum,maximum);
            const bool actual=TacticalClass::ClampCameraCenter(point,minimum,maximum);
            EXPECT_TRUE((actual==bool(changed) && point==expected)) << "original camera limits/branch order";
        } else if (kind=='V') {
            RectangleStruct viewport{},cells; Point2D center,position;
            input>>viewport.Width>>viewport.Height>>center.X>>center.Y>>position.X>>position.Y
                >>cells.X>>cells.Y>>cells.Width>>cells.Height;
            tactical.SetViewCenter(center,viewport);
            const auto& actual=tactical.ContainingMapCoords;
            if (actual.X!=cells.X || actual.Y!=cells.Y) std::cerr<<"center "<<center.X<<','<<center.Y
                <<" expected cells "<<cells.X<<','<<cells.Y<<" actual "<<actual.X<<','<<actual.Y<<'\n';
            EXPECT_TRUE((tactical.TacticalPos==position && tactical.TacticalCoord1==center && tactical.TacticalCoord2==center
                && tactical.Redrawing && actual.X==cells.X && actual.Y==cells.Y && actual.Width==cells.Width
                && actual.Height==cells.Height)) << "original center publication and inverse projection";
        } else if (kind=='T') {
            int bypass,mapWidth;RectangleStruct visible,viewport{},cells;
            CoordStruct world;Point2D center,position;
            input>>bypass>>mapWidth>>visible.X>>visible.Y>>visible.Width>>visible.Height>>viewport.Width>>viewport.Height
                >>world.X>>world.Y>>world.Z>>center.X>>center.Y>>position.X>>position.Y
                >>cells.X>>cells.Y>>cells.Width>>cells.Height;
            auto& map=MapClass::Instance;map.MapRect.Width=mapWidth;map.VisibleRect=visible;
            TacticalClass::ViewBounds=viewport;Unsorted::ArmageddonMode=bypass!=0;
            // Exercise the actual Radar virtual wrapper, not a host camera substitute.
            RadarClass::Instance.vt_entry_D0(&world);
            EXPECT_EQ(tactical.TacticalCoord1,center);EXPECT_EQ(tactical.TacticalCoord2,center);
            EXPECT_EQ(tactical.TacticalPos,position);
            EXPECT_EQ(std::memcmp(&tactical.ContainingMapCoords,&cells,sizeof(cells)),0);
            EXPECT_TRUE(tactical.Redrawing);
        } else if (kind=='I') {
            RectangleStruct viewport;Point2D point;CoordStruct expected,actual;
            input>>tactical.TacticalPos.X>>tactical.TacticalPos.Y>>viewport.X>>viewport.Y>>viewport.Width>>viewport.Height
                >>point.X>>point.Y>>expected.X>>expected.Y>>expected.Z;
            TacticalClass::ViewBounds=viewport;
            EXPECT_EQ(tactical.ClientToCoords(&actual,point),&actual);EXPECT_EQ(actual,expected);
            if(expected==CoordStruct{-1,-1,-1}) {
                CellStruct cell{99,99};
                EXPECT_EQ(RadarClass::Instance.vt_entry_CC(&cell,&point),&cell);
                EXPECT_EQ(cell,(CellStruct{-1,-1}));
            }
        } else EXPECT_TRUE((false)) << "unknown camera fixture record";
        EXPECT_TRUE((bool(input))) << "complete camera fixture record"; ++count;
    }
    Unsorted::ArmageddonMode=false;TacticalClass::ViewBounds={};
    EXPECT_TRUE((input.eof() && count==1458)) << "consume all original camera cases";
}
