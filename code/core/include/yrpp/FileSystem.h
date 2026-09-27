/*
    File related stuff
*/

#pragma once

#include "yrpp/platform/ABI.h"
#include "yrpp/ConvertClass.h"
#include "yrpp/CCFileClass.h"
#include "yrpp/FileFormats/SHP.h"
#include <type_traits>
#include "yrpp/Helpers/CompileTime.h"

class VoxLib;
class MotLib;

class DSurface;
enum class TheaterType : int;

struct VoxelStruct
{
    VoxLib* VXL;
    MotLib* HVA;
};

class FileSystem
{
public:
    // Original name-tree operations 5B4310 / 5B4270. Values are borrowed;
    // clearing nodes does NOT release cached objects or raw file buffers.
    static void ClearNameCache() noexcept;
    static void InvalidateName(const char* name);
    /// Global VA: 0x00AC1478.
    DEFINE_REFERENCE(SHPStruct*, PIPBRD_SHP, 0xAC1478u)
    /// Global VA: 0x00AC147C.
    DEFINE_REFERENCE(SHPStruct*, PIPS_SHP, 0xAC147Cu)
    /// Global VA: 0x00AC1480.
    DEFINE_REFERENCE(SHPStruct*, PIPS2_SHP, 0xAC1480u)
    /// Global VA: 0x00AC1484.
    DEFINE_REFERENCE(SHPStruct*, TALKBUBL_SHP, 0xAC1484u)
    /// Global VA: 0x0089DDC8.
    DEFINE_REFERENCE(SHPStruct*, WRENCH_SHP, 0x89DDC8u)
    /// Global VA: 0x0089DDC4.
    DEFINE_REFERENCE(SHPStruct*, POWEROFF_SHP, 0x89DDC4u)
    /// Global VA: 0x00A8F794.
    DEFINE_REFERENCE(SHPStruct*, GRFXTXT_SHP, 0xA8F794u)
    /// Global VA: 0x00B1CF98.
    DEFINE_REFERENCE(SHPStruct*, OREGATH_SHP, 0xB1CF98u)
    /// Global VA: 0x00B07BC0.
    DEFINE_REFERENCE(SHPStruct*, DARKEN_SHP, 0xB07BC0u)
    /// Global VA: 0x00B0B484.
    DEFINE_REFERENCE(SHPStruct*, GCLOCK2_SHP, 0xB0B484u)
    /// Global VA: 0x0089DDBC.
    DEFINE_REFERENCE(SHPStruct*, BUILDINGZ_SHA, 0x89DDBCu)

    static BytePalette& TEMPERAT_PAL; // Global VA: 0x00885780.
    static BytePalette& ISOx_PAL; // Global VA: 0x00ABBED0.
    // Palette portion of 5349C0 / 545150. Expands original 6-bit channels by
    // shifting two bits. Missing screen PAL uses the original gradient; missing
    // or truncated ISO PAL fails and preserves both published palettes.
    static bool LoadTheaterPalettes(TheaterType theater) noexcept;
    /// Global VA: 0x00A8F790.
    DEFINE_REFERENCE(BytePalette*, GRFXTXT_PAL, 0xA8F790u)

    /// Global VA: 0x0087F6B0.
    DEFINE_REFERENCE(ConvertClass*, CAMEO_PAL, 0x87F6B0u)
    /// Global VA: 0x0087F6B4.
    DEFINE_REFERENCE(ConvertClass*, UNITx_PAL, 0x87F6B4u)
    /// Global VA: 0x0087F6B8.
    DEFINE_REFERENCE(ConvertClass*, x_PAL, 0x87F6B8u)
    /// Global VA: 0x0087F6BC.
    DEFINE_REFERENCE(ConvertClass*, GRFTXT_TIBERIUM_PAL, 0x87F6BCu)
    /// Global VA: 0x0087F6C0.
    DEFINE_REFERENCE(ConvertClass*, ANIM_PAL, 0x87F6C0u)
    /// Global VA: 0x0087F6C4.
    DEFINE_REFERENCE(ConvertClass*, PALETTE_PAL, 0x87F6C4u)
    /// Global VA: 0x0087F6C8.
    static ConvertClass*& MOUSE_PAL;
    /// Global VA: 0x00885180.
    static BytePalette& WAYPOINT_PAL;
    /// Global VA: 0x0087F6CC.
    DEFINE_REFERENCE(ConvertClass*, SIDEBAR_PAL, 0x87F6CCu)
    /// Global VA: 0x00A8F798.
    DEFINE_REFERENCE(ConvertClass*, GRFXTXT_Convert, 0xA8F798u)

    static void* YRPP_FASTCALL LoadFile(const char* pFileName, bool bLoadAsSHP);

    static void* YRPP_FASTCALL LoadWholeFileEx(const char* pFilename, bool &outAllocated);

    static void* LoadFile(const char* pFileName)
        { return LoadFile(pFileName, false); }

    static SHPStruct* LoadSHPFile(const char* pFileName)
        { return static_cast<SHPStruct*>(LoadFile(pFileName, true)); }

    // I'm just making this up for easy palette loading
    static ConvertClass* LoadPALFile(const char* pFileName, DSurface* pSurface);

    template <typename T>
    static T* LoadWholeFileEx(const char* pFilename, bool &outAllocated) {
        return static_cast<T*>(LoadWholeFileEx(pFilename, outAllocated));
    }

    // returns a pointer to a new block of bytes. caller takes ownership and has
    // to free it from the game's pool.
    template <typename T = void>
    static T* AllocateFile(const char* pFilename) {
        CCFileClass file(pFilename);
        return static_cast<T*>(file.ReadWholeFile());
    }

    // allocates a new palette with the 6 bit colors converted to 8 bit
    // (not the proper way. how the game does it.) caller takes ownership and
    // has to free it from the game's pool.
    static BytePalette* AllocatePalette(const char* pFilename);
};
