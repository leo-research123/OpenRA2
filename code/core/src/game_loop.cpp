#include "game_loop.hpp"
#include "clock.hpp"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/Unsorted.h"
#include <cmath>
namespace game {
namespace { thread_local GameLoopContext* active_loop=nullptr; }
GameLoopContext* current_game_loop() noexcept { return active_loop; }
bool advance_game_loop(GameLoopContext& context,double seconds) noexcept {
 auto& s=context.state;
 if(active_loop||!std::isfinite(seconds)||seconds<0||
    (s.mode!=GameMode::Campaign&&s.mode!=GameMode::Skirmish)||
    GameOptionsClass::Instance.GameSpeed<0||GameOptionsClass::Instance.GameSpeed>6)return false;
 // Host-supplied virtual time is advanced once before any frame consumer.
 // It outlives map sessions, including the original global Radar constructor.
 if(!advance_clock(seconds))return false;
 s.elapsed_seconds+=seconds;
 if(!s.focused)return true;
 try {
   if(s.started&&s.frame_timer.InProgress())return true;
   active_loop=&context;
   struct Restore{~Restore(){active_loop=nullptr;}} restore;
   ClockReadScope clock_scope;
   Game::MainLoop();
   return context.success;
 }catch(...){return false;}
}
}
