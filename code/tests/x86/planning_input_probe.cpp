#include "yrpp/Unsorted.h"
#include "yrpp/EventClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/StringTable.h"
#include "yrpp/VocClass.h"
#include <new>
int& Unsorted::CurrentFrame=*reinterpret_cast<int*>(0xA8ED84);
bool& Unsorted::MoveFeedback=*reinterpret_cast<bool*>(0x822CF2);
bool& PlanningNodeClass::PlanningModeActive=*reinterpret_cast<bool*>(0xAC4CF4);
bool& Game::PlanningErrorReported=*reinterpret_cast<bool*>(0xAC4C08);
HouseClass*& HouseClass::CurrentPlayer=*reinterpret_cast<HouseClass**>(0xA83D4C);
RulesClass*& RulesClass::Instance=*reinterpret_cast<RulesClass**>(0x8871E0);
Randomizer& Randomizer::Global=*reinterpret_cast<Randomizer*>(0x886B88);
QueueClass<EventClass,EventClass::MAX_EVENTS>& EventClass::OutList=*reinterpret_cast<QueueClass<EventClass,EventClass::MAX_EVENTS>*>(0xA802C8);
bool Game::IsAttackMoveMode() noexcept {return reinterpret_cast<bool(__cdecl*)()>(0x731BF0)();}
int Randomizer::Random(){return reinterpret_cast<int(__thiscall*)(Randomizer*)>(0x65C780)(this);}
DWORD SystemTimer::GetMilliseconds() noexcept{return reinterpret_cast<DWORD(__stdcall*)()>(0x01030000)();}
TargetClass::TargetClass(AbstractClass* object) noexcept {
    reinterpret_cast<void(__thiscall*)(TargetClass*,AbstractClass*)>(0x6E6AB0)(this,object);
}
// Link-only dependency of the unused node-neighbor methods in the query TU.
// GetNode itself is compared by the planning state and node probes.
PlanningNodeClass* PlanningTokenClass::GetNode(int index) const noexcept {
    return reinterpret_cast<PlanningNodeClass*(__thiscall*)(const PlanningTokenClass*,int)>(0x636E60)(this,index);
}
AbstractClass* TargetClass::As_Abstract(){return reinterpret_cast<AbstractClass*(__thiscall*)(TargetClass*)>(0x6E6E20)(this);}
CellClass* TargetClass::As_Cell(){return reinterpret_cast<CellClass*(__thiscall*)(TargetClass*)>(0x6E7C20)(this);}
TechnoClass* TargetClass::As_Techno(){return reinterpret_cast<TechnoClass*(__thiscall*)(TargetClass*)>(0x6E6F20)(this);}
void YRPP_FASTCALL Game::ShowMessage(const wchar_t* text,int time)noexcept{reinterpret_cast<void(__fastcall*)(const wchar_t*,int)>(0x730A90)(text,time);}
const wchar_t* YRPP_FASTCALL StringTable::LoadString(const char* name,char** extra,const char* file,int line){return reinterpret_cast<const wchar_t*(__fastcall*)(const char*,char**,const char*,int)>(0x734E60)(name,extra,file,line);}
void YRPP_FASTCALL VocClass::PlayGlobal(int sound,int pan,float volume,AudioController* controller){reinterpret_cast<void(__fastcall*)(int,int,float,AudioController*)>(0x750920)(sound,pan,volume,controller);}
extern "C" {
__declspec(dllexport) CoordStruct* __fastcall EventCoords(CoordStruct* out,const EventClass* event){return Game::PlanningManager_EventCoords(out,event);}
__declspec(dllexport) bool __fastcall LocalGuard(TechnoClass* unit,Mission mission,TargetClass target){return Game::PlanningManager_IsLocalGuard(unit,mission,target);}
__declspec(dllexport) int __fastcall CheckCommand(TechnoClass* unit,const EventClass* event){return Game::PlanningManager_CheckCommand(unit,event);}
__declspec(dllexport) int __fastcall FindMember(const PlanningNodeClass* node,void*,const TechnoClass* owner){return node->FindMemberIndex(owner);}
__declspec(dllexport) bool __fastcall ClickMission(TechnoClass* self,void*,Mission mission,AbstractClass* target,AbstractClass* destination,CellClass* follow) {
    return self->TechnoClass::ClickedMission(mission,target,destination,follow);
}
__declspec(dllexport) bool __fastcall ClickEvent(TechnoClass* self,void*,EventType type){return self->TechnoClass::ClickedEvent(type);}
__declspec(dllexport) BYTE __stdcall SubmitPlan(EventClass event){return Game::PlanningManager_Submit(EventClass(event));}
__declspec(dllexport) bool __stdcall Enqueue(EventClass event){return EventClass::AddEvent(EventClass(event));}
__declspec(dllexport) void __fastcall Reject(const EventClass* event){Game::PlanningManager_RejectEvent(event);}
__declspec(dllexport) EventClass* __fastcall MissionEvent(void* storage,void*,int house,TargetClass source,Mission mission,TargetClass target,TargetClass destination,TargetClass follow) {
    return ::new(storage) EventClass(house,source,mission,target,destination,follow);
}
void* __cdecl memcpy(void* d,const void* s,unsigned n){auto* out=static_cast<unsigned char*>(d);auto* in=static_cast<const unsigned char*>(s);for(unsigned i=0;i<n;++i)out[i]=in[i];return d;}
void* __cdecl memset(void* d,int c,unsigned n){auto* out=static_cast<unsigned char*>(d);for(unsigned i=0;i<n;++i)out[i]=static_cast<unsigned char>(c);return d;}
int _fltused=0;
void __cdecl PlanningInvalid(){__debugbreak();}
auto PlanningInvalidImport=&PlanningInvalid;
void __cdecl __std_terminate(){__debugbreak();}
int __cdecl __CxxFrameHandler3(void*,void*,void*,void*){__debugbreak();return 0;}
}
#pragma comment(linker,"/alternatename:__imp__abort=_PlanningInvalidImport")
