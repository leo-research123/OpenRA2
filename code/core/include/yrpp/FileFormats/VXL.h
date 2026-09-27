#pragma once

class CCFileClass;
#include "yrpp/Matrix3D.h"
#include <cstddef>
#include <cstdint>

struct VoxelSectionHeader;
struct VoxelSectionTailer;

class VoxLib {
public:
    bool Initialized; // Historical YRpp name: true means construction FAILED.
    DWORD CountHeaders;
    DWORD CountTailers;
    DWORD TotalSize;
    VoxelSectionHeader *HeaderData;
    VoxelSectionTailer *TailerData;
    std::uint8_t * BodyData;

    // YR 1.001's 13 identified constructor call sites pass false: skip the
    // embedded palette and preserve the global table loaded from voxels.vpl.
    // true generates a table from the embedded palette, overwriting shared
    // palette/lighting data; it is not the normal path or a VPL-load fallback.
    // Scope rule: implement only paths actually called by the selected runtime.
    // Unused branches are not implementation requirements merely for completeness.
    VoxLib(CCFileClass *Source, bool UseContainedPalette = false); // 755CD0

    ~VoxLib(); // 755D10

    // 755DB0: 1 on success, 0 on failure. Does not update Initialized.
    // Reads a VXL only; neither value of UseContainedPalette loads a VPL file.
    signed int ReadFile(CCFileClass *ccFile, bool UseContainedPalette);

    // return &this->HeaderData[headerIndex];
    VoxelSectionHeader * leaSectionHeader(int headerIndex); // 7564A0

    // return &this->TailerData[a3 + this->HeaderData[headerIndex].limb_number];
    VoxelSectionTailer * leaSectionTailer(int headerIndex, int a3); // 7564B0
};

struct TransformVector {
public:
    Vector3D<float> XYZ;
    float Unknown;
};

// file header
struct VoxFileHeader {
public:
    char filename[16];
    int PaletteCount;
    int countHeaders_OrSections1;
    int countTailers_OrSections2;
    int totalSize;
};

// internal representation of the next struct
struct VoxelSectionHeader {
    int limb_number;
    int unk1;
    char unk2;
};

// in file
struct VoxelSectionFileHeader {
public:
    char Name[16];
    VoxelSectionHeader headerData;
};

// in file
struct VoxelSectionFileTailer {
public:
    int span_start_off;
    int span_end_off;
    int span_data_off;
    float DetFloat;
    Matrix3D TransformationMatrix;
    Vector3D<float> MinBounds;
    Vector3D<float> MaxBounds;
    char size_X;
    char size_Y;
    char size_Z;
    char NormalsMode;
};

// internal representation
struct VoxelSectionTailer {
    // Historical names retained. 755DB0 relocates file offsets to pointers.
    std::int32_t* span_start_off;
    std::int32_t* span_end_off;
    std::uint8_t* span_data_off;
    float HVAMultiplier;
    Matrix3D TransformationMatrix;
    Vector3D<float> Bounds[8];
    char size_X;
    char size_Y;
    char size_Z;
    char NormalsMode;
};

static_assert(sizeof(VoxFileHeader) == 32);
static_assert(sizeof(VoxelSectionHeader) == 12);
static_assert(sizeof(VoxelSectionFileHeader) == 28);
static_assert(sizeof(VoxelSectionFileTailer) == 92);
static_assert(offsetof(VoxelSectionFileTailer, MinBounds) == 64);
static_assert(offsetof(VoxelSectionFileTailer, size_X) == 88);
#if UINTPTR_MAX == UINT32_MAX
static_assert(sizeof(VoxLib) == 28 && offsetof(VoxLib, HeaderData) == 16);
static_assert(offsetof(VoxLib, CountHeaders) == 4 && offsetof(VoxLib, CountTailers) == 8);
static_assert(offsetof(VoxLib, TotalSize) == 12);
static_assert(offsetof(VoxLib, TailerData) == 20 && offsetof(VoxLib, BodyData) == 24);
static_assert(sizeof(VoxelSectionTailer) == 164);
static_assert(offsetof(VoxelSectionTailer, HVAMultiplier) == 12);
static_assert(offsetof(VoxelSectionTailer, TransformationMatrix) == 16);
static_assert(offsetof(VoxelSectionTailer, Bounds) == 64);
static_assert(offsetof(VoxelSectionTailer, size_X) == 160);
#endif
