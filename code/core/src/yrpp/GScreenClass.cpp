// Existing YRpp 9402d7da hierarchy; field initialization calibrated to fixed
// YR 7b8a0685.
#include "yrpp/GScreenClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TacticalClass.h"
#include <cstring>
GScreenClass::GScreenClass() noexcept : ScreenShakeX(0), ScreenShakeY(0), Bitfield(2) {}
GScreenClass::~GScreenClass() = default;
void GScreenClass::MarkNeedsRedraw(int mode) {
    if(auto* tactical=TacticalClass::Instance)tactical->Redrawing=true;
    if(mode){
        if(Bitfield!=2)Bitfield=mode;
        auto& redraws=MapClass::Instance.Redraws;
        // 0x00578AC0 increments only the low byte, without carrying into the
        // neighboring bytes represented by YRpp's existing BOOL field.
        redraws=static_cast<BOOL>((static_cast<unsigned>(redraws)&0xFFFFFF00u)|
            static_cast<BYTE>(static_cast<unsigned>(redraws)+1));
    }
}
ULONG YRPP_STDCALL GScreenClass::AddRef() { return 1; }
ULONG YRPP_STDCALL GScreenClass::Release() { return 1; }
HRESULT YRPP_STDCALL GScreenClass::QueryInterface(REFIID iid, void** output) {
    if (!output) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD unknown[4]{0,0,0x000000c0,0x46000000};
    constexpr DWORD map[4]{0x96f02ec7,0x11d16fe8,0xa000fdb6,0xd1afdd24};
    static_assert(sizeof(iid) == 16);
    if (std::memcmp(&iid, unknown, 16) && std::memcmp(&iid, map, 16))
        return static_cast<HRESULT>(0x80004002u); // original preserves output
    *output = static_cast<IGameMap*>(this);
    AddRef();
    return 0;
}
