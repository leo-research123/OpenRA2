#include "support/test_support.hpp"
#include "yrpp/MouseClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/Surface.h"
#include "yrpp/MessageListClass.h"
#include "yrpp/Unsorted.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

bool equal(const RectangleStruct& a,const RectangleStruct& b) {
    return a.X==b.X && a.Y==b.Y && a.Width==b.Width && a.Height==b.Height;
}
void rectangle(std::istream& in,RectangleStruct& r) { in>>r.X>>r.Y>>r.Width>>r.Height; }
TEST(GameUiLayout, Contracts) {
    ScenarioClass scenario;
    auto* previous=ScenarioClass::Instance;
    ScenarioClass::Instance=&scenario;
    struct Restore { ScenarioClass* before; ~Restore() { ScenarioClass::Instance=before; } } restore{previous};
    std::ifstream input(RA2_UI_LAYOUT_FIXTURE);
    EXPECT_TRUE((bool(input))) << "open original-code layout fixture";
    std::string line;
    unsigned observations=0;
    while (std::getline(input,line)) {
        if (line.empty() || line[0]=='#') continue;
        std::istringstream row(line);
        int w,h,side,raw; RectangleStruct map,body; Point2D origin,pitch,scroll;
        row>>w>>h>>side; rectangle(row,map); rectangle(row,body);
        row>>origin.X>>origin.Y>>pitch.X>>pitch.Y>>raw>>scroll.X>>scroll.Y;
        EXPECT_TRUE((!row.fail())) << "complete fixture row";
        scenario.PlayerSideIndex=side;
        DSurface::WindowBounds={0,0,w,h};
        DisplayClass::Instance.Set_View_Dimensions({0,0,w-168,h-32});
        auto& sidebar=SidebarClass::Instance;
        EXPECT_TRUE((equal(DSurface::ViewBounds,map))) << "original tactical rectangle";
        EXPECT_TRUE((equal(DSurface::SidebarBounds,body))) << "original sidebar body below radar";
        EXPECT_TRUE((sidebar.CameoPosition==origin && sidebar.CameoPitch==pitch)) << "original per-side cameo geometry";
        EXPECT_TRUE((sidebar.GetVisibleCameoCount()==raw)) << "raw original row arithmetic";
        EXPECT_TRUE((sidebar.GetUsableCameoCount()==std::min(60,raw))) << "60-button capacity guard";
        if (raw<=60) EXPECT_TRUE((sidebar.ScrollPosition==scroll)) << "original scroll positions";
        EXPECT_TRUE((equal(RadarClass::Instance.GetPanelBounds(),{w-152,49,140,108}))) << "native radar panel origin";
        EXPECT_TRUE((equal(TabClass::Instance.GetCommandBarBounds(),{0,h-32,w-168,32}))) << "HUD stays below map only";
        for (int tab=0;tab<4;++tab) {
            auto& strip=sidebar.Tabs[tab];
            for (int i=0;i<60;++i) {
                const auto& b=SelectClass::Array()[tab*60+i];
                EXPECT_TRUE((b.Strip==&strip && b.Index==i && b.ID==202)) << "original button ownership and identity";
                if (i<sidebar.GetUsableCameoCount()) {
                    EXPECT_TRUE((b.X==origin.X+(i%2)*pitch.X && b.Y==origin.Y+(i/2)*50+1 && b.Width==60 && b.Height==48)) << "6A8220 actual cameo hit rectangle";
                    EXPECT_TRUE((b.X>=body.X && b.X+b.Width<=w && b.Y+b.Height<=h)) << "button stays inside canvas";
                } else EXPECT_TRUE((b.Width==0 && b.Height==0)) << "resize disables stale offscreen buttons";
            }
        }
        ++observations;
    }
    EXPECT_TRUE((observations==24)) << "all original resolution/side observations";
    std::cout<<observations<<" original layouts and 5760 button rectangles checked\n";
}
}

TEST(GameUiLayout, MessageListResizesOnlyOnTransition) {
    auto& messages=MessageListClass::Instance;
    const auto cleanup=ra2::test::scope_exit([&]{messages.Init(0,0,0,0,0,0,0,0,0,0);});
    const auto epoch=Game::TickCount.StartTime;
    DSurface::WindowBounds={0,0,1024,768};DSurface::ViewBounds={};
    const RectangleStruct rect{0,0,856,736};
    DisplayClass::Instance.Set_View_Dimensions(rect);
    EXPECT_EQ(messages.MessagePos,Point2D(3,0));EXPECT_EQ(messages.MaxMessageCount,6);
    EXPECT_EQ(messages.MaxCharacters,98);EXPECT_EQ(messages.Height,19);EXPECT_EQ(messages.Width,848);
    auto* label=new TextLabelClass(messages.MessageBuffers[0],3,0,0,TextPrintType(0));
    messages.MessageList=label;messages.BufferAvail[0]=0;
    DisplayClass::Instance.Set_View_Dimensions(rect);
    EXPECT_EQ(messages.MessageList,label) << "polling unchanged layout preserves feedback";
    DisplayClass::Instance.Set_View_Dimensions({0,0,832,736});
    EXPECT_EQ(messages.MessageList,nullptr);EXPECT_EQ(messages.BufferAvail[0],1);
    EXPECT_EQ(messages.Width,824);EXPECT_EQ(Game::TickCount.StartTime,epoch);
}
