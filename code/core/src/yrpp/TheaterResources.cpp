// Theater's original package order and snow-only MD control package, 5349C0.
// The current resource environment adopts the same original MixFileClass nodes.
#include "yrpp/Theater.h"
#include "yrpp/MixFileClass.h"
#include "filesystem/resource_environment.hpp"
#include "filesystem/resource_context.hpp"
#include <cstdio>

bool Theater::UnmountResourceMixes() noexcept {
    const auto* context=game::try_current_context();
    if (!context || !context->files) return false;
    auto& g=MixFileClass::Generics;
    MixFileClass* mixes[]{g.THEATER_TEMPERATMD,g.THEATER_TEMPERAT,g.THEATER_TEM,
        g.THEATER_ISOTEM,g.THEATER_ISOTEMP};
    for (auto* mix : mixes) if (mix) context->files->unmount(*mix);
    LastTheater=TheaterType::None;
    return true;
}
bool Theater::MountResourceMixes(TheaterType theater_id) noexcept {
    const auto* context=game::try_current_context();
    if (!context || !context->files || theater_id<TheaterType::Temperate || theater_id>TheaterType::Lunar) return false;
    if (!MixFileClass::LoadTerrainMixes() || !UnmountResourceMixes()) return false;
    auto& g=MixFileClass::Generics;
    const auto& theater=GetTheater(theater_id);
    const struct { const char* stem; const char* suffix; MixFileClass** slot; bool cache; bool enabled; } entries[]{
        {theater.ControlFileName,"MD.MIX",&g.THEATER_TEMPERATMD,true,theater_id==TheaterType::Snow},
        {theater.ControlFileName,".MIX",&g.THEATER_TEMPERAT,true,true},
        {theater.Extension,".MIX",&g.THEATER_TEM,true,true},
        {theater.PaletteFileName,"MD.MIX",&g.THEATER_ISOTEM,false,true},
        {theater.ArtFileName,".MIX",&g.THEATER_ISOTEMP,false,true}};
    try {
        for (const auto& entry : entries) {
            if (!entry.enabled) continue;
            char filename[32]; std::snprintf(filename,sizeof(filename),"%s%s",entry.stem,entry.suffix);
            auto* mix=&context->files->mount(filename); *entry.slot=mix;
            if (entry.cache && mix->CountFiles && !MixFileClass::Cache(filename)) throw 1;
        }
        return true;
    } catch (...) { UnmountResourceMixes(); return false; }
}
