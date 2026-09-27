#include "support/test_support.hpp"
#include "yrpp/InfantryClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/HouseTypeClass.h"
#include "yrpp/WalkLocomotionClass.h"
#include "yrpp/MapClass.h"
#include "scenario_runtime.hpp"
#include <array>
#include <fstream>
#include <bit>
#include <cfenv>
#include <cstring>

namespace {
struct Actor:InfantryClass {
    int mode=0,flags=0,error=0;mutable std::array<int,8> calls{0,-1,0,0,0,0,0,0};
    WeaponStruct weapon{};
    Actor(InfantryTypeClass* type,HouseClass* owner):InfantryClass(type,owner){}
    bool PlayAnim(Sequence action,bool force,bool random)override {
        ++calls[0];calls[1]=int(action);EXPECT_FALSE(force);EXPECT_FALSE(random);
        if(mode==0 || !(flags&16384)){SequenceAnim=action;Animation.Value=0;return true;}
        return false;
    }
    void Scatter(const CoordStruct& at,bool forced,bool noKidding)override{
        ++calls[2];EXPECT_EQ(at,CoordStruct::Empty);EXPECT_TRUE(forced);EXPECT_FALSE(noKidding);
    }
    bool IsArmed()const override{return flags&32;}
    int SelectWeapon(AbstractClass*)const override{return bool(flags&4);}
    FireError GetFireError(AbstractClass*,int,bool ignore)const override{
        EXPECT_TRUE(ignore);++calls[3];return static_cast<FireError>(calls[3]==1 || (flags&8)?error:0);
    }
    BulletClass* Fire(AbstractClass*,int)override{++calls[4];if(flags&8192)Target=nullptr;return nullptr;}
    void Uncloak(bool sound)override{++calls[5];EXPECT_FALSE(sound);}
    bool StopMoving()override{++calls[6];return false;}
    void SetTarget(AbstractClass* target)override{++calls[7];Target=target;}
    WeaponStruct* GetWeapon(int)const override{return const_cast<WeaponStruct*>(&weapon);}
};
struct Driver:WalkLocomotionClass {
    bool jumpjet=false;
    HRESULT YRPP_STDCALL GetClassID(CLSID* id)override{*id=jumpjet?CLSIDs::Jumpjet:CLSIDs::Walk;return 0;}
};
}

