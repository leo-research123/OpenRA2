#include "yrpp/Audio.h"
#include "yrpp/Timer.h"
DWORD SystemTimer::GetMilliseconds() noexcept{return reinterpret_cast<DWORD(__stdcall*)()>(0x01030000)();}
extern "C" {
__declspec(dllexport) unsigned long long __cdecl Extended(){return Audio::GetExtendedTime();}
__declspec(dllexport) void __fastcall Globals(unsigned* p){p[0]=reinterpret_cast<unsigned>(&Audio::LastTime);p[1]=reinterpret_cast<unsigned>(&Audio::TimeReadSerial);}
void __cdecl __std_terminate(){__debugbreak();}
int __cdecl __CxxFrameHandler3(void*,void*,void*,void*){__debugbreak();return 0;}
}
