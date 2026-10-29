// SPDX-License-Identifier: GPL-3.0-or-later
// Original YR docking and destination queries: 0x447B20 / 0x447E00 / 0x447E90.
#include "yrpp/BuildingClass.h"
#include "RulesClassReaders.hpp"

CoordStruct* BuildingClass::GetDockCoords(CoordStruct* output, TechnoClass* docker) const {
    if (Type->Weeder) {
        const auto cell = GetMapCoords();
        *output = {short(cell.X + 2) * 256 + 128, short(cell.Y + 1) * 256 + 128, Location.Z};
    } else if (Type->Refinery) {
        CoordStruct at;
        *output = *GetCoords(&at);
        output->X += 128;
    } else if (Type->Bunker && docker) {
        const auto client = docker->GetCoords();
        CoordStruct at;
        GetCoords(&at);
        const DirStruct direction(
            rule_integer((Math::atan2(double(at.Y) - client.Y, double(client.X) - at.X) - 1.5707963267948966) *
                         -10430.060040584269));
        const auto facing = direction.GetValue<8>();
        CoordStruct current;
        *output = *GetCoords(&current);
        output->X += facing < 0x80 ? 128 : -128;
        output->Y += facing >= 0x40 && facing < 0xC0 ? 128 : -128;
    } else if (Type->Helipad || Type->UnitRepair) {
        const int count = Type->NumberOfDocks;
        const int index = count == 1 ? 0 : count ? FindLinkIndex(docker) : -1;
        CoordStruct at;
        *output = *GetCoords(&at);
        if (count && index >= 0 && index < count) *output += Type->DockingOffsets[index];
    } else {
        CoordStruct at;
        *output = *GetCoords(&at);
    }
    return output;
}

int BuildingClass::DistanceToDockingCoord(ObjectClass* object) const {
    const auto own = object->GetCoords();
    CoordStruct dock;
    GetDockCoords(&dock, static_cast<TechnoClass*>(object));
    const auto delta = [](int a, int b) { return double(std::bit_cast<int>(unsigned(a) - unsigned(b))); };
    const double x = delta(dock.X, own.X), y = delta(dock.Y, own.Y), z = delta(dock.Z, own.Z);
    return rule_integer(Math::sqrt(x * x + z * z + y * y));
}

CoordStruct* BuildingClass::GetDestination(CoordStruct* output, TechnoClass* docker) const {
    CoordStruct at;
    *output = *(Type->Helipad || Type->UnitRepair || Type->Bunker ? GetDockCoords(&at, docker) : GetCoords(&at));
    return output;
}
