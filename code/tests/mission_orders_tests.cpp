#include "support/test_support.hpp"
#include "yrpp/MissionClass.h"
#include "yrpp/Unsorted.h"
#include <fstream>

namespace {
// The corpus executes MissionClass entries (0x005B3650 / 0x005B36B0).
// Foot's overrides (0x004D8F40 / 0x004D8F80) also change navigation.
struct OrderObject : MissionClass {
    bool ready=true;
    int ready_calls=0,next_calls=0;
    HRESULT YRPP_STDCALL GetClassID(CLSID*) override {return 0;}
    HRESULT YRPP_STDCALL Save(IStream*,BOOL) override {return 0;}
    AbstractType WhatAmI() const override {return AbstractType::None;}
    int Size() const override {return sizeof(*this);}
    bool ReadyToNextMission() const override {
        ++const_cast<OrderObject*>(this)->ready_calls;return ready;
    }
    bool NextMission() override {++next_calls;return MissionClass::NextMission();}
};
struct FrameScope {
    int previous=Unsorted::CurrentFrame;
    FrameScope(){Unsorted::CurrentFrame=500;}
    ~FrameScope(){Unsorted::CurrentFrame=previous;}
};
}

TEST(MissionOrders, OriginalInstructionCorpus) {
    FrameScope clock;
    std::ifstream input(RA2_MISSION_ORDERS_FIXTURE);ASSERT_TRUE(input.good());
    unsigned cases=0;char op;
    OrderObject object;
    while(input>>op){
        int current,queued,suspended,requested,start,ready,result,expected[9];
        ASSERT_TRUE(bool(input>>current>>queued>>suspended>>requested>>start>>ready>>result));
        for(auto& value:expected)ASSERT_TRUE(bool(input>>value));
        object.CurrentMission=static_cast<Mission>(current);object.QueuedMission=static_cast<Mission>(queued);
        object.SuspendedMission=static_cast<Mission>(suspended);object.unknown_bool_B8=true;
        object.MissionStatus=19;object.CurrentMissionStartTime=11;object.MissionAccumulateTime=17;
        object.UpdateTimer.StartTime=13;object.UpdateTimer.TimeLeft=23;object.ready=ready!=0;
        int actual=0;const auto mission=static_cast<Mission>(requested);
        switch(op){
            case 'Q':actual=object.QueueMission(mission,start!=0);break;
            case 'N':actual=object.NextMission();break;
            case 'F':object.ForceMission(mission);break;
            case 'O':object.Override_Mission(mission,nullptr,nullptr);break;
            case 'R':actual=object.Mission_Revert();break;
            default:FAIL()<<op;
        }
        const int state[]{int(object.CurrentMission),int(object.SuspendedMission),int(object.QueuedMission),
            int(object.unknown_bool_B8),object.MissionStatus,object.CurrentMissionStartTime,object.MissionAccumulateTime,
            object.UpdateTimer.StartTime,object.UpdateTimer.TimeLeft};
        EXPECT_EQ(actual,result)<<op<<" case "<<cases;
        for(unsigned i=0;i<9;++i)EXPECT_EQ(state[i],expected[i])<<op<<" case "<<cases<<" field "<<i;
        EXPECT_EQ(object.MissionIsOverriden(),object.SuspendedMission!=Mission::None);
        ++cases;
    }
    EXPECT_EQ(cases,3200u);
}

TEST(MissionOrders, PlayerOrderWaitsForSafeTransitionAndDoesNotMoveByItself) {
    FrameScope clock;OrderObject object;const auto location=object.Location;
    object.CurrentMission=Mission::Guard;object.ready=false;
    // False is not rejection: the original queues but has not commenced.
    EXPECT_FALSE(object.QueueMission(Mission::Move,true));
    EXPECT_EQ(object.QueuedMission,Mission::Move);EXPECT_EQ(object.CurrentMission,Mission::Guard);
    EXPECT_EQ(object.ready_calls,1);EXPECT_EQ(object.next_calls,0);
    object.ready=true;EXPECT_TRUE(object.QueueMission(Mission::Move,true));
    EXPECT_EQ(object.CurrentMission,Mission::Move);EXPECT_EQ(object.QueuedMission,Mission::None);
    EXPECT_EQ(object.UpdateTimer.TimeLeft,0);EXPECT_EQ(object.CurrentMissionStartTime,500);
    EXPECT_EQ(object.Location,location);EXPECT_FALSE(object.IsInLogic);
    object.ForceMission(Mission::Guard);EXPECT_FALSE(object.QueueMission(Mission::QMove,false));
    EXPECT_EQ(object.QueuedMission,Mission::QMove); // YR does not use OpenTS's QMove->Move rewrite.
}

TEST(MissionOrders, TemporaryOrderRestoresQueuedCommandAndGuardsSelling) {
    OrderObject object;object.CurrentMission=Mission::Move;object.QueuedMission=Mission::Attack;
    object.Override_Mission(Mission::Stop,nullptr,nullptr);
    EXPECT_EQ(object.SuspendedMission,Mission::Attack);EXPECT_EQ(object.QueuedMission,Mission::Attack);
    EXPECT_TRUE(object.Mission_Revert());EXPECT_EQ(object.CurrentMission,Mission::Attack);
    EXPECT_FALSE(object.Mission_Revert());
    object.ForceMission(Mission::Selling);
    EXPECT_TRUE(object.QueueMission(Mission::Move,true)); // Original AL is nonzero, state unchanged.
    EXPECT_EQ(object.CurrentMission,Mission::Selling);EXPECT_EQ(object.QueuedMission,Mission::None);
    object.Override_Mission(Mission::Move,nullptr,nullptr);EXPECT_EQ(object.CurrentMission,Mission::Selling);
    object.ForceMission(Mission::Wait);object.ForceMission(Mission::Guard);EXPECT_EQ(object.CurrentMission,Mission::Wait);
    EXPECT_TRUE(object.MissionClass::ReadyToNextMission());
}
