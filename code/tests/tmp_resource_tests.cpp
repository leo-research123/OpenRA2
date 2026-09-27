#include "support/test_support.hpp"
#include "yrpp/IsometricTileTypeClass.h"
#include "api/software_type_drawing.hpp"
#include "yrpp/AlphaLightingRemapClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/MixFileClass.h"
#include "yrpp/Theater.h"
#include "api/filesystem.hpp"
#include "experiments/tile_half_invert.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {

void put(std::vector<byte>& data, std::size_t offset, std::uint32_t value) { std::memcpy(data.data() + offset, &value, 4); }
std::vector<byte> fixture() {
    // Two slots (one missing), non-contiguous extra and Z planes, negative extra
    // placement, nonzero block origin, and a transparent extra pixel.
    std::vector<byte> data(24 + 52 + 900 + 900 + 17 + 12 + 9 + 12);
    put(data, 0, 2); put(data, 4, 1); put(data, 8, 60); put(data, 12, 30); put(data, 16, 24);
    put(data, 24, 100); put(data, 28, 200);
    put(data, 32, 1869); put(data, 36, 952); put(data, 40, 1890);
    put(data, 44, 92); put(data, 48, 195); put(data, 52, 4); put(data, 56, 3); put(data, 60, 7);
    data[64] = 3; data[65] = 7; data[66] = 2;
    data[67] = 100; data[68] = 150; data[69] = 200;
    data[70] = 255; data[71] = 17; data[72] = 0;
    for (int i = 0; i < 900; ++i) { data[76 + i] = byte(i % 256); data[976 + i] = byte(i % 4); }
    for (int i = 0; i < 12; ++i) { data[24 + 1869 + i] = byte(i); data[24 + 1890 + i] = 5; }
    return data;
}
WORD word(const void* p) { WORD value; std::memcpy(&value, p, 2); return value; }
std::vector<int> expiration;
void expired(AbstractClass* item) noexcept { expiration.push_back(static_cast<IsometricTileTypeClass*>(item)->ArrayIndex); }
void resources() {
    const int main_count = IsometricTileTypeClass::Array.Count, all_count = IsometricTileTypeClass::AllTypes.Count;
    auto raw = fixture(); const auto original = raw;
    {
        IsometricTileTypeClass tile(10, -65, 0, "TEST", 0);
        EXPECT_TRUE((tile.unk_2DC == 191 && tile.AllowBurrowing && !tile.Selectable && tile.Immune)) << "original constructor defaults";
        EXPECT_TRUE((IsometricTileTypeClass::Array.Count == main_count + 1)) << "main type registration";
        EXPECT_TRUE((!tile.ReadTMP(nullptr, 0) && !tile.Image && !tile.ImageAllocated)) << "empty input leaves an unloaded tile unchanged";
        EXPECT_TRUE((tile.ReadTMP(raw.data(), raw.size()))) << "load TMP bytes";
        auto* tmp = reinterpret_cast<TMPStruct*>(tile.Image);
        auto* radar = tile.unk_2A4.Items;
        const auto check_preserved = [&] {
            EXPECT_TRUE((tile.Image == reinterpret_cast<SHPStruct*>(tmp) && tile.ImageAllocated &&
                tile.unk_2E4 == 2 && tile.unk_2E8 == 1 && tile.unk_2A4.Items == radar && tile.unk_2A4.Count == 2)) << "failed load preserves image ownership, dimensions and radar storage";
        };
        EXPECT_TRUE((!tile.ReadTMP(nullptr, raw.size()))) << "reject null data with a nonzero size";
        check_preserved();
        EXPECT_TRUE((!tile.ReadTMP(nullptr, 0))) << "reject null data with zero size";
        check_preserved();
        const TMPImage* image = nullptr;
        EXPECT_TRUE((tmp->GetSubTile(0, image))) << "present sub-tile";
        const TMPImage* missing = image;
        EXPECT_TRUE((!tmp->GetSubTile(1, missing) && !missing && !tmp->GetSubTile(-1, missing))) << "missing sub-tile clears output";
        EXPECT_TRUE((image && image->Height == 3 && image->TerrainType == 7 && image->RampType == 2)) << "TMP metadata";
        EXPECT_TRUE((image->At(image->ExtraOffset)[11] == 11 && image->At(image->ExtraZOffset)[11] == 5)) << "relative plane offsets";
        EXPECT_TRUE((tile.unk_2A4.Count == 2 && tile.unk_2A4[1] == nullptr)) << "radar slots preserve holes";
        EXPECT_TRUE((word(tile.unk_2A4[0]) == 0x64b9 && word(tile.unk_2A4[0] + 1) == 0xf880)) << "original radar endpoint RGB565";
        EXPECT_TRUE((word(tile.unk_2A4[0] + 24) == 0x8e9f)) << "bright radar endpoint RGB565";
        EXPECT_TRUE((raw == original)) << "disk directory never mutated";
        for (std::size_t length = 0; length < raw.size(); ++length) {
            EXPECT_TRUE((!tile.ReadTMP(raw.data(), length))) << "reject truncated payload";
            check_preserved();
        }
        for (const auto [offset, value] : std::array<std::pair<int, std::uint32_t>, 8>{{
                {0,0},{4,256},{8,48},{16,4},{36,0xffffffffu},{32,0xfffffff0u},{52,0xffffffffu},{56,0x7fffffffu}}}) {
            auto bad = raw; put(bad, offset, value);
            EXPECT_TRUE((!tile.ReadTMP(bad.data(), bad.size()))) << "reject malformed offsets/dimensions";
            check_preserved();
        }
        for (int i = 0; i < 30; ++i) EXPECT_TRUE((tile.ReadTMP(raw.data(), raw.size()))) << "repeat load owns each allocation once";
        EXPECT_TRUE((tile.UpdateRadarColors())) << "regenerate derived colors";
        tile.UnloadTMP(); tile.UnloadTMP();
        EXPECT_TRUE((!tile.Image && !tile.ImageAllocated && !tile.unk_2E4 && !tile.unk_2A4.Count)) << "idempotent unload";
        tile.NextVariant = GameCreate<IsometricTileTypeClass>(11, 191, 1, "VARIANT", 1);
        tile.unk_2F0 = 2;
        EXPECT_TRUE((IsometricTileTypeClass::Array.Count == main_count + 1 && IsometricTileTypeClass::AllTypes.Count == all_count + 2)) << "variant registration";
        EXPECT_TRUE((tile.NextVariant->ReadTMP(raw.data(), raw.size()))) << "variant owns its resource";
        IsometricTileTypeClass::PointerExpirationObserver = expired;
    }
    IsometricTileTypeClass::PointerExpirationObserver = nullptr;
    EXPECT_TRUE((expiration == std::vector<int>({10,11}))) << "expiration precedes recursive variant release";
    EXPECT_TRUE((IsometricTileTypeClass::Array.Count == main_count && IsometricTileTypeClass::AllTypes.Count == all_count)) << "type arrays released";
}
void files() {
    const auto root = std::filesystem::temp_directory_path() /
        ("ra2-tmp-resources-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
    auto raw = fixture();
    for (const char* name : {"abcdefghijklmn", "tile.tem", "tile.sno", "tile.urb", "tile.ubn", "tile.des", "tile.lun"}) {
        std::ofstream file(root / name, std::ios::binary); file.write(reinterpret_cast<const char*>(raw.data()), raw.size());
    }
    game::ResourceHandle* handle = nullptr; std::string error;
    EXPECT_TRUE((game::create_resources(root.string(), handle, error))) << "create file environment";
    std::unique_ptr<game::ResourceHandle, decltype(&game::destroy_resources)> owner(handle, game::destroy_resources);
    const bool loaded = game::with_resources(*handle, [](void*) {
        IsometricTileTypeClass tile(0, 191, 0, "FILE", 0);
        std::memcpy(tile.FileName, "abcdefghijklmn", 14); tile.unk_2F4 = true;
        auto* image = tile.GetImage();
        EXPECT_TRUE((image && tile.GetImage() == image)) << "lazy load handles unterminated 14-byte filename";
        std::strcpy(tile.FileName, "missing.tem");
        EXPECT_TRUE((tile.LoadTMP() == 0 && tile.GetImage() == image)) << "failed reload retains live image";
        for (const char* name : {"tile.tem", "tile.sno", "tile.urb", "tile.ubn", "tile.des", "tile.lun"}) {
            std::strcpy(tile.FileName, name);
            EXPECT_TRUE((tile.LoadTMP() > 0)) << "reload with each YR theater suffix";
            EXPECT_TRUE((tile.ImageAllocated && tile.unk_2A4.Count == 2)) << "file and radar ownership";
        }
    }, nullptr, error);
    if (!loaded) throw std::runtime_error(error);
    owner.reset(); std::filesystem::remove_all(root);
}
void draw_callback(void*, const game::TileDrawArguments& a) {
    static_cast<IsometricTileTypeClass*>(a.tile_type)->DrawTMP(static_cast<ConvertClass*>(a.convert), a.sub_tile,
        static_cast<Surface*>(a.surface), a.x, a.y, a.clip, a.level, a.intensity, a.use_z, a.alternate,
        a.flat, a.flag16, a.flag17, a.color);
}
void draw_unscoped() {
    {
        BSurface canvas(5,4,1); EXPECT_TRUE((canvas.Fill(7))) << "8-bit surface fill";
        RectangleStruct clip{1,1,3,2}, fill{1,0,4,3};
        EXPECT_TRUE((canvas.FillRectEx(&clip,&fill,9))) << "relative clipped fill";
        const auto* values = static_cast<const byte*>(canvas.Lock(0,0));
        for (int y=0;y<4;++y) for (int x=0;x<5;++x)
            EXPECT_TRUE((values[y*5+x] == (x>=2 && x<=3 && y>=1 && y<=2 ? 9 : 7))) << "fill clipping respects surface window";
        canvas.Unlock();
    }
    constexpr int width = 128, height = 96;
    IsometricTileTypeClass tile(0, 191, 0, "DRAW", 0);
    const auto draw_fixture = fixture();
    EXPECT_TRUE((tile.ReadTMP(draw_fixture.data(), draw_fixture.size()))) << "draw fixture load";
    ABuffer alpha({0,0,width,height}); ZBuffer depth({0,0,width,height});
    ABuffer::Instance = &alpha; ZBuffer::Instance = &depth;
    BSurface output(width,height,2); BytePalette palette{};
    ConvertClass convert(palette,palette,2,53,false);
    const int remap_count = AlphaLightingRemapClass::Array.Count;
    const int remap_references = remap_count ? AlphaLightingRemapClass::Array[0]->RefCount : 0;
    auto* colors = static_cast<WORD*>(convert.FullColorData);
    for (int i = 0; i < 53 * 256; ++i) colors[i] = WORD(i * 197);
    auto* pixels = static_cast<WORD*>(output.Lock(0,0)); output.Unlock();
    EXPECT_TRUE((alpha.BufferSize == width*height*2 && word(alpha.BufferHead) == 127 && word(depth.BufferHead) == 0xffff)) << "real A/Z buffer initialization";
    game::TileInvertScratch scratch;
    for (bool z : {false, true}) for (const auto clip : {RectangleStruct{0,0,width,height}, RectangleStruct{35,27,30,13}}) {
        output.Fill(0x1234); depth.Fill(0xffff);
        const game::TileDrawArguments args{&tile,&convert,0,&output,20,20,clip,0,1000,z,0,false,false,false,0};
        draw_callback(nullptr,args);
        std::vector<WORD> normal(pixels,pixels+width*height);
        std::vector<WORD> z_normal(static_cast<WORD*>(depth.BufferHead),static_cast<WORD*>(depth.BufferHead)+width*height);
        const auto changed = std::count_if(normal.begin(), normal.end(), [](WORD p){ return p != 0x1234; });
        EXPECT_TRUE((changed > 0)) << "baseline must actually render";
        output.Fill(0x1234); depth.Fill(0xffff);
        EXPECT_TRUE((game::draw_tile_half_inverted(args,convert.FullColorData,2,53,0xffff,scratch,draw_callback,nullptr))) << "core half inversion";
        for (int i = 0; i < width*height; ++i) {
            const WORD expected = normal[i] != 0x1234 && i % width < 50 ? WORD(normal[i] ^ 0xffff) : normal[i];
            EXPECT_TRUE((pixels[i] == expected)) << "half inversion uses core pixels";
        }
        EXPECT_TRUE((std::equal(z_normal.begin(),z_normal.end(),static_cast<WORD*>(depth.BufferHead)))) << "half inversion preserves depth";
        EXPECT_TRUE((!output.IsLocked() && !alpha.Surface->IsLocked() && !depth.Surface->IsLocked())) << "all production surfaces unlocked";
    }
    auto without_z = fixture(); put(without_z,60,1); put(without_z,40,0xfffff000u);
    EXPECT_TRUE((tile.ReadTMP(without_z.data(), without_z.size()))) << "unused extra Z offset is not a plane";
    depth.Fill(0xffff); output.Fill(0x1234);
    tile.DrawTMP(&convert,0,&output,20,20,{12,15,4,3},0,1000,true,0,false,false,false,0);
    EXPECT_TRUE((pixels[17*width+15] != 0x1234 && pixels[15*width+12] == 0x1234)) << "extra-only color and transparency";
    EXPECT_TRUE((std::all_of(static_cast<WORD*>(depth.BufferHead),static_cast<WORD*>(depth.BufferHead)+width*height,
        [](WORD value){return value==0xffff;}))) << "extra-only clip never reads an absent Z plane";
    auto alternate = fixture(); put(alternate,60,2);
    std::fill_n(alternate.data()+76,900,byte(200));
    tile.NextVariant = GameCreate<IsometricTileTypeClass>(1,191,1,"ALT",1);
    EXPECT_TRUE((tile.NextVariant->ReadTMP(alternate.data(), alternate.size()))) << "load alternate TMP"; tile.unk_2F0 = 2;
    output.Fill(0x1234);
    tile.DrawTMP(&convert,0,&output,20,20,{0,0,width,height},0,1000,false,3,false,false,false,0);
    EXPECT_TRUE((pixels[20*width+48] == colors[26*256+200])) << "alternate modulo selects the variant resource";
    output.Fill(0x1234);
    tile.DrawTMP(&convert,1,&output,20,20,{0,0,width,height},0,1000,false,0,false,false,false,0);
    EXPECT_TRUE((std::all_of(pixels,pixels+width*height,[](WORD value){return value==0x1234;}))) << "missing sub-tile has no pixel side effects";
    EXPECT_TRUE((AlphaLightingRemapClass::Array.Count == remap_count &&
        (!remap_count || AlphaLightingRemapClass::Array[0]->RefCount == remap_references))) << "draw releases its shade remap";
    ABuffer::Instance = nullptr; ZBuffer::Instance = nullptr;
    alpha.ReleaseSurface(); depth.ReleaseSurface();
}
void real_files(const char* directory) {
    game::ResourceHandle* handle = nullptr; std::string error;
    EXPECT_TRUE((game::create_resources(directory, handle, error))) << "create real resource environment";
    std::unique_ptr<game::ResourceHandle, decltype(&game::destroy_resources)> owner(handle, game::destroy_resources);
    EXPECT_TRUE((game::load_resources(*handle, {}, error) == game::ResourceLoadResult::complete)) << "mount real MIX packages";
    const bool loaded = game::with_resources(*handle, [](void*) {
        int total = 0;
        for (const auto& theater : Theater::Array) {
            // Test-only package mounting. Does not pretend to initialize the
            // original world, tile catalog or renderer through Theater::Init.
            std::vector<std::unique_ptr<MixFileClass>> mixes;
            for (const auto* stem : {theater.ControlFileName, theater.ArtFileName, theater.PaletteFileName})
                for (const auto* suffix : {".MIX", "MD.MIX"}) {
                    const auto name = std::string(stem) + suffix;
                    mixes.push_back(std::make_unique<MixFileClass>(name.c_str()));
                }
            CCINIClass ini;
            const auto name = std::string(theater.ControlFileName) + "MD.INI"; // 545504
            EXPECT_TRUE((ini.LoadFromFile(name.c_str()) > 0)) << "read real theater catalog using existing INI";
            int files = 0, extras = 0, zplanes = 0, holes = 0;
            IsometricTileTypeClass tile(0,191,0,"REAL",0);
            for (int set = 0; set < 10000; ++set) {
                char section[32]; std::snprintf(section,sizeof(section),"TileSet%04d",set);
                if (!ini.GetSection(section)) break;
                char base[128]{}; ini.ReadString(section,"FileName","",base,sizeof(base));
                const int count = ini.ReadInteger(section,"TilesInSet",0);
                for (int i = 1; i <= count; ++i) for (int variant = -1; variant < 26; ++variant) {
                    char filename[160];
                    if (variant < 0) std::snprintf(filename,sizeof(filename),"%s%02d.%s",base,i,theater.Extension);
                    else std::snprintf(filename,sizeof(filename),"%s%02d%c.%s",base,i,'a'+variant,theater.Extension);
                    CCFileClass file(filename);
                    if (!file.Exists()) { if (variant >= 0) break; else continue; }
                    const int size = file.GetFileSize();
                    std::vector<byte> bytes(static_cast<std::size_t>(size));
                    EXPECT_TRUE((file.ReadBytes(bytes.data(),size) == size)) << "read real TMP through core MIX chain";
                    if (!tile.ReadTMP(bytes.data(), bytes.size())) throw std::runtime_error(std::string("real TMP rejected: ") + filename);
                    auto* tmp = reinterpret_cast<TMPStruct*>(tile.Image);
                    for (int n = 0; n < tmp->Columns * tmp->Rows; ++n) {
                        const TMPImage* image = nullptr;
                        tmp->GetSubTile(n, image);
                        if (!image) ++holes;
                        else { extras += (image->Flags & 1) != 0; zplanes += (image->Flags & 2) != 0; }
                    }
                    ++files;
                }
            }
            EXPECT_TRUE((files > 0)) << "theater must exercise actual TMP resources";
            total += files;
            std::cout << theater.ID << ": " << files << " TMP, " << extras << " extra, " << zplanes << " Z, " << holes << " holes\n";
        }
        std::cout << "Real TMP files loaded: " << total << '\n';
    },nullptr,error);
    if (!loaded) throw std::runtime_error(error);
}
}
void draw() {
    const auto context = game::make_software_type_drawing(nullptr, nullptr);
    const auto result = game::with_type_drawing(context, [](void*) { draw_unscoped(); }, nullptr);
    EXPECT_TRUE((result == game::DrawingStatus::drawn || result == game::DrawingStatus::skipped)) << "scoped TMP backend";
}


TEST(TmpResource, Contracts) {
    const auto [argc, argv] = ra2::test::arguments();

         resources(); files(); draw(); if (argc > 1) real_files(argv[1]);
}
