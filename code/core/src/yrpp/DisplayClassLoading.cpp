// Terrain/waypoint part of YR Display::LoadFromINI 4ACE70. Existing original
// INI, compression, Map and Cell modules own the data. Full LoadFromINI also
// initializes entities, tags and UI; this explicit status entry is terrain-only.
#include "yrpp/DisplayClass.h"
#include "visibility_grid.hpp"
#include "yrpp/FileSystem.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "type_resources.hpp"
#include "yrpp/CCINIClass.h"
#include "yrpp/ConvertClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/Straws.h"
#include "yrpp/Surface.h"
#include "yrpp/GameOptionsClass.h"
#include "yrpp/Unsorted.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdio>
#include <limits>
#include <memory>
#include <stdexcept>

namespace {
SHPStruct* place_shape = nullptr;
SHPStruct* shadow_shape = nullptr;
}
SHPStruct*& DisplayClass::PlaceShape = place_shape;
SHPStruct*& DisplayClass::ShadowShape = shadow_shape;

#if !defined(RA2_YRPP_GAME)
void DisplayClass::One_Time() {
    MapClass::One_Time();
    LoadOneTimeShapes();
    const auto& window = DSurface::WindowBounds;
    const int x = !GameOptionsClass::Instance.SidebarMode && !Unsorted::ArmageddonMode ? 168 : 0;
    Set_View_Dimensions({x, 0, window.Width - 168, window.Height - 32});
}
#endif

// 0x004A885D..0x004A887C. A missing file leaves its global null.
void DisplayClass::LoadOneTimeShapes() noexcept {
    try {
        PlaceShape = static_cast<SHPStruct*>(FileSystem::LoadFile("PLACE.SHP", false));
        ShadowShape = static_cast<SHPStruct*>(FileSystem::LoadFile("SHADOW.SHP", false));
    } catch (...) {
        PlaceShape = ShadowShape = nullptr;
    }
}

