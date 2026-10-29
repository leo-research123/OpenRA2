// Native class activation for the original locomotion COM interfaces.
#include "yrpp/LocomotionClass.h"
#include "yrpp/TeleportLocomotionClass.h"
#include "yrpp/WalkLocomotionClass.h"
#include "yrpp/DriveLocomotionClass.h"
#include "yrpp/ShipLocomotionClass.h"
#include "yrpp/HoverLocomotionClass.h"
#include "yrpp/JumpjetLocomotionClass.h"
#include "yrpp/FlyLocomotionClass.h"
#include "yrpp/RocketLocomotionClass.h"
#include <cstring>

namespace {
template <typename T> T* allocate_locomotor() {
    void* storage = YRMemory::Allocate(sizeof(T));
    return storage ? ::new (storage) T() : nullptr;
}
}

HRESULT LocomotionClass::CreateInstance(ILocomotion** output, const CLSID* clsid, IUnknown* outer, DWORD) noexcept {
    if (!output || !clsid) return static_cast<HRESULT>(0x80004003u);
    if (*output) {
        (*output)->Release();
        *output = nullptr;
    }
    if (outer) return static_cast<HRESULT>(0x80040110u);
    try {
        LocomotionClass* value = nullptr;
        if (!std::memcmp(clsid, &CLSIDs::Teleport, sizeof(CLSID)))
            value = allocate_locomotor<TeleportLocomotionClass>();
        else if (!std::memcmp(clsid, &CLSIDs::Walk, sizeof(CLSID)))
            value = allocate_locomotor<WalkLocomotionClass>();
        else if (!std::memcmp(clsid, &CLSIDs::Drive, sizeof(CLSID)))
            value = allocate_locomotor<DriveLocomotionClass>();
        else if (!std::memcmp(clsid, &CLSIDs::Ship, sizeof(CLSID)))
            value = allocate_locomotor<ShipLocomotionClass>();
        else if (!std::memcmp(clsid, &CLSIDs::Hover, sizeof(CLSID)))
            value = allocate_locomotor<HoverLocomotionClass>();
        else if (!std::memcmp(clsid, &CLSIDs::Jumpjet, sizeof(CLSID)))
            value = allocate_locomotor<JumpjetLocomotionClass>();
        else if (!std::memcmp(clsid, &CLSIDs::Fly, sizeof(CLSID)))
            value = allocate_locomotor<FlyLocomotionClass>();
        else if (!std::memcmp(clsid, &CLSIDs::Rocket, sizeof(CLSID)))
            value = allocate_locomotor<RocketLocomotionClass>();
        else
            return static_cast<HRESULT>(0x80040154u);
        if (!value) return static_cast<HRESULT>(0x8007000Eu);
        value->AddRef();
        *output = static_cast<ILocomotion*>(value);
        return 0;
    } catch (...) {
        return static_cast<HRESULT>(0x8007000Eu);
    }
}
HRESULT LocomotionClass::TryPiggyback(IPiggyback** output, ILocomotion** locomotor) noexcept {
    if (!output || !locomotor) return static_cast<HRESULT>(0x80004003u);
    try {
        if (!*locomotor) {
            auto* old = *output;
            *output = nullptr;
            if (old) old->Release();
            return static_cast<HRESULT>(0x80004002u);
        }
        constexpr GUID iid{0x92FEA800, 0xA184, 0x11D1, {0xB7, 0x0A, 0, 0xA0, 0x24, 0xDD, 0xAF, 0xD1}};
        IPiggyback* value = nullptr;
        const HRESULT result = (*locomotor)->QueryInterface(iid, reinterpret_cast<void**>(&value));
        if (*output) (*output)->Release();
        *output = result >= 0 ? value : nullptr;
        return result;
    } catch (...) {
        return static_cast<HRESULT>(0x80004005u);
    }
}
