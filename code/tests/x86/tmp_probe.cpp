// Machine-code differential bridge: only public core types and methods.
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/Randomizer.h"
#include "yrpp/ScenarioClass.h"
#include <cstring>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error This probe verifies the Microsoft x86 resource ABI.
#endif
static_assert(sizeof(ObjectTypeClass) == 0x294 && sizeof(IsometricTileTypeClass) == 0x30c);
static_assert(offsetof(IsometricTileTypeClass, NextVariant) == 0x2bc);
static_assert(offsetof(IsometricTileTypeClass, FileName) == 0x2f5);
static_assert(offsetof(IsometricTileTypeClass, AllowBurrowing) == 0x305);
static_assert(offsetof(IsometricTileTypeClass, AllowTiberium) == 0x306);

extern "C" void (__cdecl* __xc_a[])();
extern "C" void (__cdecl* __xc_z[])();
// The emulator maps the PE directly, so explicitly run its real C++ static
// initializers once. Windows' CRT performs this before normal DLL use.
extern "C" __declspec(dllexport) void __cdecl TMPProbe_Initialize() {
    for (auto* entry=__xc_a;entry!=__xc_z;++entry) if (*entry) (*entry)();
}

extern "C" __declspec(dllexport) void __cdecl TMPProbe_ColorMode(int rgb555) {
    Drawing::RedShiftLeft = rgb555 ? 10 : 11; Drawing::RedShiftRight = 3;
    Drawing::GreenShiftLeft = 5; Drawing::GreenShiftRight = rgb555 ? 3 : 2;
    Drawing::BlueShiftLeft = 0; Drawing::BlueShiftRight = 3;
}

extern "C" __declspec(dllexport) int __cdecl TMPProbe_Read(const byte* data, int size, byte* output) {
    IsometricTileTypeClass tile(7, -65, 2, "TMP", 0);
    if (size < 0 || !tile.ReadTMP(data, std::size_t(size))) return 0;
    auto* tmp = reinterpret_cast<TMPStruct*>(tile.Image);
    std::memcpy(output, tmp, 16);
    std::memcpy(output + 16, &tile.unk_2E4, 8);
    int offset = 24;
    for (int i = 0; i < tmp->Columns * tmp->Rows; ++i) {
        const TMPImage* image = nullptr;
        tmp->GetSubTile(i, image);
        if (image) {
            std::memcpy(output + offset, image, 52);
            std::memcpy(output + offset + 52, tile.unk_2A4[i], 52);
        } else std::memset(output + offset, 0, 104);
        offset += 104;
    }
    return offset;
}

extern "C" __declspec(dllexport) int __cdecl TMPProbe_TerrainQueries(const byte* data, int size, int index, int* output) {
    IsometricTileTypeClass tile(7,-65,2,"TMP",0);
    if (size<0 || !tile.ReadTMP(data,static_cast<std::size_t>(size))) return 0;
    output[0]=static_cast<int>(tile.GetLandType(index));
    output[1]=tile.GetSlopeIndex(index);
    output[2]=123; output[3]=456;
    output[4]=tile.GetTileDimensions(index,output[2],output[3]);
    return 1;
}

extern "C" __declspec(dllexport) int __cdecl TMPProbe_CellVariant(const byte* data,int size,
    unsigned seed,int x,int y,int height,int count,int* output) {
    IsometricTileTypeClass tile(0,-65,2,"TMP",0);
    if (size<0 || !tile.ReadTMP(data,static_cast<std::size_t>(size))) return 0;
    auto* cell=CellClass::Create(); if (!cell) return 0;
    CellClass::TileVariantTableInitialized=false;
    Randomizer::Global=Randomizer(seed);
    cell->MapCoords={static_cast<short>(x),static_cast<short>(y)};
    cell->Height=static_cast<char>(height);
    output[0]=cell->GetTileVariant(IsometricTileTypeClass::Array.FindItemIndex(&tile),count);
    std::memcpy(output+1,CellClass::TileVariantTable,256);
    output[65]=Randomizer::Global.Next1; output[66]=Randomizer::Global.Next2;
    std::memcpy(output+67,Randomizer::Global.Table,1000);
    GameDelete(cell);
    return 1;
}

extern "C" __declspec(dllexport) int __cdecl TMPProbe_InitialLight(const int* input,DWORD* output) {
    ScenarioClass scenario;
    scenario.AmbientCurrent=input[0];
    scenario.NormalLighting.Tint={input[1],input[2],input[3]};
    scenario.NormalLighting.Ground=input[4]; scenario.NormalLighting.Level=input[5];
    auto* previous=ScenarioClass::Instance; ScenarioClass::Instance=&scenario;
    auto* cell=CellClass::Create();
    if (!cell) { ScenarioClass::Instance=previous; return 0; }
    cell->MapCoords={static_cast<short>(input[6]),static_cast<short>(input[7])};
    cell->Level=static_cast<char>(input[8]);
    const bool ok=cell->InitializeTerrainLighting();
    if (ok) {
        output[0]=cell->Intensity; output[1]=cell->Ambient;
        output[2]=cell->Intensity_Normal; output[3]=cell->Intensity_Terrain; output[4]=cell->Color1_Blue;
        output[5]=cell->Color2_Red; output[6]=cell->Color2_Green; output[7]=cell->Color2_Blue;
    }
    GameDelete(cell); ScenarioClass::Instance=previous;
    return ok;
}
