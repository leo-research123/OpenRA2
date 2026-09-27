// YR 56BAC0/56B3F0. RA1 MapClass has no IsoMapPack5 counterpart;
// XCC 70358b46 shp_decode.cpp confirms the 11-byte record / LZO framing.
// Original Cell fields and tile insertion vector are retained, not replaced
// with a second map model. Full Display startup remains a runtime boundary.
#include "yrpp/MapClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/Pipes.h"
#include "yrpp/Straws.h"
#include <bit>

namespace {
unsigned read16(const unsigned char* p) { return unsigned(p[0]) | (unsigned(p[1]) << 8); }
std::uint32_t read32(const unsigned char* p) { return read16(p) | (read16(p + 2) << 16); }
void write16(unsigned char* p, unsigned v) { p[0] = static_cast<unsigned char>(v); p[1] = static_cast<unsigned char>(v >> 8); }
void write32(unsigned char* p, std::uint32_t v) { write16(p, v); write16(p + 2, v >> 16); }
CellClass* resource_cell(MapClass& map, const CellStruct& coords) {
    const int index = MapClass::GetCellIndex(coords);
    return index >= 0 && index < MapClass::MaxCells && index < map.Cells.Capacity && map.Cells.Items ? map.Cells.Items[index] : nullptr;
}
bool read_records(MapClass& map, Straw& stream, bool has_ice) {
    for (;;) {
        unsigned char record[11];
        if (stream.Get(record, 4) != 4) return false;
        // ABD480 is initialized to (0,0) at 5618B0.
        if (!read32(record)) return true;
        const int payload = has_ice ? 7 : 6;
        if (stream.Get(record + 4, payload) != payload) return false;
        const CellStruct coords{static_cast<short>(read16(record)), static_cast<short>(read16(record + 2))};
        auto* found = resource_cell(map, coords);
        if (!found) continue;
        auto& cell = *found;
        cell.IsoTileTypeIndex = IsometricTileTypeClass::ConvertTileIndex(std::bit_cast<std::int32_t>(read32(record + 4)));
        cell.Height = std::bit_cast<char>(record[8]);
        cell.Level = std::bit_cast<char>(record[9]);
        if (has_ice) cell.IsIceGrowthAllowed = std::bit_cast<char>(record[10]);
    }
}
}
bool MapClass::ReadIsoMapPack(Straw& source) {
    LCWStraw stream(1, 8192); stream.Get_From(source);
    for (int plane = 0; plane < 3; ++plane) {
        const int width = plane == 0 ? 2 : 1;
        for (int i = 0; i < 128 * 128; ++i) {
            unsigned char bytes[2];
            if (stream.Get(bytes, width) != width) return false;
            auto* cell = resource_cell(*this, {static_cast<short>(i % 128), static_cast<short>(i / 128)});
            if (!cell) continue;
            if (!plane) cell->IsoTileTypeIndex = IsometricTileTypeClass::ConvertTileIndex(int(read16(bytes)));
            else if (plane == 1) cell->Height = std::bit_cast<char>(bytes[0]);
            else cell->Level = std::bit_cast<char>(bytes[0]);
        }
    }
    return true;
}
bool MapClass::ReadIsoMapPack2(Straw& source) {
    LCWStraw stream(1, 8192); stream.Get_From(source); return read_records(*this, stream, false);
}
bool MapClass::ReadIsoMapPack3(Straw& source) { return read_records(*this, source, false); }
bool MapClass::ReadIsoMapPack4(Straw& source) {
    LZOStraw stream(1, 8192); stream.Get_From(source); return read_records(*this, stream, false);
}
bool MapClass::ReadIsoMapPack5(Straw& source) {
    LZOStraw stream(1, 8192); stream.Get_From(source); return read_records(*this, stream, true);
}
int MapClass::WriteIsoMapPack5(Pipe& destination) const {
    LZOPipe stream(0, 8192);
    stream.Put_To(destination);
    int total = 0;
    for (int i = 1; Cells.Items && i < Cells.Capacity; ++i) {
        const auto* cell = Cells.Items[i];
        if (!cell) continue;
        const int x = cell->MapCoords.X, y = cell->MapCoords.Y;
        if ((!x && !y) || x + y <= MapRect.Width || x - y >= MapRect.Width || y - x >= MapRect.Width ||
            std::int64_t(x) + y > std::int64_t(MapRect.Width) + 2 * std::int64_t(MapRect.Height)) continue;
        if (cell->IsoTileTypeIndex == 0xffff && static_cast<signed char>(cell->Level) <= 0) continue;
        unsigned char record[11];
        write16(record, static_cast<unsigned>(x)); write16(record + 2, static_cast<unsigned>(y));
        write32(record + 4, static_cast<std::uint32_t>(cell->IsoTileTypeIndex));
        record[8] = static_cast<unsigned char>(cell->Height); record[9] = static_cast<unsigned char>(cell->Level);
        record[10] = static_cast<unsigned char>(cell->IsIceGrowthAllowed);
        const int count = stream.Put(record, sizeof(record));
        if (count < 0) return -1;
        total += count;
    }
    const unsigned char end[4]{};
    const int count = stream.Put(end, sizeof(end));
    if (count < 0) return -1;
    const int flushed = stream.Flush();
    return flushed < 0 ? -1 : total + count + flushed;
}
