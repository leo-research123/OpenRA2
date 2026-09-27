#pragma once

// HVA file

class CCFileClass;
#include "yrpp/Matrix3D.h"
#include <cstddef>
#include <cstdint>

class MotLib {
public:
    bool LoadedFailed;
    int LayerCount;
    int FrameCount;
    Matrix3D* Matrixes;

    MotLib(CCFileClass* Source); // 5BD570

    ~MotLib(); // 5BD5A0

    // 5BD5C0: 1 on success, 0 on failure. Does not update LoadedFailed.
    signed int ReadFile(CCFileClass* ccFile);

    // 5BD730: scale translation only, preserving rotation and basis scale.
    void Scale(float scale);

    Matrix3D const& GetLayerMatrix(int layer, unsigned frame) const
    {
        // Original unchecked accessor: requires loaded matrices, FrameCount > 0,
        // and 0 <= layer < LayerCount. Frames wrap modulo FrameCount.
        return Matrixes[layer + LayerCount * (frame % FrameCount)];
    }
};

static_assert(sizeof(Matrix3D) == 48);
#if UINTPTR_MAX == UINT32_MAX
static_assert(sizeof(MotLib) == 16 && offsetof(MotLib, Matrixes) == 12);
static_assert(offsetof(MotLib, LayerCount) == 4 && offsetof(MotLib, FrameCount) == 8);
#endif
