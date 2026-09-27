#include "support/test_support.hpp"
#include "yrpp/Unsorted.h"
#include "yrpp/PlanningTokenClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/UnitTypeClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/MessageListClass.h"
#include "yrpp/StringTable.h"
#include "yrpp/EventClass.h"
#include "yrpp/CellClass.h"
#include "api/clock.hpp"
#include "api/filesystem.hpp"
#include "api/audio_backend.hpp"
#include "ui_resources.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <memory>
namespace {
struct State {
    bool mode=PlanningNodeClass::PlanningModeActive,error=Game::PlanningErrorReported;
    HouseClass* player=HouseClass::CurrentPlayer;
    std::array<int,24> counts;
    State(){std::copy_n(Game::PlanningMemberCounts,24,counts.begin());}
    ~State(){ObjectClass::CurrentObjects.Clear();HouseClass::CurrentPlayer=player;PlanningNodeClass::PlanningModeActive=mode;
        Game::PlanningErrorReported=error;std::copy(counts.begin(),counts.end(),Game::PlanningMemberCounts);}
};
struct CapabilityType:UnitTypeClass {
    bool allows=true;
    CapabilityType():UnitTypeClass("PLAN_TEST"){}
    bool CanUseWaypoint()const override{return allows;}
};
// Observe only the existing device boundary. No message/selection/token logic
// is substituted, and there is no audio driver requirement for these tests.
struct Sound:game::AudioBackend {
    int count=0,index=-1,pan=0;float gain=0;
    bool play_global(int i,int p,float v,AudioController* c)noexcept override {++count;index=i;pan=p;gain=v;return false;}
    bool play_wav(AudioStream&,const char*,bool)noexcept override{return false;}
    void destroy_controller(AudioController&)noexcept override{}
    void stop(AudioController&)noexcept override{}
    void end(AudioController&)noexcept override{}
    void stop_looping(AudioController&)noexcept override{}
    void end_looping(AudioController&)noexcept override{}
    void set_event(AudioController&,game::AudioEventHandle,VocClass*)noexcept override{}
    bool get_event(AudioController&,game::AudioEventHandle&)noexcept override{return false;}
    bool get_controller_type(AudioController&,VocClass*&)noexcept override{return false;}
    void set_volume(game::AudioEventHandle,unsigned)noexcept override{}
    void set_panning(game::AudioEventHandle,unsigned)noexcept override{}
    bool get_event_type(game::AudioEventHandle,VocClass*&)noexcept override{return false;}
    void stop_playback(game::AudioPlaybackHandle)noexcept override{}
    void release_buffer(game::AudioBufferHandle)noexcept override{}
};
}
TEST(PlanningValidation, OriginalGraphMembershipAndCapabilityDelegation){
    State state;ASSERT_EQ(ObjectClass::CurrentObjects.Count,0);
    HouseTypeClass country("PLAN_HOUSE");HouseClass house(&country);HouseClass::CurrentPlayer=&house;
    CapabilityType type;UnitClass first(&type,&house),second(&type,&house);
    PlanningTokenClass a{},b{};a.OwnerUnit=&first;b.OwnerUnit=&second;
    PlanningNodeClass node_a{},node_b{},older{};
    PlanningMemberClass aa{&first},ab{&second},ba{&first},bb{&second};
    first.PlanningToken=&a;second.PlanningToken=&b;
    const auto detach=ra2::test::scope_exit([&]{ObjectClass::CurrentObjects.Clear();first.PlanningToken=second.PlanningToken=nullptr;
        a.PlanningNodes.Clear();b.PlanningNodes.Clear();node_a.PlanningMembers.Clear();node_b.PlanningMembers.Clear();});
    EXPECT_TRUE(Game::PlanningManager_CompatibleTokens(nullptr,nullptr));
    EXPECT_TRUE(Game::PlanningManager_CompatibleTokens(&a,nullptr));
    a.PlanningNodes.AddItem(&older);a.PlanningNodes.AddItem(&node_a);
    EXPECT_FALSE(Game::PlanningManager_CompatibleTokens(&a,nullptr));
    b.PlanningNodes.AddItem(&node_b);node_a.PlanningMembers.AddItem(&ab);
    EXPECT_FALSE(Game::PlanningManager_CompatibleTokens(&a,&b))<<"one-way membership is insufficient";
    node_b.PlanningMembers.AddItem(&ba);
    EXPECT_TRUE(Game::PlanningManager_CompatibleTokens(&a,&b))<<"distinct final nodes can be compatible";
    // Selection compares the first token with itself too: valid nodes include
    // their owner as well as the other selected unit.
    node_a.PlanningMembers.AddItem(&aa);node_b.PlanningMembers.AddItem(&bb);
    ObjectClass::CurrentObjects.AddItem(&first);ObjectClass::CurrentObjects.AddItem(&second);
    PlanningNodeClass::PlanningModeActive=true;Game::PlanningErrorReported=true;
    EXPECT_TRUE(Game::PlanningManager_CheckSelection());
    EXPECT_EQ(Game::PlanningManager_UnsupportedType(),-1);EXPECT_TRUE(first.CanUseWaypoint());
    type.allows=false;EXPECT_FALSE(first.CanUseWaypoint());EXPECT_EQ(Game::PlanningManager_UnsupportedType(),2);
    type.allows=true;
    Game::PlanningMemberCounts[house.ArrayIndex]=128;EXPECT_TRUE(Game::PlanningManager_CheckCapacity());
    second.PlanningToken=nullptr;
    EXPECT_FALSE(Game::PlanningManager_CheckSelection());EXPECT_FALSE(Game::PlanningManager_CheckCapacity());
    Game::PlanningMemberCounts[house.ArrayIndex]=127;EXPECT_TRUE(Game::PlanningManager_CheckCapacity());
    PlanningNodeClass::PlanningModeActive=false;Game::PlanningMemberCounts[house.ArrayIndex]=129;
    EXPECT_TRUE(Game::PlanningManager_CheckSelection());EXPECT_TRUE(Game::PlanningManager_CheckCapacity());
    EXPECT_TRUE(Game::PlanningErrorReported)<<"mode bypass does not reset the manager's shared error flag";
}
TEST(PlanningValidation, RealMessagesUseFullDurationAndOneSharedScold){
    const char* path=std::getenv("RA2_GAME_DATA");if(!path || !*path)GTEST_SKIP()<<"RA2_GAME_DATA enables original font and CSF integration";
    game::ResourceHandle* raw=nullptr;std::string error;ASSERT_TRUE(game::create_resources(path,raw,error))<<error;
    std::unique_ptr<game::ResourceHandle,decltype(&game::destroy_resources)> files(raw,game::destroy_resources);
    ASSERT_EQ(game::load_resources(*files,{},error),game::ResourceLoadResult::complete)<<error;
    ASSERT_TRUE(game::with_resources(*files,[](void*){
        State state;ASSERT_EQ(ObjectClass::CurrentObjects.Count,0);
        game::UiResources resources;ASSERT_TRUE(resources.load(0))<<resources.error();
        auto rules=std::make_unique<RulesClass>();auto* old_rules=RulesClass::Instance;RulesClass::Instance=rules.get();
        Sound sound;game::AudioBackend* previous=nullptr;const bool had_audio=game::get_audio_backend(previous);game::set_audio_backend(sound);
        const auto reset=ra2::test::scope_exit([&]{MessageListClass::Instance.Init(0,0,0,0,0,0,0,0,0,0);RulesClass::Instance=old_rules;
            if(had_audio)game::set_audio_backend(*previous);else game::reset_audio_backend();});
        rules->ScoldSound=77;rules->SystemError=88;
        HouseTypeClass country("PLAN_MSG_HOUSE");HouseClass house(&country);HouseClass::CurrentPlayer=&house;
        UnitTypeClass type("PLAN_MSG_UNIT");UnitClass first(&type,&house),second(&type,&house);
        PlanningTokenClass token{};PlanningNodeClass node{};PlanningMemberClass member{&first};
        token.OwnerUnit=&first;token.PlanningNodes.AddItem(&node);node.PlanningMembers.AddItem(&member);first.PlanningToken=&token;
        const auto detach=ra2::test::scope_exit([&]{ObjectClass::CurrentObjects.Clear();first.PlanningToken=nullptr;token.PlanningNodes.Clear();node.PlanningMembers.Clear();});
        ObjectClass::CurrentObjects.AddItem(&first);ObjectClass::CurrentObjects.AddItem(&second);
        auto& messages=MessageListClass::Instance;messages.Init(3,0,6,98,14,-1,-1,0,20,98,1200);
        PlanningNodeClass::PlanningModeActive=true;Game::PlanningErrorReported=false;
        Game::PlanningMemberCounts[house.ArrayIndex]=128;
        const auto time=Game::TickCount.GetTimeElapsed();
        EXPECT_FALSE(Game::PlanningManager_CheckSelection());ASSERT_NE(messages.MessageList,nullptr);
        EXPECT_STREQ(messages.MessageList->Text,StringTable::LoadString("MSG:PlanningModeHeteroSel"));
        EXPECT_EQ(DWORD(reinterpret_cast<std::uintptr_t>(messages.MessageList->UserData1)),DWORD(time+480));
        EXPECT_FALSE(messages.MessageList->Animate);EXPECT_EQ(sound.count,1);EXPECT_EQ(sound.index,77);EXPECT_EQ(sound.pan,0x2000);EXPECT_EQ(sound.gain,1.0f);
        EXPECT_FALSE(Game::PlanningManager_CheckCapacity());EXPECT_EQ(sound.count,1);EXPECT_EQ(messages.NumMessages(),1);
        // A new request after the (still pending) original planner maintenance
        // clears its shared flag. The validators themselves must not clear it.
        Game::PlanningErrorReported=false;
        EXPECT_FALSE(Game::PlanningManager_CheckCapacity());EXPECT_EQ(sound.count,2);ASSERT_EQ(messages.NumMessages(),2);
        auto* last=static_cast<TextLabelClass*>(messages.MessageList->GetNext());ASSERT_NE(last,nullptr);
        EXPECT_STREQ(last->Text,StringTable::LoadString("MSG:PlannerMaximum"));
        EXPECT_EQ(DWORD(reinterpret_cast<std::uintptr_t>(last->UserData1)),DWORD(time+480));
        messages.Init(3,0,6,98,14,-1,-1,0,20,98,1200);
        Game::PlanningErrorReported=false;sound.count=0;
        EXPECT_FALSE(first.ClickedEvent(EventType::Idle));
        ASSERT_EQ(messages.NumMessages(),1);
        EXPECT_STREQ(messages.MessageList->Text,StringTable::LoadString("MSG:PlanningModeNoStop"));
        EXPECT_EQ(DWORD(reinterpret_cast<std::uintptr_t>(messages.MessageList->UserData1)),DWORD(time+480));
        EXPECT_EQ(sound.count,2) << "original rejection plays both its first-message sound and the unconditional final sound";
        EXPECT_FALSE(first.ClickedEvent(EventType::Deploy));EXPECT_EQ(messages.NumMessages(),1);EXPECT_EQ(sound.count,3);
        EXPECT_FALSE(first.ClickedEvent(EventType::Empty));EXPECT_EQ(messages.NumMessages(),1);EXPECT_EQ(sound.count,4);
    },nullptr,error))<<error;
}

