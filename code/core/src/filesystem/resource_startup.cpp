#include "resource_startup.hpp"
#include "filesystem/resource_globals.hpp"
#include "filesystem/resource_environment.hpp"

namespace game {
void mount_language_md(ResourceEnvironment& files) {
    MixFileClass::Generics.LANGMD = &files.mount("LANGMD.MIX");
}
void mount_language(ResourceEnvironment& files) {
    MixFileClass::Generics.LANGUAGE = &files.mount("LANGUAGE.MIX");
}
void shutdown_resource_files(ResourceEnvironment& files,
    ShutdownObserver observer, void* context) noexcept {
    auto release = [&](MixFileClass* mix) {
        if (mix) {
            if (observer) observer(context, mix->FileName);
            files.unmount(*mix);
        }
    };
    auto& array = MixFileClass::Array;
    auto& maps=MixFileClass::Maps;
    while (maps.Count) {
        if (maps[0]) release(maps[0]); else maps.RemoveItem(0);
    }
    release(MixFileClass::Generics.MAPSMD02D); release(MixFileClass::Generics.MAPS02D);
    release(MixFileClass::MULTIMD);
    while (array.Count) {
        auto* mix = array[0];
        // The original deletes the first object then shifts the array. unmount
        // also removes aliases from slots, preventing a second host deletion.
        if (mix) release(mix);
        else array.RemoveItem(0);
    }
    auto& g = MixFileClass::Generics;
    release(g.CACHE); release(g.CACHEMD); release(g.LOCAL); release(g.LOCALMD);
    release(g.RA2); release(g.RA2MD); release(g.LANGUAGE); release(g.LANGMD);
}
}
