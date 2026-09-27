#include "support/test_support.hpp"
#include "yrpp/MissionClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/RulesClass.h"
#include <array>
#include <bit>
#include <fstream>

namespace {
struct Actor : MissionClass {
    int mode=0,flags=0,forcedLayer=-2;
    std::array<int,5> calls{0,0,0,0,-1};
    HRESULT YRPP_STDCALL GetClassID(CLSID*) override { return 0; }
    HRESULT YRPP_STDCALL Save(IStream*,BOOL) override { return 0; }
    AbstractType WhatAmI() const override { return AbstractType::Infantry; }
    int Size() const override { return sizeof(*this); }
    ObjectTypeClass* GetType() const override { return nullptr; }
    Layer InWhichLayer() const override {
        return forcedLayer!=-2?static_cast<Layer>(forcedLayer):IsFallingDown?Layer::Air:Layer::Ground;
    }
    int GetHeight() const override { return Location.Z; }
    void SetHeight(DWORD height) override { Location.Z=std::bit_cast<int>(height); }
    bool Mark(MarkType kind) override { ++calls[0];calls[1]=calls[1]*4+int(kind)+1;return true; }
    void UpdatePosition(PCPType kind) override {
        EXPECT_EQ(kind,PCPType::End);++calls[2];
        if(mode==0) {
            if(flags&32)InLimbo=true;
            if(flags&128)IsAlive=false;
        } else {
            if(flags&16)IsAlive=false;
            if(flags&32)Health=0;
            if(flags&64)CurrentMission=Mission::Move;
        }
    }
    DamageState ReceiveDamage(int* damage,int distance,WarheadTypeClass*,ObjectClass* source,
        bool ignore,bool prevent,HouseClass* house) override {
        ++calls[3];EXPECT_EQ(*damage,Health);EXPECT_EQ(distance,0);EXPECT_EQ(source,nullptr);
        EXPECT_TRUE(ignore);EXPECT_TRUE(prevent);EXPECT_EQ(house,nullptr);return DamageState{};
    }
    int dispatch(int slot) {
        calls[4]=slot;
        if(flags&128){Unsorted::CurrentFrame+=7;CurrentMission=Mission::Sleep;Health=0;}
        constexpr int delays[]{-3,0,1,450};return delays[(flags>>6)&3];
    }
    int Mission_Sleep() override { return dispatch(0); }
    int Mission_Harmless() override { return dispatch(1); }
    int Mission_Ambush() override { return dispatch(2); }
    int Mission_Attack() override { return dispatch(3); }
    int Mission_Capture() override { return dispatch(4); }
    int Mission_Eaten() override { return dispatch(5); }
    int Mission_Guard() override { return dispatch(6); }
    int Mission_AreaGuard() override { return dispatch(7); }
    int Mission_Harvest() override { return dispatch(8); }
    int Mission_Hunt() override { return dispatch(9); }
    int Mission_Move() override { return dispatch(10); }
    int Mission_Retreat() override { return dispatch(11); }
    int Mission_Return() override { return dispatch(12); }
    int Mission_Stop() override { return dispatch(13); }
    int Mission_Unload() override { return dispatch(14); }
    int Mission_Enter() override { return dispatch(15); }
    int Mission_Construction() override { return dispatch(16); }
    int Mission_Selling() override { return dispatch(17); }
    int Mission_Repair() override { return dispatch(18); }
    int Mission_Missile() override { return dispatch(19); }
    int Mission_Open() override { return dispatch(20); }
    int Mission_Rescue() override { return dispatch(21); }
    int Mission_Patrol() override { return dispatch(22); }
    int Mission_ParaDropApproach() override { return dispatch(23); }
    int Mission_ParaDropOverfly() override { return dispatch(24); }
    int Mission_Wait() override { return dispatch(25); }
    int Mission_SpyPlaneApproach() override { return dispatch(26); }
    int Mission_SpyPlaneOverfly() override { return dispatch(27); }
};
struct Scope {
    RulesClass rules;
    RulesClass* previousRules=RulesClass::Instance;
    int previousFrame=Unsorted::CurrentFrame;
    Scope() {
        RulesClass::Instance=&rules;rules.ParachuteMaxFallRate=-10;rules.NoParachuteMaxFallRate=-30;
        for(auto& layer:MapClass::ObjectsInLayers)EXPECT_EQ(layer.Count,0);
    }
    ~Scope(){
        for(auto& layer:MapClass::ObjectsInLayers)layer.Clear();
        RulesClass::Instance=previousRules;Unsorted::CurrentFrame=previousFrame;
    }
};
}

