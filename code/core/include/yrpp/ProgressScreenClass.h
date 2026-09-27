#pragma once

#include "yrpp/GeneralDefinitions.h"
#include "yrpp/LoadProgressManager.h"
#include "yrpp/FileFormats/SHP.h"
#include "yrpp/Helpers/CompileTime.h"

class ProgressScreenClass {
public:

    /// Global VA: 0x00AC4F58.
    DEFINE_REFERENCE(ProgressScreenClass, Instance, 0xAC4F58u)

    /// VA: 0x00642B10.
    void SetSide(int idx)
        { JMP_THIS(0x642B10); }

    /// VA: 0x00642B20.
    int GetSide()
        { JMP_THIS(0x642B20); }

    int field_0;
    LoadProgressManager *LoadManager;
    double PlayerProgresses[8];
    int MainProgress;
    int field_4C;
    void *PlayerStartSpot;
    SHPStruct *someSHP;
    char field_58;
    char field_59;
    char field_5A;
    char field_5B;
    int field_5C;
    char field_60;
    byte TotalPlayers;
    char field_62;
    char field_63;
    HWND hWnd;
    int field_68;
    int field_6C;
    char field_70;
    char field_71;
    char field_72;
    char field_73;
    int field_74;
    int field_78;
    int field_7C;
    int PlayerSide; // !! this is set to campaign -> CD for singleplay

protected:
    ProgressScreenClass(){};
};
