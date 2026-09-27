#pragma once

namespace game {

// Indices in the original generic MIX pointer table at 0x884DF8.
enum class GenericMixSlot : unsigned {
    ra2md = 0, ra2 = 1, language = 2, langmd = 3,
    cachemd = 20, cache = 21, localmd = 22, local = 23
};
enum class BootstrapResult { complete, failed };
constexpr unsigned bootstrap_steps = 106;

// The ABI entry supplies no observer: it runs synchronously to its AL result.
// Observers are passive and cannot cancel or throw across the original flow.
struct BootstrapObserver {
    void* context = nullptr;
    void (*before)(void*, const char*, unsigned) noexcept = nullptr;
    void (*after)(void*, const char*, unsigned) noexcept = nullptr;
};

}
