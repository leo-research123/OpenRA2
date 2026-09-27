// Existing MIX module, calibrated against YR 534FA0..5352DD.
// Uses the existing resource owner's archive ledger and original MIX lookup.
#include "yrpp/MixFileClass.h"
#include "yrpp/CCFileClass.h"
#include "filesystem/resource_environment.hpp"
#include <cstdio>
#include <cstring>

void MixFileClass::UnloadSidebarMixes() noexcept {
    const auto* context=game::try_current_context();
    if (!context || !context->files) return;
    for (auto** slot : {&Generics.SIDEC02DMD,&Generics.SIDEC02D,&SIDENC}) {
        auto* mix=*slot; *slot=nullptr;
        if (mix) context->files->unmount(*mix);
    }
}
bool MixFileClass::LoadSidebarMixes(int side) noexcept {
    const auto* context=game::try_current_context();
    if (!context || !context->files || side<0 || side>2) return false;
    const int number=(side==2 ? 1 : side)+1;
    char base[32],md[32],nc[32];
    std::snprintf(base,sizeof(base),"SIDEC%02d.MIX",number);
    std::snprintf(md,sizeof(md),"SIDEC%02dMD.MIX",number);
    std::snprintf(nc,sizeof(nc),"SIDENC%02d.MIX",number);
    if (Generics.SIDEC02D && Generics.SIDEC02D->FileName && !std::strcmp(Generics.SIDEC02D->FileName,base)) return true;
    UnloadSidebarMixes();
    try {
        const struct { const char* name; MixFileClass** slot; bool required; } files[]{
            {md,&Generics.SIDEC02DMD,false},{base,&Generics.SIDEC02D,true},{nc,&SIDENC,false}};
        for (const auto& file : files) {
            CCFileClass probe(file.name);
            if (!probe.Exists()) { if (file.required) { UnloadSidebarMixes(); return false; } continue; }
            *file.slot=&context->files->mount(file.name);
            if (!(*file.slot)->CountFiles) { UnloadSidebarMixes(); return false; }
        }
        return true;
    } catch (...) { UnloadSidebarMixes(); return false; }
}
