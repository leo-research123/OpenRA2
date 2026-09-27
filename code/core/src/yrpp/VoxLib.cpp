// Adapted from fixed EA Mission Editor 6abf0f557469baea73079c6bf6550709e2e3584e
// 3rdParty/xcc/misc/vxl_file.{h,cpp}, cc_structures.h.
// Copyright (C) 2000 Olaf van der Spek; GPL-3.0-or-later.
// See third_party/ea/MISSION_EDITOR_LICENSE.md.
// Retains YRpp VoxLib; lifecycle, relocation, bounds order and palette side
// effects calibrated against YR 1.001 755CD0..7564B0 / 758B70 / 758EA0.
#include "yrpp/FileFormats/VXL.h"
#include "yrpp/CCFileClass.h"
#include "yrpp/Memory.h"
#include "images/voxel_palette.hpp"
#include <algorithm>
#include <limits>
#include <new>

namespace {
void release(VoxLib& vox) {
    if (vox.HeaderData) YRMemory::Deallocate(vox.HeaderData);
    // Destructor callers in the original binary observe these null stores.
    *static_cast<VoxelSectionHeader* volatile*>(&vox.HeaderData) = nullptr;
    if (vox.TailerData) YRMemory::Deallocate(vox.TailerData);
    *static_cast<VoxelSectionTailer* volatile*>(&vox.TailerData) = nullptr;
    if (vox.BodyData) YRMemory::Deallocate(vox.BodyData);
    *static_cast<std::uint8_t* volatile*>(&vox.BodyData) = nullptr;
}

std::uint8_t nearest_color(const std::uint8_t* colors, float red, float green, float blue,
    int remap_begin, int remap_end, bool remap) {
    std::uint8_t best = remap ? static_cast<std::uint8_t>(remap_begin) : 1;
    float distance = 100000.0f;
    for (int index = 1; index < 256; ++index) {
        if ((index >= remap_begin && index <= remap_end) != remap) continue;
        const double dr = double(red) - colors[index * 3];
        const double dg = double(green) - colors[index * 3 + 1];
        const double db = double(blue) - colors[index * 3 + 2];
        const double candidate = db * db + dg * dg + dr * dr;
        // Original compares before rounding the new minimum to float.
        if (candidate < distance) {
            distance = static_cast<float>(candidate);
            best = static_cast<std::uint8_t>(index);
        }
    }
    return best;
}

// Embedded VXL palette + generated lighting table (758B70), not a VPL reader.
// All 13 identified YR 1.001 constructor calls disable this branch. Normal
// rendering consumes the prebuilt table loaded by 753B70 -> 758A30 at startup.
// This body was already ported, but the current false-only call chain does not
// require it. Do not implement additional unused paths for completeness; add
// them to scope only when the selected runtime has a confirmed call site.
bool read_palette(CCFileClass& file, std::uint32_t palettes) {
    const auto storage = game::GetVoxelPaletteStorage();
    if (!storage.colors || !storage.lighting || !storage.levels) return false;
    std::uint8_t remap_begin{}, remap_end{};
    if (file.ReadBytes(&remap_begin, 1) != 1 || file.ReadBytes(&remap_end, 1) != 1)
        return false;
    // 755DB0 ignores short RGB reads. Preserve existing bytes and lighting
    // side effects even if a subsequent section/body/tailer read fails.
    for (int index = 0; index < 768; ++index) file.ReadBytes(storage.colors + index, 1);
    if (palettes > 1) file.Seek(static_cast<int>(768u * (palettes - 1)), FileSeekMode::Current);
    for (int shade = 0; shade < 32; ++shade)
        storage.levels[shade] = shade < 16
            ? static_cast<float>((double(shade) * 0.0625) * double(0.8f) + double(0.6f))
            : static_cast<float>(double(2 * shade - 16) * 0.0625 + double(0.4f));
    for (int shade = 0; shade < 32; ++shade)
        for (int index = 1; index < 256; ++index) {
            const double level = storage.levels[shade];
            float red = static_cast<float>(storage.colors[index * 3] * level);
            float green = static_cast<float>(storage.colors[index * 3 + 1] * level);
            float blue = static_cast<float>(storage.colors[index * 3 + 2] * level);
            if (shade >= 16) {
                red = std::min(red, 255.0f);
                green = std::min(green, 255.0f);
                blue = std::min(blue, 255.0f);
            }
            storage.lighting[shade * 256 + index] = nearest_color(storage.colors,
                red, green, blue, remap_begin, remap_end,
                index >= remap_begin && index <= remap_end);
        }
    return true;
}
}

VoxLib::VoxLib(CCFileClass* source, bool palette)
    : Initialized(false), CountHeaders(0), CountTailers(0), TotalSize(0),
      HeaderData(nullptr), TailerData(nullptr), BodyData(nullptr) {
    Initialized = ReadFile(source, palette) == 0;
}

VoxLib::~VoxLib() { release(*this); }

