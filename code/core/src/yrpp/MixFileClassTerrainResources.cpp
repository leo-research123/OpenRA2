// Original common-map slots and ordering from 0x00530460..0x00530785.
// MIX objects and lookup remain in the existing EA-derived module; the
// resource environment records ownership and source carriers.
#include "yrpp/MixFileClass.h"
#include "filesystem/resource_environment.hpp"
#include "filesystem/resource_context.hpp"

bool MixFileClass::LoadTerrainMixes() noexcept {
    const auto* context=game::try_current_context();
    if (!context || !context->files) return false;
    auto& files=*context->files;
    const struct { const char* name; MixFileClass** slot; bool cache; } entries[]{
        {"CONQMD.MIX",&Generics.CONQMD,false},
        {"GENERMD.MIX",&Generics.GENERMD,true},{"GENERIC.MIX",&Generics.GENERIC,true},
        {"ISOGENMD.MIX",&Generics.ISOGENMD,false},{"ISOGEN.MIX",&Generics.ISOGEN,false},
        {"CONQUER.MIX",&Generics.CONQUER,false},
        {"CAMEOMD.MIX",&Generics.CAMEOMD,false},{"CAMEO.MIX",&Generics.CAMEO,false}};
    // PIPS.SHP and other common objects live in CONQMD/CONQUER. Mounting
    // only the four terrain archives leaves the original name lookup empty.
    // CAMEOMD/CAMEO follow CONQUER at 0x530680 / 0x530709; these live in
    // the language archives and contain the localized production icons.
    MixFileClass* created[8]{};
    try {
        for (int i=0;i<8;++i) {
            const auto& entry=entries[i];
            if (*entry.slot) continue;
            created[i]=&files.mount(entry.name); *entry.slot=created[i];
            if (entry.cache && created[i]->CountFiles && !MixFileClass::Cache(entry.name)) throw 1;
        }
        return true;
    } catch (...) {
        for (auto* mix : created) if (mix) files.unmount(*mix);
        return false;
    }
}
