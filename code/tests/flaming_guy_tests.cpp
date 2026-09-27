#include "support/test_support.hpp"
#include "yrpp/AnimClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/FileFormats/SHP.h"
#include <array>
#include <fstream>
#include <memory>

TEST(FlamingGuy, OriginalInstructionStates) {
    ScenarioClass scenario;auto* previous=ScenarioClass::Instance;ScenarioClass::Instance=&scenario;
    const int previousFrame=Unsorted::CurrentFrame;
    const auto restore=ra2::test::scope_exit([&]{MapClass::Instance.ReleaseCellStorage();ScenarioClass::Instance=previous;Unsorted::CurrentFrame=previousFrame;});
    AnimTypeClass type("FLAMING_TEST");type.IsFlamingGuy=true;type.RunningFrames=6;
    SHPStruct shape{};shape.Frames=124;type.Image=&shape;type.End=type.LoopEnd=124;
    const auto detach=ra2::test::scope_exit([&]{type.Image=nullptr;});
    std::unique_ptr<AnimClass> anim;
    std::ifstream fixture(RA2_FLAMING_GUY_FIXTURE);ASSERT_TRUE(fixture.good());
    int mode,frame;unsigned rows=0;
    while(fixture>>mode>>frame) {
        std::array<int,14> expected;for(auto& v:expected)ASSERT_TRUE(bool(fixture>>v));
        if(frame==0) {
            anim.reset();ASSERT_TRUE(MapClass::Instance.CreateEmptyCells({0,0,64,64},0));
            MapClass::Instance.VisibleRect={0,0,64,64};scenario.Random=Randomizer(12345+mode);
            anim=std::make_unique<AnimClass>(&type,CoordStruct{40*256+128,40*256+128,mode==4||mode==5?416:0});
            const auto cell=[](int x,int y){return MapClass::Instance.GetCellAt(CellStruct{short(x),short(y)});};
            if(mode==1||mode==7||mode==8)cell(42,40)->LandType=LandType::Water;
            if(mode==2)for(auto delta:Unsorted::AdjacentCell)cell(40+delta.X,40+delta.Y)->OccupationFlags=0xE0;
            if(mode==3)cell(41,40)->LandType=LandType::Beach;
            if(mode>=3&&mode<=6)anim->FlamingGuyCoords={41*256+128,40*256+128,0};
            if(mode==4)for(int x=39;x<44;++x)cell(x,40)->Flags|=CellFlags::Bridge;
            if(mode==5)anim->IsFallingDown=true;
            if(mode==6)anim->FlamingGuyRetries=7;
            if(mode==7)cell(41,40)->OccupationFlags=0xE0;
            if(mode==8)cell(42,40)->Flags|=CellFlags::Bridge;
            if(mode==9)for(auto delta:Unsorted::AdjacentCell)cell(40+delta.X,40+delta.Y)->Level=3;
        }
        Unsorted::CurrentFrame=frame;anim->FlamingGuyAI();
        const auto& at=anim->Location;const auto& to=anim->FlamingGuyCoords;
        const std::array<int,14> actual{at.X,at.Y,at.Z,to.X,to.Y,to.Z,anim->FlamingGuyRetries,
            anim->Animation.Value,anim->Animation.Rate,anim->FlamingGuyExpire,anim->IsFallingDown,anim->TimeToDie,
            scenario.Random.Next1,scenario.Random.Next2};
        ASSERT_EQ(actual,expected)<<"mode "<<mode<<" frame "<<frame;
        if(!anim->TimeToDie&&anim->FlamingGuyExpire)++anim->Animation.Value;
        ++rows;
    }
    EXPECT_GT(rows,500u);
}