int VoxLib::ReadFile(CCFileClass* file, bool palette) {
    release(*this);
    if (!file || !file->Open(FileAccessMode::Read)) return 0;
    const auto fail = [&]() { release(*this); file->Close(); return 0; };
    VoxFileHeader header{};
    if (file->ReadBytes(&header, sizeof(header)) != sizeof(header)) return fail();
    CountHeaders = static_cast<DWORD>(header.countHeaders_OrSections1);
    CountTailers = static_cast<DWORD>(header.countTailers_OrSections2);
    TotalSize = static_cast<DWORD>(header.totalSize);
    // Bound original 32-bit allocation/read arithmetic. Do not impose XCC's
    // signature, equal-section-count or exact-file-length validation on YR.
    constexpr auto limit = std::numeric_limits<int>::max();
    if (CountHeaders > limit / sizeof(VoxelSectionHeader) ||
        CountTailers > limit / sizeof(VoxelSectionTailer) || TotalSize > limit ||
        header.PaletteCount < 0 || header.PaletteCount > limit / 770) return fail();
    HeaderData = static_cast<VoxelSectionHeader*>(YRMemory::Allocate(
        std::size_t(CountHeaders) * sizeof(VoxelSectionHeader)));
    TailerData = static_cast<VoxelSectionTailer*>(YRMemory::Allocate(
        std::size_t(CountTailers) * sizeof(VoxelSectionTailer)));
    BodyData = static_cast<std::uint8_t*>(YRMemory::Allocate(TotalSize));
    if (!HeaderData || !TailerData || !BodyData) return fail();
    if (palette) {
        // Original consumes 2 + 768*N here, but skips 770*N below.
        if (!read_palette(*file, static_cast<std::uint32_t>(header.PaletteCount))) {
            // Remap-byte failure closes before freeing; ordinary VXL short
            // reads below free before closing (755F04 / 7560A8).
            file->Close();
            release(*this);
            return 0;
        }
    } else {
        // Normal YR path: leave the preloaded global VPL mapping untouched.
        file->Seek(770 * header.PaletteCount, FileSeekMode::Current);
    }
    for (DWORD index = 0; index < CountHeaders; ++index) {
        VoxelSectionFileHeader section;
        if (file->ReadBytes(&section, sizeof(section)) != sizeof(section)) return fail();
        new (HeaderData + index) VoxelSectionHeader;
        HeaderData[index].limb_number = section.headerData.limb_number;
        HeaderData[index].unk1 = section.headerData.unk1;
        HeaderData[index].unk2 = section.headerData.unk2;
    }
    if (file->ReadBytes(BodyData, static_cast<int>(TotalSize)) != static_cast<int>(TotalSize)) return fail();
    for (DWORD index = 0; index < CountTailers; ++index) {
        VoxelSectionFileTailer disk;
        if (file->ReadBytes(&disk, sizeof(disk)) != sizeof(disk)) return fail();
        // Avoid invalid native pointers for corrupt input. One-past-end is
        // accepted for an empty span. Original leaves these offsets unchecked.
        if (std::uint32_t(disk.span_start_off) > TotalSize ||
            std::uint32_t(disk.span_end_off) > TotalSize ||
            std::uint32_t(disk.span_data_off) > TotalSize) return fail();
        auto& tail = *new (TailerData + index) VoxelSectionTailer;
        tail.span_start_off = reinterpret_cast<std::int32_t*>(BodyData + disk.span_start_off);
        tail.span_end_off = reinterpret_cast<std::int32_t*>(BodyData + disk.span_end_off);
        tail.span_data_off = BodyData + disk.span_data_off;
        tail.HVAMultiplier = disk.DetFloat;
        tail.TransformationMatrix = disk.TransformationMatrix;
        const auto& lo = disk.MinBounds;
        const auto& hi = disk.MaxBounds;
        tail.Bounds[0] = {hi.X, hi.Y, lo.Z};
        tail.Bounds[1] = {hi.X, lo.Y, lo.Z};
        tail.Bounds[2] = {lo.X, lo.Y, lo.Z};
        tail.Bounds[3] = {lo.X, hi.Y, lo.Z};
        tail.Bounds[4] = {hi.X, hi.Y, hi.Z};
        tail.Bounds[5] = {hi.X, lo.Y, hi.Z};
        tail.Bounds[6] = {lo.X, lo.Y, hi.Z};
        tail.Bounds[7] = {lo.X, hi.Y, hi.Z};
        tail.size_X = disk.size_X;
        tail.size_Y = disk.size_Y;
        tail.size_Z = disk.size_Z;
        tail.NormalsMode = disk.NormalsMode;
    }
    file->Close();
    return 1;
}

VoxelSectionHeader* VoxLib::leaSectionHeader(int index) { return HeaderData + index; }
VoxelSectionTailer* VoxLib::leaSectionTailer(int index, int frame) {
    return TailerData + (frame + HeaderData[index].limb_number);
}
