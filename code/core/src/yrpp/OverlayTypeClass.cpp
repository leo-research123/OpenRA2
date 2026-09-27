// YRpp 9402d7da declarations, calibrated to gamemd 1.001.
#include "yrpp/OverlayTypeClass.h"
#include "type_resources.hpp"
#include <cstring>
#include <cstdio>
#include <cctype>

void YRPP_FASTCALL OverlayTypeClass::LoadFromIniList(int theater) noexcept {
    // Theater-list traversal follows the existing SmudgeTypeClass loader
    // (EA REDALERT/SDATA.CPP, f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae).
    // YR 0x005FE620 additionally handles NewTheater, generic fallback, and
    // invalidating owned demand-loaded images. Cached images stay cache-owned.
    if (theater < 0 || theater >= 6) {
        game::type_resource_result(game::TypeResourceStatus::invalid_argument); return;
    }
    try {
        for (auto* type : Array) {
            if (!type->Theater && !type->NewTheater) continue;
            if (type->ImageLoaded) {
                if (type->Image && type->ImageAllocated) {
                    YRMemory::Deallocate(type->Image);
                    type->Image = nullptr; type->ImageAllocated = false;
                }
                continue;
            }
            char filename[512]{};
            const auto* extension = type->Theater ? ::Theater::Array[theater].Extension : "shp";
            const int length = std::snprintf(filename, sizeof(filename), "%s.%s", type->ImageFile, extension);
            if (length < 2 || std::size_t(length) >= sizeof(filename)) {
                game::type_resource_result(game::TypeResourceStatus::invalid_argument); return;
            }
            if (!type->Theater) {
                const int first = std::tolower(static_cast<unsigned char>(filename[0]));
                const int second = std::tolower(static_cast<unsigned char>(filename[1]));
                if ((first=='g'||first=='n'||first=='c'||first=='y') && (second=='a'||second=='t'))
                    filename[1] = ::Theater::Array[theater].Letter[0];
            }
            type->Image = static_cast<SHPStruct*>(FileSystem::LoadFile(filename, true));
            if (!type->Image) {
                // 0x005F9710 replaces the second character unconditionally.
                filename[1] = 'G';
                type->Image = static_cast<SHPStruct*>(FileSystem::LoadFile(filename, true));
            }
        }
    } catch (...) { game::type_resource_result(game::TypeResourceStatus::failure); }
}

HRESULT YRPP_STDCALL OverlayTypeClass::GetClassID(CLSID* dest) {
    // Original 0x5fec30: E_POINTER leaves memory untouched.
    if (!dest) return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD id[4] = {0x5af2ce79u, 0x11d20634u, 0x6000a4acu, 0xb55b0508u};
    static_assert(sizeof(CLSID) == sizeof(id));
    std::memcpy(dest, id, sizeof(id));
    return 0;
}
AbstractType OverlayTypeClass::WhatAmI() const { return AbsID; } // 0x5fef00
int OverlayTypeClass::Size() const { return sizeof(*this); } // 0x5fef10
static_assert(static_cast<int>(OverlayTypeClass::AbsID) == 21);
#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(OverlayTypeClass) == 0x2bc);
#endif

int OverlayTypeClass::GetArrayIndex() const { return ArrayIndex; }

namespace { DynamicVectorClass<OverlayTypeClass*> types; }
DynamicVectorClass<OverlayTypeClass*>& OverlayTypeClass::Array = types;
OverlayTypeClass* YRPP_FASTCALL OverlayTypeClass::Find(const char* id) {
    const int index = FindIndex(id);
    return index < 0 ? nullptr : Array[index];
}
int YRPP_FASTCALL OverlayTypeClass::FindIndex(const char* id) {
    if (!id) return -1;
    for (int i = 0; i < Array.Count; ++i)
        if (_strcmpi(Array[i]->ID, id) == 0) return i;
    return -1;
}

// Supplied 5FE250; values mapped against existing x86 member offsets.
OverlayTypeClass::OverlayTypeClass(const char* id) noexcept : ObjectTypeClass(id), ArrayIndex(-1), LandType(static_cast<::LandType>(0)),
    CellAnim(nullptr), DamageLevels(1), Strength(1), Wall(false), Tiberium(false),
    Crate(false), CrateTrigger(false), NoUseTileLandType(true), IsVeinholeMonster(false),
    IsVeins(false), ImageLoaded(false), Explodes(false), ChainReaction(false),
    Overrides(false), DrawFlat(true), IsRubble(false), IsARock(false), RadarColor(0, 0, 0) {
    Create_ID();
    RadarInvisible = true; Selectable = false; Insignificant = true; AllowCellContent = false;
    Array.AddItem(this);
    ArrayIndex = Array.FindItemIndex(this);
}
// 5FEF30 contains the destructor body plus scalar-delete epilogue. The C++
// destructor implements only the body; the caller retains deletion ownership.
OverlayTypeClass::~OverlayTypeClass() {
    NotifyTypeExpired();
    Array.Remove(this);
}

#include "yrpp/CRC.h"
void OverlayTypeClass::ComputeCRC(CRCEngine& crc) const {
    AbstractTypeClass::ComputeCRC(crc);
    crc(ArrayIndex);
    crc(static_cast<int>(LandType));
    crc(DamageLevels);
    crc(Strength);
    crc(Wall);
    crc(Tiberium);
    crc(Crate);
    crc(CrateTrigger);
    crc(Explodes);
}

SHPStruct* OverlayTypeClass::GetImage() const {
    if (Image || !ImageLoaded) return Image;
    try {
        // Without either flag the target reads an uninitialized filename.
        // Native invalid input is reported instead of reproducing undefined reads.
        if (!Theater && !NewTheater) {
            game::type_resource_result(game::TypeResourceStatus::invalid_argument); return nullptr;
        }
        char filename[260];
        if (!game::type_image_filename(*this, filename, sizeof(filename), true)) return nullptr;
        SHPStruct* shape = nullptr;
        if (!game::load_owned_type_shape(filename, shape)) return nullptr;
        auto* self = const_cast<OverlayTypeClass*>(this);
        self->Image = shape;
        if (shape) self->ImageAllocated = true;
        return shape;
    } catch (...) { game::type_resource_result(game::TypeResourceStatus::failure); return nullptr; }
}

CoordStruct* OverlayTypeClass::vt_entry_6C(CoordStruct* dest, CoordStruct* source) const {
    *dest = *source; return dest;
}

#include <new>
// Supplied 005FEC70; independent native allocation size, same lookup/sentinel order.
OverlayTypeClass* YRPP_FASTCALL OverlayTypeClass::FindOrAllocate(const char* id) {
    if (!id || _strcmpi(id, "none") == 0 || _strcmpi(id, "<none>") == 0) return nullptr;
    if (auto* existing = Find(id)) return existing;
    void* storage = YRMemory::Allocate(sizeof(OverlayTypeClass));
    return storage ? new (storage) OverlayTypeClass(id) : nullptr;
}
