// TMP format/diamond layout: XCC 70358b46858973426c1ecf204485cb2a88716217,
// misc/cc_structures.h and tmp_ts_file.cpp, Copyright Olaf van der Spek,
// GPL-3.0-or-later. YR class ownership, lazy loading and radar colors are
// calibrated at 544CB0, 547020 and 549E90.
#include "yrpp/IsometricTileTypeClass.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include <span>
#include <array>

bool IsometricTileTypeClass::IsTileIndexValid(int subTile,bool reload) {
    if(reload && !Image && unk_2F4)LoadTMP();
    auto* tmp=reinterpret_cast<const TMPStruct*>(GetImage());
    const TMPImage* image=nullptr;
    return tmp && tmp->GetSubTile(subTile,image) && image;
}
// OpenTS isotype.cpp Shadow_Caster_List, fixed 44fac744; YR 0x547370.
const CellStruct* IsometricTileTypeClass::ShadowCasterList() const {
    if(!ShadowCaster)return nullptr;
    static const auto lists=[] {
        std::array<std::array<CellStruct,6>,40> out{};
        for(auto& row:out)row[5]={0x7FFF,0x7FFF};
        for(int i:{20,21}){out[i][0]={0,-1};out[i][1]={0,-1};out[i][2]={-1,-2};}
        for(int i:{22,23,24}){out[i][0]={-1,-3};out[i][1]={0,-3};}
        out[25][0]={0,-3};
        out[26][0]={0,-3};out[26][1]={1,-3};out[26][2]={-1,-3};out[26][3]={-2,-3};
        for(int i:{27,28,29}){out[i][0]={0,-3};out[i][1]={-1,-3};out[i][2]={-2,-3};}
        out[27][3]={-3,-2};out[30][0]={-2,-3};out[31][0]={-2,-3};out[32][0]={0,-3};
        return out;
    }();
    for(int start:ShadowTileSets){const auto index=static_cast<unsigned>(ArrayIndex)-static_cast<unsigned>(start);
        if(index<40)return lists[index].data();}
    return nullptr;
}

namespace {
const TMPImage* sub_tile(const IsometricTileTypeClass& type, int index) {
    auto* tmp=reinterpret_cast<const TMPStruct*>(type.GetImage());
    if (!tmp || index<0 || tmp->Columns<=0 || tmp->Rows<=0) return nullptr;
    const TMPImage* image=nullptr;
    tmp->GetSubTile(index%(tmp->Columns*tmp->Rows),image);
    return image;
}
bool range(std::size_t offset, std::size_t count, std::size_t size) {
    return offset <= size && count <= size - offset;
}
std::uint32_t entry(const byte* data) {
    std::uint32_t value; std::memcpy(&value, data, sizeof(value)); return value;
}
void release_colors(DynamicVectorClass<Color16Struct*>& colors) {
    for (int i = 0; i < colors.Count; ++i) YRMemory::Deallocate(colors[i]);
    colors.Clear();
}
WORD radar_color(const byte* rgb, int step) {
    // YR's 7C5F00 leaves x87 at control word 0xE7F (53-bit, toward zero).
    // Its preceding brightness conversions therefore also affect the ratio's
    // double-to-float store. Reproduce that rounding locally, without changing
    // the host's floating-point environment. The exact binary64 1/12 is
    // 0x15555555555555 / 2^56; its product with 0..12 fits in uint64.
    float amount = static_cast<float>(double(step) / 12.0);
    if (static_cast<std::uint64_t>(double(amount) * 0x1p56) >
        0x15555555555555ull * unsigned(step))
        amount = std::bit_cast<float>(std::bit_cast<std::uint32_t>(amount) - 1u);
    byte color[3];
    for (int i = 0; i < 3; ++i) {
        const int bright = static_cast<int>(std::min(255.0, double(rgb[i]) * double(1.4f)));
        // 661190 first truncates the brighter endpoint; 661020 interpolates it
        // with the original color using a float parameter and x87 intermediates.
        color[i] = static_cast<byte>((1.0 - double(amount)) * rgb[i] + double(amount) * bright);
    }
    return WORD((color[0] >> Drawing::RedShiftRight << Drawing::RedShiftLeft) |
        (color[1] >> Drawing::GreenShiftRight << Drawing::GreenShiftLeft) |
        (color[2] >> Drawing::BlueShiftRight << Drawing::BlueShiftLeft));
}
bool make_colors(const TMPStruct* tmp, DynamicVectorClass<Color16Struct*>& colors) {
    const int count = tmp->Columns * tmp->Rows;
    if (!colors.SetCapacity(count, nullptr)) return false;
    for (int i = 0; i < count; ++i) {
        const TMPImage* image = nullptr;
        tmp->GetSubTile(i, image);
        auto* pairs = image ? static_cast<Color16Struct*>(YRMemory::Allocate(26 * sizeof(WORD))) : nullptr;
        if (image && !pairs) { release_colors(colors); return false; }
        colors.AddItem(pairs);
        if (!image) continue;
        for (int j = 0; j <= 12; ++j) {
            const WORD left = radar_color(image->RadarLeft, j), right = radar_color(image->RadarRight, j);
            std::memcpy(pairs + j * 2, &left, sizeof(left));
            std::memcpy(pairs + j * 2 + 1, &right, sizeof(right));
        }
    }
    return true;
}
}

