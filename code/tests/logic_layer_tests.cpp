#include "support/test_support.hpp"
#include "yrpp/MapClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/Unsorted.h"
#include "game_loop.hpp"
#include <fstream>
#include <vector>
#include <memory>
#include <limits>

namespace {
struct Object : TerrainClass {
 int key,identity;std::vector<int>* calls=nullptr;
 Object(int value,int id=0):TerrainClass(nullptr,CellStruct{}),key(value),identity(id){}
 int GetYSort() const override {if(calls)calls->push_back(identity);return key;}
};
struct FailingLayer : LayerClass {
 bool SetCapacity(int,ObjectClass**) override {return false;}
};
}
TEST(LogicLayer, StableSortedInsertAndIncrementalPass) {
 Object a(3,1),b(1,2),c(2,3),equal(2,4);LayerClass layer;
 ASSERT_TRUE(layer.AddObject(&a,false));ASSERT_TRUE(layer.AddObject(&b,false));ASSERT_TRUE(layer.AddObject(&c,false));
 std::vector<int> calls;a.calls=b.calls=c.calls=&calls;
 layer.Sort();EXPECT_EQ(layer[0],&b);EXPECT_EQ(layer[1],&c);EXPECT_EQ(layer[2],&a);
 EXPECT_EQ(calls,(std::vector<int>{2,1,3,1}));calls.clear();
 equal.calls=&calls;ASSERT_TRUE(layer.AddObject(&equal,true));
 EXPECT_EQ(layer[2],&equal);EXPECT_EQ(layer[1],&c);EXPECT_EQ(layer[3],&a);
 EXPECT_EQ(calls,(std::vector<int>{4,2,4,3,4,1}));
 LayerClass reversed;reversed.AddObject(&a,false);reversed.AddObject(&c,false);reversed.AddObject(&b,false);
 reversed.Sort();EXPECT_EQ(reversed[0],&c);EXPECT_EQ(reversed[1],&b);EXPECT_EQ(reversed[2],&a);
 // One pass is intentionally NOT fully sorted.
 reversed.Sort();EXPECT_EQ(reversed[0],&b);EXPECT_EQ(reversed[1],&c);
 a.calls=b.calls=c.calls=equal.calls=nullptr;
}
TEST(LogicLayer, GrowthFailureAndBorrowedStorage) {
 Object a(1),b(2);ObjectClass* slots[1]{};LayerClass borrowed;
 ASSERT_TRUE(borrowed.SetCapacity(1,slots));ASSERT_TRUE(borrowed.AddObject(&a,true));
 EXPECT_FALSE(borrowed.AddObject(&b,true));EXPECT_EQ(borrowed.Count,1);EXPECT_EQ(slots[0],&a);
 FailingLayer failing;EXPECT_FALSE(failing.AddObject(&a,true));EXPECT_FALSE(failing.AddObject(&a,false));EXPECT_EQ(failing.Count,0);
 LayerClass disabled;disabled.CapacityIncrement=0;EXPECT_FALSE(disabled.AddObject(&a,true));
 EXPECT_FALSE(disabled.AddObject(&a,false));
}
TEST(LogicLayer, MembershipFlagsAndFirstOccurrenceRemoval) {
 Object a(3),b(1),c(2);LogicClass logic;
 ASSERT_TRUE(logic.AddObject(&a,true));ASSERT_TRUE(logic.AddObject(&b,true));ASSERT_TRUE(logic.AddObject(&c,true));
 EXPECT_TRUE(a.IsInLogic);EXPECT_EQ(logic.Count,3);EXPECT_EQ(logic[0],&b);EXPECT_EQ(logic[2],&a);
 ASSERT_TRUE(logic.AddObject(&a,false));EXPECT_EQ(logic.Count,3);
 logic.RemoveObject(&c);EXPECT_FALSE(c.IsInLogic);EXPECT_EQ(logic.Count,2);EXPECT_EQ(logic[1],&a);
 logic.AddItem(&a);logic.RemoveObject(&a);EXPECT_FALSE(a.IsInLogic);EXPECT_EQ(logic.Count,2);
 EXPECT_EQ(logic[1],&a); // original removes first match, not every duplicate
 logic.RemoveObject(&a);EXPECT_EQ(logic.Count,2);
 logic.RemoveObject(&b);logic.Clear();c.IsInLogic=true;logic.RemoveObject(&c);EXPECT_FALSE(c.IsInLogic);
 logic.CapacityIncrement=0;EXPECT_FALSE(logic.AddObject(&c,true));EXPECT_FALSE(c.IsInLogic);
}
TEST(LogicLayer, GlobalMembershipEndsAtObjectDestruction) {
 auto& logic=LogicClass::Instance;const int before=logic.Count;
 {
  auto object=std::make_unique<Object>(7);
  ASSERT_TRUE(logic.AddObject(object.get(),false));EXPECT_EQ(logic.Count,before+1);
 }
 EXPECT_EQ(logic.Count,before);
}
TEST(LogicLayer, OriginalInstructionCorpus) {
 std::ifstream input(RA2_LAYER_FIXTURE);ASSERT_TRUE(input.good());int size,sorted,passes;unsigned cases=0;
 while(input>>size>>sorted>>passes) {
  LayerClass layer;std::vector<std::unique_ptr<Object>> objects;
  for(int i=0;i<size;++i){int key;ASSERT_TRUE(bool(input>>key));objects.push_back(std::make_unique<Object>(key,i));
   ASSERT_TRUE(layer.AddObject(objects.back().get(),sorted!=0));}
  for(int i=0;i<passes;++i)layer.Sort();
  for(int i=0;i<size;++i){int expected;ASSERT_TRUE(bool(input>>expected));EXPECT_EQ(layer[i],objects[expected].get())<<cases;}
  ++cases;
 }
 EXPECT_EQ(cases,96u);
}

