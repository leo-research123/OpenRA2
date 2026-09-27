#pragma once

#include "yrpp/YRPPCore.h"
#include "yrpp/ArrayClasses.h"
#include "yrpp/Helpers/String.h"
#include "yrpp/Helpers/CompileTime.h"

class AudioStream;

struct ThemeControl
{
    FixedString<0x100> ID;
    FixedString<0x100> Sound;
    FixedWString<0x40> UIName;
    int Scenario;
    float Length;
    bool Normal;
    bool Repeat;
    bool Exists;
    int Side;
};

class NOVTABLE ThemeClass
{
public:
    /// Global VA: 0x00A83D10.
    DEFINE_REFERENCE(ThemeClass, Instance, 0xA83D10)
    /// Global VA: 0x00A8EC74.
    DEFINE_REFERENCE(bool, ScoresPresen, 0xA8EC74)

    /// VA: 0x00721270.
    const char* GetID(unsigned int index) const
        JMP_THIS(0x721270);

    /// VA: 0x00720940.
    const char* GetName(unsigned int index) const
        JMP_THIS(0x720940);

    /// VA: 0x00720E10.
    const char* GetFilename(unsigned int index) const
        JMP_THIS(0x720E10);

    /// VA: 0x007209B0.
    const wchar_t* GetUIName(unsigned int index) const
        JMP_THIS(0x7209B0);

    /// VA: 0x00720E50.
    int GetLength(unsigned int index) const
        JMP_THIS(0x720E50);

    /// VA: 0x00721140.
    bool IsAvailable(int index) const
        JMP_THIS(0x721140);

    /// VA: 0x007211E0.
    bool IsNormal(int index) const
        JMP_THIS(0x7211E0);

    /// VA: 0x00721210.
    int FindIndex(const char* pID) const
        JMP_THIS(0x721210);

    /// VA: 0x00720A80.
    int GetRandomIndex(unsigned int lastTheme) const
        JMP_THIS(0x720A80);

    /// VA: 0x00720B20.
    void Queue(int index)
        JMP_THIS(0x720B20);

    /// VA: 0x00720BB0.
    int Play(int index)
        JMP_THIS(0x720BB0);

    /// VA: 0x00720EA0.
    void Stop(bool fade = false)
        JMP_THIS(0x720EA0);

    /// VA: 0x00720F70.
    void Suspend()
        JMP_THIS(0x720F70);

    /// VA: 0x007209D0.
    void AI()
        JMP_THIS(0x7209D0);

    /// VA: 0x007207F0.
    void Scan()
        JMP_THIS(0x7207F0);

    int CurrentTheme; // the playing theme's index
    int LastTheme; // the theme that cannot be selected randomly
    int QueuedTheme; // the next theme to be played
    int Volume;
    bool IsScoreRepeat;
    bool IsFading;
    bool IsScoreShuffle;
    DynamicVectorClass<ThemeClass*> Themes; // the list of all themes
    AudioStream* Stream;
};
