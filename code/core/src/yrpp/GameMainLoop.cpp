// EA REDALERT/CONQUER.CPP Main_Loop, fixed revision
// f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae. Copyright 2020 Electronic Arts Inc.
// GPL-3.0-or-later plus third_party/ea/LICENSE.TXT terms.
// YR 0x0055D360 / 0x0055E160: campaign/skirmish branch. Host yields during
// the final wait; original-game builds keep the original entry and clock.
#include "yrpp/Unsorted.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/MessageListClass.h"
#include "game_loop.hpp"
#include <bit>
void YRPP_FASTCALL Game::MainLoop() {
 auto* c=game::current_game_loop();if(!c)return;
 auto& s=c->state;
 if(s.mode==GameMode::Campaign&&!s.campaign_speed_setting)GameOptionsClass::Instance.GameSpeed=2;
 s.frame_timer.Start(GameOptionsClass::Instance.GameSpeed);s.started=true;
 if(auto* scenario=c->scenario){
  // 0x0055D7D0 / 0x00684180: +0x630 is a timed-release flag;
  // +0x62C is the nested logic-pause count. The YRpp name is misleading.
  if(scenario->IsGamePaused&&!scenario->PauseTimer.HasTimeLeft()){
   scenario->IsGamePaused=false;
   if(scenario->unknown_62C&&!--scenario->unknown_62C)scenario->ElapsedTimer.Resume();
   // Original keyboard/audio resumption belongs to the full scenario services,
   // which are not active in the standalone inert display session.
  }
  c->paused=scenario->unknown_62C!=0;
 }
 // 0x0055D903 clears drag_select_aborted before the mouse input dispatch.
 // The standalone input callback currently groups polling and dispatch.
 if(!c->paused)Unsorted::DragSelectAborted=false;
 if(!c->paused&&c->input&&!c->input(c->context)){c->success=false;return;}
 // 0x0055D8F2 renders before 0x0055DC9E Logic.Update.
 if(c->render&&!c->render(c->context)){c->success=false;return;}
 if(c->paused)return; // 0x0055D826 paused path draws and waits, no frame advance.
 // 0x0055DBC8: one incremental Ground pass after rendering, before logic.
 // This belongs to the game iteration, never to each host repaint.
 MapClass::ObjectsInLayers[static_cast<int>(Layer::Ground)].Sort();
 if(c->logic&&!c->logic(c->context)){c->success=false;return;}
 // 0x0055DDA5: after logic, before CurrentFrame advances. No presentation side effects.
 MessageListClass::Instance.Manage();
 // 0x0055DE81, AFTER all frame consumers, and BEFORE the final wait.
 Unsorted::CurrentFrame=std::bit_cast<int>(static_cast<unsigned>(Unsorted::CurrentFrame)+1u);
 ++s.iterations;
}
