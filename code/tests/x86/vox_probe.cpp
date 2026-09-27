#include "yrpp/VoxClass.h"
#include "yrpp/Audio.h"
#include "yrpp/Memory.h"
#include <cstddef>
#include <new>
static_assert(offsetof(VoxClass,unknown_int_50)==0x50);
static_assert(offsetof(VoxClass::QueueEntry,Voice)==0x0C);
static_assert(offsetof(VoxClass::QueueEntry,Priority)==0x14);
static_assert(offsetof(VoxClass::QueueEntry,Control)==0x18);
static_assert(offsetof(VoxClass::QueueEntry,Serial)==0x1C);
bool& Audio::Quiet=*reinterpret_cast<bool*>(0xA8ED64);
AudioVolumeControl*& Audio::MasterVolume=*reinterpret_cast<AudioVolumeControl**>(0x87E740);
AudioVolumeControl*& Audio::SpeechVolume=*reinterpret_cast<AudioVolumeControl**>(0x87E750);
bool Audio::IsAvailable() noexcept{return reinterpret_cast<bool(__cdecl*)()>(0x407000)();}
unsigned long long Audio::GetTime() noexcept{return reinterpret_cast<unsigned long long(__cdecl*)()>(0x4093B0)();}
AudioStream* AudioStream::Create(int buffer,int flags) noexcept {return reinterpret_cast<AudioStream*(__fastcall*)(void*,void*,int,int)>(0x407860)(reinterpret_cast<void*>(0x01001000),nullptr,buffer,flags);}
void AudioStream::Destroy()noexcept{reinterpret_cast<void(__thiscall*)(AudioStream*)>(0x407A90)(this);}
void __fastcall AudioStream::SetName(const char* s)noexcept{reinterpret_cast<void(__fastcall*)(AudioStream*,const char*)>(0x408080)(this,s);}
void __fastcall AudioStream::SetVolumeControl(AudioVolumeControl* v)noexcept{reinterpret_cast<void(__fastcall*)(AudioStream*,AudioVolumeControl*)>(0x407B40)(this,v);}
void __fastcall AudioStream::SetSecondaryVolumeControl(AudioVolumeControl* v)noexcept{reinterpret_cast<void(__fastcall*)(AudioStream*,AudioVolumeControl*)>(0x407B50)(this,v);}
void AudioStream::Stop()noexcept{reinterpret_cast<void(__thiscall*)(AudioStream*)>(0x407F40)(this);}
void AudioStream::Pause()noexcept{reinterpret_cast<void(__thiscall*)(AudioStream*)>(0x407FB0)(this);}
void AudioStream::Resume()noexcept{reinterpret_cast<void(__thiscall*)(AudioStream*)>(0x408000)(this);}
bool AudioStream::IsPlaying()const noexcept{return reinterpret_cast<bool(__thiscall*)(const AudioStream*)>(0x408070)(this);}
unsigned long long AudioStream::EndTime()const noexcept{return reinterpret_cast<unsigned long long(__thiscall*)(const AudioStream*)>(0x408140)(this);}
bool __fastcall AudioStream::PlayWAV(const char* name,bool resource){return reinterpret_cast<bool(__fastcall*)(AudioStream*,const char*,int)>(0x407B60)(this,name,resource);}
void* __cdecl YRMemory::Allocate(std::size_t n){return reinterpret_cast<void*(__cdecl*)(std::size_t)>(0x7C8E17)(n);}
void __cdecl YRMemory::Deallocate(const void* p){reinterpret_cast<void(__cdecl*)(const void*)>(0x7C8B3D)(p);}
void* __cdecl operator new(std::size_t n){return YRMemory::Allocate(n);}
void __cdecl operator delete(void* p)noexcept{YRMemory::Deallocate(p);}
void __cdecl operator delete(void* p,std::size_t)noexcept{YRMemory::Deallocate(p);}
extern "C" {
__declspec(dllexport) void __fastcall Globals(unsigned* o){
    o[0]=reinterpret_cast<unsigned>(&VoxClass::Array);o[1]=reinterpret_cast<unsigned>(&VoxClass::EVAIndex);
    o[2]=reinterpret_cast<unsigned>(&VoxClass::InterruptQueue);o[3]=reinterpret_cast<unsigned>(&VoxClass::CriticalQueue);
    o[4]=reinterpret_cast<unsigned>(&VoxClass::PriorityQueues);o[5]=reinterpret_cast<unsigned>(&VoxClass::StandardSlot);
    o[6]=reinterpret_cast<unsigned>(&VoxClass::Initialized);o[7]=reinterpret_cast<unsigned>(&VoxClass::NextSerial);
    o[8]=reinterpret_cast<unsigned>(&VoxClass::Current);o[9]=reinterpret_cast<unsigned>(&VoxClass::Stream);
    o[10]=reinterpret_cast<unsigned>(&VoxClass::GapMilliseconds);o[11]=reinterpret_cast<unsigned>(&VoxClass::CurrentControl);
    o[12]=reinterpret_cast<unsigned>(&VoxClass::CurrentPriority);o[13]=reinterpret_cast<unsigned>(&VoxClass::SuppressCount);
    o[14]=reinterpret_cast<unsigned>(&VoxClass::PauseCount);
}
__declspec(dllexport) bool __cdecl Init(){return VoxClass::Initialize();}
__declspec(dllexport) void __cdecl Shutdown(){VoxClass::Shutdown();}
__declspec(dllexport) void __cdecl Clear(){VoxClass::ClearQueue();}
__declspec(dllexport) void __fastcall Enqueue(VoxClass* v,int c,int p){VoxClass::Enqueue(v,c,p);}
__declspec(dllexport) VoxClass::QueueEntry* __fastcall Find(const VoxClass* v){return VoxClass::FindQueued(v);}
__declspec(dllexport) void __fastcall Play(int i,int c,int p){VoxClass::PlayIndex(i,c,p);}
__declspec(dllexport) void __fastcall PlayName(const char* n,int c,int p){VoxClass::Play(n,c,p);}
__declspec(dllexport) void __cdecl Update(){VoxClass::Update();}
__declspec(dllexport) void __fastcall Silence(int i){VoxClass::SilenceIndex(i);}
__declspec(dllexport) void __fastcall Stop(bool all){VoxClass::Stop(all);}
__declspec(dllexport) bool __cdecl Speaking(){return VoxClass::IsSpeaking();}
__declspec(dllexport) int __cdecl Suppress(){return VoxClass::Suppress();}
__declspec(dllexport) int __cdecl Unsuppress(){return VoxClass::Unsuppress();}
__declspec(dllexport) int __cdecl Pause(){return VoxClass::Pause();}
__declspec(dllexport) int __cdecl Resume(){return VoxClass::Resume();}
__declspec(dllexport) void __cdecl Reset(){VoxClass::Reset();}
__declspec(dllexport) VoxClass* __fastcall Construct(void* p,void*,const char* n){return ::new(p)VoxClass(n);}
__declspec(dllexport) void __fastcall Destruct(VoxClass* p){p->~VoxClass();}
__declspec(dllexport) const char* __fastcall Filename(VoxClass* p){return p->GetFilename();}
__declspec(dllexport) const char* __fastcall Name(int i){return VoxClass::GetName(i);}
__declspec(dllexport) void __cdecl DeleteAll(){VoxClass::DeleteAll();}
void __cdecl __std_terminate(){__debugbreak();}
int __cdecl __CxxFrameHandler3(void*,void*,void*,void*){__debugbreak();return 0;}
void __stdcall _CxxThrowException(void*,void*){__debugbreak();}
int __cdecl Strcmpi(const char* a,const char* b){return reinterpret_cast<int(__cdecl*)(const char*,const char*)>(0x7C8D20)(a,b);}
auto StrcmpiImport=&Strcmpi;
int _fltused=0;
void* type_info_vtable[1]{};
}
#pragma comment(linker,"/alternatename:__imp___strcmpi=_StrcmpiImport")
#pragma comment(linker,"/alternatename:??_7type_info@@6B@=_type_info_vtable")
void* YRMemory::AllocateChecked(std::size_t n){return Allocate(n);}
void INIClass::Reset(){reinterpret_cast<void(__thiscall*)(INIClass*)>(0x526B00)(this);}
INIClass::INISection* INIClass::GetSection(const char* s){return reinterpret_cast<INISection*(__thiscall*)(INIClass*,const char*)>(0x526810)(this,s);}
double INIClass::ReadDouble(const char* s,const char* k,double v){return reinterpret_cast<double(__thiscall*)(INIClass*,const char*,const char*,double)>(0x5283D0)(this,s,k,v);}
int INIClass::ReadString(const char* s,const char* k,const char* d,char* o,std::size_t n){return reinterpret_cast<int(__thiscall*)(INIClass*,const char*,const char*,const char*,char*,std::size_t)>(0x528A10)(this,s,k,d,o,n);}
int INIClass::GetKeyCount(const char* s){return reinterpret_cast<int(__thiscall*)(INIClass*,const char*)>(0x526960)(this,s);}
const char* INIClass::GetKeyName(const char* s,int i){return reinterpret_cast<const char*(__thiscall*)(INIClass*,const char*,int)>(0x526CC0)(this,s,i);}
extern "C" {
void __cdecl __std_exception_copy(const __std_exception_data*,__std_exception_data*){__debugbreak();}
void __cdecl __std_exception_destroy(__std_exception_data*){__debugbreak();}
int __cdecl atexit(void(__cdecl*)()){return 0;}
void __cdecl Trap(){__debugbreak();}
auto AbortImport=&Trap;
auto WatsonImport=&Trap;
std::size_t __cdecl strlen(const char* s){std::size_t n=0;auto* p=static_cast<const volatile char*>(s);while(p[n])++n;return n;}
char* __cdecl strcpy(char* a,const char* b){auto* p=static_cast<volatile char*>(a);auto* q=static_cast<const volatile char*>(b);while((*p++=*q++));return a;}
char* __cdecl strcat(char* a,const char* b){strcpy(a+strlen(a),b);return a;}
char* __cdecl CopyBounded(char* a,const char* b,std::size_t n){std::size_t i=0;auto* p=static_cast<volatile char*>(a);auto* q=static_cast<const volatile char*>(b);for(;i<n&&q[i];++i)p[i]=q[i];for(;i<n;++i)p[i]=0;return a;}
auto StrncpyImport=&CopyBounded;
void* __cdecl memset(void* a,int v,std::size_t n){auto* p=static_cast<volatile unsigned char*>(a);for(std::size_t i=0;i<n;++i)p[i]=v;return a;}
void* __cdecl memcpy(void* a,const void* b,std::size_t n){auto* p=static_cast<volatile unsigned char*>(a);auto* q=static_cast<const volatile unsigned char*>(b);for(std::size_t i=0;i<n;++i)p[i]=q[i];return a;}
}
#pragma comment(linker,"/alternatename:__imp__abort=_AbortImport")
#pragma comment(linker,"/alternatename:__imp___invoke_watson=_WatsonImport")
#pragma comment(linker,"/alternatename:__imp__strncpy=_StrncpyImport")
