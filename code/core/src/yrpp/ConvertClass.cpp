// YRpp 9402d7da0fe14d46703ba871ce3e6b3cde855bfc, FileSystem.h:
// reuse the AllocatePalette / LoadPALFile BYTE << 2 conversion.
// Loading, output order and allocation ownership calibrated at 0x72ADE0.
#include "yrpp/Memory.h"
#include "filesystem/file_system.hpp"
#include "yrpp/ConvertClass.h"
#include "yrpp/CCFileClass.h"
#include "ConvertClassHelpers.hpp"
#include "yrpp/Surface.h"
#include <new>
#include <cstring>

void YRPP_FASTCALL ConvertClass::CreateFromFile(const char* filename,
    BytePalette*& palette, ConvertClass*& destination) {
    // The shared core file hierarchy owns the object lifetime in this storage.
    alignas(CCFileClass) std::byte storage[sizeof(CCFileClass)];
    auto* file = game::ConstructFile(storage, filename);
    auto* raw = static_cast<const ColorStruct*>(file->ReadWholeFile());
    if (raw) {
        auto* colors = static_cast<BytePalette*>(YRMemory::Allocate(sizeof(BytePalette)));
        if (colors) std::memset(colors, 0, sizeof(BytePalette));
        palette = colors;
        // Do not mask to 6 bits, saturate, or normalize to 255. Do not change
        // the game's null-allocation fault into success or reset both outputs.
        for (int i = 0; i < 256; ++i) {
            palette->Entries[i].R = static_cast<BYTE>(raw[i].R << 2);
            palette->Entries[i].G = static_cast<BYTE>(raw[i].G << 2);
            palette->Entries[i].B = static_cast<BYTE>(raw[i].B << 2);
        }
        YRMemory::Deallocate(const_cast<ColorStruct*>(raw));
        void* object = YRMemory::Allocate(sizeof(ConvertClass));
        destination = object ? game::construct_convert(object, *palette,
            *palette, DSurface::Alternate, 1, false) : nullptr;
    }
    // In the original, CCFile survives until after Convert construction.
    game::DestroyFile(file);
}
