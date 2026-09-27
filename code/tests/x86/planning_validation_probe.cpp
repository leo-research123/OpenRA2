#include "yrpp/Unsorted.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/StringTable.h"
#include <cstddef>
static_assert(offsetof(TechnoClass,PlanningToken)==0x514);
static_assert(offsetof(PlanningTokenClass,PlanningNodes)==4);
static_assert(offsetof(PlanningNodeClass,PlanningMembers)==0);
static_assert(offsetof(RulesClass,ScoldSound)==0x700);
bool& PlanningNodeClass::PlanningModeActive=*reinterpret_cast<bool*>(0xAC4CF4);
DynamicVectorClass<ObjectClass*>& ObjectClass::CurrentObjects=*reinterpret_cast<DynamicVectorClass<ObjectClass*>*>(0xA8ECB8);
HouseClass*& HouseClass::CurrentPlayer=*reinterpret_cast<HouseClass**>(0xA83D4C);
RulesClass*& RulesClass::Instance=*reinterpret_cast<RulesClass**>(0x8871E0);
void YRPP_FASTCALL Game::ShowMessage(const wchar_t* text,int time)noexcept{reinterpret_cast<void(__fastcall*)(const wchar_t*,int)>(0x730A90)(text,time);}
const wchar_t* YRPP_FASTCALL StringTable::LoadString(const char* name,char** extra,const char* file,int line){return reinterpret_cast<const wchar_t*(__fastcall*)(const char*,char**,const char*,int)>(0x734E60)(name,extra,file,line);}
void YRPP_FASTCALL VocClass::PlayGlobal(int sound,int pan,float volume,AudioController* controller){reinterpret_cast<void(__fastcall*)(int,int,float,AudioController*)>(0x750920)(sound,pan,volume,controller);}
extern "C" {
__declspec(dllexport) bool __cdecl CheckSelection(){return Game::PlanningManager_CheckSelection();}
__declspec(dllexport) bool __cdecl CheckCapacity(){return Game::PlanningManager_CheckCapacity();}
__declspec(dllexport) bool __fastcall Compatible(const PlanningTokenClass* a,const PlanningTokenClass* b){return Game::PlanningManager_CompatibleTokens(a,b);}
__declspec(dllexport) int __cdecl Unsupported(){return Game::PlanningManager_UnsupportedType();}
__declspec(dllexport) bool __fastcall CanUseWaypoint(TechnoClass* t){return t->TechnoClass::CanUseWaypoint();}
__declspec(dllexport) void __fastcall Globals(unsigned* out){out[0]=reinterpret_cast<unsigned>(&Game::PlanningErrorReported);out[1]=reinterpret_cast<unsigned>(&Game::PlanningMemberCounts);}
int _fltused=0;
void __cdecl __std_terminate(){__debugbreak();}
int __cdecl __CxxFrameHandler3(void*,void*,void*,void*){__debugbreak();return 0;}
}
