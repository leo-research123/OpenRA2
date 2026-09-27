#include "resource_globals.hpp"

namespace game {
namespace {
using G = MixFileClass::GenericMixFiles;
constexpr MixFileClass* G::* members[] = {
    &G::RA2MD,
    &G::RA2,
    &G::LANGUAGE,
    &G::LANGMD,
    &G::THEATER_TEMPERAT,
    &G::THEATER_TEMPERATMD,
    &G::THEATER_TEM,
    &G::GENERIC,
    &G::GENERMD,
    &G::THEATER_ISOTEMP,
    &G::THEATER_ISOTEM,
    &G::ISOGEN,
    &G::ISOGENMD,
    &G::MOVIES02D,
    &G::UNKNOWN_1,
    &G::MAIN,
    &G::CONQMD,
    &G::CONQUER,
    &G::CAMEOMD,
    &G::CAMEO,
    &G::CACHEMD,
    &G::CACHE,
    &G::LOCALMD,
    &G::LOCAL,
    &G::NTRLMD,
    &G::NEUTRAL,
    &G::MAPSMD02D,
    &G::MAPS02D,
    &G::UNKNOWN_2,
    &G::UNKNOWN_3,
    &G::SIDEC02DMD,
    &G::SIDEC02D,
};
static_assert(sizeof(G) == sizeof(MixFileClass*) * 32);
}
MixFileClass*& generic_mix(GenericMixSlot slot) {
    return MixFileClass::Generics.*members[static_cast<unsigned>(slot)];
}
#ifndef RA2_YRPP_GAME
// Only the standalone host owns MIX deletion; original teardown stays original.
void forget_mix(MixFileClass* mix) noexcept {
    auto& array = MixFileClass::Array;
    for (int i = array.Count - 1; i >= 0; --i)
        if (array[i] == mix) array.RemoveItem(i);
    for (auto member : members)
        if (MixFileClass::Generics.*member == mix) MixFileClass::Generics.*member = nullptr;
    auto& maps=MixFileClass::Maps;
    for (int i=maps.Count-1;i>=0;--i) if (maps[i]==mix) maps.RemoveItem(i);
    if (MixFileClass::MULTIMD==mix) MixFileClass::MULTIMD=nullptr;
    if (MixFileClass::SIDENC==mix) MixFileClass::SIDENC=nullptr;
}
#endif
}
