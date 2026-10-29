#pragma once

#include "yrpp/YRPPCore.h"
#include "yrpp/ArrayClasses.h"
#include "yrpp/Helpers/String.h"
#include "yrpp/Helpers/CompileTime.h"

class AudioStream;
class CCINIClass;

struct ThemeControl {
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

class NOVTABLE ThemeClass {
public:
    /// Global VA: 0x00A83D10.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(ThemeClass, Instance, 0xA83D10)
#else
    static ThemeClass& Instance;
#endif
    /// Global VA: 0x00A8EC74.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(bool, ScoresPresen, 0xA8EC74)
#else
    static bool& ScoresPresen;
#endif

    /// VA: 0x00721270.
#if defined(RA2_YRPP_GAME)
    const char* GetID(unsigned int index) const JMP_THIS(0x721270);
#else
    const char* GetID(unsigned int index) const;
#endif

    /// VA: 0x00720940.
#if defined(RA2_YRPP_GAME)
    const char* GetName(unsigned int index) const JMP_THIS(0x720940);
#else
    const char* GetName(unsigned int index) const;
#endif

    /// VA: 0x00720E10.
#if defined(RA2_YRPP_GAME)
    const char* GetFilename(unsigned int index) const JMP_THIS(0x720E10);
#else
    const char* GetFilename(unsigned int index) const;
#endif

    /// VA: 0x007209B0.
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIName(unsigned int index) const JMP_THIS(0x7209B0);
#else
    const wchar_t* GetUIName(unsigned int index) const;
#endif

    /// VA: 0x00720E50.
#if defined(RA2_YRPP_GAME)
    int GetLength(unsigned int index) const JMP_THIS(0x720E50);
#else
    int GetLength(unsigned int index) const;
#endif

    /// VA: 0x00721140.
#if defined(RA2_YRPP_GAME)
    bool IsAvailable(int index) const JMP_THIS(0x721140);
#else
    bool IsAvailable(int index) const;
#endif

    /// VA: 0x007211E0.
#if defined(RA2_YRPP_GAME)
    bool IsNormal(int index) const JMP_THIS(0x7211E0);
#else
    bool IsNormal(int index) const;
#endif

    /// VA: 0x00721210.
#if defined(RA2_YRPP_GAME)
    int FindIndex(const char* pID) const JMP_THIS(0x721210);
#else
    int FindIndex(const char* pID) const;
#endif

    /// VA: 0x00720A80.
#if defined(RA2_YRPP_GAME)
    int GetRandomIndex(unsigned int lastTheme) const JMP_THIS(0x720A80);
#else
    int GetRandomIndex(unsigned int lastTheme) const;
#endif

    /// VA: 0x00720B20.
#if defined(RA2_YRPP_GAME)
    void Queue(int index) JMP_THIS(0x720B20);
#else
    void Queue(int index);
#endif

    /// VA: 0x00720BB0.
#if defined(RA2_YRPP_GAME)
    int Play(int index) JMP_THIS(0x720BB0);
#else
    int Play(int index);
#endif

    /// VA: 0x00720EA0.
#if defined(RA2_YRPP_GAME)
    void Stop(bool fade = false) JMP_THIS(0x720EA0);
#else
    void Stop(bool fade = false);
#endif

    /// VA: 0x00720F70.
#if defined(RA2_YRPP_GAME)
    void Suspend() JMP_THIS(0x720F70);
#else
    void Suspend();
#endif

    /// VA: 0x007209D0.
#if defined(RA2_YRPP_GAME)
    void AI() JMP_THIS(0x7209D0);
#else
    void AI();
#endif

    /// VA: 0x007207F0.
#if defined(RA2_YRPP_GAME)
    void Scan() JMP_THIS(0x7207F0);
#else
    void Scan();
#endif

#if !defined(RA2_YRPP_GAME)
    ThemeClass() noexcept;
#endif
    /// VA: 0x00720590
#if defined(RA2_YRPP_GAME)
    int LoadFromINI(CCINIClass* ini) JMP_THIS(0x720590);
#else
    int LoadFromINI(CCINIClass* ini) noexcept;
#endif
    /// VA: 0x00720770
#if defined(RA2_YRPP_GAME)
    void Clear() JMP_THIS(0x720770);
#else
    void Clear() noexcept;
#endif

    int CurrentTheme;  // the playing theme's index
    int LastTheme;     // the theme that cannot be selected randomly
    int QueuedTheme;   // the next theme to be played
    int Volume;
    bool IsScoreRepeat;
    bool IsFading;
    bool IsScoreShuffle;
    DynamicVectorClass<ThemeControl*> Themes;  // the list of all themes
    AudioStream* Stream;
};