LandType IsometricTileTypeClass::GetLandType(int index) const noexcept {
    // Original signed-byte lookup at 8288E4, covering the TMP terrain codes.
    static constexpr int lands[]{0,8,8,8,8,10,9,3,3,2,6,1,1,0,7,3};
    try {
        const auto* image=sub_tile(*this,index);
        if (!image || image->TerrainType<0 || image->TerrainType>=16) return LandType::Clear;
        return static_cast<LandType>(lands[image->TerrainType]);
    } catch (...) { return LandType::Clear; }
}
int IsometricTileTypeClass::GetSlopeIndex(int index) const noexcept {
    try { const auto* image=sub_tile(*this,index); return image ? image->RampType : 0; }
    catch (...) { return 0; }
}
bool IsometricTileTypeClass::GetTileDimensions(int index, int& width, int& height) const noexcept {
    try {
    auto* tmp=reinterpret_cast<const TMPStruct*>(GetImage());
    if (!tmp || index<0 || tmp->Columns<=0 || tmp->Rows<=0) return false;
    const TMPImage* image=nullptr; tmp->GetSubTile(index%(tmp->Columns*tmp->Rows),image);
    width=tmp->Width; height=tmp->Height;
    if (image && (image->Flags&1)) height=std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(height)+static_cast<std::uint32_t>(image->Y)-static_cast<std::uint32_t>(image->ExtraY));
    return true;
    } catch (...) { return false; }
}
bool TMPStruct::GetSubTile(int index, const TMPImage*& result) const {
    result = nullptr;
    if (Columns <= 0 || Rows <= 0 || Columns > 255 || Rows > 255 || index < 0 || index >= Columns * Rows)
        return false;
    std::memcpy(&result, reinterpret_cast<const byte*>(this + 1) + std::size_t(index) * sizeof(result), sizeof(result));
    return result != nullptr;
}

