#pragma once

#include <wchar.h>
#include "yrpp/Helpers/CompileTime.h"

// Multiplayer capacities. The original has eight player slots, sixteen start
// assignments and twenty scored houses. Native builds widen them for large
// skirmishes: thirty players plus Neutral and Special fill the 32-bit
// one-bit-per-house masks (HouseClass::Allies, CellClass::CloakedByHouses).
#if defined(RA2_YRPP_GAME)
constexpr int MaxPlayerSlots = 8;
constexpr int MaxStartingPositions = 0x10;
constexpr int MaxScoredHouses = 20;
#else
constexpr int MaxPlayerSlots = 30;
constexpr int MaxStartingPositions = 30;
constexpr int MaxScoredHouses = 32;
#endif
constexpr int MaxHouseBits = 32;

struct AISlotsStruct {
    int Difficulties[MaxPlayerSlots];
    int Countries[MaxPlayerSlots];
    int Colors[MaxPlayerSlots];
    int Starts[MaxPlayerSlots];
    int Allies[MaxPlayerSlots];
};

class GameModeOptionsClass {
public:
    // this is the same as SessionClass::Instance.Config
    /// Global VA: 0x00A8B250.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(GameModeOptionsClass, Instance, 0xA8B250u)
#else
    static GameModeOptionsClass& Instance;
#endif

    int MPModeIndex;
    int ScenarioIndex;
    bool Bases;
    int Money;
    bool BridgeDestruction;
    bool Crates;
    bool ShortGame;
    bool SWAllowed;
    bool BuildOffAlly;
    int GameSpeed;
    bool MultiEngineer;
    int UnitCount;
    int AIPlayers;
    int AIDifficulty;
    AISlotsStruct AISlots;
    bool AlliesAllowed;
    bool HarvesterTruce;
    bool CaptureTheFlag;
    bool FogOfWar;
    bool MCVRedeploy;
    wchar_t MapDescription[45];
};
