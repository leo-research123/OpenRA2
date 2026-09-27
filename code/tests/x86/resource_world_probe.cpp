// Emit real loaded-map INPUTS for the x86 instruction comparator. These are
// not expected outputs: both the EXE and candidate consume the same snapshot.
#include "api/filesystem.hpp"
#include "api/map_view.hpp"
#include "map_view.hpp"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/TiberiumClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/FileSystem.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <cstring>

void dump_resource_world(void* context){
 auto& out=*static_cast<std::ostream*>(context);out<<std::setprecision(17);
 const auto rect=[&](const RectangleStruct& r){out<<'['<<r.X<<','<<r.Y<<','<<r.Width<<','<<r.Height<<']';};
 auto& map=MapClass::Instance;auto& scenario=*ScenarioClass::Instance;
 out<<"{\"map_rect\":";rect(map.MapRect);out<<",\"visible_rect\":";rect(map.VisibleRect);
 unsigned flags;std::memcpy(&flags,&scenario.SpecialFlags,sizeof flags);
 out<<",\"special_flags\":"<<flags<<",\"growth_enabled\":"<<int(scenario.TiberiumGrowthEnabled);
 out<<",\"rng\":{\"disabled\":"<<int(scenario.Random.unknown_00)<<",\"next1\":"<<scenario.Random.Next1<<",\"next2\":"<<scenario.Random.Next2<<",\"table\":[";
 for(int i=0;i<250;++i){if(i)out<<',';out<<scenario.Random.Table[i];}out<<"]}";
 out<<",\"ground_buildable\":[";for(int i=0;i<12;++i){if(i)out<<',';out<<int(GroundType::Array[i].Buildable);}out<<']';
 out<<",\"tile_allow\":[";for(int i=0;i<IsometricTileTypeClass::Array.Count;++i){if(i)out<<',';out<<int(IsometricTileTypeClass::Array[i]->AllowTiberium);}out<<']';
 out<<",\"overlay_tiberium\":[";for(int i=0;i<OverlayTypeClass::Array.Count;++i){if(i)out<<',';out<<int(OverlayTypeClass::Array[i]->Tiberium);}out<<']';
 out<<",\"cells\":[";bool first=true;
 for(int i=0;i<map.Cells.Capacity;++i)if(auto* c=map.Cells[i]){
  if(!first)out<<',';first=false;
  out<<'['<<c->MapCoords.X<<','<<c->MapCoords.Y<<','<<c->OverlayTypeIndex<<','<<int(c->OverlayData)<<','<<int(c->Level)<<','<<int(c->SlopeIndex)<<','<<int(c->LandType)<<','<<c->IsoTileTypeIndex<<','<<unsigned(c->Flags)<<",[";
  bool first_object=true;
  for(auto* obj=c->FirstObject;obj;obj=obj->NextObject){
   if(!first_object)out<<',';first_object=false;
   bool invisible=false,in_game=false,spawns=false;
   if(obj->WhatAmI()==AbstractType::Building){auto* b=static_cast<BuildingClass*>(obj);invisible=b->Type->Invisible;in_game=b->Type->InvisibleInGame;}
   if(obj->WhatAmI()==AbstractType::Terrain)spawns=static_cast<TerrainClass*>(obj)->Type->SpawnsTiberium;
   out<<'['<<int(obj->WhatAmI())<<','<<obj->Health<<','<<int(invisible)<<','<<int(in_game)<<','<<int(spawns)<<']';
  }
  out<<"]]";
 }
 out<<"],\"resources\":[";first=true;
 const auto queue=[&](const TiberiumLogic& l){
  out<<"{\"allocated\":"<<l.Count<<",\"nodes\":[";
  for(int i=0;i<l.Count;++i){if(i)out<<',';const auto& n=l.Nodes[i];out<<'['<<n.MapCoord.X<<','<<n.MapCoord.Y<<','<<n.Score<<']';}
  out<<"],\"heap\":[";for(int i=1;i<=l.Queue->Count;++i){if(i>1)out<<',';out<<(l.Queue->Nodes[i]-l.Nodes);}
  out<<"],\"members\":[";bool first_member=true;
  for(int i=0;i<PriorityQueueClassNode::SurfaceDataCount();++i)if(l.CellIndexesWithTiberium[i]){if(!first_member)out<<',';first_member=false;out<<i;}
  out<<"],\"timer\":["<<l.Timer.StartTime<<','<<l.Timer.TimeLeft<<"]}";
 };
 for(auto* t:TiberiumClass::Array){
  if(!first)out<<',';first=false;
  out<<"{\"index\":"<<t->ArrayIndex<<",\"image\":"<<t->Image->ArrayIndex<<",\"frames\":"<<t->NumFrames<<",\"images\":"<<t->NumImages<<",\"slopes\":"<<t->NumSlopes<<",\"growth\":"<<t->Growth<<",\"spread\":"<<t->Spread<<",\"growth_percentage\":"<<t->GrowthPercentage<<",\"spread_percentage\":"<<t->SpreadPercentage<<",\"queues\":[";
  queue(t->SpreadLogic);out<<',';queue(t->GrowthLogic);out<<"]}";
 }
 out<<"]}\n";
}

int main(int argc,char** argv){
 if(argc!=4)return 2;
 game::ResourceHandle* resources=nullptr;game::MapViewHandle* map=nullptr;std::string error;
 if(!game::create_resources(argv[1],resources,error)||game::load_resources(*resources,{},error)!=game::ResourceLoadResult::complete){std::cerr<<error;return 3;}
 if(!game::create_map_view(*resources,map)||!game::load_map_view(*map,argv[2],unsigned(std::strlen(argv[2]))))return 4;
 std::ofstream output(argv[3]);const bool ok=game::with_map_view(*map,dump_resource_world,&output);
 game::destroy_map_view(map);FileSystem::ClearNameCache();Destroy_All_Shapes();Unload_All_Shapes();game::destroy_resources(resources);
 return ok&&output?0:5;
}
