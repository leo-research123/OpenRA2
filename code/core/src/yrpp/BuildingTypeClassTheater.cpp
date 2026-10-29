// BuildingTypeClass theater art, YR 0x0045E970 and its depth shape 0x0045E8F0.
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/CCFileClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/FileSystem.h"
#include "yrpp/Memory.h"
#include "yrpp/Surface.h"
#include "type_resources.hpp"
#include <bit>
#include <cstdio>
#include <cstdint>
#include <stdexcept>

// The original reads the file through the name cache (0x005B40B0), which for
// a non-SHP name is a whole-file read (0x004A3890). Nothing else looks the
// name up, so the read skips the cache entry and the buffer is owned by the
// global; the host releases it with the resource owner. The original indexes
// a missing or malformed file without checks; those layouts are rejected.
void BuildingTypeClass::LoadDepthShape() {
    auto& depth = FileSystem::BUILDINGZ_SHA;
    if (depth) return;
    CCFileClass file("BUILDNGZ.SHA");
    if (!file.Exists(false)) return;
    const int bytes = file.GetFileSize();
    if (bytes < int(sizeof(SHPStruct) + sizeof(SHPFrame))) throw std::runtime_error("Truncated BUILDNGZ.SHA");
    auto* shape = static_cast<SHPStruct*>(file.ReadWholeFile());
    if (!shape) throw std::runtime_error("Cannot load BUILDNGZ.SHA");
    struct Release {
        SHPStruct*& shape;
        ~Release() { YRMemory::Deallocate(shape); }
    } release{shape};
    if (shape->Frames != 1 || shape->HasCompression(0)) throw std::runtime_error("Invalid BUILDNGZ.SHA layout");
    const auto bounds = shape->GetFrameBounds(0);
    const int offset = shape->GetData()->GetFrameHeader(0).Offset;
    if (bounds.X || bounds.Y || bounds.Width <= 0 || bounds.Height <= 0 || bounds.Width != shape->Width ||
        bounds.Height != shape->Height || offset < int(sizeof(SHPStruct) + sizeof(SHPFrame)) ||
        std::int64_t(offset) + std::int64_t(bounds.Width) * bounds.Height > bytes)
        throw std::runtime_error("Invalid BUILDNGZ.SHA bounds");
    auto* pixels = shape->GetPixels(0);
    if (!pixels) throw std::runtime_error("Missing BUILDNGZ.SHA pixels");
    for (int i = 0; i < bounds.Width * bounds.Height; ++i)
        if (pixels[i]) pixels[i] = BYTE(pixels[i] - 0x41);
    depth = shape;
    shape = nullptr;
}

#if !defined(RA2_YRPP_GAME)
namespace {
BSurface* cloak_radius_surface = nullptr;
}
BSurface*& BuildingTypeClass::CloakRadiusSurface = cloak_radius_surface;

void YRPP_FASTCALL BuildingTypeClass::Init(TheaterType theater) {
    try {
        LoadDepthShape();
        FileSystem::POWEROFF_SHP = static_cast<SHPStruct*>(FileSystem::LoadFile("POWEROFF.SHP", false));
        FileSystem::WRENCH_SHP = static_cast<SHPStruct*>(FileSystem::LoadFile("WRENCH.SHP", false));
        const auto clear = [](SHPStruct*& image, bool& owned) {
            if (image) {
                YRMemory::Deallocate(image);
                image = nullptr;
                owned = false;
            }
        };
        int radius = 0;
        for (int index = 0; index < Array.Count; ++index) {
            auto* type = Array[index];
            if (!type) throw std::logic_error("Invalid BuildingType theater entry");
            const int current_radius = std::bit_cast<signed char>(type->CloakRadiusInCells);
            if (radius <= current_radius) radius = current_radius;
            if (type->Theater) {
                const auto* selected = Theater::Get(theater);
                if (!selected) {
                    game::type_resource_result(game::TypeResourceStatus::unavailable);
                    return;
                }
                char filename[260];
                if (type->DemandLoad)
                    clear(type->Image, type->ImageAllocated);
                else {
                    std::snprintf(filename, sizeof(filename), "%s.%s", type->ImageFile, selected->Extension);
                    type->Image = static_cast<SHPStruct*>(FileSystem::LoadFile(filename, false));
                }
                if (type->DemandLoadBuildup)
                    clear(type->Buildup, type->BuildupLoaded);
                else {
                    std::snprintf(filename, sizeof(filename), "%s.%s", type->BuildupFile, selected->Extension);
                    type->Buildup = static_cast<SHPStruct*>(FileSystem::LoadFile(filename, false));
                }
                if (type->Buildup) {
                    auto& sequence = type->BuildingAnimFrame[0];
                    sequence.dwUnknown = 0;
                    sequence.FrameCount = type->Buildup->Frames;
                    sequence.FrameDuration = sequence.FrameCount ? 75 / sequence.FrameCount : 1;
                }
            } else if (type->NewTheater) {
                if (type->DemandLoad) clear(type->Image, type->ImageAllocated);
                if (type->DemandLoadBuildup) clear(type->Buildup, type->BuildupLoaded);
                type->LoadArt(theater);
            }
        }
        if (radius) {
            BSurface::Destroy(CloakRadiusSurface);
            CloakRadiusSurface = nullptr;
            CloakRadiusSurface = BSurface::Create(radius + 16, radius + 16, 1);
        }
    } catch (...) {
        game::type_resource_failure("BuildingTypeClass::Init");
    }
}

// YR 0x00465CC0.
void YRPP_FASTCALL BuildingTypeClass::InitToTiles() {
    for (int i = 0; i < Array.Count; ++i) {
        auto* type = Array[i];
        char name[60];
        name[0] = 0;
        CCINIClass::INI_Rules->ReadString(type->ID, "ToTile", nullptr, name, sizeof(name));
        const int tile = IsometricTileTypeClass::FindIndex(name);
        if (tile >= 0 && tile < IsometricTileTypeClass::Array.Count) type->ToTile = IsometricTileTypeClass::Array[tile];
    }
}
#endif