TEST(ObjectMissionUpdate, OriginalFrameCorpus) {
    Scope scope;Actor actor;AnimClass parachute(nullptr,CoordStruct{});actor.Parachute=&parachute;
    std::ifstream input(RA2_OBJECT_MISSION_UPDATE_FIXTURE);ASSERT_TRUE(input.good());
    constexpr std::array<std::array<int,3>,9> timers{{{-1,0,500},{-1,3,500},{-1,-2,500},
        {490,10,500},{490,11,500},{490,0,500},{510,2,500},{0x7FFFFFF0,32,-0x7FFFFFF0},{0x7FFFFFF0,33,-0x7FFFFFF0}}};
    int mode;unsigned cases=0;
    while(input>>mode) {
        int a,b,flags;ASSERT_TRUE(bool(input>>a>>b>>flags));
        actor.mode=mode;actor.flags=flags;actor.calls={0,0,0,0,-1};
        actor.CustomSound=-1;actor.Location={0,0,0};actor.FallRate=0;actor.IsFallingDown=false;
        actor.IsOnMap=false;actor.HasParachute=false;actor.IsABomb=false;actor.LastLayer=Layer::None;
        parachute.RemainingIterations=1;
        for(auto& layer:MapClass::ObjectsInLayers)layer.Clear();
        if(mode==0) {
            std::array<int,13> expected;for(auto& value:expected)ASSERT_TRUE(bool(input>>value));
            actor.Location.Z=a;actor.FallRate=b;actor.IsFallingDown=flags&1;actor.InLimbo=flags&2;
            actor.IsOnMap=flags&4;actor.HasParachute=flags&8;actor.IsABomb=flags&16;
            actor.Health=flags&64?0:25;actor.IsAlive=true;actor.LastLayer=Layer::Air;
            ASSERT_TRUE(MapClass::ObjectsInLayers[int(Layer::Air)].AddItem(&actor));
            actor.ObjectClass::Update();
            const std::array<int,13> actual{actor.Location.Z,actor.FallRate,actor.IsFallingDown,actor.InLimbo,
                actor.IsAlive,parachute.RemainingIterations,int(actor.LastLayer),
                actor.calls[0],actor.calls[1],actor.calls[2],actor.calls[3],
                MapClass::ObjectsInLayers[int(Layer::Ground)].Count,MapClass::ObjectsInLayers[int(Layer::Air)].Count};
            ASSERT_EQ(actual,expected)<<"object z "<<a<<" rate "<<b<<" flags "<<flags;
        } else {
            std::array<int,7> expected;for(auto& value:expected)ASSERT_TRUE(bool(input>>value));
            actor.IsAlive=flags&1;actor.Health=flags&2?25:0;actor.IsFallingDown=flags&4;actor.InLimbo=flags&8;
            actor.CurrentMission=static_cast<Mission>(a);actor.UpdateTimer.StartTime=timers[b][0];
            actor.UpdateTimer.TimeLeft=timers[b][1];Unsorted::CurrentFrame=timers[b][2];
            actor.MissionClass::Update();
            const std::array<int,7> actual{actor.calls[2],actor.calls[4],actor.UpdateTimer.StartTime,
                actor.UpdateTimer.TimeLeft,int(actor.CurrentMission),actor.Health,Unsorted::CurrentFrame};
            ASSERT_EQ(actual,expected)<<"mission "<<a<<" timer "<<b<<" flags "<<flags;
        }
        ++cases;
    }
    EXPECT_EQ(cases,93696u);
    actor.Parachute=nullptr;actor.IsOnMap=false;actor.InLimbo=true;
}

TEST(ObjectMissionUpdate, OriginalBaseMissionDelays) {
    Actor actor;
    EXPECT_EQ(actor.MissionClass::Mission_Sleep(),450);
    EXPECT_EQ(actor.MissionClass::Mission_Harmless(),450);
    EXPECT_EQ(actor.MissionClass::Mission_Ambush(),450);
    EXPECT_EQ(actor.MissionClass::Mission_Attack(),450);
    EXPECT_EQ(actor.MissionClass::Mission_Capture(),450);
    EXPECT_EQ(actor.MissionClass::Mission_Eaten(),450);
    EXPECT_EQ(actor.MissionClass::Mission_Guard(),450);
    EXPECT_EQ(actor.MissionClass::Mission_AreaGuard(),450);
    EXPECT_EQ(actor.MissionClass::Mission_Harvest(),450);
    EXPECT_EQ(actor.MissionClass::Mission_Hunt(),450);
    EXPECT_EQ(actor.MissionClass::Mission_Move(),450);
    EXPECT_EQ(actor.MissionClass::Mission_Retreat(),450);
    EXPECT_EQ(actor.MissionClass::Mission_Return(),450);
    EXPECT_EQ(actor.MissionClass::Mission_Stop(),450);
    EXPECT_EQ(actor.MissionClass::Mission_Unload(),450);
    EXPECT_EQ(actor.MissionClass::Mission_Enter(),450);
    EXPECT_EQ(actor.MissionClass::Mission_Construction(),450);
    EXPECT_EQ(actor.MissionClass::Mission_Selling(),450);
    EXPECT_EQ(actor.MissionClass::Mission_Repair(),450);
    EXPECT_EQ(actor.MissionClass::Mission_Missile(),450);
    EXPECT_EQ(actor.MissionClass::Mission_Open(),450);
    EXPECT_EQ(actor.MissionClass::Mission_Rescue(),450);
    EXPECT_EQ(actor.MissionClass::Mission_Patrol(),450);
    EXPECT_EQ(actor.MissionClass::Mission_ParaDropApproach(),450);
    EXPECT_EQ(actor.MissionClass::Mission_ParaDropOverfly(),450);
    EXPECT_EQ(actor.MissionClass::Mission_Wait(),450);
    EXPECT_EQ(actor.MissionClass::Mission_SpyPlaneApproach(),450);
    EXPECT_EQ(actor.MissionClass::Mission_SpyPlaneOverfly(),450);
}

