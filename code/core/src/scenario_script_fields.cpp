// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 tevent.cpp / taction.cpp Read_INI field formats.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
// Internal status adapters, not implementations of original LoadFromINI entry
// points. YR 0x71F4E0 / 0x6DD5B0 consume the caller's CRT strtok state;
// these adapters consume an explicit cursor and report malformed input.
#include "scenario_script_ini.hpp"
#include "yrpp/TActionClass.h"
#include "yrpp/TEventClass.h"
#include "yrpp/TeamTypeClass.h"
#include "yrpp/TriggerTypeClass.h"
#include "yrpp/TagTypeClass.h"
#include "yrpp/VoxClass.h"
#include "yrpp/ThemeClass.h"
#include "type_resources.hpp"
#include "rules_runtime.hpp"
#include <cstdio>
namespace game {
bool read_trigger_action_fields(TActionClass& action,const char*& cursor) noexcept {
 try {
  int kind,mode;std::string data;
  if(!game::script_integer(cursor,kind)||!game::script_integer(cursor,mode)||!game::script_token(cursor,data))return false;
  action.ActionKind=TriggerAction(kind);action.Value=0;
  switch(mode){
   case 0:case 11:if(!game::scenario_number(data,action.Value))return false;break;
   case 1:case 5:action.TeamType=game::scenario_reference<TeamTypeClass>(data);break;
   case 2:action.TriggerType=game::scenario_reference<TriggerTypeClass>(data);break;
   case 3:action.TagType=game::scenario_reference<TagTypeClass>(data);break;
   case 4:std::snprintf(action.Text,sizeof(action.Text),"%s",data=="-1"?"":data.c_str());break;
   case 6:action.Value=game::type_resources().audio_unavailable?-1:VoxClass::FindIndex(data.c_str());break;
   case 7:action.Value=-1;if(!game::type_resources().audio_unavailable&&!game::rules_sound_index(data.c_str(),action.Value))return false;break;
   case 8:action.Value=game::type_resources().audio_unavailable?-1:ThemeClass::Instance.FindIndex(data.c_str());break;
   case 9:case 10:std::snprintf(action.TechnoID,sizeof(action.TechnoID),"%s",data.c_str());break;
   default:return false;
  }
  for(auto* field:{&action.Param3,&action.Param4,&action.Param5,&action.Param6})if(!game::script_integer(cursor,*field))return false;
  if(!game::script_token(cursor,data))return false;
  if(mode==5||mode==9)return game::scenario_number(data,action.Value);
  if(mode==11)return game::scenario_number(data,action.Value2);
  return game::scenario_waypoint(data,action.Waypoint);
 }catch(...){return false;}
}

bool read_trigger_event_fields(TEventClass& event,const char*& cursor) noexcept {
 try {
  int kind,mode;std::string data;
  if(!game::script_integer(cursor,kind)||!game::script_integer(cursor,mode)||!game::script_token(cursor,data))return false;
  event.EventKind=TriggerEvent(kind);event.Value=0;
  if(mode==1){event.TeamType=TeamTypeClass::FindByNameOrID(data.c_str());return true;}
  if(mode!=0&&mode!=2)return false;
  if(!game::scenario_number(data,event.Value))return false;
  if(mode==2){if(!game::script_token(cursor,data))return false;std::snprintf(event.String,25,"%s",data.c_str());}
  return true;
 }catch(...){return false;}
}
} // namespace game
