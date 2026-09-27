// Key branch of the production Radar input implementation. Parent update,
// cursor, audio and movies are separate boundaries in this bounded audit.
#include "yrpp/RadarClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/Unsorted.h"
#include "scenario_runtime.hpp"
int& Game::SpecialDialog=*reinterpret_cast<int*>(0xA8EDA0);
extern "C" {
__declspec(dllexport) void __fastcall RadarButtonKey(RadarClass* self,void*,DWORD key){self->ProcessButtonKey(key);}
void __cdecl __std_terminate(){__debugbreak();}
int __cdecl __CxxFrameHandler3(void*,void*,void*,void*){__debugbreak();return 0;}
void __cdecl RadarButtonsUnexpected(){__debugbreak();}
auto RadarButtonsUnexpectedImport=&RadarButtonsUnexpected;
}
#pragma comment(linker,"/alternatename:__imp____stdio_common_vsprintf=_RadarButtonsUnexpectedImport")
#pragma comment(linker,"/alternatename:__imp____stdio_common_vswprintf=_RadarButtonsUnexpectedImport")
namespace game {
const ScenarioRuntimeServices& scenario_runtime(){
    static const ScenarioHouseServices houses{.session=reinterpret_cast<SessionClass*>(0xA8B238)};
    static const ScenarioRuntimeServices runtime{.houses=&houses};return runtime;
}
}
