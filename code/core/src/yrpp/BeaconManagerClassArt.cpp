// YR-only BeaconManagerClass::LoadArt, 0x004309D0. Existing RawFile and
// FileSystem resource paths, not a second image format or resource registry.
#include "yrpp/BeaconManagerClass.h"
#include "yrpp/RawFileClass.h"
#include "yrpp/FileSystem.h"
#include "yrpp/Memory.h"
#if !defined(RA2_YRPP_GAME)
namespace {
SHPStruct* beacon_art{};
SHPStruct* radar_art{};
SHPStruct* owned_beacon{};
SHPStruct* owned_radar{};
bool cached_beacon{},cached_radar{};
}
SHPStruct*& BeaconManagerClass::BeaconArt=beacon_art;
SHPStruct*& BeaconManagerClass::RadarBeaconArt=radar_art;
void BeaconManagerClass::ReleaseArt() noexcept {
    if(BeaconArt==owned_beacon)BeaconArt=nullptr;
    if(RadarBeaconArt==owned_radar)RadarBeaconArt=nullptr;
    // FileSystem owns its lookup nodes, while this consumer owns their values.
    // Invalidate before deleting a fallback SHPReference (which also unlinks
    // itself from the existing shape registry and releases its loaded pixels).
    if(cached_beacon)FileSystem::InvalidateName("PBEACON.SHP");
    if(cached_radar)FileSystem::InvalidateName("RDRBEACN.SHP");
    GameDelete(owned_beacon);owned_beacon=nullptr;cached_beacon=false;
    GameDelete(owned_radar);owned_radar=nullptr;cached_radar=false;
}
void BeaconManagerClass::LoadArt() noexcept {
    // Original LoadArt runs once at startup. Standalone reload explicitly
    // retires its previous raw allocations before replacing borrowed globals.
    ReleaseArt();
    RawFileClass beacon_file("PBEACON.SHP");
    owned_beacon=static_cast<SHPStruct*>(beacon_file.ReadWholeFile());
    BeaconArt=owned_beacon;
    if(!BeaconArt)try {
        BeaconArt=static_cast<SHPStruct*>(FileSystem::LoadFile("PBEACON.SHP",false));
        owned_beacon=BeaconArt;cached_beacon=BeaconArt!=nullptr;
    } catch(...) {}
    if(BeaconArt) {
        BeaconSize={BeaconArt->Width,BeaconArt->Height};
        BeaconFrameCount=BeaconArt->Frames;
    }
    RawFileClass radar_file("RDRBEACN.SHP");
    owned_radar=static_cast<SHPStruct*>(radar_file.ReadWholeFile());
    RadarBeaconArt=owned_radar;
    if(!RadarBeaconArt)try {
        RadarBeaconArt=static_cast<SHPStruct*>(FileSystem::LoadFile("RDRBEACN.SHP",false));
        owned_radar=RadarBeaconArt;cached_radar=RadarBeaconArt!=nullptr;
    } catch(...) {}
    if(RadarBeaconArt) {
        RadarBeaconSize={RadarBeaconArt->Width,RadarBeaconArt->Height};
        RadarBeaconFrameCount=RadarBeaconArt->Frames;
        RadarBeaconAnimPeriod=4*RadarBeaconFrameCount;
    }
}
#endif
