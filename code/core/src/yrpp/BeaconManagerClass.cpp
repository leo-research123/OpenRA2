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
    for (auto& house : Beacons)
        for (auto*& beacon : house)
            beacon = nullptr;
    AllocatedCount = 0;
    RadarBeaconAnimPeriod = 0;
}
BeaconManagerClass::~BeaconManagerClass() noexcept { Reset(); }
void BeaconManagerClass::Reset() noexcept {
    if (!AllocatedCount) return;
    for (auto& house : Beacons)
        for (auto*& beacon : house)
            if (beacon) {
                // Original reset calls raw operator delete; Beacon owns no subresources.
                YRMemory::Deallocate(beacon);
                beacon = nullptr;
            }
    AllocatedCount = 0;
}
bool BeaconManagerClass::CanPlaceBeacon(int house) noexcept {
    for (auto* beacon : Beacons[house])
        if (!beacon) return true;
    return false;
}
bool BeaconManagerClass::SelectBeacon(int x, int y, int z) noexcept {
    if (!AllocatedCount) return false;
    const auto delta = [](int a, int b) noexcept { return std::bit_cast<int>(unsigned(a) - unsigned(b)); };
    for (auto& house : Beacons)
        for (auto* beacon : house)
            if (beacon) {
                const double dx = delta(x, beacon->Coord.X), dy = delta(y, beacon->Coord.Y),
                             dz = delta(z, beacon->Coord.Z);
                const int distance = game::x87_integer(Math::sqrt(dx * dx + dz * dz + dy * dy));
                if (distance < 128) {
                    MapClass::UnselectAll();
                    beacon->Bitfield |= 2;
                    return true;
                }
            }
    return false;
}
BeaconClass* BeaconManagerClass::GetBeaconAt(int x, int y, int z) const noexcept {
    if (!AllocatedCount) return nullptr;
    const auto delta = [](int a, int b) noexcept { return std::bit_cast<int>(unsigned(a) - unsigned(b)); };
    for (const auto& house : Beacons)
        for (auto* beacon : house)
            if (beacon) {
                const double dx = delta(x, beacon->Coord.X), dy = delta(y, beacon->Coord.Y),
                             dz = delta(z, beacon->Coord.Z);
                if (game::x87_integer(Math::sqrt(dx * dx + dz * dz + dy * dy)) < 128) return beacon;
            }
    return nullptr;
}
#endif

#include "yrpp/HouseClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/StringTable.h"
#include "yrpp/VocClass.h"
#include "yrpp/VoxClass.h"
#include "yrpp/Unsorted.h"
#if !defined(RA2_YRPP_GAME)
void BeaconManagerClass::PlaceBeacon(int house, CoordStruct coord, int slot) noexcept {
    if (house < 0 || house >= 8 || slot < -1 || slot >= 3) return;
    int free = 0;
    while (free < 3 && Beacons[house][free])
        ++free;
    if (slot == -1 && free == 3) return;
    auto* beacon = GameCreate<BeaconClass>();
    if (slot == -1)
        slot = free;
    else if (Beacons[house][slot]) {
        GameDelete(Beacons[house][slot]);
        --AllocatedCount;
    }
    Beacons[house][slot] = beacon;
    beacon->SetCoordAndHouse(coord, house);
    ++AllocatedCount;
    // Campaign and skirmish do not send beacon packets or play network cues.
    // The standalone host currently has no multiplayer message transport.
    if (SessionClass::Instance.GameMode != GameMode::Campaign && int(SessionClass::Instance.GameMode) != 5 &&
        HouseClass::CurrentPlayer && house == HouseClass::CurrentPlayer->ArrayIndex) {
        Game::ShowMessage(StringTable::LoadString("TXT_BEACON_HELP1"), 225);
        Game::ShowMessage(StringTable::LoadString("TXT_BEACON_HELP2"), 225);
        VoxClass::Play("EVA_BeaconPlaced");
        VocClass::PlayGlobal(RulesClass::Instance->PlaceBeaconSound, 0x2000, 1.0f, nullptr);
    }
}
void BeaconManagerClass::DeleteBeacon(int house, int slot) noexcept {
    if (!AllocatedCount) return;
    const bool selected = house == -1 && slot == -1;
    if (selected) {
        bool found = false;
        for (int h = 0; h < 8 && !found; ++h)
            for (int s = 0; s < 3; ++s)
                if (Beacons[h][s] && (Beacons[h][s]->Bitfield & 2)) {
                    house = h;
                    slot = s;
                    found = true;
                    break;
                }
        if (!found) return;
    }
    if (house < 0 || house >= 8 || slot < 0 || slot >= 3) return;
    auto* beacon = Beacons[house][slot];
    if (!beacon) return;
    if (selected && (!HouseClass::CurrentPlayer || beacon->HouseID != HouseClass::CurrentPlayer->ArrayIndex)) {
        beacon->Bitfield &= static_cast<byte>(~3u);
        return;
    }
    // Whole-frame radar/tactical composition removes the old pixels; no legacy
    // surface erase is needed before retiring this original beacon object.
    GameDelete(beacon);
    Beacons[house][slot] = nullptr;
    --AllocatedCount;
}
void BeaconManagerClass::DeleteAllBeacons(int house) noexcept {
    if (!AllocatedCount || house < 0 || house >= 8) return;
    for (int slot = 0; slot < 3; ++slot)
        if (Beacons[house][slot]) DeleteBeacon(house, slot);
}
#endif
