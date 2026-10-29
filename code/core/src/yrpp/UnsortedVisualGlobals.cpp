#include "yrpp/Unsorted.h"
// YR 0xA8ED6B, called Debug_Map by OpenTS; zero in normal gameplay.
namespace {
byte armageddonMode = 0;
}
byte& Unsorted::ArmageddonMode = armageddonMode;

// Original requested rendering rate used by radiation beam detail, 0xA8B558.
namespace {
int requestedFPS = 60;
}
int& Game::Network::RequestedFPS = requestedFPS;

// Online game identification read while the loading screen is built, 0xB779C4 and 0xB779D4.
namespace {
int tournament = 0;
DWORD wol_game_id = 0;
}
int& Game::Network::Tournament = tournament;
DWORD& Game::Network::WOLGameID = wol_game_id;

#include "yrpp/SessionClass.h"
int& Game::PlayerCount = SessionClass::Instance.MPlayerCount;
namespace { byte chatMask[8]{}; }
byte (&Game::ChatMask)[8] = chatMask;
