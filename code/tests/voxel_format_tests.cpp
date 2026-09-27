#include "support/test_support.hpp"
#include "yrpp/FileFormats/VXL.h"
#include "yrpp/FileFormats/HVA.h"
#include "yrpp/CCFileClass.h"
#include "images/voxel_palette.hpp"
#include "filesystem/resource_environment.hpp"
#include "filesystem/resource_context.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
using Bytes = std::vector<std::uint8_t>;

void word(Bytes& b, std::size_t at, std::uint32_t value) {
    for (int n = 0; n < 4; ++n) b[at + n] = static_cast<std::uint8_t>(value >> (8 * n));
}
void real(Bytes& b, std::size_t at, float value) { word(b, at, std::bit_cast<std::uint32_t>(value)); }
Bytes hva(int frames = 2, int layers = 3) {
    Bytes b(24 + 16 * layers + 48 * frames * layers);
    word(b, 16, frames); word(b, 20, layers);
    for (int f = 0; f < frames; ++f)
        for (int l = 0; l < layers; ++l)
            for (int n = 0; n < 12; ++n)
                real(b, 24 + 16 * layers + 48 * (f * layers + l) + 4 * n,
                    float(100 * f + 20 * l + n) + 0.25f);
    return b;
}
Bytes vxl(int palettes = 1, bool contained = false) {
    const int palette_bytes = contained ? 2 + 768 * palettes : 770 * palettes;
    const int headers = 32 + palette_bytes, body = headers + 56, tailers = body + 16;
    Bytes b(tailers + 3 * 92);
    std::memcpy(b.data(), "Voxel Animation", 15);
    word(b, 16, palettes); word(b, 20, 2); word(b, 24, 3); word(b, 28, 16);
    if (palette_bytes) {
        b[32] = 16; b[33] = 31;
        for (int n = 0; n < 768; ++n) b[34 + n] = static_cast<std::uint8_t>(n * 73 + 11);
    }
    word(b, headers + 16, 1); word(b, headers + 20, 0x12345678); b[headers + 24] = 0xFE;
    word(b, headers + 28 + 16, 0); word(b, headers + 28 + 20, 1);
    for (int n = 0; n < 16; ++n) b[body + n] = static_cast<std::uint8_t>(n);
    for (int t = 0; t < 3; ++t) {
        const int at = tailers + 92 * t;
        word(b, at, 0); word(b, at + 4, 4); word(b, at + 8, 8);
        real(b, at + 12, float(t) + 0.5f);
        for (int n = 0; n < 12; ++n) real(b, at + 16 + 4 * n, float(t * 20 + n));
        for (int n = 0; n < 6; ++n) real(b, at + 64 + 4 * n, float(n + 1));
        b[at + 88] = 201; b[at + 89] = 2; b[at + 90] = 3; b[at + 91] = 4;
    }
    return b;
}

class MemoryFile : public CCFileClass {
public:
    Bytes bytes;
    std::size_t position{};
    int opens{}, closes{};
    bool available{true};
    std::vector<int> seeks;
    explicit MemoryFile(Bytes data) : bytes(std::move(data)) {}
    bool Open(FileAccessMode mode) override {
        EXPECT_TRUE((mode == FileAccessMode::Read)) << "read-only open";
        ++opens; position = 0; return available;
    }
    int ReadBytes(void* output, int size) override {
        EXPECT_TRUE((size >= 0)) << "nonnegative read";
        auto count = std::min<std::size_t>(size, bytes.size() - position);
        if (count) std::memcpy(output, bytes.data() + position, count);
        position += count;
        return static_cast<int>(count);
    }
    int Seek(int offset, FileSeekMode mode) override {
        EXPECT_TRUE((mode == FileSeekMode::Current && offset >= 0)) << "relative forward seek";
        seeks.push_back(offset);
        position = std::min(bytes.size(), position + offset);
        return static_cast<int>(position);
    }
    void Close() override { ++closes; }
};

