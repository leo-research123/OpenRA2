// Original FileSystem palette construction; optional CPU conversion only.
#include "yrpp/FileSystem.h"
#include "yrpp/Surface.h"
#include "yrpp/Memory.h"
#include "ConvertClassHelpers.hpp"
ConvertClass* FileSystem::LoadPALFile(const char* name, DSurface* surface) {
    const auto* raw = static_cast<const ColorStruct*>(LoadFile(name, false));
    if (!raw) return nullptr;
    BytePalette palette;
    for (int i = 0; i < 256; ++i) {
        palette.Entries[i].R = BYTE(raw[i].R << 2);
        palette.Entries[i].G = BYTE(raw[i].G << 2);
        palette.Entries[i].B = BYTE(raw[i].B << 2);
    }
    void* storage = YRMemory::Allocate(sizeof(ConvertClass));
    return storage ? game::construct_convert(storage, palette, TEMPERAT_PAL, surface, 0x35, false) : nullptr;
}
