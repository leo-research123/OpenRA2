#include "support/test_support.hpp"
#include "yrpp/ScenarioClass.h"
#include "yrpp/AbstractClass.h"
#include "api/scenario_runtime.hpp"
#include <algorithm>
#include <array>
#include <climits>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Collection=game::ScenarioObjectCollection;
using Step=game::ScenarioDestructionStep;
constexpr int collection_count=static_cast<int>(Collection::types)+1;
struct Fixture {
    struct Object : AbstractClass {
        Fixture& world;
        int collection,key,references;
        bool indexed=false;
        Object(Fixture& w,int group,int id,int refs=1):world(w),collection(group),key(id),references(refs) {
            world.live.push_back(this);
            if(collection>=0)EXPECT_TRUE((world.groups[collection].AddItem(this))) << "register fixture object";
            UniqueID=static_cast<unsigned>(key);
        }
        HRESULT YRPP_STDCALL GetClassID(CLSID*) override{return 0;}
        HRESULT YRPP_STDCALL Load(IStream*) override{return 0;}
        HRESULT YRPP_STDCALL Save(IStream*,BOOL) override{return 0;}
        AbstractType WhatAmI() const override{return collection==0?AbstractType::Bullet:AbstractType::None;}
        int Size() const override{return sizeof(*this);}
        ULONG YRPP_STDCALL Release() override {
            ++world.release_calls;const int remaining=--references;
            if(!remaining)delete this;
            return remaining;
        }
        ~Object() override {
            if(world.running) {
                world.valid_depth=world.valid_depth&&world.depth==world.expected_depth;
                world.deleted.push_back(key);
            }
            if(indexed)world.targets.RemoveIndex(key);
            if(collection>=0)world.groups[collection].Remove(this);
            world.live.erase(std::find(world.live.begin(),world.live.end(),this));
            if(world.running&&collection==static_cast<int>(Collection::objects)&&world.spawn&&!world.spawned) {
                world.spawned=true;new Object(world,collection,999);
            }
        }
    };
    std::array<DynamicVectorClass<AbstractClass*>,collection_count> groups;
    DynamicVectorClass<AbstractClass*> notices;
    AbstractClass* borrowed[2]{};
    IndexClass<int,AbstractClass*> targets;
    std::vector<Object*> live;
    std::vector<int> deleted;
    std::vector<Step> steps;
    int depth=2,expected_depth=3,release_calls=0,spotlight_deletes=0,tactical_deletes=0;
    int spotlight_cookie=0,tactical_cookie=0;
    bool running=false,valid_depth=true,spawn=false,spawned=false,fail_query=false;
    SpotlightClass* spotlight=reinterpret_cast<SpotlightClass*>(&spotlight_cookie);
    TacticalClass* tactical=reinterpret_cast<TacticalClass*>(&tactical_cookie);
    game::ScenarioRuntimeServices runtime;
    game::ScenarioWorldServices world;
    game::ScenarioInitializeServices initialize;
    game::ScenarioDestructionServices destruction;
    static Fixture& self(void* p){return *static_cast<Fixture*>(p);}
    explicit Fixture(bool borrowed_notices=false) {
        for(int i=0;i<collection_count;++i) {
            new Object(*this,i,100+i,i==0?3:1);
            new Object(*this,i,200+i,i==0?2:1);
        }
        for(int key:{7,-1,0,INT_MAX,INT_MIN}) {
            auto* object=new Object(*this,-1,key);
            object->indexed=true;EXPECT_TRUE((targets.AddIndex(key,object))) << "register target index entry";
        }
        auto* indexed=static_cast<Object*>(groups[static_cast<int>(Collection::objects)].Items[0]);
        indexed->indexed=true;EXPECT_TRUE((targets.AddIndex(indexed->key,indexed))) << "target also belongs to object collection";
        if(borrowed_notices){notices.SetCapacity(2,borrowed);notices.Count=2;}
        else {notices.AddItem(indexed);notices.AddItem(indexed);}
        world.initialization_depth=&depth;
        initialize.context=this;initialize.tactical=&tactical;
        initialize.destroy_tactical=[](void* p,TacticalClass* object){auto& w=self(p);EXPECT_TRUE((object==w.tactical)) << "destroy current tactical";++w.tactical_deletes;};
        destruction={
            .context=this,.notices=&notices,.target_index=&targets,
            .first_object=[](void* p,Collection group,AbstractClass*& result){auto& w=self(p);
                if(w.fail_query&&group==Collection::waves)return false;
                auto& array=w.groups[static_cast<int>(group)];result=array.Count?array.Items[0]:nullptr;return true;
            },
            .first_spotlight=[](void* p,SpotlightClass*& result){result=self(p).spotlight;return true;},
            .destroy_spotlight=[](void* p,SpotlightClass* object){auto& w=self(p);EXPECT_TRUE((object==w.spotlight)) << "owning spotlight operation";w.spotlight=nullptr;++w.spotlight_deletes;},
            .step=[](void* p,Step step){auto& w=self(p);w.valid_depth=w.valid_depth&&w.depth==w.expected_depth;w.steps.push_back(step);
                if(step==Step::unload_shapes)EXPECT_TRUE((!w.notices.Count&&!w.notices.Capacity&&!w.notices.IsAllocated)) << "notices detached before unloading resources";
            }
        };
        runtime={this,[](void*) noexcept{return 0;},[](void*,bool,int){},
            [](void*,const char*,int,const wchar_t*& result){result=L"fixture";return true;}};
        runtime.world=&world;runtime.initialize=&initialize;runtime.destruction=&destruction;
    }
    ~Fixture(){running=false;while(!live.empty())delete live.back();}
    void run() {
        struct Call {Fixture* fixture;int result=0;bool threw=false;} call{this};
        running=true;
        EXPECT_TRUE((game::with_scenario_runtime(runtime,[](void* p){auto& call=*static_cast<Call*>(p);auto& w=*call.fixture;
            try{call.result=ScenarioClass::DestroyWorldObjects();}
            catch(const std::logic_error&){EXPECT_TRUE((w.fail_query)) << "unexpected destruction query failure";call.threw=true;}
        },&call))) << "bind destruction dependencies";
        running=false;
        EXPECT_TRUE((valid_depth&&depth==2)) << "balanced initialization depth, including failure unwinding";
        if(fail_query){EXPECT_TRUE((call.threw&&!live.empty())) << "failed collection query leaves an explicit partial transaction";return;}
        EXPECT_TRUE((!call.threw&&call.result==2&&live.empty())) << "destroy all registered objects and return original depth";
        EXPECT_TRUE((targets.Count()==0&&!targets.Archive&&targets.IsSorted)) << "sorted target index bookkeeping";
        EXPECT_TRUE((std::equal(deleted.begin(),deleted.begin()+6,std::array<int,6>{INT_MIN,-1,0,7,101,INT_MAX}.begin()))) << "signed target order, before type collection destruction";
        EXPECT_TRUE((release_calls==5&&spotlight_deletes==1&&tactical_deletes==1&&!spotlight&&!tactical)) << "COM release and distinct object ownership operations";
        EXPECT_TRUE((std::count(steps.begin(),steps.end(),Step::expired_objects)==32)) << "expiration after every collection and final map cleanup";
        EXPECT_TRUE((steps.front()==Step::unload_shapes&&steps.back()==Step::beacons)) << "resource cleanup boundaries";
        auto bolts=std::find(steps.begin(),steps.end(),Step::electric_bolts);
        EXPECT_TRUE((bolts!=steps.end()&&bolts[1]==Step::line_trails&&bolts[2]==Step::lasers&&bolts[3]==Step::expired_objects&&bolts[4]==Step::map_objects)) << "effect, abstract type and map cleanup order";
        if(spawn)EXPECT_TRUE((spawned&&std::find(deleted.begin(),deleted.end(),999)!=deleted.end())) << "consume objects added during a destructor";
    }
};
}

TEST(ScenarioDestruction, OwnedNoticeSlots) {
    Fixture owned;
    owned.run();
    EXPECT_TRUE((!owned.notices.Items)) << "owned notice slots released";
}

TEST(ScenarioDestruction, BorrowedNoticeSlots) {
    Fixture borrowed(true);
    borrowed.run();
    EXPECT_TRUE((borrowed.notices.Items==borrowed.borrowed)) << "borrowed notice slots retained";
}

TEST(ScenarioDestruction, LiveCollectionChanges) {
    Fixture changed;
    changed.spawn=true;
    changed.run();
}

TEST(ScenarioDestruction, QueryFailure) {
    Fixture failed;
    failed.fail_query=true;
    failed.run();
}