void motion() {
    MemoryFile file(hva());
    MotLib mot(&file);
    EXPECT_TRUE((!mot.LoadedFailed && mot.LayerCount == 3 && mot.FrameCount == 2)) << "HVA header and failure flag";
    EXPECT_TRUE((file.closes == 1 && file.seeks == std::vector<int>{48})) << "HVA owns open and close";
    EXPECT_TRUE((&mot.GetLayerMatrix(2, 5) == mot.Matrixes + 5)) << "HVA frame wrap and frame-major indexing";
    mot.Scale(-2.0f);
    for (int f = 0; f < 2; ++f)
        for (int l = 0; l < 3; ++l)
            for (int n = 0; n < 12; ++n) {
                float expected = float(100 * f + 20 * l + n) + 0.25f;
                if (n % 4 == 3) expected *= -2.0f;
                EXPECT_TRUE((mot.GetLayerMatrix(l, f).Data[n] == expected)) << "HVA translation-only scale";
            }
    file.available = false;
    EXPECT_TRUE((mot.ReadFile(&file) == 0 && !mot.Matrixes && !mot.LoadedFailed)) << "HVA reread frees old data and retains construction flag";
    EXPECT_TRUE((mot.LayerCount == 3 && mot.FrameCount == 2 && file.closes == 1)) << "HVA open failure retains counts without close";
    file.available = true;
    EXPECT_TRUE((mot.ReadFile(&file) == 1 && mot.Matrixes)) << "HVA reload after failure";
    const auto complete = hva();
    for (std::size_t length = 0; length < complete.size(); ++length) {
        MemoryFile cut(Bytes(complete.begin(), complete.begin() + length));
        MotLib failed(&cut);
        EXPECT_TRUE((failed.LoadedFailed && !failed.Matrixes && cut.closes == 1)) << "HVA truncated input cleanup";
    }
    MemoryFile empty(hva(0, 0));
    MotLib zero(&empty);
    EXPECT_TRUE((!zero.LoadedFailed && zero.Matrixes)) << "YR accepts zero matrix counts";
    auto bad = hva(); word(bad, 16, 0xFFFFFFFF);
    MemoryFile oversized(bad); MotLib rejected(&oversized);
    EXPECT_TRUE((rejected.LoadedFailed && !rejected.Matrixes)) << "HVA rejects negative allocation dimensions";
}

void voxels() {
    MemoryFile file(vxl()); VoxLib vox(&file);
    EXPECT_TRUE((!vox.Initialized && vox.CountHeaders == 2 && vox.CountTailers == 3 && vox.TotalSize == 16)) << "VXL unequal section counts and inverted historical flag";
    EXPECT_TRUE((file.seeks == std::vector<int>{770} && file.closes == 1)) << "VXL palette skip and close";
    EXPECT_TRUE((vox.leaSectionHeader(0)->limb_number == 1 && vox.leaSectionTailer(0, 1) == vox.TailerData + 2)) << "VXL header and limb-based tailer addressing";
    const float corners[8][3] = {{4,5,3},{4,2,3},{1,2,3},{1,5,3},{4,5,6},{4,2,6},{1,2,6},{1,5,6}};
    for (int t = 0; t < 3; ++t) {
        auto& tail = vox.TailerData[t];
        EXPECT_TRUE((reinterpret_cast<std::uint8_t*>(tail.span_start_off) == vox.BodyData &&
            reinterpret_cast<std::uint8_t*>(tail.span_end_off) == vox.BodyData + 4 &&
            tail.span_data_off == vox.BodyData + 8)) << "VXL runtime pointer relocation";
        EXPECT_TRUE((tail.HVAMultiplier == float(t) + 0.5f && static_cast<std::uint8_t>(tail.size_X) == 201)) << "VXL scalar and unsigned dimension bytes";
        for (int n = 0; n < 12; ++n)
            EXPECT_TRUE((tail.TransformationMatrix.Data[n] == float(20 * t + n))) << "VXL unmodified disk matrix";
        EXPECT_TRUE((std::memcmp(tail.Bounds, corners, sizeof(corners)) == 0)) << "VXL original eight-corner order";
    }
    file.available = false;
    EXPECT_TRUE((vox.ReadFile(&file, false) == 0 && !vox.HeaderData && !vox.TailerData && !vox.BodyData &&
        !vox.Initialized && vox.CountHeaders == 2 && file.closes == 1)) << "VXL failed reread state";
    file.available = true;
    EXPECT_TRUE((vox.ReadFile(&file, false) == 1)) << "VXL repeat load";
    const auto complete = vxl(0);
    for (std::size_t length = 0; length < complete.size(); ++length) {
        MemoryFile cut(Bytes(complete.begin(), complete.begin() + length)); VoxLib failed(&cut);
        EXPECT_TRUE((failed.Initialized && !failed.HeaderData && !failed.TailerData && !failed.BodyData && cut.closes == 1)) << "VXL truncated input cleanup";
    }
    auto bad = vxl(0); word(bad, 32 + 56 + 16, 17);
    MemoryFile invalid(bad); VoxLib rejected(&invalid);
    EXPECT_TRUE((rejected.Initialized && !rejected.BodyData)) << "VXL rejects out-of-body relocated offset";
}

