// Original YR transport notifications: 0x710470..0x7104E9, 0x70D980 and 0x4598A0.
#include "yrpp/TechnoClass.h"
#include "yrpp/FootClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/MapClass.h"

void TechnoClass::EnteredOpenTopped(TechnoClass* passenger) {
    if (!passenger) return;
    passenger->InOpenToppedTransport = true;
    passenger->Guard();
    LogicClass::Instance.AddObject(passenger, false);
}

void TechnoClass::ExitedOpenTopped(TechnoClass* passenger) {
    if (passenger) passenger->InOpenToppedTransport = false;
}

void TechnoClass::MarkPassengersAsExited() {
    auto* passenger = Passengers.FirstPassenger;
    while (passenger) {
        passenger->InOpenToppedTransport = false;
        auto* next = passenger->NextObject;
        passenger = next && (next->AbstractFlags & ::AbstractFlags::Foot) != ::AbstractFlags::None
                        ? static_cast<FootClass*>(next)
                        : nullptr;
    }
}

void TechnoClass::GotHijacked() {}

bool TechnoClass::IsAbsorbAllowed() const {
    const auto* type = static_cast<const BuildingClass*>(this)->Type;
    return type->UnitAbsorb || type->InfantryAbsorb;
}