TEST(PlanningValidation, SubmissionCopiesFrameAndPlanningFlagIntoOriginalRing) {
    const auto saved=EventClass::OutList;const int frame=Unsorted::CurrentFrame;
    const auto restore=ra2::test::scope_exit([&]{EventClass::OutList=saved;Unsorted::CurrentFrame=frame;});
    EventClass::OutList.Init();EventClass event;
    event.Type=EventType::MegaMission;event.Frame=123;event.MegaMission.IsPlanningEvent=false;
    event.MegaMission.Follow=TargetClass(CellStruct{76,84});
    Unsorted::CurrentFrame=456;const auto time=game::clock_milliseconds();
    EXPECT_EQ(Game::PlanningManager_Submit(EventClass(event)),1);
    ASSERT_EQ(EventClass::OutList.Count,1);
    EXPECT_EQ(EventClass::OutList.First().Frame,456u);EXPECT_TRUE(EventClass::OutList.First().MegaMission.IsPlanningEvent);
    EXPECT_EQ(EventClass::OutList.First().MegaMission.Follow.m_ID,event.MegaMission.Follow.m_ID);
    EXPECT_EQ(event.Frame,123u);EXPECT_FALSE(event.MegaMission.IsPlanningEvent);
    EXPECT_EQ(game::clock_milliseconds(),time);
    event.Type=EventType::Idle;
    EXPECT_EQ(Game::PlanningManager_Submit(EventClass(event)),static_cast<BYTE>(EventType::Idle));
    EXPECT_EQ(EventClass::OutList.Count,1) << "non-MegaMission returns its type without entering the queue";
    EXPECT_TRUE(EventClass::AddEvent(EventClass(event)));EXPECT_EQ(EventClass::OutList.Count,2);
    EXPECT_EQ(EventClass::OutList[1].Frame,456u);EXPECT_EQ(event.Frame,123u);
    while(EventClass::OutList.Count<128)ASSERT_TRUE(EventClass::OutList.Add(event,17));
    const auto full=EventClass::OutList;event.Type=EventType::MegaMission;Unsorted::CurrentFrame=789;
    EXPECT_EQ(Game::PlanningManager_Submit(EventClass(event)),0);
    EXPECT_EQ(std::memcmp(&EventClass::OutList,&full,sizeof(full)),0);EXPECT_EQ(event.Frame,123u);
    EXPECT_EQ(game::clock_milliseconds(),time);
}

