// Terrain field portion of original Cell::RecalcAttributes (47D2B0), using
// original IsometricTileTypeClass queries and TMP ownership. No world model.
#include "yrpp/CellClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/ConvertClass.h"
#include <algorithm>
#include <bit>

bool CellClass::RefreshTerrainGeometry() noexcept {
    using Tile=IsometricTileTypeClass;
    if (OverlayTypeIndex!=-1) return false;
    try {
        if (IsoTileTypeIndex<0 || IsoTileTypeIndex>=Tile::Array.Count) IsoTileTypeIndex=0xffff;
        if (IsoTileTypeIndex==0xffff) {
            Height=0; SlopeIndex=0; LandType=::LandType::Clear; unknown_11D=0;
            return Tile::ClearTile>=0 && Tile::ClearTile<Tile::Array.Count && Tile::Array[Tile::ClearTile]->GetImage();
        }
        auto* type=Tile::Array[IsoTileTypeIndex];
        auto* tmp=reinterpret_cast<const TMPStruct*>(type->GetImage());
        if (!tmp) return false;
        const TMPImage* image=nullptr;
        if (!tmp->GetSubTile(static_cast<unsigned char>(Height),image)) {
            IsoTileTypeIndex=0xffff; Height=0; SlopeIndex=0; LandType=::LandType::Clear; unknown_11D=0;
            return Tile::ClearTile>=0 && Tile::ClearTile<Tile::Array.Count && Tile::Array[Tile::ClearTile]->GetImage();
        }
        const int subtile=static_cast<unsigned char>(Height);
        SlopeIndex=static_cast<BYTE>(type->GetSlopeIndex(subtile));
        LandType=type->GetLandType(subtile);
        int width=0,height=0;
        if (!type->GetTileDimensions(subtile,width,height)) return false;
        const auto excess=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(height)-30u);
        unknown_11D=static_cast<BYTE>(excess/15);
        return true;
    } catch (...) { return false; }
}

bool CellClass::InitializeTerrainLighting() noexcept {
    const auto* scenario=ScenarioClass::Instance;
    if (!scenario) return false;
    int intensity,ambient,normal,terrain,bridge,red,green,blue;
    CalculateLightSourceLighting(intensity,ambient,normal,terrain,bridge,red,green,blue);
    Intensity=intensity; Ambient=ambient;
    Intensity_Normal=WORD(normal); Intensity_Terrain=WORD(terrain); Color1_Blue=WORD(bridge);
    Color2_Red=WORD(red); Color2_Green=WORD(green); Color2_Blue=WORD(blue);
    return true;
}
