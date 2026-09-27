#include "support/test_support.hpp"
// Real PCX reader + production BSurface. The FileClass only supplies test bytes;
// it does not replace the decoder, allocator, Surface or color implementation.
#include "yrpp/PCX.h"
#include "yrpp/FileClass.h"
#include "yrpp/Surface.h"
#include "yrpp/Drawing.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {

class Input final : public FileClass {
public:
    explicit Input(bool rgb) : bytes(128 + 2048 + 768, 0) {
        bytes[0] = 10; bytes[1] = 5; bytes[2] = 1; bytes[3] = 8;
        bytes[8] = 1; // xMin=0, xMax=1: two pixels
        bytes[65] = rgb ? 3 : 1; bytes[66] = 2;
        if (rgb) {
            // Planar R,G,B, all below C0 so this is a literal RLE stream.
            bytes[128] = 120; bytes[129] = 0;
            bytes[130] = 0; bytes[131] = 80;
            bytes[132] = bytes[133] = 0;
        } else {
            bytes[128] = 7; bytes[129] = 11;
        }
        for (unsigned i = 0; i < 768; ++i) bytes[bytes.size() - 768 + i] = BYTE(i);
    }
    const char* GetFileName() const override { return "synthetic.pcx"; }
    const char* SetFileName(const char*) override { return GetFileName(); }
    BOOL CreateFile() override { return false; }
    BOOL DeleteFile() override { return false; }
    bool Exists(bool = false) override { return true; }
    bool HasHandle() override { return opened; }
    bool Open(FileAccessMode = FileAccessMode::Read) override { opened = true; cursor = 0; return true; }
    bool OpenEx(const char*, FileAccessMode mode) override { return Open(mode); }
    int ReadBytes(void* output, int count) override {
        if (!opened || count < 0) return 0;
        const auto n = std::min<std::size_t>(std::size_t(count), bytes.size() - cursor);
        std::memcpy(output, bytes.data() + cursor, n); cursor += n;
        return int(n);
    }
    int Seek(int offset, FileSeekMode mode) override {
        const auto base = mode == FileSeekMode::End ? std::int64_t(bytes.size()) :
            mode == FileSeekMode::Current ? std::int64_t(cursor) : 0;
        const auto next = base + offset;
        EXPECT_TRUE((next >= 0 && next <= std::int64_t(bytes.size()))) << "test file seek bounds";
        cursor = std::size_t(next); return int(cursor);
    }
    int GetFileSize() override { return int(bytes.size()); }
    int WriteBytes(void*, int) override { return 0; }
    void Close() override { opened = false; ++closes; }
    void CDCheck(DWORD, bool = false, const char* = nullptr) override {}
    std::vector<BYTE> bytes;
    std::size_t cursor = 0;
    bool opened = false;
    int closes = 0;
};
using Image = std::unique_ptr<BSurface, decltype(&BSurface::Destroy)>;
void indexed(bool borrow) {
    Input file(false);
    std::array<BYTE, 4> pixels{0,0,0xa5,0x5a};
    BytePalette palette{};
    Image image(Read_PCX_File(&file, &palette, borrow ? pixels.data() : nullptr,
        borrow ? unsigned(pixels.size()) : 0u), &BSurface::Destroy);
    EXPECT_TRUE((image && image->Width == 2 && image->Height == 1)) << "PCX dimensions";
    EXPECT_TRUE((image->GetBytesPerPixel() == 1 && image->GetPitch() == 2)) << "indexed surface format";
    const auto* output = static_cast<const BYTE*>(image->Lock(0, 0));
    EXPECT_TRUE((output && output[0] == 7 && output[1] == 11)) << "literal indexed pixels";
    if (borrow) EXPECT_TRUE((output == pixels.data())) << "host must borrow pixels, not a temporary MemoryBuffer address";
    image->Unlock();
    EXPECT_TRUE((!image->IsLocked() && !file.opened && file.closes == 1)) << "success releases lock and closes file";
    EXPECT_TRUE((palette.Entries[0].R == 0 && palette.Entries[0].G == 1 && palette.Entries[0].B == 2)) << "palette is read from the file tail";
    image.reset();
    if (borrow) EXPECT_TRUE((pixels[0] == 7 && pixels[2] == 0xa5 && pixels[3] == 0x5a)) << "borrowed memory remains owned by caller";
}
void rgb() {
    Drawing::SetColorMode(static_cast<RGBMode>(2));
    Input file(true);
    Image image(Read_PCX_File(&file, nullptr, nullptr, 0), &BSurface::Destroy);
    EXPECT_TRUE((image && image->GetBytesPerPixel() == 2)) << "planar PCX output format";
    const auto* output = static_cast<const WORD*>(image->Lock(0,0));
    EXPECT_TRUE((output && output[0] == 0x7800 && output[1] == 0x0280)) << "RGB565 golden pixels";
    image->Unlock();
    EXPECT_TRUE((!file.opened && file.closes == 1)) << "RGB file close";
}
void original_rejection_contract() {
    Input file(false);
    file.bytes[0] = file.bytes[1] = file.bytes[3] = 0;
    Image image(Read_PCX_File(&file, nullptr, nullptr, 0), &BSurface::Destroy);
    EXPECT_TRUE((!image && file.opened && file.closes == 0)) << "original header failure leaves caller's file open";
    file.Close();
}
}

TEST(PcxRead, Contracts) {
    indexed(false); indexed(true); rgb(); original_rejection_contract();
}