TEST(PlanningValidation, OriginalCommandCoordinatesAndPreviousNodeRules) {
    State state;
    ScenarioClass scenario;auto* old_scenario=ScenarioClass::Instance;ScenarioClass::Instance=&scenario;
    const auto restore_scenario=ra2::test::scope_exit([&]{ScenarioClass::Instance=old_scenario;});
    HouseTypeClass country("PLAN_COMMAND_HOUSE");HouseClass house(&country);HouseClass::CurrentPlayer=&house;
    UnitTypeClass type("PLAN_COMMAND_UNIT");UnitClass unit(&type,&house),other(&type,&house),third(&type,&house);
    unit.Location=CellClass::Cell2Coord({40,50},104);other.Location=CellClass::Cell2Coord({60,70},208);
    third.Location=CellClass::Cell2Coord({80,90},312);
    EventClass event;event.Type=EventType::MegaMission;event.MegaMission.Mission=static_cast<BYTE>(Mission::Move);
    event.MegaMission.Target=TargetClass(&unit);event.MegaMission.Follow=TargetClass(&other);
    event.MegaMission.Destination=TargetClass(&third);
    CoordStruct coords;
    EXPECT_EQ(Game::PlanningManager_EventCoords(&coords,&event),&coords);EXPECT_EQ(coords,unit.Location);
    event.MegaMission.Target=TargetClass();Game::PlanningManager_EventCoords(&coords,&event);EXPECT_EQ(coords,other.Location);
    event.MegaMission.Follow=TargetClass();Game::PlanningManager_EventCoords(&coords,&event);EXPECT_EQ(coords,third.Location);
    event.MegaMission.Target=TargetClass(&unit);event.MegaMission.Target.m_ID=INT_MAX;
    Game::PlanningManager_EventCoords(&coords,&event);EXPECT_EQ(coords,CoordStruct(-1,-1,-1));
    EXPECT_TRUE(Game::PlanningManager_IsLocalGuard(nullptr,Mission::Area_Guard,TargetClass()));
    EXPECT_FALSE(Game::PlanningManager_IsLocalGuard(nullptr,Mission::Move,TargetClass()));
    EXPECT_FALSE(Game::PlanningManager_IsLocalGuard(nullptr,Mission::Area_Guard,TargetClass(&unit)));
    EXPECT_TRUE(Game::PlanningManager_IsLocalGuard(&unit,Mission::Area_Guard,TargetClass(&unit)));
    EXPECT_FALSE(Game::PlanningManager_IsLocalGuard(&unit,Mission::Area_Guard,TargetClass(&other)));
    EXPECT_TRUE(Game::PlanningManager_IsLocalGuard(&unit,Mission::Area_Guard,event.MegaMission.Target))
        << "unresolved nonempty target retains the original second self-cell query";
    event.MegaMission.Target=TargetClass();
    EXPECT_EQ(Game::PlanningManager_CheckCommand(&unit,&event),-1);
    PlanningTokenClass token{};token.OwnerUnit=&unit;
    PlanningNodeClass older{},last{};EventClass previous;
    PlanningMemberClass member{&unit,&previous,-1,0},duplicate{&unit,&event,-1,0};
    last.PlanningMembers.AddItem(&member);last.PlanningMembers.AddItem(&duplicate);
    token.PlanningNodes.AddItem(&older);token.PlanningNodes.AddItem(&last);unit.PlanningToken=&token;
    const auto detach=ra2::test::scope_exit([&]{unit.PlanningToken=nullptr;token.PlanningNodes.Clear();last.PlanningMembers.Clear();});
    EXPECT_EQ(last.FindMemberIndex(&unit),0);EXPECT_EQ(last.FindMemberIndex(&other),-1);
    EXPECT_EQ(last.FindMember(&unit),&member);EXPECT_EQ(last.FindMember(&other),nullptr);
    EXPECT_EQ(last.GetOwner(0),&unit);
    previous.Type=EventType::MegaMission;
    for(int mission:{7,8,9}) {
        previous.MegaMission.Mission=static_cast<BYTE>(mission);
        EXPECT_EQ(Game::PlanningManager_CheckCommand(&unit,&event),1);
    }
    previous.MegaMission.Mission=1;previous.MegaMission.Target.m_RTTI=11;
    EXPECT_EQ(Game::PlanningManager_CheckCommand(&unit,&event),2);
    previous.MegaMission.Mission=11;previous.MegaMission.Target.m_RTTI=0;
    EXPECT_EQ(Game::PlanningManager_CheckCommand(&unit,&event),2);
    previous.MegaMission.Mission=2;EXPECT_EQ(Game::PlanningManager_CheckCommand(&unit,&event),-1);
    event.MegaMission.Mission=static_cast<BYTE>(Mission::Area_Guard);
    EXPECT_EQ(Game::PlanningManager_CheckCommand(&unit,&event),0);
    event.Type=EventType::Deploy;EXPECT_EQ(Game::PlanningManager_CheckCommand(nullptr,&event),0);
}

