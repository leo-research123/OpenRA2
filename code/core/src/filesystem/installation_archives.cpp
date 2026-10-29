#include "installation_archives.hpp"
#include "resource_environment.hpp"
#include "yrpp/RawFileClass.h"
#include "yrpp/ArrayClasses.h"
#include <cstdio>

namespace game {
void mount_installation_expansions(ResourceEnvironment& files) {
    // The extended executable searches EXPANDSPAWN%02d.MIX, highest first.
    // Some installations retain those exact numbered MIX carriers as .txt.
    // This is a host source mapping, never a text-file or backup-tree scan.
    for (int index = 99; index >= 0; --index) {
        char name[32];
        std::snprintf(name, sizeof(name), "EXPANDSPAWN%02d.MIX", index);
        RawFileClass file(name);
        if (!file.Exists()) {
            std::snprintf(name, sizeof(name), "EXPANDSPAWN%02d.TXT", index);
            file.SetFileName(name);
            if (!file.Exists()) continue;
        }
        auto* mix = &files.mount(name);
        if (!MixFileClass::Array.AddItem(mix)) {
            files.unmount(*mix);
            throw std::bad_alloc();
        }
    }
}
}
