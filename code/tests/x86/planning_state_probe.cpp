// Original-layout node/token state methods; no substituted planner graph.
#include "yrpp/PlanningTokenClass.h"
#include "yrpp/Unsorted.h"
#include <new>
#if !defined(RA2_YRPP_GAME)
CoordStruct* YRPP_FASTCALL Game::PlanningManager_EventCoords(CoordStruct* output,const EventClass* event) noexcept {
    return reinterpret_cast<CoordStruct*(__fastcall*)(CoordStruct*,const EventClass*)>(0x633BC0)(output,event);
}
#endif
void* __cdecl YRMemory::Allocate(std::size_t n){__debugbreak();return nullptr;}
void* YRMemory::AllocateChecked(std::size_t n){__debugbreak();return nullptr;}
void __cdecl YRMemory::Deallocate(const void* p){__debugbreak();}
void* __cdecl operator new(std::size_t){__debugbreak();return nullptr;}
void __cdecl operator delete(void*)noexcept{__debugbreak();}
void __cdecl operator delete(void*,std::size_t)noexcept{__debugbreak();}
extern "C" {
__declspec(dllexport) PlanningBranchClass* __fastcall ConstructBranch(void* storage){return ::new(storage) PlanningBranchClass;}
__declspec(dllexport) PlanningTokenClass* __fastcall ConstructToken(void* storage,void*,TechnoClass* owner){return ::new(storage) PlanningTokenClass(owner);}
__declspec(dllexport) TechnoClass* __fastcall Owner(const PlanningNodeClass* node,void*,int index){return node->GetOwner(index);}
__declspec(dllexport) PlanningMemberClass* __fastcall Member(const PlanningNodeClass* node,void*,const TechnoClass* owner){return node->FindMember(owner);}
__declspec(dllexport) CoordStruct* __fastcall Coords(const PlanningNodeClass* node,void*,CoordStruct* output){return node->GetCoords(output);}
__declspec(dllexport) BOOL __fastcall IsAt(const PlanningNodeClass* node,void*,CoordStruct coords){return node->IsAt(coords);}
__declspec(dllexport) PlanningNodeClass* __fastcall TokenNode(const PlanningTokenClass* token,void*,int index){return token->GetNode(index);}
__declspec(dllexport) PlanningNodeClass* __fastcall Last(const PlanningTokenClass* token){return token->GetLastNode();}
__declspec(dllexport) BOOL __fastcall Committed(const PlanningTokenClass* token){return token->HasCommittedNodes();}
__declspec(dllexport) EventClass* __fastcall Event(const PlanningTokenClass* token,void*,EventClass* output,int index){return token->GetEvent(output,index);}
__declspec(dllexport) void __fastcall Commit(PlanningTokenClass* token){token->Commit();}
void* __cdecl memcpy(void* d,const void* s,unsigned n){auto* out=static_cast<unsigned char*>(d);auto* in=static_cast<const unsigned char*>(s);for(unsigned i=0;i<n;++i)out[i]=in[i];return d;}
void* __cdecl memmove(void* d,const void* s,unsigned n){auto* out=static_cast<unsigned char*>(d);auto* in=static_cast<const unsigned char*>(s);if(out<in){for(unsigned i=0;i<n;++i)out[i]=in[i];}else{for(unsigned i=n;i;--i)out[i-1]=in[i-1];}return d;}
void* __cdecl memset(void* d,int c,unsigned n){auto* out=static_cast<unsigned char*>(d);for(unsigned i=0;i<n;++i)out[i]=static_cast<unsigned char>(c);return d;}
void __cdecl __std_terminate(){__debugbreak();}
int __cdecl __CxxFrameHandler3(void*,void*,void*,void*){__debugbreak();return 0;}
void __stdcall _CxxThrowException(void*,void*){__debugbreak();}
void __cdecl __std_exception_copy(const __std_exception_data*,__std_exception_data*){__debugbreak();}
void __cdecl __std_exception_destroy(__std_exception_data*){__debugbreak();}
void* type_info_vtable[1]{};
void __cdecl PlanningWatson(){__debugbreak();}
auto PlanningWatsonImport=&PlanningWatson;
int _fltused=0;
}
#pragma comment(linker,"/alternatename:??_7type_info@@6B@=_type_info_vtable")
#pragma comment(linker,"/alternatename:__imp___invoke_watson=_PlanningWatsonImport")
