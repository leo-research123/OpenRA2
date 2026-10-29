#include "yrpp/YRPPCore.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/EvadeClass.h"
namespace {
ScenarioClass* instance = nullptr;
int new_ini_format = 0;
bool was_game_saved = false;
int paused_audio_volume = 0x4000;
#if !defined(RA2_YRPP_GAME)
ScenarioFlags global_special_flags{};
DWORD scenario_crc = 0;
bool building_read_flag = true;
bool custom_ai = false;
EvadeClass evade{{}, 0, 0, 1, 0};
byte alternate_campaign = 0;
char alternate_filename[128]{};
#endif
}
ScenarioClass*& ScenarioClass::Instance = instance;
int& ScenarioClass::NewINIFormat = new_ini_format;
bool& ScenarioClass::WasGameSaved = was_game_saved;
int& ScenarioClass::PausedAudioVolume = paused_audio_volume;
TheaterType& ScenarioClass::LastTheater = Theater::LastTheater;
#if !defined(RA2_YRPP_GAME)
ScenarioFlags& ScenarioClass::GlobalSpecialFlags = global_special_flags;
DWORD& ScenarioClass::ScenarioCRC = scenario_crc;
bool& ScenarioClass::BuildingReadFlag = building_read_flag;
bool& ScenarioClass::CustomAI = custom_ai;
EvadeClass& EvadeClass::Instance = evade;
// Retain the historical byte alias; consumers read CarryOverGlobals as bytes.
bool& ScenarioClass::UseAlternateHome = reinterpret_cast<bool&>(evade.CarryOverGlobals[0]);
byte& ScenarioClass::AlternateCampaign = alternate_campaign;
char (&ScenarioClass::AlternateFilename)[128] = alternate_filename;
#endif
