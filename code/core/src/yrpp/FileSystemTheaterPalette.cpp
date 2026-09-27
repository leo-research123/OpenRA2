// Original theater palette loads in 5349C0 and 545150. Palette data stays in
// the existing FileSystem globals; no Surface/Convert or renderer is created.
#include "yrpp/FileSystem.h"
#include "yrpp/Theater.h"
#include <cstdio>

namespace {
bool read_palette(const char* name,BytePalette& output) {
    CCFileClass file(name);
    if (!file.Exists() || file.GetFileSize()<int(sizeof(BytePalette)) ||
        file.ReadBytes(&output,sizeof(output))!=int(sizeof(output))) return false;
    for (auto& color : output.Entries) {
        color.R=BYTE(color.R<<2); color.G=BYTE(color.G<<2); color.B=BYTE(color.B<<2);
    }
    return true;
}
}
bool FileSystem::LoadTheaterPalettes(TheaterType id) noexcept {
    if (id<TheaterType::Temperate || id>TheaterType::Lunar) return false;
    try {
        const auto& theater=Theater::GetTheater(id);
        char name[32]; BytePalette screen,iso;
        std::snprintf(name,sizeof(name),"%s.PAL",theater.ControlFileName);
        if (!read_palette(name,screen)) {
            // 5349C0 original diagnostic palette when the screen PAL is absent.
            for (int i=0;i<256;++i) {
                screen.Entries[i].R=BYTE(i); screen.Entries[i].G=BYTE(255-i); screen.Entries[i].B=BYTE(4*i);
            }
        }
        std::snprintf(name,sizeof(name),"ISO%s.PAL",theater.Extension);
        if (!read_palette(name,iso)) return false;
        TEMPERAT_PAL=screen; ISOx_PAL=iso;
        return true;
    } catch (...) { return false; }
}
