// Adapted from fixed EA Mission Editor 6abf0f557469baea73079c6bf6550709e2e3584e
// 3rdParty/xcc/misc/hva_file.{h,cpp}, cc_structures.h.
// Copyright (C) 2000 Olaf van der Spek; GPL-3.0-or-later.
// See third_party/ea/MISSION_EDITOR_LICENSE.md.
#include "yrpp/FileFormats/HVA.h"
#include "yrpp/CCFileClass.h"
#include "yrpp/Memory.h"
#include <limits>
#include <new>

MotLib::MotLib(CCFileClass* source)
    : LoadedFailed(false), LayerCount(0), FrameCount(0), Matrixes(nullptr) {
    LoadedFailed = ReadFile(source) == 0;
}

MotLib::~MotLib() {
    if (Matrixes) YRMemory::Deallocate(Matrixes);
    // Preserve the original destruction store for raw ABI callers; ordinary
    // dead-store elimination can otherwise remove it after the lifetime ends.
    *static_cast<Matrix3D* volatile*>(&Matrixes) = nullptr;
}

int MotLib::ReadFile(CCFileClass* file) {
    if (Matrixes) YRMemory::Deallocate(Matrixes);
    Matrixes = nullptr;
    if (!file || !file->Open(FileAccessMode::Read)) return 0;
    struct Header { char name[16]; std::int32_t frames, layers; } header{};
    static_assert(sizeof(Header) == 24);
    // Original ignores a short fixed header. Reject it deterministically rather
    // than consuming uninitialized stack counts. Later short reads match YR.
    if (file->ReadBytes(&header, sizeof(header)) != sizeof(header)) {
        file->Close();
        return 0;
    }
    LayerCount = header.layers;
    FrameCount = header.frames;
    const auto count = std::uint64_t(std::uint32_t(LayerCount)) * std::uint32_t(FrameCount);
    if (LayerCount < 0 || FrameCount < 0 ||
        LayerCount > std::numeric_limits<int>::max() / 16 ||
        count > std::numeric_limits<int>::max() / sizeof(Matrix3D)) {
        file->Close();
        return 0;
    }
    Matrixes = static_cast<Matrix3D*>(YRMemory::Allocate(std::size_t(count) * sizeof(Matrix3D)));
    if (!Matrixes) { file->Close(); return 0; }
    file->Seek(16 * LayerCount, FileSeekMode::Current);
    // XCC's accessor is section-major. The target binary reads consecutive
    // frames, each containing LayerCount matrices; no transposition is needed.
    for (std::size_t index = 0; index < count; ++index) {
        Matrix3D matrix{noinit_t()};
        if (file->ReadBytes(&matrix, sizeof(matrix)) != sizeof(matrix)) {
            file->Close();
            YRMemory::Deallocate(Matrixes);
            Matrixes = nullptr;
            return 0;
        }
        new (Matrixes + index) Matrix3D(matrix);
    }
    file->Close();
    return 1;
}

void MotLib::Scale(float scale) {
    // Empty/failed objects have no matrix storage. Valid objects preserve the
    // original per-component float rounding and leave the 3x3 basis untouched.
    if (!Matrixes) return;
    for (int frame = 0; frame < FrameCount; ++frame)
        for (int layer = 0; layer < LayerCount; ++layer) {
            auto& matrix = Matrixes[std::size_t(frame) * LayerCount + layer];
            matrix.row[0][3] *= scale;
            matrix.row[1][3] *= scale;
            matrix.row[2][3] *= scale;
        }
}
