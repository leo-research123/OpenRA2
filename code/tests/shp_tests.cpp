#include "support/test_support.hpp"
#include "api/images.hpp"
#include "yrpp/FileFormats/SHP.h"
#include "filesystem/resource_environment.hpp"
#include "filesystem/resource_context.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
struct UnloadScope { ~UnloadScope() { Unload_All_Shapes(); } };

struct Fixture {
    std::filesystem::path root = std::filesystem::temp_directory_path() /
        ("ra2-shp-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Fixture() { std::filesystem::create_directories(root); }
    ~Fixture() { std::error_code error; std::filesystem::remove_all(root, error); }
    void write(const std::vector<uint8_t>& bytes, const char* name) {
        std::ofstream stream(root / name, std::ios::binary);
        stream.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        EXPECT_TRUE((bool(stream))) << "write synthetic SHP";
    }
};
void word(std::vector<uint8_t>& bytes, size_t at, unsigned value) {
    bytes[at] = uint8_t(value); bytes[at + 1] = uint8_t(value >> 8);
}
std::vector<uint8_t> shape(bool compressed, std::initializer_list<uint8_t> payload) {
    std::vector<uint8_t> bytes(32, 0);
    word(bytes, 2, 8); word(bytes, 4, 6); word(bytes, 6, 1);
    word(bytes, 8, 2); word(bytes, 10, 1); word(bytes, 12, 4); word(bytes, 14, 2);
    bytes[16] = compressed ? 3 : 1; bytes[28] = 32;
    bytes.insert(bytes.end(), payload);
    return bytes;
}
void original_resources() {
    Fixture fixture;
    // Final transparent run overshoots, as in real GI. Row byte lengths differ.
    fixture.write(shape(true, {7, 0, 1, 0, 1, 3, 4, 8, 0, 0, 1, 5, 6, 0, 255}), "rle.shp");
    fixture.write(shape(false, {1, 0, 3, 4, 0, 5, 6, 0}), "raw.shp");
    game::ResourceEnvironment environment(fixture.root);
    game::ResourceScope scope({&environment, nullptr, {}});
    const UnloadScope unload;
    for (const char* name : {"raw.shp", "rle.shp"}) {
        SHPReference reference(name);
        reference.Load();
        EXPECT_TRUE((reference.Loaded && reference.Data)) << "original Load";
        auto& data = *reference.GetData();
        EXPECT_TRUE((data.Width == 8 && data.Height == 6 && data.Frames == 1)) << "original dimensions";
        const auto bounds = data.GetFrameBounds(0);
        EXPECT_TRUE((bounds.X == 2 && bounds.Y == 1 && bounds.Width == 4 && bounds.Height == 2)) << "original cropped frame anchor";
        const auto* payload = data.GetPixels(0);
        EXPECT_TRUE((payload == reinterpret_cast<const uint8_t*>(&data) + 32)) << "original payload address";
        std::vector<uint8_t> pixels;
        std::string error;
        EXPECT_TRUE((game::decode_shp_pixels(payload, bounds.Width, bounds.Height, data.HasCompression(0), pixels, error))) << "decode directly from original GetPixels without a total length";
        EXPECT_TRUE((pixels == std::vector<uint8_t>({1, 0, 3, 4, 0, 5, 6, 0}))) << "raw / XCC decode3 output";
    }
    {
        SHPReference first("rle.shp");
        SHPReference second("raw.shp");
        EXPECT_TRUE((second.Next == &first && first.Prev == &second && !second.Prev && !first.Next)) << "original reference list";
        EXPECT_TRUE((first.Index != 0 && first.Index != second.Index)) << "shared-cache sentinel is not a reference index";
        auto* shared = first.GetData();
        EXPECT_TRUE((shared && !first.Loaded)) << "original shared cache";
        EXPECT_TRUE((second.GetData() == shared)) << "shared buffer reuse";
        const auto* payload = first.GetPixels(0);
        EXPECT_TRUE((first.Loaded && payload && payload[0] == 7 && payload[1] == 0 && first.HasCompression(0))) << "GetPixels loads persistently and preserves the RLE header";
        RectangleStruct invalid{1,2,3,4};
        first.GetFrameBounds(invalid, -1);
        EXPECT_TRUE((invalid.X == 0 && invalid.Y == 0 && invalid.Width == 0 && invalid.Height == 0 &&
            !first.GetPixels(1) && !first.HasCompression(1))) << "original invalid-frame results";
        Unload_All_Shapes();
        environment.clear();
        EXPECT_TRUE((!first.Loaded && !first.Data && second.Next == &first && first.Prev == &second)) << "explicit SHP unload releases bytes without deleting caller-owned references";
        fixture.write(shape(false, {9, 0, 3, 4, 0, 5, 6, 0}), "raw.shp");
        auto* reloaded = second.GetData();
        EXPECT_TRUE((reloaded && reinterpret_cast<const BYTE*>(reloaded)[32] == 9)) << "explicit SHP unload invalidates shared data; the next access reads fresh bytes";
    }
    SHPReference missing("missing.shp");
    EXPECT_TRUE((!missing.Next && !missing.Prev)) << "previous references unlinked from the private module list";
    missing.Load();
    EXPECT_TRUE((!missing.Loaded && !missing.Data && !missing.GetData())) << "original missing-resource result";
}
void decode_contract() {
    std::vector<uint8_t> pixels{42};
    std::string error;
    EXPECT_TRUE((!game::decode_shp_pixels(nullptr, 4, 2, false, pixels, error) && pixels == std::vector<uint8_t>{42})) << "missing payload does not publish pixels";
    EXPECT_TRUE((!game::decode_shp_pixels(nullptr, -1, 2, true, pixels, error))) << "negative dimensions";
    EXPECT_TRUE((!game::decode_shp_pixels(nullptr, 32767, 32767, false, pixels, error))) << "bounded output allocation";
    EXPECT_TRUE((game::decode_shp_pixels(nullptr, 0, 2, true, pixels, error) && pixels.empty())) << "empty frame";
    // Storage is readable for the declared rows. Whole-file truncation is not
    // part of the original pointer-based input contract.
    for (const std::vector<uint8_t>& row : {
            std::vector<uint8_t>{1, 0},             // invalid row length
            std::vector<uint8_t>{3, 0, 0},          // missing transparent run count
            std::vector<uint8_t>{7, 0, 1, 2, 3, 4, 5}, // too many opaque pixels
            std::vector<uint8_t>{3, 0, 1}}) {       // incomplete decoded row
        pixels = {42}; error.clear();
        EXPECT_TRUE((!game::decode_shp_pixels(row.data(), 4, 1, true, pixels, error) && !error.empty() &&
            pixels == std::vector<uint8_t>{42})) << "invalid RLE row leaves output unchanged";
    }
}
void local_file(const char* path) {
    const auto absolute = std::filesystem::absolute(path);
    game::ResourceEnvironment environment(absolute.parent_path());
    game::ResourceScope scope({&environment, nullptr, {}});
    const UnloadScope unload;
    SHPReference reference(absolute.filename().string().c_str());
    reference.Load();
    EXPECT_TRUE((reference.Loaded && reference.Data)) << "load local SHP";
    auto& data = *reference.GetData();
    std::vector<uint8_t> pixels;
    std::string error;
    for (int i = 0; i < data.Frames; ++i) {
        const auto bounds = data.GetFrameBounds(i);
        const auto* payload = data.GetPixels(i);
        if (!payload) continue; // original Offset == 0 denotes an empty frame
        if (!game::decode_shp_pixels(payload, bounds.Width, bounds.Height, data.HasCompression(i), pixels, error))
            throw std::runtime_error("frame " + std::to_string(i) + ": " + error);
    }
    std::cout << "Processed " << data.Frames << " local frames (" << data.Width << 'x' << data.Height << ")\n";
}
}

TEST(Shp, Contracts) {
    const auto [argc, argv] = ra2::test::arguments();


            original_resources(); decode_contract();
            if (argc == 2) local_file(argv[1]);
}
