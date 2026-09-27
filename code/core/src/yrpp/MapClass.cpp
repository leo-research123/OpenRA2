// YRpp 9402d7da. RA1 MAP.CPP uses a rectangular cell array, not YR's diamond.
// Iterator calibrated to 578350/578290; land names to 48DF80/81DA28.
#include "yrpp/MapClass.h"
#include "yrpp/HouseClass.h"
#include <bit>
#include <cstdint>

#if !defined(RA2_YRPP_GAME)
int MapClass::GetThreatPosed(const CellStruct& coordinate, HouseClass* house) const {
    const auto cell=GetCellAt(coordinate)->MapCoords;
    // The 130x130 house array has a one-entry border. 0x56BCD0 indexes
    // from 0x59F0, while the actual member begins at 0x57E4 (+131 ints).
    return std::bit_cast<int>(house->ThreatPosedEstimates[cell.Y/4+1][cell.X/4+1]);
}

#endif

LandType YRPP_FASTCALL GroundType::GetLandTypeFromName(const char* name) {
    constexpr const char* names[] = {"Clear", "Road", "Water", "Rock", "Wall",
        "Tiberium", "Beach", "Rough", "Ice", "Railroad", "Tunnel", "Weeds"};
    if (!name) return LandType::None; // Host guard before the CRT comparison.
    for (int i = 0; i < 12; ++i)
        if (_strcmpi(name, names[i]) == 0) return static_cast<LandType>(i);
    return LandType::None;
}
void MapClass::CellIteratorReset() {
    CellIterator_NextX = 1;
    CellIterator_NextY = MapRect.Width;
    CellIterator_CurrentY = MapRect.Width - 1;
    CellIterator_NextCell = &Cells.Items[MapRect.Width * 512 + 1];
}
CellClass* MapClass::CellIteratorNext() {
    CellClass** current = CellIterator_NextCell;
    if (CellIterator_CurrentY) {
        ++CellIterator_NextX;
        --CellIterator_NextY;
        --CellIterator_CurrentY;
        CellIterator_NextCell -= 511;
    } else {
        const int old_x = CellIterator_NextX;
        const int old_y = CellIterator_NextY;
        CellIterator_NextX = old_y;
        CellIterator_NextY = old_x;
        if ((old_y - MapRect.Width + old_x - 1) & 1) {
            CellIterator_CurrentY = MapRect.Width - 1;
            CellIterator_NextY = old_x + 1;
        } else {
            CellIterator_CurrentY = MapRect.Width - 2;
            CellIterator_NextX = old_y + 1;
        }
        CellIterator_NextCell = &Cells.Items[CellIterator_NextX + CellIterator_NextY * 512];
    }
    return *current;
}

int MapClass::GetCellIndex(const CellStruct& coords) {
    // The original shift/add mapping includes flattened aliases such as (-1,1).
    // Multiplication preserves it without a negative signed left shift.
    return coords.Y * 512 + coords.X;
}

bool MapClass::IsWithinUsableArea2D(const CellStruct& cell) const {
    return IsWithinUsableArea(cell,false);
}
#if !defined(RA2_YRPP_GAME)
bool MapClass::CoordinatesLegal(const CellStruct& cell) const {
    const int x=cell.X,y=cell.Y,width=MapRect.Width;
    const int limit=std::bit_cast<int>(static_cast<unsigned>(width)+2u*static_cast<unsigned>(MapRect.Height));
    return x+y>width && x-y<width && y-x<width && x+y<=limit;
}
bool MapClass::CoordinatesLegal(const CoordStruct& coords) const {
    return CoordinatesLegal(CellStruct{short(coords.X/256),short(coords.Y/256)});
}
bool MapClass::IsWithinUsableArea(CellClass* cell, bool checkLevel) const {
    const auto wrap = [](std::uint32_t value) { return std::bit_cast<std::int32_t>(value); };
    const auto width = static_cast<std::uint32_t>(MapRect.Width);
    const auto x = static_cast<std::uint32_t>(VisibleRect.X), y = static_cast<std::uint32_t>(VisibleRect.Y);
    const auto w = static_cast<std::uint32_t>(VisibleRect.Width), h = static_cast<std::uint32_t>(VisibleRect.Height);
    const int sum = cell->MapCoords.X + cell->MapCoords.Y;
    int level = checkLevel ? static_cast<signed char>(cell->Level) : 0;
    if (checkLevel && cell->SlopeIndex && sum < wrap(static_cast<std::uint32_t>(level)+width+2u*y+4u)) ++level;
    return sum > wrap(static_cast<std::uint32_t>(level)+width+2u*y)
        && sum <= wrap(static_cast<std::uint32_t>(level)+width+2u*(y+h)+2u)
        && cell->MapCoords.X-cell->MapCoords.Y < wrap(2u*(x+w)-width)
        && cell->MapCoords.Y-cell->MapCoords.X < wrap(width-2u*x);
}
#endif

