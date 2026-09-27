// EA REDALERT/LOGIC.CPP, f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae.
// Copyright 2020 Electronic Arts Inc.; third_party/ea/LICENSE.TXT.
// YR 0x0055AFB0: growth, spread, live Logic list, then houses.
// Trigger worklist and team snapshot order: OpenTS 44fac744 logic.cpp,
// calibrated to YR 0x0055AFB0. Network logic remains outside this session.
#include "yrpp/MapClass.h"
#include "yrpp/FactoryClass.h"
#include "yrpp/SidebarClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/TiberiumClass.h"
#include "yrpp/LightSourceClass.h"
#include "yrpp/SpotlightClass.h"
#include "yrpp/AlphaShapeClass.h"
#include "map_world.hpp"
#include "yrpp/TagClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/ScenarioClass.h"
#include <vector>
namespace { LogicClass logic; }
LogicClass& LogicClass::Instance=logic;
void LogicClass::Update(){
 // Tags can remove themselves from PendingTags while firing. Preserve the
 // original live-list index progression, including the shifted successor.
 auto& scenario=*ScenarioClass::Instance;
 for(int i=0;i<PendingTags.Count;++i){
  auto* tag=PendingTags[i];
  const auto spring=[&](int kind){return tag->RaiseEvent(static_cast<TriggerEvent>(kind),nullptr,CellStruct::Empty);};
  if(scenario.VariablesChanged&&(spring(27)||spring(28)||spring(36)||spring(37)))continue;
  if(scenario.AmbientChanged&&(spring(45)||spring(46)))continue;
  if(spring(13)||spring(51))continue;
  if(scenario.MissionTimer.IsTicking()&&!scenario.MissionTimer.GetTimeLeft())spring(14);
 }
 if(scenario.MissionTimer.IsTicking()&&!scenario.MissionTimer.GetTimeLeft())scenario.MissionTimer.Stop();
 scenario.VariablesChanged=false;scenario.AmbientChanged=false;
 // A team can delete itself after its final script step. Do not iterate the
 // compacting registry directly or the following team would miss this frame.
 std::vector<TeamClass*> teams;
 for(auto* team:TeamClass::Array)teams.push_back(team);
 for(auto* team:teams)team->Update();
 TiberiumClass::UpdateGrowth();TiberiumClass::UpdateSpread();
 // 0x0055B606 re-reads Count, does not snapshot or group by object type.
 // An insertion can run in this frame; removal shifts the next slot.
 for(int i=0;i<Count;++i){
  auto* object=Items[i];object->Update();
  if(i<Count&&Items[i]==object&&object->WhatAmI()==AbstractType::Anim&&static_cast<AnimClass*>(object)->TimeToDie){
   delete object;game::map_object_changed();
  }
 }
 // 0x0055B650: purge expired alpha attachments after object updates, before
 // factories. Render may run several times without advancing this lifecycle.
 AlphaShapeClass::UpdateAll();
 // 0x55B66A: factories advance after objects and before houses.
 for(int i=0;i<FactoryClass::Array.Count;++i)FactoryClass::Array[i]->Update();
 for(auto* house:HouseClass::Array)if(house) {
  // House AI 0x004F8440 expires these one frame before zero and forces the
  // same update pass. Keeping the original ==1 boundary avoids stale power.
  if(house->PowerBlackoutTimer.GetTimeLeft()==1) {
   house->PowerBlackoutTimer.Start(0);house->RecheckPower=true;
  }
  if(house->RadarBlackoutTimer.GetTimeLeft()==1) {
   house->RadarBlackoutTimer.Start(0);house->RecheckRadar=true;
  }
  if(house->RecheckPower)house->UpdatePower();
  if(house->RecheckRadar)house->UpdateRadarAvailability();
 }
 SidebarClass::Instance.UpdateProduction();
 LightSourceClass::UpdateLightConverts(0,false);
 for(int i=SpotlightClass::Array.Count-1;i>=0;--i){SpotlightClass::Array[i]->Update();game::map_object_changed();}
}