bool DisplayClass::LoadTerrainFromINI(CCINIClass& ini) noexcept {
    ReleaseCellStorage();
    if (!ScenarioClass::Instance) return false;
    ini.Reset();
    struct ResetINI {
        CCINIClass& ini;
        ~ResetINI() { ini.Reset(); }
    } reset{ini};
    try {
        auto& scenario = *ScenarioClass::Instance;
        if (!scenario.ReadLightingINI(ini)) return false;
        int dimensions[4], defaults[]{1, 1, 50, 50};
        ini.Read4Integers(dimensions, "Map", "Size", defaults);
        const int level = ini.ReadInteger("Map", "Level", 0);
        MaxLevel = 13;
        if (level < 0 || level > 13 ||
            !CreateEmptyCells({dimensions[0], dimensions[1], dimensions[2], dimensions[3]}, 0))
            return false;
        const int next_id = scenario.UniqueID;
        char catalog_name[32];
        if (scenario.Theater < TheaterType::Temperate || scenario.Theater > TheaterType::Lunar)
            throw std::runtime_error("Invalid theater");
        std::snprintf(catalog_name, sizeof(catalog_name), "%sMD.INI",
                      Theater::GetTheater(scenario.Theater).ControlFileName);
        int theater_index = static_cast<int>(scenario.Theater);
        CCINIClass catalog;
        if (catalog.LoadFromFile(catalog_name) <= 0 ||
            !IsometricTileTypeClass::LoadTileSetCatalog(catalog, scenario.Theater))
            throw std::runtime_error("Cannot load theater tile catalog");
        if (!IsometricTileTypeClass::LoadSlopeDepthShapes(scenario.Theater))
            throw std::runtime_error("Cannot load slope depth shapes");
        // 0x005454EB: the theater phase resets the terrain converters before
        // any cell of this map borrows one.
        if (!LightConvertClass::InitTerrainConverters())
            throw std::runtime_error("Cannot create the terrain light converter");
        // 4AD048/4AD059 reserves 10,000 IDs around theater type initialization.
        scenario.UniqueID = std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(next_id) + 10000u);
        // 4ACE70 then runs the per-class theater art: overlay images
        // (0x005FE620), then the building one, which loads the shared depth,
        // POWEROFF and WRENCH shapes. The overlay loader reports failures
        // through the type resource status, which its own scope collects.
        const auto overlay_status = game::with_type_resources(
            game::TypeResourceServices{},
            [](void* theater) { OverlayTypeClass::LoadFromIniList(*static_cast<int*>(theater)); }, &theater_index);
        if (overlay_status != game::TypeResourceStatus::complete)
            throw std::runtime_error("Cannot load overlay theater images");
        BuildingTypeClass::Init(scenario.Theater);
        char fill[32];
        ini.ReadString("Map", "Fill", "Clear", fill, sizeof(fill));
        const bool water = !_strcmpi(fill, "Water");
        const int first = water ? IsometricTileTypeClass::WaterSet : IsometricTileTypeClass::ClearTile;
        if (first < 0 || first + (water ? 3 : 0) >= IsometricTileTypeClass::Array.Count)
            throw std::runtime_error("Missing fill tile set");
        CellIteratorReset();
        while (auto* cell = CellIteratorNext()) {
            cell->IsoTileTypeIndex = first + scenario.Random.RandomRanged(0, water ? 3 : 0);
            cell->Level = static_cast<char>(level);
            game::visibility_terrain_changed();
            cell->Height = 0;
        }
        scenario.StartX = dimensions[0];
        scenario.StartY = dimensions[1];
        scenario.Width = dimensions[2];
        scenario.Height = dimensions[3];
        ScenarioClass::NewINIFormat = ini.ReadInteger("Basic", "NewINIFormat", 0);
        if (!scenario.ReadWaypoints(ini)) throw std::runtime_error("Cannot read map waypoints");
        if (scenario.HomeCell >= 0 && scenario.HomeCell < 702 && !scenario.IsDefinedWaypoint(scenario.HomeCell)) {
            const auto center = static_cast<short>((MapRect.Width + MapRect.Height) / 2);
            scenario.Waypoints[scenario.HomeCell] = {center, center};
        }
        const struct {
            const char* section;
            bool (MapClass::*read)(Straw&);
        } packs[]{{"IsoMapPack", &MapClass::ReadIsoMapPack},
                  {"IsoMapPack2", &MapClass::ReadIsoMapPack2},
                  {"IsoMapPack3", &MapClass::ReadIsoMapPack3},
                  {"IsoMapPack4", &MapClass::ReadIsoMapPack4},
                  {"IsoMapPack5", &MapClass::ReadIsoMapPack5}};
        for (const auto& pack : packs) {
            if (!ini.GetSection(pack.section)) continue;
            // Intentional standalone extension: original 0x004AD356 allocates
            // 0x7D000 bytes (640 * 400 * 2), truncating larger encoded packs.
            // Allocate for the complete decoded Base64 payload instead, so
            // literal-heavy LZO maps retain every terrain record. This changes
            // the original size limit, not the original-game ABI or jump path.
            // Keep ReadUUBlock's 128-byte line reads and bounded writes intact.
            constexpr auto max_encoded_size = (std::size_t(std::numeric_limits<int>::max()) / 3) * 4;
            std::size_t encoded_size = 0;
            const int count = ini.GetKeyCount(pack.section);
            for (int i = 0; i < count; ++i) {
                char text[128]{};
                const int length =
                    ini.ReadString(pack.section, ini.GetKeyName(pack.section, i), "=", text, sizeof(text));
                if (length < 0 || std::size_t(length) > max_encoded_size - encoded_size)
                    throw std::runtime_error("Map pack exceeds stream size range");
                encoded_size += std::size_t(length);
            }
            const auto capacity = ((encoded_size + 3) / 4) * 3;
            if (!capacity) throw std::runtime_error("Invalid map pack encoding");
            auto buffer = std::make_unique<byte[]>(capacity);
            const auto size = ini.ReadUUBlock(pack.section, buffer.get(), capacity);
            if (!size) throw std::runtime_error("Invalid map pack encoding");
            BufferStraw source(buffer.get(), static_cast<int>(size));
            if (!(this->*pack.read)(source)) throw std::runtime_error("Incomplete map pack");
        }
        for (int i = 0; i < Cells.Capacity; ++i)
            if (auto* cell = Cells.Items[i]) {
                if (!cell->RefreshTerrainGeometry())
                    throw std::runtime_error("Map references an unavailable terrain resource");
                // Native addition, ahead of Overpass (0x00568BB0): views that draw
                // without starting the scenario still need bound converters.
                cell->InitLightConvert(nullptr, 0x10000, 0, 1000, 1000, 1000);
                MaxLevel = std::max(0, std::min(MaxLevel, int(static_cast<signed char>(cell->Level))));
            }
        if (!IsometricTileTypeClass::LoadMapImages(false, false))
            throw std::runtime_error("Cannot load terrain tile variants");
        int local[4], map_defaults[]{MapRect.X, MapRect.Y, MapRect.Width, MapRect.Height};
        ini.Read4Integers(local, "Map", "LocalSize", map_defaults);
        // 567230: rectangle intersection, then original 2/2/2/6 borders.
        const int x = std::max(0, local[0]), y = std::max(0, local[1]);
        const auto right = std::min<std::int64_t>(MapRect.Width, std::int64_t(local[0]) + local[2]);
        const auto bottom = std::min<std::int64_t>(MapRect.Height, std::int64_t(local[1]) + local[3]);
        VisibleRect = {std::max(2, x), std::max(2, y), static_cast<int>(std::max<std::int64_t>(0, right - x)),
                       static_cast<int>(std::max<std::int64_t>(0, bottom - y))};
        VisibleRect.Width = std::min(VisibleRect.Width, MapRect.Width - VisibleRect.X - 2);
        VisibleRect.Height = std::min(VisibleRect.Height, MapRect.Height - VisibleRect.Y - 6);
        if (VisibleRect.Width <= 0 || VisibleRect.Height <= 0) throw std::runtime_error("Empty visible map rectangle");
        Redraws = 2;
        return true;
    } catch (...) {
        ReleaseCellStorage();
        return false;
    }
}
