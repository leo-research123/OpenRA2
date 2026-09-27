#include "yrpp/YRPPCore.h"
#include "yrpp/ScenarioClass.h"
namespace {
ScenarioClass* instance = nullptr;
int new_ini_format = 0;
bool was_game_saved = false;
int paused_audio_volume = 0x4000;
}
ScenarioClass*& ScenarioClass::Instance = instance;
int& ScenarioClass::NewINIFormat = new_ini_format;
bool& ScenarioClass::WasGameSaved = was_game_saved;
int& ScenarioClass::PausedAudioVolume = paused_audio_volume;
TheaterType& ScenarioClass::LastTheater = Theater::LastTheater;
