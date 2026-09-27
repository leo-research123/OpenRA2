#pragma once

#include "yrpp/YRPPCore.h"

class CDDriveManagerClass
{
public:
    // Static
    static CDDriveManagerClass* Global()
        { return *((CDDriveManagerClass**)0x89E414); }

protected:
    // CTOR
    /// VA: 0x004E6070.
    CDDriveManagerClass()
        { JMP_THIS(0x4E6070); }

public:
    /*
    Retrieves the number of the currently inserted disc
    0 = RA2 Allied,
    1 = RA2 Soviet,
    2 = YR
    */
    /// VA: 0x004A80D0.
    int GetCDNumber()
        { JMP_THIS(0x4A80D0); }

    // Properties

public:

    int CDDriveNames [26]; //int + 'A' would be the drive's name
    int NumCDDrives;
    DWORD unknown_6C;
};

class CD
{
public:
    virtual bool ForceAvailable(int nCDNumber);
    virtual bool InsertCDDialog();
    virtual void SwapToDisk();

public:

    DWORD unknown_04;

public:
    CD() noexcept;
};