void palettes() {
    const auto state = game::GetVoxelPaletteStorage();
    std::fill_n(state.lighting, 32768, 0xA5);
    MemoryFile file(vxl(2, true)); VoxLib vox(&file, true);
    EXPECT_TRUE((!vox.Initialized && file.seeks == std::vector<int>{768})) << "VXL multiple embedded palettes consume 2+768*N";
    for (int n = 0; n < 768; ++n) EXPECT_TRUE((state.colors[n] == file.bytes[34 + n])) << "VXL palette remains raw RGB bytes";
    for (int shade = 0; shade < 32; ++shade) {
        EXPECT_TRUE((state.lighting[shade * 256] == 0xA5)) << "palette transparent column untouched";
        for (int index = 1; index < 256; ++index) {
            int mapped = state.lighting[shade * 256 + index];
            EXPECT_TRUE((mapped > 0 && ((index >= 16 && index <= 31) == (mapped >= 16 && mapped <= 31)))) << "lighting preserves remap color range";
        }
    }
    EXPECT_TRUE((state.levels[0] == 0.6f && state.levels[16] == 1.4f)) << "original two-part light curve";
    for (int n = 8192; n < 32768; ++n) EXPECT_TRUE((state.lighting[n] == 0xA5)) << "unused lighting storage untouched";
    std::array<std::uint8_t, 768> saved; std::copy_n(state.colors, 768, saved.begin());
    MemoryFile skip(vxl()); VoxLib skipped(&skip, false);
    EXPECT_TRUE((std::equal(saved.begin(), saved.end(), state.colors))) << "skip branch preserves shared palette";
    auto cut = vxl(); cut.resize(33);
    MemoryFile short_palette(cut); VoxLib failed(&short_palette, true);
    EXPECT_TRUE((failed.Initialized && !failed.BodyData && short_palette.closes == 1)) << "short remap range cleanup";
}

void real_file() {
    const auto root = std::filesystem::temp_directory_path() /
        ("ra2-voxel-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::filesystem::remove_all(path); } } cleanup{root};
    for (const auto& pair : {std::pair{"test.hva", hva()}, std::pair{"test.vxl", vxl()}}) {
        std::ofstream out(root / pair.first, std::ios::binary);
        out.write(reinterpret_cast<const char*>(pair.second.data()), pair.second.size());
        EXPECT_TRUE((bool(out))) << "write fixture";
    }
    game::ResourceEnvironment environment(root);
    game::ResourceScope scope({&environment, nullptr, {}});
    CCFileClass h("test.hva"), v("test.vxl");
    MotLib mot(&h); VoxLib vox(&v);
    EXPECT_TRUE((!mot.LoadedFailed && !vox.Initialized && !h.HasHandle() && !v.HasHandle())) << "real core CCFile loading";
}
}


TEST(VoxelFormat, Contracts) {
    motion(); voxels(); palettes(); real_file();
}