bool IsometricTileTypeClass::ReadTMP(const byte* data, size_t size) {
    if (!data || size < sizeof(TMPStruct)) return false;
    const std::span<const byte> bytes(data, size);
    TMPStruct header; std::memcpy(&header, bytes.data(), sizeof(header));
    // YR's draw routine uses the 60 x 30 format. Do not silently interpret a
    // TS 48 x 24 image with YR's geometry or accept truncated byte-sized counts.
    if (header.Columns < 1 || header.Columns > 255 || header.Rows < 1 || header.Rows > 255 ||
        header.Width != 60 || header.Height != 30) return false;
    const std::size_t count = std::size_t(header.Columns) * header.Rows;
    const std::size_t disk_start = sizeof(header) + count * sizeof(std::uint32_t);
    if (disk_start > bytes.size()) return false;
    for (std::size_t i = 0; i < count; ++i) {
        const std::size_t offset = entry(bytes.data() + sizeof(header) + i * 4);
        if (!offset) continue;
        if (offset < disk_start || !range(offset, sizeof(TMPImage) + 900, bytes.size())) return false;
        TMPImage image; std::memcpy(&image, bytes.data() + offset, sizeof(image));
        const auto payload = [&](std::int32_t where, std::size_t length) {
            return where >= int(sizeof(TMPImage)) && range(std::size_t(where), length, bytes.size() - offset);
        };
        if ((image.Flags & 2) && !payload(image.ZOffset, 900)) return false;
        if (image.Flags & 1) {
            if (image.ExtraWidth <= 0 || image.ExtraHeight <= 0) return false;
            const std::uint64_t pixels = std::uint64_t(image.ExtraWidth) * image.ExtraHeight;
            if (pixels > bytes.size() || !payload(image.ExtraOffset, std::size_t(pixels)) ||
                ((image.Flags & 2) && !payload(image.ExtraZOffset, std::size_t(pixels)))) return false;
        }
    }
    const std::size_t expansion = count * (sizeof(TMPImage*) - sizeof(std::uint32_t));
    if (bytes.size() > std::numeric_limits<std::size_t>::max() - expansion) return false;
    auto* storage = static_cast<byte*>(YRMemory::Allocate(bytes.size() + expansion));
    if (!storage) return false;
    std::memcpy(storage, &header, sizeof(header));
    std::memcpy(storage + disk_start + expansion, bytes.data() + disk_start, bytes.size() - disk_start);
    for (std::size_t i = 0; i < count; ++i) {
        const auto offset = entry(bytes.data() + sizeof(header) + i * 4);
        const TMPImage* image = offset ? reinterpret_cast<const TMPImage*>(storage + offset + expansion) : nullptr;
        std::memcpy(storage + sizeof(header) + i * sizeof(image), &image, sizeof(image));
    }
    DynamicVectorClass<Color16Struct*> colors;
    if (!make_colors(reinterpret_cast<const TMPStruct*>(storage), colors)) { YRMemory::Deallocate(storage); return false; }
    UnloadTMP();
    Image = reinterpret_cast<SHPStruct*>(storage); ImageAllocated = true;
    unk_2E4 = header.Columns; unk_2E8 = header.Rows;
    unk_2A4.Swap(colors);
    return true;
}

int IsometricTileTypeClass::LoadTMP() {
    char filename[sizeof(FileName) + 1]{};
    std::memcpy(filename, FileName, sizeof(FileName));
    if (!filename[0]) return 0;
    CCFileClass file(filename);
    if (!file.Open(FileAccessMode::Read)) return 0;
    const int size = file.GetFileSize();
    if (size < int(sizeof(TMPStruct))) return 0;
    auto* data = static_cast<byte*>(YRMemory::Allocate(std::size_t(size)));
    if (!data) return 0;
    const bool ok = file.ReadBytes(data, size) == size && ReadTMP(data, std::size_t(size));
    YRMemory::Deallocate(data);
    return ok ? size : 0;
}
SHPStruct* IsometricTileTypeClass::GetImage() const {
    if (!Image && unk_2F4) const_cast<IsometricTileTypeClass*>(this)->LoadTMP();
    return Image;
}
void IsometricTileTypeClass::UnloadTMP() noexcept {
    release_colors(unk_2A4);
    if (ImageAllocated) YRMemory::Deallocate(Image);
    Image = nullptr; ImageAllocated = false; unk_2E4 = unk_2E8 = 0;
}
bool IsometricTileTypeClass::UpdateRadarColors() {
    if (!Image) return false;
    DynamicVectorClass<Color16Struct*> colors;
    if (!make_colors(reinterpret_cast<const TMPStruct*>(Image), colors)) return false;
    release_colors(unk_2A4); unk_2A4.Swap(colors);
    return true;
}
