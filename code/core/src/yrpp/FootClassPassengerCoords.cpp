// Original 0x007104F0: the callback can change the next passenger link.
#include "yrpp/FootClass.h"

void FootClass::UpdatePassengerCoords() {
    auto* passenger = Passengers.GetFirstPassenger();
    const auto location = Location;
    while (passenger) {
        passenger->SetLocation(location);
        auto* next = passenger->NextObject;
        passenger = next && (next->AbstractFlags & ::AbstractFlags::Foot) != ::AbstractFlags::None
                        ? static_cast<FootClass*>(next)
                        : nullptr;
    }
}
