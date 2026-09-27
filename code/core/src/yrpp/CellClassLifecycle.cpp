// EA REDALERT/CELL.CPP f1f0d42b: empty-cell member initialization, adapted to
// existing YRpp 9402d7da fields. Copyright 2020 Electronic Arts Inc.; GPL-3.0
// with additional terms in third_party/ea/LICENSE.TXT. YR 47BBF0/47BB60 calibrate
// defaults, Create_ID, owned vectors/effects and borrowed light-convert release.
#include "yrpp/CellClass.h"
#include "yrpp/ConvertClass.h"
#include "map_runtime.hpp"
#include <bit>
#include <cstdlib>

CellClass* CellClass::Create() noexcept {
    // Same allocator domain as GameDelete, with recoverable failure instead
    // of GameAllocator's process-terminating AllocateChecked policy.
    try {
        auto* storage = YRMemory::Allocate(sizeof(CellClass));
        return storage ? ::new (storage) CellClass() : nullptr;
    } catch (...) { return nullptr; }
}

CellClass::CellClass() noexcept
    : AbstractClass(), MapCoords{}, FoggedObjects(nullptr), BridgeOwnerCell(nullptr),
      unknown_30(0), LightConvert(nullptr), IsoTileTypeIndex(0xffff), AttachedTag(nullptr),
      Rubble(nullptr), OverlayTypeIndex(-1), SmudgeTypeIndex(-1), Passability{},
      WallOwnerIndex(-1), InfantryOwnerIndex(-1), AltInfantryOwnerIndex(-1),
      unknown_5C(0xffffffffu), unknown_60(0xffffffffu), RedrawFrame(0xffffffffu),
      InViewportRect{}, CloakedByHouses(0), SensorsOfHouses{}, DisguiseSensorsOfHouses{},
      BaseSpacerOfHouses(0), Jumpjet(nullptr), FirstObject(nullptr), AltObject(nullptr),
      LandType{}, RadLevel(0), RadSite(nullptr), PixelFX(nullptr), OccupyHeightsCoveringMe(0),
      Intensity(0x10000), Ambient(0), Intensity_Normal(1000), Intensity_Terrain(1000),
      Color1_Blue(1000), Color2_Red(1000), Color2_Green(1000), Color2_Blue(1000),
      TubeIndex(-1), unknown_118(-1), IsIceGrowthAllowed(0), Height(0), Level(0),
      SlopeIndex(0), unknown_11D(0), OverlayData(0), SmudgeData(0), Visibility(-2),
      Foggedness(-2), BlockedNeighbours(0), OccupationFlags(0), AltOccupationFlags(0),
      AltFlags{}, ShroudCounter(1), GapsCoveringThisCell(0), VisibilityChanged(false),
      unknown_13C(0), Flags{} {
    // The original reads the zero cell initialized at 47B2F0. Unspecified high
    // flag/occupation bits are zero for fresh native objects, not allocator data.
    Create_ID();
}

CellClass::~CellClass() {
    // The container owns its pointer buffer, not the FoggedObject instances.
    // Virtual deletion preserves the allocator domain of the actual vector.
    delete FoggedObjects;
    FoggedObjects = nullptr;
    if (PixelFX) {
        const auto destroy = game::map_runtime().destroy_pixel_fx;
        // PixelFX is forward-declared in YRpp. An owning module must provide
        // its destruction boundary before installing a non-null effect.
        if (!destroy) std::abort();
        destroy(PixelFX);
        PixelFX = nullptr;
    }
    FirstObject = nullptr;
    if ((MapCoords.X || MapCoords.Y) && LightConvert) {
        const auto* enabled = game::map_runtime().count_light_convert_references;
        if (enabled && *enabled)
            LightConvert->RefCount = std::bit_cast<std::int32_t>(
                static_cast<std::uint32_t>(LightConvert->RefCount) - 1u);
        LightConvert = nullptr;
    }
}