TEST(ObjectMissionUpdate, LayerRememberedEntryAndStaleFallbackDiffer) {
    Scope scope;Actor a,b;auto& ground=MapClass::ObjectsInLayers[int(Layer::Ground)];
    auto& air=MapClass::ObjectsInLayers[int(Layer::Air)];
    a.LastLayer=Layer::Ground;ground.AddItem(&a);ground.AddItem(&a);air.AddItem(&a);
    DisplayClass::Remove(&a);
    EXPECT_EQ(a.LastLayer,Layer::None);EXPECT_EQ(ground.Count,1);EXPECT_EQ(air.Count,1);
    DisplayClass::Remove(&a);EXPECT_EQ(ground.Count,1); // None does not sweep duplicates
    a.LastLayer=Layer::Top;DisplayClass::Remove(&a);
    EXPECT_EQ(a.LastLayer,Layer::None);EXPECT_EQ(ground.Count,0);EXPECT_EQ(air.Count,0);
    b.Location={5,5,0};a.Location={2,2,0};a.forcedLayer=b.forcedLayer=int(Layer::Ground);
    a.LastLayer=b.LastLayer=Layer::None;DisplayClass::Submit(&b);DisplayClass::Submit(&a);
    ASSERT_EQ(ground.Count,2);EXPECT_EQ(ground[0],&a);EXPECT_EQ(ground[1],&b);
    a.forcedLayer=int(Layer::Air);DisplayClass::Submit(&a);
    ASSERT_EQ(ground.Count,1);ASSERT_EQ(air.Count,1);EXPECT_EQ(air[0],&a);EXPECT_EQ(a.LastLayer,Layer::Air);
    a.forcedLayer=-1;DisplayClass::Submit(&a);EXPECT_EQ(a.LastLayer,Layer::None);EXPECT_EQ(air.Count,0);
    const int increment=ground.CapacityIncrement;ground.CapacityIncrement=0;
    while(ground.Count<ground.Capacity)ground.AddItem(&b);
    a.forcedLayer=int(Layer::Ground);DisplayClass::Submit(&a);EXPECT_EQ(a.LastLayer,Layer::None);
    ground.CapacityIncrement=increment;
    DisplayClass::Submit(nullptr);DisplayClass::Remove(nullptr);
}

TEST(ObjectMissionUpdate, FlamingLandingPreservesOriginalStageAndParachuteExpiry) {
    Scope scope;AnimTypeClass type("FRAME_FLAMING");type.IsFlamingGuy=true;type.RunningFrames=7;
    struct FallingAnim:AnimClass {
        FallingAnim(AnimTypeClass* type):AnimClass(type,CoordStruct{}){}
        Layer InWhichLayer() const override { return Layer::Ground; }
        int GetHeight() const override { return Location.Z; }
        void SetHeight(DWORD height) override { Location.Z=std::bit_cast<int>(height); }
        void UpdatePosition(PCPType) override {}
    } anim(&type);
    anim.InLimbo=false;anim.IsOnMap=false;anim.IsFallingDown=true;anim.FallRate=-1;
    // Dry invalid-cell fixture: water splash allocation is not exercised here.
    auto& cell=MapClass::InvalidCell;const auto land=cell.LandType;cell.LandType=LandType::Clear;
    auto restore=ra2::test::scope_exit([&]{cell.LandType=land;});
    Unsorted::CurrentFrame=71;anim.Location={-256,-256,0};anim.ObjectClass::Update();
    EXPECT_TRUE(anim.FlamingGuyExpire);EXPECT_EQ(anim.Animation.Value,57);
    EXPECT_EQ(anim.Animation.Rate,1);EXPECT_EQ(anim.Animation.Timer.StartTime,71);EXPECT_EQ(anim.Animation.Timer.TimeLeft,1);
}
