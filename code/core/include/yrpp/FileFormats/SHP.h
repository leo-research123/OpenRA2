#pragma once

#include "yrpp/BasicStructures.h"

/*
*	SHP structs come in different forms: the plain file data, and a kind of
*	reference used for caching. Usually, it is not needed to know what type a
*	SHPStruct is of, because the the member functions work with both.
*/

struct SHPReference;
struct SHPFile;

// SHP file stuff
struct SHPStruct
{
    SHPStruct() : Type(0), Width(0), Height(0), Frames(0)
        {}

    ~SHPStruct();

    // Loads persistent, reference-owned data, if this is a reference.
    /// VA: 0x0069E090
    void Load();

    // unloads the data, if this is a reference
    void Unload();

    // Unloaded references resolve through one shared scratch buffer. Another
    // reference's GetData invalidates that view; Load first to retain data.
    /// VA: 0x0069E580
    SHPFile* GetData();

    RectangleStruct* GetFrameBounds(RectangleStruct &buffer, int idxFrame) const;

    RectangleStruct GetFrameBounds(int idxFrame) const {
        RectangleStruct buffer;
        return *GetFrameBounds(buffer, idxFrame);
    }

    ColorStruct* GetColor(ColorStruct &buffer, int idxFrame) const;

    ColorStruct GetColor(int idxFrame) const {
        ColorStruct buffer;
        return *GetColor(buffer, idxFrame);
    }

    /// VA: 0x0069E740
    BYTE* GetPixels(int idxFrame) const;

    // Flags & 2
    bool HasCompression(int idxFrame) const;

    bool IsReference() const {
        return Type == 0xFFFF;
    }

    SHPReference* AsReference();

    const SHPReference* AsReference() const;

    SHPFile* AsFile();

    const SHPFile* AsFile() const;

    WORD	Type;
    short	Width;
    short	Height;
    short	Frames;
};

struct SHPReference : public SHPStruct
{
    SHPReference(const char* filename);

    char*			Filename;
    SHPStruct*		Data;
    bool			Loaded;
    int				Index;
    // linked list of all SHPReferences
    SHPReference*	Next;
    SHPReference*	Prev;
    DWORD			unknown_20;
};

struct SHPFrame
{
    short		Left;
    short		Top;
    short		Width;
    short		Height;
    DWORD		Flags;
    ColorStruct	Color;
    DWORD		unknown_10;
    int			Offset;
};

struct SHPFile : public SHPStruct
{
    const SHPFrame& GetFrameHeader(int idxFrame) const {
        return (&FirstFrame)[idxFrame];
    }

    SHPFrame	FirstFrame;
};

inline SHPReference* SHPStruct::AsReference() {
    return IsReference() ? static_cast<SHPReference*>(this) : nullptr;
}

inline const SHPReference* SHPStruct::AsReference() const {
    return IsReference() ? static_cast<const SHPReference*>(this) : nullptr;
}

inline SHPFile* SHPStruct::AsFile() {
    return !IsReference() ? static_cast<SHPFile*>(this) : nullptr;
}

inline const SHPFile* SHPStruct::AsFile() const {
    return !IsReference() ? static_cast<const SHPFile*>(this) : nullptr;
}

//=== GLOBAL LINKED LIST OF ALL LOADED SHP FILES
// defined but not used
// static SHPStruct* SHPStruct_first=(SHPStruct*)0xB077B0;
//==============================================

// Original SHP module lifecycle operations; compat keeps the existing entry mapping.
// Host shutdown: stop/join all users, unload shapes, then close files.
// Unload keeps caller-owned references (including stack references) alive.
void Initialize_Shape_Bounds() noexcept;
void Destroy_All_Shapes();
void Unload_All_Shapes() noexcept;
void Unload_Shapes_Through(unsigned int threshold) noexcept;