TEST(PlanningValidation, OriginalTokenLoopsAndCommitPreserveEventGraph) {
    State state;
    ScenarioClass scenario;auto* old_scenario=ScenarioClass::Instance;ScenarioClass::Instance=&scenario;
    const auto restore_scenario=ra2::test::scope_exit([&]{ScenarioClass::Instance=old_scenario;});
    HouseTypeClass country("PLAN_TOKEN_HOUSE");HouseClass house(&country);
    UnitTypeClass type("PLAN_TOKEN_UNIT");UnitClass unit(&type,&house),other(&type,&house);
    unit.Location={400,600,120};other.Location={800,900,240};
    PlanningTokenClass token(&unit);
    EXPECT_EQ(token.OwnerUnit,&unit);EXPECT_EQ(token.PlanningNodes.Count,0);
    EXPECT_EQ(token.field_8C,-1);EXPECT_EQ(token.ClosedLoopNodeCount,-1);EXPECT_EQ(token.StepsToClosedLoop,-1);
    EXPECT_FALSE(token.field_1C);EXPECT_EQ(token.CurrentEvent.Type,EventType::Empty);
    EXPECT_FALSE(token.field_98);EXPECT_FALSE(token.field_99);
    EXPECT_FALSE(token.HasCommittedNodes());EXPECT_EQ(token.GetNode(0),nullptr);EXPECT_EQ(token.GetLastNode(),nullptr);
    PlanningNodeClass first{},second{},third{};
    EventClass packets[3];PlanningMemberClass members[3];
    PlanningNodeClass* nodes[]={&first,&second,&third};
    PlanningBranchClass branches[3];
    // This query fixture borrows stack members/branches. Detach them before
    // exercising the now-restored owning node destructor.
    const auto detach=ra2::test::scope_exit([&]{token.PlanningNodes.Clear();for(auto* node:nodes){node->PlanningMembers.Clear();node->PlanningBranches.Clear();}});
    for(int i=0;i<3;++i) {
        packets[i].Type=EventType::MegaMission;packets[i].Frame=unsigned(i+100);
        packets[i].MegaMission.Target=TargetClass(i==1?&other:&unit);
        members[i]={&unit,&packets[i],-1,0};
        nodes[i]->PlanningMembers.AddItem(&members[i]);token.PlanningNodes.AddItem(nodes[i]);
        nodes[i]->PlanningBranches.AddItem(&branches[i]);
        nodes[i]->field_B0=30+i;nodes[i]->field_B4=40+i;
        nodes[i]->CommonBranch.field_74=50+i;branches[i].field_74=60+i;
        branches[i].MemberCount=2;
    }
    EXPECT_EQ(second.CommonBranch.Packet.Type,EventType::Empty);
    EXPECT_EQ(second.CommonBranch.MemberCount,0);
    CoordStruct coords;EXPECT_EQ(second.GetCoords(&coords),&coords);EXPECT_EQ(coords,other.Location);
    EXPECT_TRUE(second.IsAt(other.Location));EXPECT_FALSE(second.IsAt(unit.Location));
    EXPECT_EQ(token.GetLastNode(),&third);EXPECT_EQ(token.GetNode(-1),nullptr);
    EXPECT_EQ(token.GetNode(2),&third);EXPECT_EQ(token.GetNode(3),nullptr);
    token.StepsToClosedLoop=1;token.ClosedLoopNodeCount=2;
    EXPECT_EQ(token.GetNode(3),&second);EXPECT_EQ(token.GetNode(4),&third);EXPECT_EQ(token.GetNode(5),&second);
    EventClass output;EXPECT_EQ(token.GetEvent(&output,1),&output);
    EXPECT_EQ(std::memcmp(&output,&packets[1],sizeof(output)),0);
    EXPECT_EQ(token.GetEvent(&packets[1],1),&packets[1]);
    EXPECT_TRUE(token.HasCommittedNodes());token.field_8C=0;EXPECT_FALSE(token.HasCommittedNodes());
    token.field_8C=2;EXPECT_TRUE(token.HasCommittedNodes());token.field_98=true;token.field_99=true;
    token.Commit();
    EXPECT_EQ(token.field_8C,-1);EXPECT_FALSE(token.field_98);EXPECT_TRUE(token.field_99);
    EXPECT_EQ(first.field_B4,40);EXPECT_EQ(branches[0].field_74,60);
    for(int i:{1,2}) {
        EXPECT_EQ(nodes[i]->field_B4,-1);EXPECT_EQ(nodes[i]->CommonBranch.field_74,-1);
        EXPECT_EQ(branches[i].field_74,-1);EXPECT_EQ(branches[i].MemberCount,2);
        EXPECT_EQ(nodes[i]->field_B0,30+i);
    }
    EXPECT_EQ(token.PlanningNodes.Count,3);EXPECT_EQ(token.GetNode(3),&second);
    EXPECT_EQ(std::memcmp(&output,&packets[1],sizeof(output)),0);
    token.Commit();EXPECT_EQ(first.field_B4,-1);EXPECT_EQ(branches[0].field_74,-1);
}

