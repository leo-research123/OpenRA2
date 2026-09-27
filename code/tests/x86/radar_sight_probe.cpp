// Production sight/range, Display visibility and shroud-counter bodies.
// Device registration, already-calibrated occlusion and discovery dispatch
// are explicit boundaries. No entity constructors are replaced by a host model.
#include "yrpp/RadarClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/Unsorted.h"
#include "scenario_runtime.hpp"
#include <cstddef>
static_assert(offsetof(RulesClass,LeptonsPerSightIncrease)==0x16BC);
static_assert(offsetof(RulesClass,VeteranSight)==0x680);
static_assert(offsetof(RulesClass,FlightLevel)==0x7B4);
static_assert(offsetof(AircraftClass,Type)==0x6C4);
static_assert(offsetof(HouseClass,RadarVisibleTo)==0x54E4);
static_assert(offsetof(HouseTypeClass,ArrayIndex2)==0xB8);
static_assert(offsetof(TechnoTypeClass,Sight)==0x5E8);
static_assert(offsetof(TechnoTypeClass,VeteranAbilities)+offsetof(AbilitiesStruct,SIGHT)==0x2A1);
MapClass& MapClass::Instance=*reinterpret_cast<MapClass*>(0x87F7E8);
RadarClass& RadarClass::Instance=*reinterpret_cast<RadarClass*>(0x87F7E8);
TacticalClass*& TacticalClass::Instance=*reinterpret_cast<TacticalClass**>(0x887324);
ScenarioClass*& ScenarioClass::Instance=*reinterpret_cast<ScenarioClass**>(0xA8B230);
HouseClass*& HouseClass::CurrentPlayer=*reinterpret_cast<HouseClass**>(0xA83D4C);
RulesClass*& RulesClass::Instance=*reinterpret_cast<RulesClass**>(0x8871E0);
CellClass* MapClass::GetCellAt(const CellStruct& cell) const {
    return reinterpret_cast<CellClass*(__thiscall*)(const MapClass*,const CellStruct*)>(0x5657A0)(this,&cell);
}
char TacticalClass::GetOcclusion(const CellStruct& cell,bool fog) const {
    return reinterpret_cast<char(__thiscall*)(const TacticalClass*,const CellStruct*,bool)>(0x6D8700)(this,&cell,fog);
}
void TacticalClass::RegisterCellAsVisible(CellClass* cell) {
    reinterpret_cast<void(__thiscall*)(TacticalClass*,CellClass*)>(0x6DA7D0)(this,cell);
}
int YRPP_FASTCALL TacticalClass::AdjustForZ(int height) noexcept {return reinterpret_cast<int(__fastcall*)(int)>(0x6D20E0)(height);}
void MapClass::RevealArea3(CoordStruct* c,int start,int radius,bool skip) {
    reinterpret_cast<void(__thiscall*)(MapClass*,CoordStruct*,int,int,bool)>(0x567DA0)(this,c,start,radius,skip);
}
TechnoClass* CellClass::FindTechnoNearestTo(const Point2D& at,bool alt,const TechnoClass* exclude) const {
    return reinterpret_cast<TechnoClass*(__thiscall*)(const CellClass*,const Point2D*,bool,const TechnoClass*)>(0x47C3D0)(this,&at,alt,exclude);
}
void RadarClass::RadarCell(const CellStruct& cell) {reinterpret_cast<void(__thiscall*)(RadarClass*,const CellStruct*)>(0x6565A0)(this,&cell);}
bool RadarClass::MapCell(CellStruct* cell,HouseClass* house){return DisplayClass::MapCell(cell,house);}
bool RadarClass::RevealFogShroud(CellStruct* cell,HouseClass* house,bool increase){return DisplayClass::RevealFogShroud(cell,house,increase);}
extern "C" {
__declspec(dllexport) void __fastcall SightUpdate(TechnoClass* self,void*,bool a,int b,bool c,HouseClass* d,int e){self->TechnoClass::UpdateSight(a,b,c,d,e);}
__declspec(dllexport) void __fastcall SightDrop(TechnoClass* self,void*,bool a,int b,bool c,HouseClass* d){self->TechnoClass::vt_entry_48C(a,b,c,d);}
__declspec(dllexport) void __fastcall SightAircraftSee(AircraftClass* self,void*,DWORD a,DWORD b){self->AircraftClass::See(a,b);}
__declspec(dllexport) void __fastcall SightSee(TechnoClass* self,void*,DWORD incremental,DWORD dontMap){self->TechnoClass::See(incremental,dontMap);}
__declspec(dllexport) void __fastcall SightArea1(MapClass* self,void*,CoordStruct* c,int r,HouseClass* h,BYTE a,BYTE b,BYTE d,BYTE e,BYTE f){self->RevealArea1(c,r,h,a,b,d,e,f);}
__declspec(dllexport) void __fastcall SightArea2(MapClass* self,void*,CoordStruct* c,int r,HouseClass* h,BYTE a,int b,BYTE d,BYTE e,BYTE f){self->RevealArea2(c,r,h,a,b,d,e,f);}
__declspec(dllexport) bool __fastcall SightMapCell(DisplayClass* self,void*,CellStruct* c,HouseClass* h){return self->DisplayClass::MapCell(c,h);}
__declspec(dllexport) bool __fastcall SightReveal(DisplayClass* self,void*,CellStruct* c,HouseClass* h,bool up){return self->DisplayClass::RevealFogShroud(c,h,up);}
__declspec(dllexport) bool __fastcall SightFog(DisplayClass* self,void*,CellStruct* c,HouseClass* h){return self->DisplayClass::MapCellFoggedness(c,h);}
__declspec(dllexport) void __fastcall SightDown(CellClass* self,void*){self->ReduceShroudCounter();}
__declspec(dllexport) void __fastcall SightUp(CellClass* self,void*){self->IncreaseShroudCounter();}
__declspec(dllexport) void __fastcall SightUnshroud(CellClass* self,void*){self->Unshroud();}
__declspec(dllexport) int __cdecl SightSetRound(int){__debugbreak();return 0;}
int (__cdecl* SightRoundImport)(int)=SightSetRound;
int _fltused=0;
void __cdecl __std_terminate(){__debugbreak();}
int __cdecl __CxxFrameHandler3(void*,void*,void*,void*){__debugbreak();return 0;}
void __cdecl abort(){__debugbreak();}
void* __cdecl memset(void* p,int v,std::size_t n){auto* b=static_cast<volatile byte*>(p);while(n--)*b++=byte(v);return p;}
void __cdecl SightUnexpected(){__debugbreak();}
auto SightUnexpectedImport=&SightUnexpected;
}
#pragma comment(linker,"/alternatename:__imp____stdio_common_vsprintf=_SightUnexpectedImport")
#pragma comment(linker,"/alternatename:__imp____stdio_common_vswprintf=_SightUnexpectedImport")
namespace game {
const ScenarioRuntimeServices& scenario_runtime(){
    static const ScenarioHouseServices houses{.session=reinterpret_cast<SessionClass*>(0xA8B238)};
    static const ScenarioRuntimeServices runtime{.houses=&houses};return runtime;
}
}

CellStruct (&Unsorted::AdjacentCell)[8]=*reinterpret_cast<CellStruct(*)[8]>(0x89F688);
double Math::sqrt(double value){return reinterpret_cast<double(__cdecl*)(double)>(0x4CAC40)(value);}

#pragma comment(linker,"/alternatename:__imp__fesetround=_SightRoundImport")
