#include "yrpp/BeaconClass.h"
#include "yrpp/BeaconManagerClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/YRMath.h"
#include "yrpp/Memory.h"
#include <cstddef>
#include <new>
static_assert(offsetof(BeaconClass,Text)==0xE);
static_assert(offsetof(BeaconClass,HouseID)==0x110);
static_assert(offsetof(BeaconManagerClass,AllocatedCount)==0x60);
static_assert(offsetof(BeaconManagerClass,RadarBeaconAnimPeriod)==0x7C);
static_assert(offsetof(DisplayClass,RepairMode)==0x11B0);
static_assert(offsetof(DisplayClass,PlaceBeaconMode)==0x11B4);
static_assert(offsetof(HouseClass,Allies)==0x5788);
HouseClass*& HouseClass::CurrentPlayer=*reinterpret_cast<HouseClass**>(0xA83D4C);
DynamicVectorClass<HouseClass*>& HouseClass::Array=*reinterpret_cast<DynamicVectorClass<HouseClass*>*>(0xA80228);
BeaconManagerClass& BeaconManagerClass::Instance=*reinterpret_cast<BeaconManagerClass*>(0x89C3B0);
void YRPP_FASTCALL MapClass::UnselectAll(){reinterpret_cast<void(__cdecl*)()>(0x48DC90)();}
double Math::sqrt(double value){return reinterpret_cast<double(__cdecl*)(double)>(0x4CAC40)(value);}
namespace YRMemory {void YRPP_CDECL Deallocate(const void* value){reinterpret_cast<void(__cdecl*)(const void*)>(0x7C8B3D)(value);}}
extern "C" {
__declspec(dllexport) BeaconClass* __fastcall ConstructBeacon(BeaconClass* object){return new(object) BeaconClass;}
__declspec(dllexport) void __fastcall SetBeacon(BeaconClass* object,void*,CoordStruct at,int house){object->SetCoordAndHouse(at,house);}
__declspec(dllexport) void __fastcall BeaconText(BeaconClass* object,void*,const wchar_t* text){object->SetText(text);}
__declspec(dllexport) bool __fastcall BeaconVisible(BeaconClass* object){return object->VisibleToPlayer();}
__declspec(dllexport) BeaconManagerClass* __fastcall ConstructManager(BeaconManagerClass* manager){return new(manager) BeaconManagerClass;}
__declspec(dllexport) void __fastcall ResetManager(BeaconManagerClass* manager){manager->Reset();}
__declspec(dllexport) void __fastcall DestroyManager(BeaconManagerClass* manager){manager->~BeaconManagerClass();}
__declspec(dllexport) bool __fastcall BeaconCapacity(BeaconManagerClass* manager,void*,int house){return manager->CanPlaceBeacon(house);}
__declspec(dllexport) bool __fastcall SelectBeacon(BeaconManagerClass* manager,void*,int x,int y,int z){return manager->SelectBeacon(x,y,z);}
__declspec(dllexport) void __fastcall BeaconMode(DisplayClass* display,void*,int mode){display->SetBeaconMode(mode);}
__declspec(dllexport) int __cdecl BeaconSetRound(int){__debugbreak();return 0;}
auto BeaconRoundImport=&BeaconSetRound;
void __cdecl __std_terminate(){__debugbreak();}
int __cdecl __CxxFrameHandler3(void*,void*,void*,void*){__debugbreak();return 0;}
int _fltused=0;
void* __cdecl memset(void* output,int value,std::size_t count){
 auto* to=static_cast<volatile unsigned char*>(output);for(std::size_t i=0;i<count;++i)to[i]=static_cast<unsigned char>(value);return output;
}
wchar_t* __cdecl BeaconWmemset(wchar_t* output,wchar_t value,std::size_t count){
 for(std::size_t i=0;i<count;++i)output[i]=value;return output;
}
wchar_t* __cdecl BeaconWcsncpy(wchar_t* to,const wchar_t* from,std::size_t count){
 return reinterpret_cast<wchar_t*(__cdecl*)(wchar_t*,const wchar_t*,std::size_t)>(0x7CA422)(to,from,count);
}
auto BeaconSetImport=&BeaconWmemset;
auto BeaconCopyImport=&BeaconWcsncpy;
}
#pragma comment(linker,"/alternatename:__imp__wmemset=_BeaconSetImport")
#pragma comment(linker,"/alternatename:__imp__wcsncpy=_BeaconCopyImport")
#pragma comment(linker,"/alternatename:__imp__fesetround=_BeaconRoundImport")