TEST(PlanningValidation, PlannedNodeExecutionKeepsRawStateAndTokenGraph) {
    State state;
    ScenarioClass scenario;auto* old_scenario=ScenarioClass::Instance;ScenarioClass::Instance=&scenario;
    const auto restore_scenario=ra2::test::scope_exit([&]{ScenarioClass::Instance=old_scenario;});
    HouseTypeClass country("PLAN_EXEC_HOUSE");HouseClass house(&country);HouseClass::CurrentPlayer=&house;
    UnitTypeClass type("PLAN_EXEC_UNIT");UnitClass unit(&type,&house);
    PlanningTokenClass token(&unit);PlanningNodeClass node{};
    token.PlanningNodes.AddItem(&node);token.field_1C=true;unit.PlanningToken=&token;
    const auto detach=ra2::test::scope_exit([&]{unit.PlanningToken=nullptr;token.PlanningNodes.Clear();});
    EventClass event;event.Type=EventType::MegaMission;
    event.MegaMission.Whom=TargetClass(&unit);event.MegaMission.Mission=static_cast<BYTE>(Mission::Move);
    event.MegaMission.IsPlanningEvent=2;
    unit.IsAlive=true;unit.Health=100;unit.InLimbo=false;
    unit.CurrentMission=Mission::Guard;unit.QueuedMission=Mission::None;
    event.Execute();
    EXPECT_EQ(event.MegaMission.IsPlanningEvent,2);
    EXPECT_EQ(unit.QueuedMission,Mission::Move);
    EXPECT_EQ(unit.PlanningToken,&token);EXPECT_EQ(token.PlanningNodes.Count,1);
    EXPECT_EQ(token.PlanningNodes[0],&node);EXPECT_FALSE(token.field_1C);
}

