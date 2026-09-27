// This can be used to load PCX files into BSurfaces!

#pragma once

#include "yrpp/BasicStructures.h"
#include <type_traits>
class BSurface;
class DSurface;

#include "yrpp/Helpers/CompileTime.h"

class PCX
{
public:
        // Load a PCX file
        bool ForceLoadFile(const char* pFileName, int flag1, int flag2);

public:
    static PCX& Instance; // Global VA: 0x00AC4848; optional software-render storage or EXE alias.

    static WORD const DefaultTransparentColor = 0xF81F;

    // Load a PCX file
    bool LoadFile(const char* pFileName, int flag1 = 2, int flag2 = 0);

    // Get a BSurface for a PCX file. File needs to be loaded some time first!
    BSurface* GetSurface(const char* pFileName, BytePalette * pPalette = nullptr);

    // Draws a PCX file
    bool BlitToSurface(RectangleStruct *BoundingRect, DSurface *TargetSurface, BSurface *PCXSurface, WORD TransparentColor = DefaultTransparentColor);

    PCX(); // original dictionary constructor 0x6B9450
    ~PCX(); // original surface-owning teardown 0x6B9530
    // Each instance owns its dictionary buckets and cached surfaces.
    PCX(const PCX&) = delete;
    PCX& operator=(const PCX&) = delete;

    // The original instance is a 0x38-byte Dictionary, not a single pointer.
    // Fields calibrated at 6B9450/6B9530; source: YRpp Dictionary.h.
    void *Buffer;
    DWORD Count;
    DWORD TableSize;
    DWORD TableBits;
    DWORD Log2Size;
    bool KeepSize;
    void* HashFunction;
    double ShrinkThreshold;
    double ExpandThreshold;
    int MinTableSize;
};

// Existing WWLib Read_PCX_File entry (YR 630310). Uses the optional software
// renderer; palette and output buffer are borrowed. Unchecked legacy format
// handling is retained; callers must provide a valid PCX, not untrusted input.
class FileClass;
BSurface* Read_PCX_File(FileClass* file, BytePalette* palette, void* buffer, unsigned int size);
