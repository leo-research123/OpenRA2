// YR 0x4DB3C0 / 0x4DB690: mobile stream tail, after the Techno record.
#include "yrpp/FootClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/ParasiteClass.h"
#include "yrpp/LocomotionClass.h"
#include "yrpp/SwizzleManagerClass.h"
#include "path_requests.hpp"
#include <cstdint>
#include <cstdlib>
#include <new>
#if !defined(RA2_YRPP_GAME)
namespace {
// IStream stays opaque on portable hosts. Read/Write are the original COM
// slots 3/4; no host stream object or alternate serialization format is used.
HRESULT stream_read(IStream* stream, void* output, ULONG length) {
    const auto table = *reinterpret_cast<void* const* const*>(stream);
    const auto call = reinterpret_cast<HRESULT(YRPP_STDCALL*)(IStream*, void*, ULONG, ULONG*)>(table[3]);
    return call(stream, output, length, nullptr);
}
HRESULT stream_write(IStream* stream, const void* input, ULONG length) {
    const auto table = *reinterpret_cast<void* const* const*>(stream);
    const auto call = reinterpret_cast<HRESULT(YRPP_STDCALL*)(IStream*, const void*, ULONG, ULONG*)>(table[4]);
    return call(stream, input, length, nullptr);
}
constexpr GUID persistStreamID{0x109, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};
constexpr GUID locomotionID{0x070F3290, 0x9841, 0x11D1, {0xB7, 0x09, 0, 0xA0, 0x24, 0xDD, 0xAF, 0xD1}};
void save_locomotor(IPersistStream* object, IStream* stream) {
    if (!object) return;
    CLSID id;
    if (object->GetClassID(&id) < 0) return;
    if (stream_write(stream, &id, sizeof(id)) < 0) return;
    object->Save(stream, true);
}
void load_locomotor(IStream* stream, ILocomotion** output) {
    CLSID id;
    if (stream_read(stream, &id, sizeof(id)) < 0) return;
    ILocomotion* object = nullptr;
    const auto result = LocomotionClass::CreateInstance(&object, &id, nullptr, 1);
    if (result < 0 || !object) return;
    IPersistStream* persist = nullptr;
    if (object->QueryInterface(persistStreamID, reinterpret_cast<void**>(&persist)) >= 0 && persist) {
        if (persist->Load(stream) >= 0) object->QueryInterface(locomotionID, reinterpret_cast<void**>(output));
        persist->Release();
    }
    object->Release();
}
}
HRESULT YRPP_STDCALL FootClass::Save(IStream* stream, BOOL clearDirty) {
    try {
        auto result = TechnoClass::Save(stream, clearDirty);
        if (result < 0) return result;
        const auto write = [&](const DynamicVectorClass<AbstractClass*>& list) {
            const int count = list.Count;
            auto status = stream_write(stream, &count, 4);
            if (status < 0) return status;
            for (int i = 0; i < count; ++i) {
                const auto identity = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(list[i]));
                status = stream_write(stream, &identity, 4);
                if (status < 0) return status;
            }
            return HRESULT(0);
        };
        result = write(unknown_abstract_array_588);
        if (result < 0) return result;
        result = write(NavQueue);
        if (result < 0) return result;
        IPersistStream* persist = nullptr;
        if (Locomotor) {
            result = Locomotor->QueryInterface(persistStreamID, reinterpret_cast<void**>(&persist));
            if (result < 0) {
                persist = nullptr;
                if (result != static_cast<HRESULT>(0x80004002u)) std::abort();
            }
        }
        save_locomotor(persist, stream);
        if (persist) persist->Release();
        return 0;
    } catch (...) {
        std::abort();
    }
}
HRESULT YRPP_STDCALL FootClass::Load(IStream* stream) {
    game::forget_path_requests(*this);
    try {
#if defined(_MSC_VER)
        Locomotor = nullptr;
#else
        if (Locomotor) Locomotor->Release();
        Locomotor = nullptr;
#endif
        auto result = TechnoClass::Load(stream);
        if (result < 0) return result;
        const auto read = [&](DynamicVectorClass<AbstractClass*>& list) {
            int count;
            auto status = stream_read(stream, &count, 4);
            if (status < 0) return status;
            // The parent record contains saved addresses, not owned allocations.
            ::new (&list) DynamicVectorClass<AbstractClass*>(0);
            for (int i = 0; i < count; ++i) {
                std::uint32_t identity = 0;
                status = stream_read(stream, &identity, 4);
                if (status < 0) return status;
                (void)list.AddItem(reinterpret_cast<AbstractClass*>(static_cast<std::uintptr_t>(identity)));
            }
            return HRESULT(0);
        };
        result = read(unknown_abstract_array_588);
        if (result < 0) return result;
        result = read(NavQueue);
        if (result < 0) return result;
        ILocomotion* restored = nullptr;
        load_locomotor(stream, &restored);
#if defined(_MSC_VER)
        Locomotor.Attach(restored);
#else
        Locomotor = restored;
#endif
        unknown_5A0 = nullptr;
        auto& swizzle = SwizzleManagerClass::Instance;
        swizzle.Swizzle(reinterpret_cast<void**>(&Team));
        swizzle.Swizzle(reinterpret_cast<void**>(&NextTeamMember));
        swizzle.Swizzle(reinterpret_cast<void**>(&Destination));
        swizzle.Swizzle(reinterpret_cast<void**>(&LastDestination));
        swizzle.Swizzle(reinterpret_cast<void**>(&MegaDestination));
        swizzle.Swizzle(reinterpret_cast<void**>(&MegaTarget));
        swizzle.Swizzle(reinterpret_cast<void**>(&PatrolCell));
        swizzle.Swizzle(reinterpret_cast<void**>(&ParasiteEatingMe));
        swizzle.Swizzle(reinterpret_cast<void**>(&ParasiteImUsing));
        ::new (&MoveSoundAudioController) AudioController();
        IsMoveSoundPlaying = false;
        MoveSoundDelay = 0;
        for (int i = 0; i < NavQueue.Count; ++i)
            swizzle.Swizzle(reinterpret_cast<void**>(&NavQueue[i]));
        for (int i = 0; i < unknown_abstract_array_588.Count; ++i)
            swizzle.Swizzle(reinterpret_cast<void**>(&unknown_abstract_array_588[i]));
        return 0;
    } catch (...) {
        std::abort();
    }
}
#endif
