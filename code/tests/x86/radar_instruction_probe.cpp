// Test-only x86 entry adapters. Production Radar methods execute unchanged.
// Callers supply original-layout field fixtures; this is not a lifecycle test.
#include "yrpp/RadarClass.h"
#include <cstddef>
#include "yrpp/HouseClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/VocClass.h"
#include "scenario_runtime.hpp"

static_assert(sizeof(RadarClass)==0x150C);
static_assert(offsetof(RadarClass,unknown_121C)==0x121C);
static_assert(offsetof(RadarClass,unknown_1220)==0x1220);
static_assert(offsetof(RadarClass,unknown_123C)==0x123C);
static_assert(offsetof(RadarClass,RadarSizeFactor)==0x1488);
static_assert(offsetof(RadarClass,unknown_14AC)==0x14AC);
static_assert(offsetof(RadarClass,unknown_14B0)==0x14B0);
static_assert(offsetof(RadarClass,IsAvailableNow)==0x14D8);
static_assert(offsetof(RadarClass,unknown_14FC)==0x14FC);
static_assert(offsetof(RadarClass,unknown_timer_1500)==0x1500);
static_assert(offsetof(HouseClass,Defeated)==0x1F5);
static_assert(offsetof(SessionClass,MultiplayerObserver)==0x300);
static_assert(offsetof(RulesClass,RadarOn)==0x6F0);

extern "C" {
__declspec(dllexport) DWORD RadarProbeTick=0;
__declspec(dllexport) bool RadarProbeSound=false;
int _fltused=0;
// Exception infrastructure is a trap boundary in this no-CRT probe. Any
// execution of it fails the emulator run; no exception behavior is claimed.
void __cdecl __std_terminate(){__debugbreak();}
int __cdecl __CxxFrameHandler3(void*,void*,void*,void*){__debugbreak();return 0;}
void __cdecl __std_exception_copy(const __std_exception_data*,__std_exception_data*){__debugbreak();}
void __cdecl __std_exception_destroy(__std_exception_data*){__debugbreak();}
void __stdcall _CxxThrowException(void*,void*){__debugbreak();}
void __cdecl RadarProbeUnexpectedRuntime(){__debugbreak();}
void (*RadarProbeWatson)()=RadarProbeUnexpectedRuntime;
void* RadarProbeTypeInfo[2]={nullptr,nullptr};
void* __cdecl memset(void* output,int value,std::size_t count){
    auto* bytes=static_cast<volatile unsigned char*>(output);
    for(std::size_t i=0;i<count;++i)bytes[i]=static_cast<unsigned char>(value);
    return output;
}
__declspec(dllexport) void __fastcall RadarProbeActivate(RadarClass* radar,void*,int value){radar->ActivateRadar(value!=0,RadarProbeSound);}
__declspec(dllexport) void __fastcall RadarProbeMode(RadarClass* radar,void*,int value){radar->SetRadarMode(value,RadarProbeSound);}
__declspec(dllexport) void __fastcall RadarProbeAvailable(RadarClass* radar,void*,int value){radar->SetRadarAvailability(value!=0);}
__declspec(dllexport) bool __fastcall RadarProbeAdvance(RadarClass* radar,void*){return radar->AdvanceRadarAnimation();}
__declspec(dllexport) void __fastcall RadarProbeClear(RadarClass* radar,void*){radar->RadarClass::Init_Clear();}
__declspec(dllexport) bool __fastcall RadarProbeFoundation(RadarClass* radar,void*){return radar->BuildFoundationPixels();}
__declspec(dllexport) void __fastcall RadarProbeRefresh(RadarClass* radar,void*,Point2D* point){radar->RefreshCrd(point);}
__declspec(dllexport) Point2D* __fastcall RadarProbeProject(RadarClass* radar,void*,Point2D* out,CoordStruct* world,int restrict){return radar->GetCrdOnRadar(out,world,restrict!=0);}
__declspec(dllexport) bool __fastcall RadarProbeInverse(RadarClass* radar,void*,Point2D* point,CellStruct* out){return radar->RadarToTerrainCell(*point,*out);}
__declspec(dllexport) bool __fastcall RadarProbeFit(RadarClass*,void*,int width,int height,Point2D* size,float* scale){return RadarClass::FitTerrainRadar(width,height,*size,*scale);}
__declspec(dllexport) bool __fastcall RadarProbeFrame(RadarClass* radar,void*,CellStruct* center,Point2D* viewport){return radar->UpdateViewportFrame(*center,*viewport);}
__declspec(dllexport) bool __fastcall RadarProbeResample(RadarClass*,void*,ColorStruct* source,unsigned source_count,int width,int height,WORD* output,unsigned output_count){return RadarClass::ResampleTerrainRadar(source,source_count,width,height,output,output_count);}
__declspec(dllexport) void __fastcall RadarProbeQueueNextMovie(RadarClass* radar,void*){radar->QueueNextMovie();}
__declspec(dllexport) bool __fastcall RadarProbeExisting(RadarClass* radar,void*){return radar->IsRadarExisting();}
__declspec(dllexport) bool __fastcall RadarProbeNames(RadarClass* radar,void*){return radar->IsPlayerNames();}
__declspec(dllexport) bool __fastcall RadarProbeMovie(RadarClass* radar,void*){return radar->IsPlayingMovie();}
__declspec(dllexport) void __fastcall RadarProbeRedraw(RadarClass* radar,void*,bool complete){radar->RedrawRadar(complete);}
__declspec(dllexport) void __fastcall RadarProbeBackground(RadarClass* radar,void*,CellStruct* cell){radar->RadarBackground(*cell);}
__declspec(dllexport) RectangleStruct* __fastcall RadarProbeCellRect(RadarClass* radar,void*,RectangleStruct* output,CellStruct* cell){return radar->CellRadarRect(output,*cell);}
__declspec(dllexport) RectangleStruct* __fastcall RadarProbeCellPixel(RadarClass* radar,void*,RectangleStruct* output,CellStruct* cell){return radar->CellToRadarPixel(output,*cell);}
}
void* __cdecl operator new(std::size_t){__debugbreak();return nullptr;}
void __cdecl operator delete(void*,std::size_t) noexcept{__debugbreak();}
#pragma comment(linker,"/alternatename:__imp___invoke_watson=_RadarProbeWatson")
#pragma comment(linker,"/alternatename:??_7type_info@@6B@=_RadarProbeTypeInfo")
DWORD SystemTimer::GetTime(){return RadarProbeTick;}

