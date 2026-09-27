// Host binding for the existing Tactical class. Normal lifetime/projection
// are shared with compat. Savegame reconstruction and game Update/drawing
// remain unavailable until their actual dependencies are connected.
#include "yrpp/TacticalClass.h"
#include <cstdlib>
#include <cstring>

namespace { TacticalClass* instance = nullptr; }
TacticalClass*& TacticalClass::Instance = instance;
HRESULT YRPP_STDCALL TacticalClass::QueryInterface(REFIID id, void** output) {
    return AbstractClass::QueryInterface(id, output);
}
ULONG YRPP_STDCALL TacticalClass::AddRef() { return AbstractClass::AddRef(); }
ULONG YRPP_STDCALL TacticalClass::Release() { return AbstractClass::Release(); }
HRESULT YRPP_STDCALL TacticalClass::GetClassID(CLSID* output) {
    if (!output) return static_cast<HRESULT>(0x80004003u);
    // 6DBCE0 / 7E9950.
    constexpr DWORD id[]{0xcf56b38a, 0x11d2240d, 0x60007c81, 0xb55b0508};
    static_assert(sizeof(id) == sizeof(CLSID));
    std::memcpy(output, id, sizeof(id));
    return 0;
}
HRESULT YRPP_STDCALL TacticalClass::Load(IStream*) { return static_cast<HRESULT>(0x80004001u); }
HRESULT YRPP_STDCALL TacticalClass::Save(IStream*, BOOL) { return static_cast<HRESULT>(0x80004001u); }
void YRPP_STDCALL TacticalClass::Create_ID() { AbstractClass::Create_ID(); }
void TacticalClass::PointerExpired(AbstractClass*, bool) { std::abort(); }
void TacticalClass::ComputeCRC(CRCEngine& crc) const { AbstractClass::ComputeCRC(crc); }
int TacticalClass::GetOwningHouseIndex() const { return AbstractClass::GetOwningHouseIndex(); }
CoordStruct* TacticalClass::GetCoords(CoordStruct* output) const { return AbstractClass::GetCoords(output); }
CoordStruct* TacticalClass::GetDestination(CoordStruct* output, TechnoClass* target) const {
    return AbstractClass::GetDestination(output, target);
}
CoordStruct* TacticalClass::GetCenterCoords(CoordStruct* output) const { return AbstractClass::GetCenterCoords(output); }
void TacticalClass::Update() { std::abort(); }
bool TacticalClass::sub_6DBB60(const CoordStruct&, const CoordStruct&, COLORREF, DWORD) { std::abort(); }
