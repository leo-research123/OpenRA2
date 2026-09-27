#include "yrpp/MessageListClass.h"
#include "yrpp/BitFont.h"
#include "yrpp/Unsorted.h"
#include "yrpp/RulesClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/SessionClass.h"
#include "game_ui_runtime.hpp"
#include "scenario_runtime.hpp"
#include <cstddef>
#include <new>
bool TextLabelClass::Draw(bool forced){return reinterpret_cast<bool(__thiscall*)(TextLabelClass*,bool)>(0x72A4A0)(this,forced);}
static_assert(sizeof(TextLabelClass)==0x4C);
static_assert(offsetof(TextLabelClass,Text)==0x30);
static_assert(offsetof(MessageListClass,MessageBuffers)==0x2C8);
static_assert(offsetof(MessageListClass,BufferAvail)==0x1480);
static_assert(offsetof(RulesClass,IncomingMessage)==0x6AC);
SysElapsedTimerClass& Game::TickCount=*reinterpret_cast<SysElapsedTimerClass*>(0x887338);
MessageListClass& MessageListClass::Instance=*reinterpret_cast<MessageListClass*>(0xA8BC60);
bool& SessionClass::AnimateLanMessages=*reinterpret_cast<bool*>(0xA8D1F8);
bool& SessionClass::AnimateInternetMessages=*reinterpret_cast<bool*>(0xA8D1F9);
RulesClass*& RulesClass::Instance=*reinterpret_cast<RulesClass**>(0x8871E0);
GadgetClass*& GadgetClass::StuckOn=*reinterpret_cast<GadgetClass**>(0x8B3E88);
GadgetClass*& GadgetClass::LastList=*reinterpret_cast<GadgetClass**>(0x8B3E8C);
GadgetClass*& GadgetClass::Focused=*reinterpret_cast<GadgetClass**>(0x8B3E90);
GadgetClass*& GadgetClass::Hovered=*reinterpret_cast<GadgetClass**>(0x8B3E94);
BitFont*& BitFont::Instance=*reinterpret_cast<BitFont**>(0x89C4D0);
bool BitFont::GetTextDimension(const wchar_t* text,int* width,int* height,int maximum){
    return reinterpret_cast<bool(__thiscall*)(BitFont*,const wchar_t*,int*,int*,int)>(0x433CF0)(this,text,width,height,maximum);
}
unsigned char* BitFont::GetCharacterBitmap(wchar_t){return reinterpret_cast<unsigned char*>(0x01009000);}
DWORD SystemTimer::GetTime(){return reinterpret_cast<DWORD(__cdecl*)()>(0x6C8C40)();}
void YRPP_FASTCALL VocClass::PlayGlobal(int index,int pan,float volume,AudioController* controller){
    reinterpret_cast<void(__fastcall*)(int,int,float,AudioController*)>(0x750920)(index,pan,volume,controller);
}
namespace game {
const GameUiInput* game_ui_input() noexcept{return nullptr;}
const ScenarioRuntimeServices& scenario_runtime(){
 static const ScenarioRuntimeServices services{.session_mode=[](void*)noexcept{return *reinterpret_cast<int*>(0xA8B238);}};return services;
}
}
extern "C" {
__declspec(dllexport) void __fastcall MessageConstruct(MessageListClass* list){new(list)MessageListClass;}
__declspec(dllexport) void __fastcall MessageDestroy(MessageListClass* list){list->~MessageListClass();}
__declspec(dllexport) void __fastcall MessageInit(MessageListClass* list,void*,int x,int y,int count,int chars,int height,int ex,int ey,int overflow,int first,int last,int width){list->Init(x,y,count,chars,height,ex,ey,overflow,first,last,width);}
__declspec(dllexport) TextLabelClass* __fastcall MessageAdd(MessageListClass* list,void*,const wchar_t* name,int id,const wchar_t* text,int color,TextPrintType style,int timeout,bool silent){return list->AddMessage(name,id,text,color,style,timeout,silent);}
__declspec(dllexport) int __fastcall MessageManage(MessageListClass* list){return list->Manage();}
__declspec(dllexport) TextLabelClass* __fastcall MessageGetLabel(MessageListClass* list,void*,int id){return list->GetLabel(id);}
__declspec(dllexport) wchar_t* __fastcall MessageGet(MessageListClass* list,void*,int id){return list->GetMessage(id);}
__declspec(dllexport) int __fastcall MessageCount(MessageListClass* list){return list->NumMessages();}
__declspec(dllexport) void __fastcall MessageWidth(MessageListClass* list,void*,int width){list->SetWidth(width);}
__declspec(dllexport) void __fastcall MessageY(MessageListClass* list){list->ComputeY();}
__declspec(dllexport) int __fastcall MessageTrim(MessageListClass* list,void*,wchar_t* dest,wchar_t* source,int min,int max,int direction){return list->TrimMessage(dest,source,min,max,direction);}
__declspec(dllexport) int __fastcall FontFit(BitFont* font,void*,const wchar_t* text,int width,int limit,bool words){return font->GetTextFit(text,width,limit,words);}
__declspec(dllexport) int __fastcall FontWidth(BitFont* font,void*,const wchar_t* text,int width){return font->GetTextWidth(text,width);}
__declspec(dllexport) void __fastcall BindFont(BitFont* font){BitFont::Instance=font;}
int _fltused=0;
void __cdecl __std_terminate(){__debugbreak();}
int __cdecl __CxxFrameHandler3(void*,void*,void*,void*){__debugbreak();return 0;}
int __cdecl _purecall(){__debugbreak();return 0;}
void* __cdecl memset(void* output,int value,std::size_t count){auto* to=static_cast<volatile unsigned char*>(output);for(std::size_t i=0;i<count;++i)to[i]=static_cast<unsigned char>(value);return output;}
void* __cdecl memcpy(void* output,const void* input,std::size_t count){return reinterpret_cast<void*(__cdecl*)(void*,const void*,std::size_t)>(0x7CA090)(output,input,count);}
void* __cdecl memmove(void* output,const void* input,std::size_t count){return reinterpret_cast<void*(__cdecl*)(void*,const void*,std::size_t)>(0x7CA090)(output,input,count);}
std::size_t __cdecl wcslen(const wchar_t* p){return reinterpret_cast<std::size_t(__cdecl*)(const wchar_t*)>(0x7CA405)(p);}
wchar_t* __cdecl wcscpy(wchar_t* out,const wchar_t* p){return reinterpret_cast<wchar_t*(__cdecl*)(wchar_t*,const wchar_t*)>(0x7CA489)(out,p);}
wchar_t* __cdecl wcscat(wchar_t* out,const wchar_t* p){return reinterpret_cast<wchar_t*(__cdecl*)(wchar_t*,const wchar_t*)>(0x7CA45F)(out,p);}
wchar_t* __cdecl wcsncat(wchar_t* out,const wchar_t* p,std::size_t n){return reinterpret_cast<wchar_t*(__cdecl*)(wchar_t*,const wchar_t*,std::size_t)>(0x7CB504)(out,p,n);}
wchar_t* __cdecl MessageWmemset(wchar_t* out,wchar_t c,std::size_t n){for(std::size_t i=0;i<n;++i)out[i]=c;return out;}
auto MessageWmemsetImport=&MessageWmemset;
}
void* __cdecl operator new(std::size_t n){return reinterpret_cast<void*(__cdecl*)(std::size_t)>(0x7C8E17)(n);}
void __cdecl operator delete(void* p) noexcept{reinterpret_cast<void(__cdecl*)(void*)>(0x7C8B3D)(p);}
void __cdecl operator delete(void* p,std::size_t) noexcept{::operator delete(p);}
#pragma comment(linker,"/alternatename:__imp__wmemset=_MessageWmemsetImport")
