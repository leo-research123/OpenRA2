// YR BeaconClass state, calibrated to fixed gamemd.exe. OpenTS 44fac744
// has no BeaconClass implementation; retain the existing YRpp class/layout.
#include "yrpp/BeaconClass.h"
#include "yrpp/HouseClass.h"
#include <cwchar>
#if !defined(RA2_YRPP_GAME)
BeaconClass::BeaconClass() noexcept {
    // 0x00430150 initializes the original empty coordinate to (0,0,0).
    // The constructor leaves HouseID and unused bits/bytes untouched.
    Coord={0,0,0};Bitfield&=static_cast<byte>(~3u);
    std::wmemset(Text,L'\0',128);
}
void BeaconClass::SetCoordAndHouse(CoordStruct coord,int house) noexcept {
    if(coord!=CoordStruct{0,0,0})Coord=coord;
    if(house<8){HouseID=house;Bitfield|=1;}
}
void BeaconClass::SetText(const wchar_t* text) noexcept {
    std::wmemset(Text,L'\0',128);
    if(text)std::wcsncpy(Text,text,127);
}
bool BeaconClass::VisibleToPlayer() const noexcept {
    if(!(Bitfield&1) || !HouseClass::CurrentPlayer->IsAlliedWith(HouseID))return false;
    auto* owner=HouseClass::Array[HouseID];
    return owner->IsAlliedWith(HouseClass::CurrentPlayer) && !owner->Defeated;
}
#endif
