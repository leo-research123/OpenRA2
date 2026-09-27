#pragma once

#include "yrpp/platform/ABI.h"

enum class TheaterType : int { None = -1, Temperate, Snow, Urban, Desert, NewUrban, Lunar };

struct Theater //US English spelling to keep it consistent with the game
{
public:
    static Theater const (&Array)[6];

    // Resource MIX/palette setup and renderer initialization are coupled in
    // original 0x5349C0. Full initialization is outside this resource-only module.
    /// VA: 0x005349C0.
    static void YRPP_FASTCALL Init(TheaterType theater);
    // Resource-only portion of 5349C0. Caller stops consumers and clears the
    // old tile catalog first. Rendering/palette initialization is separate.
    static bool MountResourceMixes(TheaterType theater) noexcept;
    static bool UnmountResourceMixes() noexcept;

    static Theater const* Get(TheaterType theater);
    static Theater const& GetTheater(TheaterType theater);

    [[deprecated]]
    static int YRPP_FASTCALL FindIndex(const char* pName);

    static TheaterType& LastTheater;

    char	ID[0x10];               //e.g. "TEMPERATE"
    char	UIName[0x20];           //e.g. "Name:Temperate"
    char	ControlFileName[0xA];   //e.g. "TEMPERAT" -> INI and MIX
    char	ArtFileName[0xA];       //e.g. "ISOTEMP" -> MIX
    char	PaletteFileName[0xA];   //e.g. "ISOTEM" -> PAL
    char	Extension[0x4];         //e.g. "TEM" -> Iso tile file extension
    char	MMExtension[0x4];       //e.g. "MMT" -> Marble Madness tile file extension
    char	Letter[0x2];            //e.g. "T" -> Theater specific IDs (GTCNST, NTWEAP, YTBARRACKS)

    // Used by CellClass 47C060's terrain radar color (Snow: 0.8, others: 1).
    float	RadarTerrainBrightness; // 0.0 to 1.0
    float	unknown_float_5C;
    float	unknown_float_60;
    float	unknown_float_64;
    int		unknown_int_68;
    int		unknown_int_6C;
};
