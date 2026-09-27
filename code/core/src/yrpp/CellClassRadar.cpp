// YRpp 9402d7da CellClass; gamemd 47C240..47C367 terrain branch.
#include "yrpp/CellClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/Theater.h"
#include "yrpp/MapClass.h"
#include <algorithm>
#include <bit>

CellClass* CellClass::GetNeighbourCell(FacingType facing) const {
    // 49F2F0 initializes 89F688; 481810 adds low words and uses GetCellAt.
    constexpr short offsets[8][2]={{0,-1},{1,-1},{1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1}};
    if (unsigned(facing)>=8) return const_cast<CellClass*>(this);
    return MapClass::Instance.GetCellAt(CellStruct{
        std::bit_cast<short>(static_cast<unsigned short>(MapCoords.X+offsets[int(facing)][0])),
        std::bit_cast<short>(static_cast<unsigned short>(MapCoords.Y+offsets[int(facing)][1]))});
}
bool CellClass::IsShrouded() const {
    CoordStruct world;GetCellCoords(&world);return MapClass::Instance.IsLocationShrouded(world);
}
bool CellClass::IsFogged() {
    CoordStruct world;GetCellCoords(&world);return MapClass::Instance.IsLocationFogged(world);
}

ColorStruct CellClass::GetTerrainRadarColor() const noexcept {
    try {
        IsometricTileTypeClass* tile=nullptr; int variant=0;
        if (!GetTerrainTile(tile,variant)) return {60,60,60};
        for (int i=0;i<variant && tile;++i) tile=tile->NextVariant;
        const auto* tmp=tile ? reinterpret_cast<const TMPStruct*>(tile->GetImage()) : nullptr;
        const TMPImage* image=nullptr;
        const int sub=IsoTileTypeIndex==0xffff ? 0 : static_cast<unsigned char>(Height);
        if (!tmp || !tmp->GetSubTile(sub,image)) return {60,60,60};
        const int theater=static_cast<int>(Theater::LastTheater);
        if (theater<0 || theater>=6) return {60,60,60};
        const double brightness=Theater::Array[theater].RadarTerrainBrightness;
        const auto channel=[&](byte value) { return static_cast<byte>(int(std::clamp(value*brightness,0.0,255.0))>>1); };
        return {channel(image->RadarLeft[0]),channel(image->RadarLeft[1]),channel(image->RadarLeft[2])};
    } catch (...) { return {60,60,60}; }
}