TEST(InfantryFrameDecisions, OriginalFearAndFiringCorpus) {
    const int rounding=std::fegetround();auto restoreRound=ra2::test::scope_exit([&]{std::fesetround(rounding);});
    std::fesetround(FE_TOWARDZERO);
    auto runtime=game::default_scenario_runtime();runtime.session_mode=[](void*)noexcept{return 0;};
    ASSERT_TRUE(game::with_scenario_runtime(runtime,[](void*){
        RulesClass rules;auto* previousRules=RulesClass::Instance;const int previousFrame=Unsorted::CurrentFrame;
        RulesClass::Instance=&rules;Unsorted::CurrentFrame=500;rules.ConditionGreen=0.75;rules.Incoming=0;
        auto restore=ra2::test::scope_exit([&]{RulesClass::Instance=previousRules;Unsorted::CurrentFrame=previousFrame;});
        HouseTypeClass country("FRAME_COUNTRY");HouseClass owner(&country);InfantryTypeClass type("FRAME_GI");type.Strength=100;
        SequenceStruct sequence{};type.Sequence=&sequence;
        auto restoreType=ra2::test::scope_exit([&]{type.Sequence=nullptr;});
        WeaponTypeClass weapon("FRAME_GUN");weapon.Speed=1000;weapon.AmbientDamage=0;
        Actor actor(&type,&owner);InfantryClass other(&type,&owner);actor.weapon.WeaponType=&weapon;
        auto* driver=GameCreate<Driver>();ASSERT_NE(driver,nullptr);driver->AddRef();driver->Link_To_Object(&actor);actor.Locomotor=driver;
        actor.Location={8*256+192,6*256+64,0};other.Location={11*256+128,9*256+128,0};
        std::ifstream input(RA2_INFANTRY_FEAR_FIRE_FIXTURE);ASSERT_TRUE(input.good());int mode;unsigned cases=0;
        while(input>>mode){
            int action,phase,flags;ASSERT_TRUE(bool(input>>action>>phase>>flags));actor.mode=mode;actor.flags=flags;
            actor.calls={0,-1,0,0,0,0,0,0};actor.SequenceAnim=static_cast<Sequence>(action);
            actor.PrimaryFacing.SetCurrent(DirStruct(0x1234));actor.PrimaryFacing.SetROT(127);driver->jumpjet=flags&1024;
            if(mode==0){
                std::array<int,6> expected;for(auto& value:expected)ASSERT_TRUE(bool(input>>value));
                actor.PanicDurationLeft=static_cast<unsigned>(phase);type.Fearless=flags&1;actor.Crawling=flags&2;
                type.Fraidycat=flags&4;owner.IsHumanPlayer=flags&8;actor.Destination=flags&16?&other:nullptr;
                actor.Ammo=flags&64?1:0;type.Ammo=7;driver->IsMoving=flags&128;actor.IsFallingDown=flags&256;
                actor.Fear_AI();
                const std::array<int,6> actual{std::bit_cast<int>(actor.PanicDurationLeft),actor.Ammo,int(actor.SequenceAnim),actor.calls[0],actor.calls[1],actor.calls[2]};
                ASSERT_EQ(actual,expected)<<"fear action "<<action<<" panic "<<phase<<" flags "<<flags;
            }else{
                std::array<int,17> expected;for(auto& value:expected)ASSERT_TRUE(bool(input>>value));
                actor.error=phase;actor.Target=flags&1?nullptr:&other;actor.IsFiring=flags&2;actor.Crawling=flags&16;
                type.FireUp=3;type.FireProne=4;type.SecondaryFire=5;type.SecondaryProne=6;
                sequence.Sequences[int(Sequence::SecondaryFire)].CountFrames=flags&32?5:0;
                sequence.Sequences[int(Sequence::SecondaryProne)].CountFrames=flags&64?5:0;
                actor.Destination=flags&128?&other:nullptr;
                actor.Airstrike=flags&256?reinterpret_cast<AirstrikeClass*>(&other):nullptr;type.JumpJet=flags&512;
                actor.Animation.Value=(flags>>8)%7;weapon.Damage=flags&2048?-10:10;other.Health=flags&4096?100:10;
                actor.Firing_AI();actor.Airstrike=nullptr;
                // Representation contract only: the original Facing timer is
                // at +0x8/+0x10; do not add public mutable timer access for tests.
                static_assert(sizeof(FacingClass)==24);
                std::array<int,6> facing{};std::memcpy(facing.data(),&actor.PrimaryFacing,sizeof(FacingClass));
                const std::array<int,17> actual{int(actor.SequenceAnim),actor.Animation.Value,actor.IsFiring,int(actor.Target!=nullptr),int(actor.Destination!=nullptr),
                    actor.PrimaryFacing.Desired().Raw,facing[2],facing[4],int(driver->RefCount),
                    actor.calls[0],actor.calls[1],actor.calls[2],actor.calls[3],actor.calls[4],actor.calls[5],actor.calls[6],actor.calls[7]};
                ASSERT_EQ(actual,expected)<<"fire action "<<action<<" error "<<phase<<" flags "<<flags;
            }
            ++cases;
        }
        EXPECT_EQ(cases,50688u);
    },nullptr));
}

TEST(InfantryFrameDecisions, WarpPreludeDoesNotRunNormalMissionOrFootFrame) {
    RulesClass rules;auto* previous=RulesClass::Instance;const int frame=Unsorted::CurrentFrame;
    RulesClass::Instance=&rules;rules.ChronoSparkle1=nullptr;Unsorted::CurrentFrame=25;
    auto restore=ra2::test::scope_exit([&]{RulesClass::Instance=previous;Unsorted::CurrentFrame=frame;});
    InfantryTypeClass type("WARP_FRAME");Actor actor(&type,nullptr);InfantryClass other(&type,nullptr);
    struct WarpDriver:WalkLocomotionClass {
        int processed=0;bool kill=false;
        bool YRPP_STDCALL Process()override{++processed;if(kill)LinkedTo->IsAlive=false;return true;}
    };
    auto* driver=GameCreate<WarpDriver>();ASSERT_NE(driver,nullptr);driver->AddRef();driver->Link_To_Object(&actor);actor.Locomotor=driver;
    struct DestinationActor:Actor {
        using Actor::Actor;
        void SetDestination(AbstractClass* value,bool immediate)override{EXPECT_TRUE(immediate);Destination=value;}
    } isolated(&type,nullptr);
    driver->Link_To_Object(&isolated);actor.Locomotor=nullptr;isolated.Locomotor=driver;
    for(int bits=0;bits<8;++bits){
        isolated.calls={0,-1,0,0,0,0,0,0};isolated.IsAlive=true;isolated.BeingWarpedOut=true;
        isolated.WarpingOut=bits&1;isolated.IsImmobilized=bits&2;driver->kill=bits&4;driver->processed=0;
        isolated.Target=isolated.Destination=&other;isolated.Update();
        EXPECT_EQ(driver->processed,(bits&3)?1:0);
        EXPECT_EQ(isolated.Target==nullptr,!((bits&3) && (bits&4)));
        EXPECT_EQ(isolated.Destination==nullptr,!((bits&3) && (bits&4)));
    }
}