bool MapClass::IsWithinUsableArea(const CellStruct& cell, bool checkLevel) const {
    const auto wrap = [](std::uint32_t value) { return std::bit_cast<std::int32_t>(value); };
    const auto width = static_cast<std::uint32_t>(MapRect.Width);
    const auto x = static_cast<std::uint32_t>(VisibleRect.X), y = static_cast<std::uint32_t>(VisibleRect.Y);
    const auto w = static_cast<std::uint32_t>(VisibleRect.Width), h = static_cast<std::uint32_t>(VisibleRect.Height);
    const int sum = cell.X + cell.Y;
    int level=0;
    if (checkLevel) {
        const auto* source=MapClass::Instance.GetCellAt(cell);
        level=static_cast<signed char>(source->Level);
        if (source->SlopeIndex && sum<wrap(static_cast<std::uint32_t>(level)+width+2u*y+4u)) ++level;
    }
    return sum > wrap(static_cast<std::uint32_t>(level)+width+2u*y) &&
        sum <= wrap(static_cast<std::uint32_t>(level)+width+2u*(y+h)+2u) &&
        cell.X - cell.Y < wrap(2u * (x + w) - width) && cell.Y - cell.X < wrap(width - 2u * x);
}

// Non-template interface helpers; bodies retained from the corresponding header.

LayerClass* MapClass::GetLayer(Layer lyr)
{
    return (lyr >= Layer::Underground && lyr <= Layer::Top)
        ? &ObjectsInLayers[static_cast<int>(lyr)]
        : nullptr;
}

CellClass* MapClass::TryGetCellAt(const CellStruct& MapCoords) const
{
    int idx = GetCellIndex(MapCoords);
    return (Cells.Items && idx >= 0 && idx < MaxCells && idx < Cells.Capacity) ? Cells[idx] : nullptr;
}

CellClass* MapClass::TryGetCellAt(const CoordStruct& Crd) const
{
    CellStruct cell = CellClass::Coord2Cell(Crd);
    return TryGetCellAt(cell);
}

CellClass* MapClass::GetCellAt(const CellStruct &MapCoords) const
{
    auto pCell = TryGetCellAt(MapCoords);

    if(!pCell) {
        pCell = &InvalidCell;
        pCell->MapCoords = MapCoords;
    }

    return pCell;
}

CellClass* MapClass::GetCellAt(const CoordStruct &Crd) const
{
    CellStruct cell = CellClass::Coord2Cell(Crd);
    return GetCellAt(cell);
}

bool MapClass::CellExists(const CellStruct &MapCoords) const
{
    return TryGetCellAt(MapCoords) != nullptr;
}

CoordStruct MapClass::GetRandomCoordsNear(const CoordStruct &coords, int distance, bool center)
{
    CoordStruct outBuffer;
    GetRandomCoordsNear(outBuffer, coords, distance, center);
    return outBuffer;
}

CoordStruct MapClass::PickInfantrySublocation(const CoordStruct &coords, bool ignoreContents )
{
    CoordStruct outBuffer;
    PickInfantrySublocation(outBuffer, coords, ignoreContents);
    return outBuffer;
}

CellStruct MapClass::PickCellOnEdge(Edge Edge, const CellStruct &CurrentLocation, const CellStruct &Fallback,
        SpeedType SpeedType, bool ValidateReachability, MovementZone MovZone) const
{
    CellStruct buffer;
    this->PickCellOnEdge(buffer, Edge, CurrentLocation, Fallback, SpeedType, ValidateReachability, MovZone);
    return buffer;
}

CellStruct MapClass::NearByLocation(const CellStruct &position, SpeedType SpeedType, int a5, MovementZone MovementZone, bool alt, int SpaceSizeX, int SpaceSizeY, bool disallowOverlay, bool a11, bool requireBurrowable, bool allowBridge, const CellStruct &closeTo, bool a15, bool buildable)
{
    CellStruct outBuffer;
    NearByLocation(outBuffer, position, SpeedType, a5, MovementZone, alt, SpaceSizeX, SpaceSizeY, disallowOverlay, a11, requireBurrowable, allowBridge, closeTo, a15, buildable);
    return outBuffer;
}

CoordStruct MapClass::FindFirstFirestorm(
        const CoordStruct& start, const CoordStruct& end,
        HouseClass const* pHouse ) const
{
    CoordStruct outBuffer;
    FindFirstFirestorm(&outBuffer, start, end, pHouse);
    return outBuffer;
}

bool MapClass::IsLocationFogged(CoordStruct&& coord)
{ return IsLocationFogged(coord); }
