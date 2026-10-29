// Original infantry trigger teleport entrance, 0x00522FE0.
#include "yrpp/InfantryClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/LocomotionClass.h"
#include <bit>
#include <cstdlib>

bool InfantryClass::ChronoWarpTo(CoordStruct destination) noexcept {
    try {
        if (IsInAir() || IsIronCurtained() || IsImmobilized || IsBeingWarpedOut() || IsWarpingIn()) return false;
        if (!Locomotor) std::abort();
        Locomotor->Mark_All_Occupation_Bits(MarkType::Up);
        UnmarkAllOccupationBits(destination);
        FrozenStill = true;
        SetDestination(MapClass::Instance.GetCellAt(destination), true);
        if (!Locomotor) std::abort();
        Locomotor->Force_Immediate_Destination(destination);
        ILocomotion* next = nullptr;
        IPiggyback* piggy = nullptr;
        const auto check = [](HRESULT result) {
            if (result < 0 && result != static_cast<HRESULT>(0x80004002u)) std::abort();
        };
        check(LocomotionClass::CreateInstance(&next, &LocomotionClass::CLSIDs::Teleport, nullptr, 7));
        if (next) check(LocomotionClass::TryPiggyback(&piggy, &next));
        if ((unsigned(MapClass::Instance.GetCellAt(destination)->Flags) & 0x100u) != 0)
            destination.Z = std::bit_cast<int>(unsigned(destination.Z) + unsigned(CellClass::BridgeHeight));
        if (!next) std::abort();
        next->Link_To_Object(this);
        if (!piggy) std::abort();
        piggy->Begin_Piggyback(Locomotor);
#if defined(_MSC_VER)
        Locomotor = next;
#else
        if (Locomotor != next) {
            auto* old = Locomotor;
            Locomotor = next;
            next->AddRef();
            if (old) old->Release();
        }
#endif
        IsImmobilized = true;
        ChronoDestCoords = destination;
        SetDestination(MapClass::Instance.GetCellAt(destination), true);
        if (!Locomotor) std::abort();
        Locomotor->Mark_All_Occupation_Bits(MarkType::Up);
        if (piggy) piggy->Release();
        if (next) next->Release();
        return true;
    } catch (...) {
        std::abort();
    }
}
