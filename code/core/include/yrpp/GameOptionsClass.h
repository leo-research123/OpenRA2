#pragma once

#include "yrpp/ASMMacros.h"
#include "yrpp/Helpers/CompileTime.h"

class GameOptionsClass
{
public:
    /// Global VA: 0x00A8EB60.
    static GameOptionsClass& Instance;
    /// Global VA: 0x0089F978.
    DEFINE_REFERENCE(bool, WindowedMode, 0x89F978u)

    /// VA: 0x005FB2E0.
#if defined(RA2_YRPP_GAME)
    int GetAnimSpeed(int rate) { JMP_THIS(0x5FB2E0); }
#else
    int GetAnimSpeed(int rate);
#endif

    int GameSpeed;
    int Difficulty;
    int CampDifficulty;
    int ScrollMethod;
    int ScrollRate;
    bool AutoScroll;
    int DetailLevel;
    bool SidebarMode; // True -> right False -> left
    bool SidebarCameoText;
    bool UnitActionLines;
    bool ShowHidden;
    bool Tooltips;
    int ScreenWidth;
    int ScreenHeight;
    int ShellWidth;
    int ShellHeight;
    bool StretchMovies;
    bool AllowHiResModes;
    bool AllowVRAMSidebar;
    float SoundVolume;
    float VoiceVolume;
    float MovieVolume;
    bool IsScoreRepeat;
    bool InGameMusic;
    bool IsScoreShuffle;
    short SoundLatency;
    short Socket;
    int LastUnlockedSovMovie;
    int LastUnlockedAllMovie;
    int NetCard;
    char NetID[32];
    int unknown_int_78;
    int unknown_int_7C;
    int unknown_int_80;
    int unknown_int_84;
    int unknown_int_88;
    int unknown_int_8C;
    int unknown_int_90;
    int unknown_int_94;

    // virtual key constants, each of them doubled
    // defaulting to VK_MENU, VK_CONTROL and VK_SHIFT

    int KeyForceMove1;
    int KeyForceMove2;
    int KeyForceFire1;
    int KeyForceFire2;
    int KeyForceSelect1;
    int KeyForceSelect2;

    int KeyQueueMove1;
    int KeyQueueMove2;
};