TEST(LogicLayer, AnimationPlacementAttachmentAndExpiration) {
 const bool active=Game::IsActive;const int init=Unsorted::ScenarioInit;
 const auto restore=ra2::test::scope_exit([&]{Game::IsActive=active;Unsorted::ScenarioInit=init;});
 Game::IsActive=true;Unsorted::ScenarioInit=1;
 AnimTypeClass type("LAYERANIM");type.End=type.LoopEnd=4;type.Layer=Layer::Air;type.IsLogic=true;
 Object owner(0);owner.Location={256,512,104};
 auto& ground=DisplayClass::ObjectsInLayers[int(Layer::Ground)];
 auto& air=DisplayClass::ObjectsInLayers[int(Layer::Air)];
 const int grounds=ground.Count,airs=air.Count,logic=LogicClass::Instance.Count;
 {
  AnimClass animation(&type,{768,1024,208});
  EXPECT_TRUE(animation.IsOnMap);EXPECT_FALSE(animation.InLimbo);
  EXPECT_EQ(animation.LastLayer,Layer::Air);EXPECT_EQ(air.Count,airs+1);
  EXPECT_EQ(LogicClass::Instance.Count,logic+1);
  const auto location=animation.GetCoords();
  animation.SetOwnerObject(&owner);
  EXPECT_EQ(animation.GetCoords(),location);EXPECT_EQ(animation.LastLayer,Layer::Ground);
  EXPECT_EQ(ground.Count,grounds+1);EXPECT_EQ(air.Count,airs);
  EXPECT_TRUE(owner.HasParachute);
  animation.SetOwnerObject(&owner);EXPECT_EQ(ground.Count,grounds+1);
  animation.SetOwnerObject(nullptr);EXPECT_EQ(animation.GetCoords(),location);
  EXPECT_FALSE(owner.HasParachute);EXPECT_EQ(animation.LastLayer,Layer::Air);
  animation.SetOwnerObject(&owner);animation.PointerExpired(&owner,false);
  EXPECT_EQ(animation.OwnerObject,nullptr);EXPECT_TRUE(animation.UnableToContinue);
  EXPECT_EQ(animation.LastLayer,Layer::None);EXPECT_EQ(ground.Count,grounds);
 }
 EXPECT_EQ(ground.Count,grounds);EXPECT_EQ(air.Count,airs);EXPECT_EQ(LogicClass::Instance.Count,logic);
 Game::IsActive=false;
 AnimClass detached(&type,{768,1024,208});
 EXPECT_FALSE(detached.IsOnMap);EXPECT_TRUE(detached.InLimbo);EXPECT_EQ(air.Count,airs);
}

TEST(LogicLayer, MainLoopSortsOnceAfterRenderBeforeLogic) {
 Object high(3),middle(2),low(1);
 auto& ground=DisplayClass::ObjectsInLayers[int(Layer::Ground)];ASSERT_EQ(ground.Count,0);
 ground.AddObject(&high,false);ground.AddObject(&middle,false);ground.AddObject(&low,false);
 const auto restore=ra2::test::scope_exit([&]{ground.Clear();});
 struct State{LayerClass& layer;Object* high;Object* middle;Object* low;int renders=0,logics=0;} state{ground,&high,&middle,&low};
 game::GameLoopState loop;
 game::GameLoopContext context{loop,false,&state,nullptr,
  [](void* p){auto& s=*static_cast<State*>(p);if(!s.renders++)EXPECT_EQ(s.layer[0],s.high);return true;},
  [](void* p){auto& s=*static_cast<State*>(p);++s.logics;EXPECT_EQ(s.layer[0],s.middle);EXPECT_EQ(s.layer[1],s.low);EXPECT_EQ(s.layer[2],s.high);return true;}};
 ASSERT_TRUE(game::advance_game_loop(context,0));EXPECT_EQ(state.logics,1);
 // A paused iteration renders, but does not perform another sorting pass.
 context.paused=true;ASSERT_TRUE(game::advance_game_loop(context,1));
 EXPECT_EQ(state.renders,2);EXPECT_EQ(state.logics,1);EXPECT_EQ(ground[0],&middle);
}
