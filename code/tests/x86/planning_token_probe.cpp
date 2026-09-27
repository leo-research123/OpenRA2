#include "yrpp/PlanningTokenClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/Unsorted.h"
#if !defined(RA2_YRPP_GAME)
DynamicVectorClass<PlanningTokenClass*>& PlanningTokenClass::Array=*reinterpret_cast<DynamicVectorClass<PlanningTokenClass*>*>(0xAC4C78);
int (&Game::PlanningMemberCounts)[24]=*reinterpret_cast<int(*)[24]>(0xAC4B84);
HouseClass*& HouseClass::CurrentPlayer=*reinterpret_cast<HouseClass**>(0xA83D4C);
bool HouseClass::IsControlledByCurrentPlayer() const {return reinterpret_cast<bool(__thiscall*)(const HouseClass*)>(0x50B6F0)(this);}
#endif
extern "C" {
__declspec(dllexport) void __fastcall TokenGlobals(unsigned* o){
    o[0]=reinterpret_cast<unsigned>(&Game::PlanningUnits);
    o[1]=reinterpret_cast<unsigned>(&Game::PlanningDeletingAll);
}
__declspec(dllexport) void __fastcall TokenClear(PlanningTokenClass* token){token->ClearNodes();}
__declspec(dllexport) void __fastcall TokenDestruct(PlanningTokenClass* token){token->~PlanningTokenClass();}
__declspec(dllexport) void __fastcall TokenDestroy(PlanningTokenClass* token){token->Destroy();}
__declspec(dllexport) void __fastcall ClearUnit(TechnoClass* unit,const EventClass* event){Game::PlanningManager_ClearToken(unit,event);}
#if defined(RA2_YRPP_GAME)
__declspec(dllexport) void __fastcall ClearMember(TechnoClass* unit,EventClass* event){unit->ClearPlanningTokens(event);}
#endif
__declspec(dllexport) int __fastcall OwnerIndex(const TechnoClass* unit){return Game::PlanningManager_OwnerIndex(unit);}
__declspec(dllexport) void __fastcall CountDrop(unsigned house){Game::PlanningManager_RemoveHouseMember(house);}
}
