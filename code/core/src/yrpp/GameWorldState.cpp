// Native storage for the original GameActive global at 0xA8E9A0.
// The owning world binds/restores it while operating on its object chains.
#include "yrpp/Unsorted.h"
namespace { bool game_active=false; }
bool& Game::IsActive=game_active;
namespace { bool allow_voice=true,attack_move_mode=false; }
bool& Unsorted::MoveFeedback=allow_voice;
bool& Game::AttackMoveMode=attack_move_mode;
