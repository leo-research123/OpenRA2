// YR-only original BeaconManagerClass; existing YRpp declaration owns the
// 8x3 table. No equivalent exists in the fixed OpenTS 44fac744 baseline.
#include "yrpp/BeaconManagerClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/Memory.h"
#include "yrpp/YRMath.h"
#include "x87_integer.hpp"
#include <bit>
#if defined(__clang__)
#pragma STDC FENV_ACCESS ON
#elif defined(_MSC_VER)
#pragma fenv_access(on)
#endif
#if !defined(RA2_YRPP_GAME)
BeaconManagerClass::BeaconManagerClass() noexcept {
    for(auto& house:Beacons)for(auto*& beacon:house)beacon=nullptr;
    AllocatedCount=0;RadarBeaconAnimPeriod=0;
}
BeaconManagerClass::~BeaconManagerClass() noexcept {Reset();}
void BeaconManagerClass::Reset() noexcept {
    if(!AllocatedCount)return;
    for(auto& house:Beacons)for(auto*& beacon:house)if(beacon) {
        // Original reset calls raw operator delete; Beacon owns no subresources.
        YRMemory::Deallocate(beacon);beacon=nullptr;
    }
    AllocatedCount=0;
}
bool BeaconManagerClass::CanPlaceBeacon(int house) noexcept {
    for(auto* beacon:Beacons[house])if(!beacon)return true;
    return false;
}
bool BeaconManagerClass::SelectBeacon(int x,int y,int z) noexcept {
    if(!AllocatedCount)return false;
    const auto delta=[](int a,int b) noexcept {return std::bit_cast<int>(unsigned(a)-unsigned(b));};
    for(auto& house:Beacons)for(auto* beacon:house)if(beacon) {
        const double dx=delta(x,beacon->Coord.X),dy=delta(y,beacon->Coord.Y),dz=delta(z,beacon->Coord.Z);
        const int distance=game::x87_integer(Math::sqrt(dx*dx+dz*dz+dy*dy));
        if(distance<128) {
            MapClass::UnselectAll();
            beacon->Bitfield|=2;
            return true;
        }
    }
    return false;
}
#endif