TEST(PlanningValidation, OwnedNodeMembersShareBranchesAndRetireOriginalRegistries) {
    State state;
    ASSERT_EQ(PlanningNodeClass::Unknown1.Count,0);ASSERT_EQ(PlanningNodeClass::Unknown2.Count,0);
    ASSERT_EQ(PlanningNodeClass::Unknown3.Count,0);
    ScenarioClass scenario;auto* old_scenario=ScenarioClass::Instance;ScenarioClass::Instance=&scenario;
    const auto restore_scenario=ra2::test::scope_exit([&]{ScenarioClass::Instance=old_scenario;});
    HouseTypeClass country("PLAN_GRAPH_HOUSE");HouseClass house(&country);
    UnitTypeClass type("PLAN_GRAPH_UNIT");UnitClass first(&type,&house),second(&type,&house);
    first.Location={100,200,30};second.Location={400,500,60};
    PlanningTokenClass a(&first),b(&second);first.PlanningToken=&a;second.PlanningToken=&b;
    PlanningNodeClass* nodes[3]{};
    const auto cleanup=ra2::test::scope_exit([&]{
        first.PlanningToken=second.PlanningToken=nullptr;a.PlanningNodes.Clear();b.PlanningNodes.Clear();
        for(auto* node:nodes)if(node) {
            // Token-side iteration/count maintenance is restored separately;
            // the node now owns removal and deletes itself at its last member.
            while(node->PlanningMembers.Count>1)node->RemoveMember(node->PlanningMembers[0]->Owner);
            if(node->PlanningMembers.Count)node->RemoveMember(node->PlanningMembers[0]->Owner);
            else GameDelete(node);
        }
        Game::PlanningNodeAtAC4CCC=Game::PlanningNodeAtAC4C38=Game::PlanningNodeAtAC4BF0=nullptr;
    });
    for(auto*& node:nodes) {
        node=GameCreate<PlanningNodeClass>();node->field_18=40;node->field_1C=false;node->field_B0=node->field_B4=-1;
        ASSERT_TRUE(PlanningNodeClass::Unknown1.AddItem(node));
    }
    EventClass event;event.Type=EventType::MegaMission;event.Frame=37;
    event.MegaMission.IsPlanningEvent=1;event.MegaMission.Mission=static_cast<BYTE>(Mission::Move);
    event.MegaMission.Target=TargetClass(&first);
    EXPECT_EQ(nodes[0]->AddMember(&first,&event),-1);EXPECT_EQ(nodes[0]->AddMember(&second,&event),-1);
    EXPECT_EQ(a.field_8C,0);EXPECT_EQ(b.field_8C,0);
    EXPECT_EQ(nodes[1]->AddMember(&first,&event),0);
    event.MegaMission.Target=TargetClass(&second);
    EXPECT_EQ(nodes[1]->AddMember(&second,&event),0);
    ASSERT_EQ(nodes[1]->PlanningBranches.Count,1);
    EXPECT_EQ(nodes[1]->PlanningBranches[0]->MemberCount,2);
    EXPECT_NE(nodes[1]->PlanningMembers[0]->Packet,&event);
    EXPECT_EQ(nodes[1]->PlanningMembers[0]->Packet->MegaMission.Target.As_Techno(),&first);
    EXPECT_EQ(nodes[1]->PlanningMembers[1]->Packet->MegaMission.Target.As_Techno(),&second);
    event.MegaMission.Target=TargetClass(&first);
    EXPECT_EQ(nodes[2]->AddMember(&first,&event),0);EXPECT_EQ(nodes[2]->AddMember(&second,&event),1);
    ASSERT_EQ(nodes[2]->PlanningBranches.Count,2);
    EXPECT_EQ(nodes[2]->PlanningBranches[0]->MemberCount,1);EXPECT_EQ(nodes[2]->PlanningBranches[1]->MemberCount,1);
    EXPECT_EQ(nodes[0]->GetPrevious(&first),nodes[2]);EXPECT_EQ(nodes[2]->GetNext(&first),nodes[0]);
    a.StepsToClosedLoop=b.StepsToClosedLoop=1;a.ClosedLoopNodeCount=b.ClosedLoopNodeCount=2;
    nodes[1]->UpdateLoopBranch();
    EXPECT_EQ(nodes[1]->CommonBranch.MemberCount,2);EXPECT_EQ(nodes[1]->field_B0,0);
    EXPECT_EQ(nodes[1]->PlanningMembers[0]->field_C,1);EXPECT_EQ(nodes[1]->PlanningMembers[1]->field_C,1);
    auto* branch=nodes[1]->PlanningBranches[0];branch->field_74=9;nodes[1]->CommonBranch.field_74=8;
    nodes[1]->field_B4=7;nodes[1]->InvalidateDisplay(false);
    EXPECT_EQ(nodes[1]->field_B4,-1);EXPECT_EQ(branch->field_74,9);EXPECT_EQ(nodes[1]->CommonBranch.field_74,8);
    nodes[1]->InvalidateDisplay(true);EXPECT_EQ(branch->field_74,-1);EXPECT_EQ(nodes[1]->CommonBranch.field_74,-1);
    nodes[1]->ReleaseBranch(nodes[1]->PlanningMembers[0]);
    EXPECT_EQ(branch->MemberCount,1);EXPECT_EQ(nodes[1]->CommonBranch.MemberCount,1);EXPECT_EQ(PlanningNodeClass::Unknown3.Count,0);
    nodes[1]->ReleaseBranch(nodes[1]->PlanningMembers[1]);
    EXPECT_EQ(branch->MemberCount,0);EXPECT_EQ(nodes[1]->CommonBranch.MemberCount,0);EXPECT_EQ(nodes[1]->field_B0,-1);
    ASSERT_EQ(PlanningNodeClass::Unknown3.Count,1);EXPECT_EQ(PlanningNodeClass::Unknown3[0],nodes[1]);
    EXPECT_EQ(nodes[1]->PlanningBranches.Count,1)<<"zero references schedule maintenance; the branch still exists";
    Game::PlanningManager_FlagNode(nodes[1]);nodes[1]->ReleaseBranch(nodes[1]->PlanningMembers[1]);
    EXPECT_EQ(PlanningNodeClass::Unknown3.Count,1);EXPECT_EQ(branch->MemberCount,0);
    nodes[1]->ClearBranches();EXPECT_EQ(nodes[1]->PlanningBranches.Count,0);EXPECT_TRUE(nodes[1]->field_1C);
    ASSERT_TRUE(PlanningNodeClass::Unknown2.AddItem(nodes[1]));
    Game::PlanningNodeAtAC4CCC=Game::PlanningNodeAtAC4C38=Game::PlanningNodeAtAC4BF0=nodes[1];
    auto* retiring=nodes[1];nodes[1]=nullptr;
    retiring->RemoveMember(&first);EXPECT_EQ(retiring->PlanningMembers.Count,1);
    retiring->RemoveMember(&second);
    EXPECT_EQ(PlanningNodeClass::Unknown1.Count,2);EXPECT_EQ(PlanningNodeClass::Unknown2.Count,0);EXPECT_EQ(PlanningNodeClass::Unknown3.Count,0);
    EXPECT_EQ(Game::PlanningNodeAtAC4CCC,nullptr);EXPECT_EQ(Game::PlanningNodeAtAC4C38,nullptr);EXPECT_EQ(Game::PlanningNodeAtAC4BF0,nullptr);
}

