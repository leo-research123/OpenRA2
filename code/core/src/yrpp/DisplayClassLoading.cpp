// Terrain/waypoint part of YR Display::LoadFromINI 4ACE70. Existing original
// INI, compression, Map and Cell modules own the data. Full LoadFromINI also
// initializes entities, tags and UI; this explicit status entry is terrain-only.
#include "yrpp/DisplayClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/Straws.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdio>
#include <memory>
#include <stdexcept>

bool DisplayClass::LoadTerrainFromINI(CCINIClass& ini) noexcept {
    ReleaseCellStorage();
    if (!ScenarioClass::Instance) return false;
    ini.Reset();
    struct ResetINI { CCINIClass& ini; ~ResetINI() { ini.Reset(); } } reset{ini};
    try {
        auto& scenario=*ScenarioClass::Instance;
        if (!scenario.ReadLightingINI(ini)) return false;
        int dimensions[4],defaults[]{1,1,50,50};
        ini.Read4Integers(dimensions,"Map","Size",defaults);
        const int level=ini.ReadInteger("Map","Level",0);
        MaxLevel=13;
        if (level<0 || level>13 || !CreateEmptyCells({dimensions[0],dimensions[1],dimensions[2],dimensions[3]},0)) return false;
        const int next_id=scenario.UniqueID;
        char catalog_name[32];
        if (scenario.Theater<TheaterType::Temperate || scenario.Theater>TheaterType::Lunar)
            throw std::runtime_error("Invalid theater");
        std::snprintf(catalog_name,sizeof(catalog_name),"%sMD.INI",Theater::GetTheater(scenario.Theater).ControlFileName);
        CCINIClass catalog;
        if (catalog.LoadFromFile(catalog_name)<=0 || !IsometricTileTypeClass::LoadTileSetCatalog(catalog,scenario.Theater))
            throw std::runtime_error("Cannot load theater tile catalog");
        // 4AD048/4AD059 reserves 10,000 IDs around theater type initialization.
        scenario.UniqueID=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(next_id)+10000u);
        char fill[32]; ini.ReadString("Map","Fill","Clear",fill,sizeof(fill));
        const bool water=!_strcmpi(fill,"Water");
        const int first=water ? IsometricTileTypeClass::WaterSet : IsometricTileTypeClass::ClearTile;
        if (first<0 || first+(water ? 3 : 0)>=IsometricTileTypeClass::Array.Count) throw std::runtime_error("Missing fill tile set");
        CellIteratorReset();
        while (auto* cell=CellIteratorNext()) {
            cell->IsoTileTypeIndex=first+scenario.Random.RandomRanged(0,water ? 3 : 0);
            cell->Level=static_cast<char>(level); cell->Height=0;
        }
        scenario.StartX=dimensions[0]; scenario.StartY=dimensions[1];
        scenario.Width=dimensions[2]; scenario.Height=dimensions[3];
        ScenarioClass::NewINIFormat=ini.ReadInteger("Basic","NewINIFormat",0);
        if (!scenario.ReadWaypoints(ini)) throw std::runtime_error("Cannot read map waypoints");
        if (scenario.HomeCell>=0 && scenario.HomeCell<702 && !scenario.IsDefinedWaypoint(scenario.HomeCell)) {
            const auto center=static_cast<short>((MapRect.Width+MapRect.Height)/2);
            scenario.Waypoints[scenario.HomeCell]={center,center};
        }
        // Original reuses a 640*400*2 byte BSurface as scratch, not as map state.
        constexpr int capacity=640*400*2;
        auto buffer=std::make_unique<byte[]>(capacity);
        const struct { const char* section; bool (MapClass::*read)(Straw&); } packs[]{
            {"IsoMapPack",&MapClass::ReadIsoMapPack},{"IsoMapPack2",&MapClass::ReadIsoMapPack2},
            {"IsoMapPack3",&MapClass::ReadIsoMapPack3},{"IsoMapPack4",&MapClass::ReadIsoMapPack4},
            {"IsoMapPack5",&MapClass::ReadIsoMapPack5}};
        for (const auto& pack : packs) {
            if (!ini.GetSection(pack.section)) continue;
            const auto size=ini.ReadUUBlock(pack.section,buffer.get(),capacity);
            if (!size) throw std::runtime_error("Invalid map pack encoding");
            BufferStraw source(buffer.get(),static_cast<int>(size));
            if (!(this->*pack.read)(source)) throw std::runtime_error("Incomplete map pack");
        }
        for (int i=0;i<Cells.Capacity;++i) if (auto* cell=Cells.Items[i]) {
            if (!cell->RefreshTerrainGeometry()) throw std::runtime_error("Map references an unavailable terrain resource");
            if (!cell->InitializeTerrainLighting()) throw std::runtime_error("Cannot initialize terrain lighting");
            MaxLevel=std::max(0,std::min(MaxLevel,int(static_cast<signed char>(cell->Level))));
        }
        if (!IsometricTileTypeClass::LoadMapImages(false,false)) throw std::runtime_error("Cannot load terrain tile variants");
        int local[4],map_defaults[]{MapRect.X,MapRect.Y,MapRect.Width,MapRect.Height};
        ini.Read4Integers(local,"Map","LocalSize",map_defaults);
        // 567230: rectangle intersection, then original 2/2/2/6 borders.
        const int x=std::max(0,local[0]),y=std::max(0,local[1]);
        const auto right=std::min<std::int64_t>(MapRect.Width,std::int64_t(local[0])+local[2]);
        const auto bottom=std::min<std::int64_t>(MapRect.Height,std::int64_t(local[1])+local[3]);
        VisibleRect={std::max(2,x),std::max(2,y),static_cast<int>(std::max<std::int64_t>(0,right-x)),
            static_cast<int>(std::max<std::int64_t>(0,bottom-y))};
        VisibleRect.Width=std::min(VisibleRect.Width,MapRect.Width-VisibleRect.X-2);
        VisibleRect.Height=std::min(VisibleRect.Height,MapRect.Height-VisibleRect.Y-6);
        if (VisibleRect.Width<=0 || VisibleRect.Height<=0) throw std::runtime_error("Empty visible map rectangle");
        Redraws=2;
        return true;
    } catch (...) { ReleaseCellStorage(); return false; }
}