// Unused methods in the same production translation unit refer to the world.
// These dependencies must never execute in this bounded field/vector audit.
MapClass& MapClass::Instance=*reinterpret_cast<MapClass*>(0x01000000);
bool MapClass::IsWithinUsableArea(const CellStruct&,bool) const{__debugbreak();return false;}
CellClass* MapClass::TryGetCellAt(const CellStruct&) const{__debugbreak();return nullptr;}
CellClass* MapClass::GetCellAt(const CellStruct&) const{__debugbreak();return nullptr;}
ColorStruct CellClass::GetTerrainRadarColor() const noexcept{__debugbreak();return {};}
int CellClass::GetFloorHeight(const Point2D&) const{__debugbreak();return 0;}
bool MapClass::IsLocationGapped(const CoordStruct&) const{__debugbreak();return false;}
bool MapClass::IsLocationShrouded(const CoordStruct&) const{__debugbreak();return false;}
bool RadarClass::RebuildTerrainRadarCache() noexcept{__debugbreak();return false;}

HouseClass*& HouseClass::CurrentPlayer=*reinterpret_cast<HouseClass**>(0xA83D4C);
RulesClass*& RulesClass::Instance=*reinterpret_cast<RulesClass**>(0x8871E0);
void AudioController::EndLooping(){reinterpret_cast<void(__thiscall*)(AudioController*)>(0x406060)(this);}
void __fastcall VocClass::PlayGlobal(int index,int pan,float volume,AudioController* controller){
    reinterpret_cast<void(__fastcall*)(int,int,float,AudioController*)>(0x750920)(index,pan,volume,controller);
}
namespace game {
const ScenarioRuntimeServices& scenario_runtime(){
    static const ScenarioHouseServices houses{.session=reinterpret_cast<SessionClass*>(0xA8B238)};
    static const ScenarioRuntimeServices runtime{.houses=&houses};
    return runtime;
}
}