TEST(PlanningValidation, TokenClearAndUnitDestructionReleaseSharedGraphExactlyOnce) {
    State state;
    ASSERT_EQ(PlanningTokenClass::Array.Count,0);ASSERT_EQ(Game::PlanningUnits.Count,0);
    ASSERT_EQ(PlanningNodeClass::Unknown1.Count,0);ASSERT_FALSE(Game::PlanningDeletingAll);
    ScenarioClass scenario;auto* old_scenario=ScenarioClass::Instance;ScenarioClass::Instance=&scenario;
    const auto restore_scenario=ra2::test::scope_exit([&]{ScenarioClass::Instance=old_scenario;});
    HouseTypeClass country("PLAN_TOKEN_LIFE");HouseClass house(&country);HouseClass::CurrentPlayer=&house;
    UnitTypeClass type("PLAN_TOKEN_OWNER");UnitClass first(&type,&house);
    auto second=std::make_unique<UnitClass>(&type,&house);
    std::unique_ptr<PlanningTokenClass,GameDeleter> a(GameCreate<PlanningTokenClass>(&first));
    auto* b=GameCreate<PlanningTokenClass>(second.get());
    first.PlanningToken=a.get();second->PlanningToken=b;
    ASSERT_TRUE(PlanningTokenClass::Array.AddItem(a.get()));ASSERT_TRUE(PlanningTokenClass::Array.AddItem(b));
    ASSERT_TRUE(Game::PlanningUnits.AddItem(&first));ASSERT_TRUE(Game::PlanningUnits.AddItem(second.get()));
    Game::PlanningMemberCounts[house.ArrayIndex]=2;
    const auto make_node=[] {
        auto* node=GameCreate<PlanningNodeClass>();node->field_18=51;node->field_1C=false;
        node->field_B0=node->field_B4=-1;PlanningNodeClass::Unknown1.AddItem(node);return node;
    };
    auto* shared=make_node();auto* unique=make_node();
    EventClass plan;plan.Type=EventType::MegaMission;plan.MegaMission.IsPlanningEvent=1;
    plan.MegaMission.Target=TargetClass(&first);plan.MegaMission.Mission=static_cast<BYTE>(Mission::Move);
    shared->AddMember(&first,&plan);shared->AddMember(second.get(),&plan);unique->AddMember(&first,&plan);
    a->StepsToClosedLoop=0;a->ClosedLoopNodeCount=2;shared->UpdateLoopBranch();
    a->CurrentEvent=plan;a->field_1C=true;a->field_98=a->field_99=true;
    a->ClearNodes();
    EXPECT_EQ(first.PlanningToken,a.get());EXPECT_EQ(a->OwnerUnit,&first);EXPECT_EQ(PlanningTokenClass::Array.Count,2);
    EXPECT_EQ(a->PlanningNodes.Count,0);EXPECT_TRUE(a->field_1C)<<"ClearNodes preserves the command-in-progress byte";
    EXPECT_EQ(a->field_8C,-1);EXPECT_EQ(a->ClosedLoopNodeCount,-1);EXPECT_EQ(a->StepsToClosedLoop,-1);
    EXPECT_FALSE(a->field_98);EXPECT_FALSE(a->field_99);EXPECT_TRUE(a->CurrentEvent==plan);
    ASSERT_EQ(PlanningNodeClass::Unknown1.Count,1);EXPECT_EQ(PlanningNodeClass::Unknown1[0],shared);
    ASSERT_EQ(shared->PlanningMembers.Count,1);EXPECT_EQ(shared->GetOwner(0),second.get());
    EXPECT_EQ(shared->CommonBranch.MemberCount,0);EXPECT_EQ(Game::PlanningMemberCounts[house.ArrayIndex],1);
    ASSERT_EQ(Game::PlanningUnits.Count,1);EXPECT_EQ(Game::PlanningUnits[0],second.get());
    a->ClearNodes();EXPECT_EQ(Game::PlanningMemberCounts[house.ArrayIndex],1);

    // Recreate the original manager's admission bookkeeping, then exercise
    // the actual event caller. Manager admission itself is restored separately.
    shared->AddMember(&first,&plan);Game::PlanningUnits.AddItem(&first);
    ++Game::PlanningMemberCounts[house.ArrayIndex];a->field_1C=true;
    plan.MegaMission.IsPlanningEvent=2;first.ClearPlanningTokens(&plan);
    EXPECT_EQ(a->PlanningNodes.Count,1);EXPECT_FALSE(a->field_1C);EXPECT_EQ(Game::PlanningMemberCounts[house.ArrayIndex],2);
    a->field_1C=true;
    EventClass order;order.Type=EventType::MegaMission;order.MegaMission.Whom=TargetClass(&first);
    order.MegaMission.Mission=static_cast<BYTE>(Mission::Move);order.MegaMission.IsPlanningEvent=0;
    first.IsAlive=true;first.Health=100;first.InLimbo=false;first.CurrentMission=Mission::Guard;first.QueuedMission=Mission::None;
    order.Execute();
    EXPECT_EQ(first.QueuedMission,Mission::Move);EXPECT_EQ(a->PlanningNodes.Count,0);EXPECT_FALSE(a->field_1C);
    EXPECT_EQ(first.PlanningToken,a.get());EXPECT_EQ(Game::PlanningMemberCounts[house.ArrayIndex],1);
    a.reset();EXPECT_EQ(first.PlanningToken,nullptr);EXPECT_EQ(PlanningTokenClass::Array.Count,1);
    Game::PlanningNodeAtAC4CCC=Game::PlanningNodeAtAC4C38=Game::PlanningNodeAtAC4BF0=shared;
    second.reset(); // Unit -> Techno destructor -> token.Destroy -> owning graph cleanup.
    EXPECT_EQ(PlanningTokenClass::Array.Count,0);EXPECT_EQ(Game::PlanningUnits.Count,0);
    EXPECT_EQ(PlanningNodeClass::Unknown1.Count,0);EXPECT_EQ(PlanningNodeClass::Unknown2.Count,0);EXPECT_EQ(PlanningNodeClass::Unknown3.Count,0);
    EXPECT_EQ(Game::PlanningMemberCounts[house.ArrayIndex],0);
    EXPECT_EQ(Game::PlanningNodeAtAC4CCC,nullptr);EXPECT_EQ(Game::PlanningNodeAtAC4C38,nullptr);EXPECT_EQ(Game::PlanningNodeAtAC4BF0,nullptr);
}

TEST(PlanningValidation, BulkTokenDestructionKeepsRegistryForItsOuterOwner) {
    State state;ASSERT_EQ(PlanningTokenClass::Array.Count,0);ASSERT_FALSE(Game::PlanningDeletingAll);
    HouseTypeClass country("PLAN_BULK_HOUSE");HouseClass house(&country);
    UnitTypeClass type("PLAN_BULK_UNIT");UnitClass unit(&type,&house);
    auto* token=GameCreate<PlanningTokenClass>(&unit);unit.PlanningToken=token;
    PlanningTokenClass::Array.AddItem(token);Game::PlanningDeletingAll=true;
    const auto reset=ra2::test::scope_exit([&]{Game::PlanningDeletingAll=false;PlanningTokenClass::Array.Clear();});
    GameDelete(token);
    EXPECT_EQ(unit.PlanningToken,nullptr);EXPECT_EQ(PlanningTokenClass::Array.Count,1);
    EXPECT_EQ(PlanningTokenClass::Array[0],token)<<"the original bulk-delete caller owns the registry iteration";
    EXPECT_EQ(Game::PlanningUnits.Count,0);
}
