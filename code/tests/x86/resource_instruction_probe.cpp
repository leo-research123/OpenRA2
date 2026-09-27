// Test-only adapters. The resource algorithms are compiled from production
// translation units; original-layout inputs are supplied by the emulator.
#include "yrpp/TiberiumClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/TerrainClass.h"
#include "map_world.hpp"
#include <cstddef>

static_assert(sizeof(TiberiumClass)==0x128);
static_assert(offsetof(TiberiumClass,Growth)==0xA8);
static_assert(offsetof(TiberiumClass,SpreadLogic)==0xF0);
static_assert(offsetof(TiberiumClass,GrowthLogic)==0x10C);
static_assert(sizeof(CellClass)==0x148);
static_assert(offsetof(CellClass,OverlayTypeIndex)==0x44);
static_assert(offsetof(CellClass,OverlayData)==0x11E);
static_assert(offsetof(ScenarioClass,Random)==0x218);
static_assert(offsetof(MapClass,Cells)==0x138);
static_assert(offsetof(BuildingClass,Type)==0x520);
static_assert(offsetof(BuildingTypeClass,Invisible)==0xC9A);
static_assert(offsetof(BuildingTypeClass,InvisibleInGame)==0x1701);
static_assert(offsetof(TerrainClass,Type)==0xC8);
static_assert(offsetof(TerrainTypeClass,SpawnsTiberium)==0x2B1);

DynamicVectorClass<TiberiumClass*>& TiberiumClass::Array=*reinterpret_cast<DynamicVectorClass<TiberiumClass*>*>(0xB0F4E8);
DynamicVectorClass<OverlayTypeClass*>& OverlayTypeClass::Array=*reinterpret_cast<DynamicVectorClass<OverlayTypeClass*>*>(0xA83D80);
DynamicVectorClass<IsometricTileTypeClass*>& IsometricTileTypeClass::Array=*reinterpret_cast<DynamicVectorClass<IsometricTileTypeClass*>*>(0xA8ED28);
ScenarioClass*& ScenarioClass::Instance=*reinterpret_cast<ScenarioClass**>(0xA8B230);
MapClass& MapClass::Instance=*reinterpret_cast<MapClass*>(0x87F7E8);
CellClass& MapClass::InvalidCell=*reinterpret_cast<CellClass*>(0xABDC50);
GroundType (&GroundType::Array)[12]=*reinterpret_cast<GroundType(*)[12]>(0x89EA40);
bool& Game::IsActive=*reinterpret_cast<bool*>(0xA8E9A0);
int& Unsorted::CurrentFrame=*reinterpret_cast<int*>(0xA8ED84);
DWORD& Randomizer::DefaultSeed=*reinterpret_cast<DWORD*>(0xA8ED94);

extern "C" {
int _fltused=0;
void __cdecl __std_terminate(){__debugbreak();}
int __cdecl __CxxFrameHandler3(void*,void*,void*,void*){__debugbreak();return 0;}
__declspec(dllexport) void __fastcall ResourceProbeGrowth(){TiberiumClass::UpdateGrowth();}
__declspec(dllexport) void __fastcall ResourceProbeSpread(){TiberiumClass::UpdateSpread();}
__declspec(dllexport) void __fastcall ResourceProbeGrow(TiberiumClass* t,void*){t->Grow();}
__declspec(dllexport) void __fastcall ResourceProbeSpreadCells(TiberiumClass* t,void*){t->SpreadCells();}
__declspec(dllexport) void __fastcall ResourceProbeRegisterGrowth(TiberiumClass* t,void*,CellStruct* cell){t->RegisterForGrowth(cell);}
__declspec(dllexport) void __fastcall ResourceProbeRegisterSpread(TiberiumClass* t,void*,CellStruct* cell){t->RegisterForSpread(cell);}
__declspec(dllexport) bool __fastcall ResourceProbeGerminate(CellClass* c,void*,TiberiumClass* t){return c->CanTiberiumGerminate(t);}
__declspec(dllexport) bool __fastcall ResourceProbeIncrease(CellClass* c,void*,int type,int amount){return c->IncreaseTiberium(type,amount);}
__declspec(dllexport) bool __fastcall ResourceProbeCellSpread(CellClass* c,void*,bool forced){return c->SpreadTiberium(forced);}
__declspec(dllexport) int __cdecl ResourceProbeRound(int){__debugbreak();return 0;}
int (__cdecl* __imp_fesetround)(int)=ResourceProbeRound;
__declspec(dllexport) void __cdecl ResourceProbeTrap(){__debugbreak();}
void (__cdecl* ResourceProbeTrapPointer)()=ResourceProbeTrap;
__declspec(dllexport) void* __cdecl ResourceProbeMemset(void* dst,int value,unsigned count){
    auto* bytes=static_cast<volatile unsigned char*>(dst);
    for(unsigned i=0;i<count;++i)bytes[i]=static_cast<unsigned char>(value);
    return dst;
}
}

// Rendering invalidation is outside this resource-state comparison.
void game::map_resource_changed(CellClass&) noexcept {}
