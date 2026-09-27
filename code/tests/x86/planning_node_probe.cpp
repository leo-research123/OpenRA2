#include "yrpp/PlanningTokenClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/TechnoClass.h"
#include <new>
#if !defined(RA2_YRPP_GAME)
DynamicVectorClass<PlanningNodeClass*>& PlanningNodeClass::Unknown1=*reinterpret_cast<DynamicVectorClass<PlanningNodeClass*>*>(0xAC4B30);
DynamicVectorClass<PlanningNodeClass*>& PlanningNodeClass::Unknown2=*reinterpret_cast<DynamicVectorClass<PlanningNodeClass*>*>(0xAC4C18);
DynamicVectorClass<PlanningNodeClass*>& PlanningNodeClass::Unknown3=*reinterpret_cast<DynamicVectorClass<PlanningNodeClass*>*>(0xAC4C98);
CoordStruct* YRPP_FASTCALL Game::PlanningManager_EventCoords(CoordStruct* output,const EventClass* event) noexcept {
    return reinterpret_cast<CoordStruct*(__fastcall*)(CoordStruct*,const EventClass*)>(0x633BC0)(output,event);
}
#endif
void* __cdecl YRMemory::Allocate(std::size_t n){return reinterpret_cast<void*(__cdecl*)(std::size_t)>(0x7C8E17)(n);}
void* YRMemory::AllocateChecked(std::size_t n){return Allocate(n);}
void __cdecl YRMemory::Deallocate(const void* p){reinterpret_cast<void(__cdecl*)(const void*)>(0x7C8B3D)(p);}
void* __cdecl operator new(std::size_t n){return YRMemory::Allocate(n);}
void __cdecl operator delete(void* p)noexcept{YRMemory::Deallocate(p);}
void __cdecl operator delete(void* p,std::size_t)noexcept{YRMemory::Deallocate(p);}
extern "C" {
__declspec(dllexport) void __fastcall Globals(unsigned* o){
    o[0]=reinterpret_cast<unsigned>(&Game::PlanningNodeAtAC4CCC);
    o[1]=reinterpret_cast<unsigned>(&Game::PlanningNodeAtAC4C38);
    o[2]=reinterpret_cast<unsigned>(&Game::PlanningNodeAtAC4BF0);
}
__declspec(dllexport) PlanningNodeClass* __fastcall Previous(const PlanningNodeClass* node,void*,TechnoClass* owner){return node->GetPrevious(owner);}
__declspec(dllexport) PlanningNodeClass* __fastcall Next(const PlanningNodeClass* node,void*,TechnoClass* owner){return node->GetNext(owner);}
__declspec(dllexport) int __fastcall FindBranch(const PlanningNodeClass* node,void*,TechnoClass* owner){return node->FindBranch(owner);}
__declspec(dllexport) int __fastcall AddBranch(PlanningNodeClass* node,void*,PlanningMemberClass* member){return node->AddBranch(member);}
__declspec(dllexport) int __fastcall AddMember(PlanningNodeClass* node,void*,TechnoClass* owner,const EventClass* event){return node->AddMember(owner,event);}
__declspec(dllexport) void __fastcall RemoveMember(PlanningNodeClass* node,void*,TechnoClass* owner){node->RemoveMember(owner);}
__declspec(dllexport) void __fastcall ReleaseBranch(PlanningNodeClass* node,void*,PlanningMemberClass* member){node->ReleaseBranch(member);}
__declspec(dllexport) int __fastcall ReleaseLoop(PlanningNodeClass* node,void*,PlanningMemberClass* member){return node->ReleaseLoopBranch(member);}
__declspec(dllexport) void __fastcall ClearBranches(PlanningNodeClass* node){node->ClearBranches();}
__declspec(dllexport) void __fastcall Invalidate(PlanningNodeClass* node,void*,bool branches){node->InvalidateDisplay(branches);}
__declspec(dllexport) void __fastcall UpdateLoop(PlanningNodeClass* node){node->UpdateLoopBranch();}
__declspec(dllexport) void __fastcall FlagNode(PlanningNodeClass* node){Game::PlanningManager_FlagNode(node);}
__declspec(dllexport) void __fastcall UnregisterNode(PlanningNodeClass* node){Game::PlanningManager_UnregisterNode(node);}
__declspec(dllexport) void __fastcall DestroyNode(PlanningNodeClass* node){node->~PlanningNodeClass();}
void* __cdecl memcpy(void* d,const void* s,unsigned n){auto* out=static_cast<unsigned char*>(d);auto* in=static_cast<const unsigned char*>(s);for(unsigned i=0;i<n;++i)out[i]=in[i];return d;}
void* __cdecl memmove(void* d,const void* s,unsigned n){auto* out=static_cast<unsigned char*>(d);auto* in=static_cast<const unsigned char*>(s);if(out<in){for(unsigned i=0;i<n;++i)out[i]=in[i];}else{for(unsigned i=n;i;--i)out[i-1]=in[i-1];}return d;}
void* __cdecl memset(void* d,int c,unsigned n){auto* out=static_cast<unsigned char*>(d);for(unsigned i=0;i<n;++i)out[i]=static_cast<unsigned char>(c);return d;}
void __cdecl __std_terminate(){__debugbreak();}
int __cdecl __CxxFrameHandler3(void*,void*,void*,void*){__debugbreak();return 0;}
int __cdecl atexit(void(__cdecl*)()){return 0;}
void __stdcall _CxxThrowException(void*,void*){__debugbreak();}
void __cdecl __std_exception_copy(const __std_exception_data*,__std_exception_data*){__debugbreak();}
void __cdecl __std_exception_destroy(__std_exception_data*){__debugbreak();}
void* type_info_vtable[1]{};
void __cdecl PlanningTrap(){__debugbreak();}
auto PlanningTrapImport=&PlanningTrap;
int _fltused=0;
}
#pragma comment(linker,"/alternatename:??_7type_info@@6B@=_type_info_vtable")
#pragma comment(linker,"/alternatename:__imp___invoke_watson=_PlanningTrapImport")
#pragma comment(linker,"/alternatename:__imp__abort=_PlanningTrapImport")
