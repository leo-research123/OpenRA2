#include "yrpp/BeaconManagerClass.h"
#include "yrpp/RawFileClass.h"
#include "yrpp/FileSystem.h"
#include "yrpp/Memory.h"
#include <cstddef>
static_assert(sizeof(RawFileClass)==0x24);
RawFileClass::RawFileClass(const char* name):FileClass(noinit_t{}) {
    reinterpret_cast<void(__thiscall*)(RawFileClass*,const char*)>(0x65CA80)(this,name);
}
RawFileClass::~RawFileClass(){reinterpret_cast<void(__thiscall*)(RawFileClass*)>(0x65CA00)(this);}
void* FileClass::ReadWholeFile(){return reinterpret_cast<void*(__thiscall*)(FileClass*)>(0x4A3890)(this);}
void* YRPP_FASTCALL FileSystem::LoadFile(const char* name,bool shape){return reinterpret_cast<void*(__fastcall*)(const char*,bool)>(0x5B40B0)(name,shape);}
// These virtuals are not invoked: original ctor/read/dtor are explicit shared
// resource boundaries, checked in the driver. Trap accidental extra execution.
#define TRAP_RETURN(type,name,args) type RawFileClass::name args {__debugbreak();return {};}
TRAP_RETURN(const char*,GetFileName,() const)
TRAP_RETURN(const char*,SetFileName,(const char*))
TRAP_RETURN(BOOL,CreateFile,())
TRAP_RETURN(BOOL,DeleteFile,())
TRAP_RETURN(bool,Exists,(bool))
TRAP_RETURN(bool,HasHandle,())
TRAP_RETURN(bool,Open,(FileAccessMode))
TRAP_RETURN(bool,OpenEx,(const char*,FileAccessMode))
TRAP_RETURN(int,ReadBytes,(void*,int))
TRAP_RETURN(int,Seek,(int,FileSeekMode))
TRAP_RETURN(int,GetFileSize,())
TRAP_RETURN(int,WriteBytes,(void*,int))
TRAP_RETURN(DWORD,GetFileTime,())
TRAP_RETURN(bool,SetFileTime,(DWORD))
void RawFileClass::Close(){__debugbreak();}
void RawFileClass::CDCheck(DWORD,bool,const char*){__debugbreak();}
DWORD FileClass::GetFileTime(){__debugbreak();return 0;}
bool FileClass::SetFileTime(DWORD){__debugbreak();return false;}
namespace YRMemory {void YRPP_CDECL Deallocate(const void* p){reinterpret_cast<void(__cdecl*)(const void*)>(0x7C8B3D)(p);}}
SHPStruct::~SHPStruct(){} // Cleanup outside the startup oracle; native test runs the real destructor.
void FileSystem::InvalidateName(const char*){} // Same cleanup boundary, not a LoadArt proof.
extern "C" {
__declspec(dllexport) void __fastcall LoadBeaconArt(BeaconManagerClass* manager){manager->LoadArt();}
__declspec(dllexport) void __fastcall ReleaseBeaconArt(BeaconManagerClass*){BeaconManagerClass::ReleaseArt();}
__declspec(dllexport) SHPStruct* __fastcall BeaconArtPointer(BeaconManagerClass*,void*,int radar){return radar?BeaconManagerClass::RadarBeaconArt:BeaconManagerClass::BeaconArt;}
void __cdecl __std_terminate(){__debugbreak();}
int __cdecl __CxxFrameHandler3(void*,void*,void*,void*){__debugbreak();return 0;}
int _fltused=0;
void* __cdecl memset(void* out,int value,std::size_t count){auto* p=static_cast<volatile unsigned char*>(out);while(count--)*p++=static_cast<unsigned char>(value);return out;}
}
void __cdecl operator delete(void*) noexcept {__debugbreak();}

void __cdecl operator delete(void*,std::size_t) noexcept {__debugbreak();}
extern "C" int __cdecl _purecall(){__debugbreak();return 0;}
