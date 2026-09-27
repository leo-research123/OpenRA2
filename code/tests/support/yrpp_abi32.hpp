#pragma once

#include "yrpp/MixFileClass.h"
#include "yrpp/ArrayClasses.h"
#include "yrpp/CCFileClass.h"
#include "yrpp/AbstractTypeClass.h"
#include "yrpp/TargetClass.h"
#include "yrpp/SwizzleManagerClass.h"
#include "yrpp/FoggedObjectClass.h"
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace game {
struct FoggedObjectLayoutChecks {
    static_assert(!std::is_abstract_v<FoggedObjectClass>);
    static_assert(std::is_same_v<decltype(&FoggedObjectClass::GetClassID),
        HRESULT (YRPP_STDCALL FoggedObjectClass::*)(CLSID*)>);
    static_assert(std::is_same_v<decltype(&FoggedObjectClass::Load),
        HRESULT (YRPP_STDCALL FoggedObjectClass::*)(IStream*)>);
    static_assert(std::is_same_v<decltype(&FoggedObjectClass::Save),
        HRESULT (YRPP_STDCALL FoggedObjectClass::*)(IStream*, BOOL)>);
    static_assert(std::is_same_v<decltype(&FoggedObjectClass::GetCell),
        CellStruct* (YRPP_THISCALL FoggedObjectClass::*)(CellStruct*)>);
    // A field with the same meaning does not justify adding an override.
    static_assert(std::is_same_v<decltype(&FoggedObjectClass::GetOwningHouse),
        HouseClass* (YRPP_THISCALL AbstractClass::*)() const>);
    static_assert(std::is_same_v<decltype(&FoggedObjectClass::PointerExpired),
        void (YRPP_THISCALL AbstractClass::*)(AbstractClass*, bool)>);
    static_assert(std::is_same_v<decltype(&FoggedObjectClass::Update),
        void (YRPP_THISCALL AbstractClass::*)()>);
#if defined(_M_IX86) || defined(__i386__)
    static_assert(sizeof(OverlayType) == 0x04 && sizeof(SmudgeType) == 0x04);
    static_assert(sizeof(FoggedObjectClass) == 0x78 && alignof(FoggedObjectClass) == 0x04);
    static_assert(offsetof(FoggedObjectClass, Overlay) == 0x24);
    static_assert(offsetof(FoggedObjectClass, House) == 0x28);
    static_assert(offsetof(FoggedObjectClass, OverlayData) == 0x2C);
    static_assert(offsetof(FoggedObjectClass, RTTI) == 0x30);
    static_assert(offsetof(FoggedObjectClass, Position) == 0x34);
    static_assert(offsetof(FoggedObjectClass, BoundingRect) == 0x40);
    static_assert(offsetof(FoggedObjectClass, CellHeight) == 0x50);
    static_assert(offsetof(FoggedObjectClass, Smudge) == 0x54);
    static_assert(offsetof(FoggedObjectClass, SmudgeData) == 0x58);
    static_assert(offsetof(FoggedObjectClass, Records) == 0x5C);
    static_assert(offsetof(FoggedObjectClass, CanDraw) == 0x74);
    using DrawRecord = FoggedObjectClass::DrawRecord;
    using DrawRecords = DynamicVectorClass<DrawRecord>;
    static_assert(sizeof(DrawRecord) == 0x10 && alignof(DrawRecord) == 0x04);
    static_assert(offsetof(DrawRecord, TypeClass) == 0x00);
    static_assert(offsetof(DrawRecord, FrameNumber) == 0x04);
    static_assert(offsetof(DrawRecord, HeightAdjust) == 0x08);
    static_assert(offsetof(DrawRecord, ZAdjust) == 0x0C);
    static_assert(sizeof(DrawRecords) == 0x18);
    static_assert(offsetof(FoggedObjectClass, Records) + offsetof(DrawRecords, Items) == 0x60);
    static_assert(offsetof(FoggedObjectClass, Records) + offsetof(DrawRecords, Count) == 0x6C);
#endif
};
struct AbstractLayoutChecks {
    static_assert(!std::has_virtual_destructor_v<SwizzleManagerClass>);
#if defined(_M_IX86) || defined(__i386__)
    static_assert(sizeof(SwizzleManagerClass) == 0x34);
    static_assert(sizeof(SwizzlePointerClass) == 8);
    static_assert(offsetof(SwizzleManagerClass, Swizzles_Old) == 4);
    static_assert(offsetof(SwizzleManagerClass, Swizzles_New) == 0x1C);
    static_assert(sizeof(AbstractClass) == 0x24 && alignof(AbstractClass) == 4);
    static_assert(offsetof(AbstractClass, UniqueID) == 0x10);
    static_assert(offsetof(AbstractClass, AbstractFlags) == 0x14);
    static_assert(offsetof(AbstractClass, unknown_18) == 0x18);
    static_assert(offsetof(AbstractClass, RefCount) == 0x1C);
    static_assert(offsetof(AbstractClass, Dirty) == 0x20);
    static_assert(sizeof(AbstractTypeClass) == 0x98);
    static_assert(offsetof(AbstractTypeClass, ID) == 0x24);
    static_assert(offsetof(AbstractTypeClass, UINameLabel) == 0x3D);
    static_assert(offsetof(AbstractTypeClass, UIName) == 0x60);
    static_assert(offsetof(AbstractTypeClass, Name) == 0x64);
    static_assert(sizeof(IndexClass<int, AbstractClass*>) == 0x14);
#endif
    static_assert(sizeof(TargetClass) == 5 && alignof(TargetClass) == 1);
    static_assert(offsetof(TargetClass, m_RTTI) == 4);
};
// Check the migrated YRpp declarations themselves, with EA-derived methods.
struct RawFileLayoutChecks {
    static_assert(std::is_same_v<decltype(&RawFileClass::Exists),
        bool (RawFileClass::*)(bool)>);
    static_assert(std::is_same_v<decltype(&RawFileClass::HasHandle),
        bool (RawFileClass::*)()>);
#if defined(_M_IX86) || defined(__i386__)
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
    static_assert(sizeof(FileClass) == 8 && sizeof(RawFileClass) == 0x24);
    static_assert(alignof(RawFileClass) == 4);
    static_assert(offsetof(FileClass, SkipCDCheck) == 0x04);
    static_assert(offsetof(RawFileClass, FileAccess) == 0x08);
    static_assert(offsetof(RawFileClass, FilePointer) == 0x0c);
    static_assert(offsetof(RawFileClass, FileSize) == 0x10);
    static_assert(offsetof(RawFileClass, Handle) == 0x14);
    static_assert(offsetof(RawFileClass, FileName) == 0x18);
    static_assert(offsetof(RawFileClass, unknown_short_1C) == 0x1c);
    static_assert(offsetof(RawFileClass, unknown_short_1E) == 0x1e);
    static_assert(offsetof(RawFileClass, FileNameAllocated) == 0x20);
    static_assert(sizeof(RawFileClass::Handle) == 4 && sizeof(RawFileClass::FileName) == 4);
    static_assert(sizeof(RawFileClass::FileNameAllocated) == 1);
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#endif
};
}

static_assert(std::is_same_v<decltype(&MixFileClass::Offset),
    bool (YRPP_FASTCALL*)(const char*, void**, MixFileClass**, int*, int*)>);
static_assert(std::is_same_v<decltype(&MixFileClass::Bootstrap), bool (YRPP_CDECL*)()>);
static_assert(std::is_same_v<decltype(&FileClass::Open), bool (FileClass::*)(FileAccessMode)>);
static_assert(std::is_same_v<decltype(&FileClass::OpenEx), bool (FileClass::*)(const char*, FileAccessMode)>);

namespace game {
// The common declaration uses native pointers. Only x86 builds can expose it
// to the original game. These compilers support offsetof on this polymorphic
// layout as an extension. Microsoft x86 tests compile ordinary virtual dispatch
// using the Microsoft ABI; core-owned objects retain their compiler-generated tables.
#if defined(_M_IX86) || defined(__i386__)
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
// Derived probes expose protected names only for layout checks, never fields
// on the original public class. No instances or downcasts are used.
struct GenericNodeLayoutChecks : GenericNode {
    using GenericNode::NextNode;
    using GenericNode::PrevNode;
};
struct GenericListLayoutChecks : GenericList {
    using GenericList::FirstNode;
    using GenericList::LastNode;
};
static_assert(sizeof(GenericNodeLayoutChecks) == sizeof(GenericNode));
static_assert(sizeof(GenericListLayoutChecks) == sizeof(GenericList));
using MixArray = DynamicVectorClass<MixFileClass*>;
static_assert(sizeof(MixArray) == 0x18 && alignof(MixArray) == 4);
static_assert(offsetof(MixArray, Items) == 4 && offsetof(MixArray, Capacity) == 8);
static_assert(offsetof(MixArray, IsInitialized) == 0x0c && offsetof(MixArray, IsAllocated) == 0x0d);
static_assert(offsetof(MixArray, Count) == 0x10 && offsetof(MixArray, CapacityIncrement) == 0x14);
using GenericMixFiles = MixFileClass::GenericMixFiles;
static_assert(sizeof(GenericMixFiles) == 0x80);
static_assert(offsetof(GenericMixFiles, RA2MD) == 0 && offsetof(GenericMixFiles, RA2) == 4);
static_assert(offsetof(GenericMixFiles, LANGUAGE) == 8 && offsetof(GenericMixFiles, LANGMD) == 12);
static_assert(offsetof(GenericMixFiles, CACHEMD) == 80 && offsetof(GenericMixFiles, CACHE) == 84);
static_assert(offsetof(GenericMixFiles, LOCALMD) == 88 && offsetof(GenericMixFiles, LOCAL) == 92);
static_assert(sizeof(MemoryBuffer) == 12);
static_assert(sizeof(BufferIOFileClass) == 0x54);
static_assert(offsetof(BufferIOFileClass, IsAllocated) == 0x24);
static_assert(offsetof(BufferIOFileClass, IsOpen) == 0x25);
static_assert(offsetof(BufferIOFileClass, IsDiskOpen) == 0x26);
static_assert(offsetof(BufferIOFileClass, IsCached) == 0x27);
static_assert(offsetof(BufferIOFileClass, IsChanged) == 0x28);
static_assert(offsetof(BufferIOFileClass, UseBuffer) == 0x29);
static_assert(offsetof(BufferIOFileClass, BufferRights) == 0x2c);
static_assert(offsetof(BufferIOFileClass, IOBuffer) == 0x30);
static_assert(offsetof(BufferIOFileClass, BufferSize) == 0x34);
static_assert(offsetof(BufferIOFileClass, BufferPos) == 0x38);
static_assert(offsetof(BufferIOFileClass, BufferFilePos) == 0x3c);
static_assert(offsetof(BufferIOFileClass, BufferChangeBeg) == 0x40);
static_assert(offsetof(BufferIOFileClass, BufferChangeEnd) == 0x44);
static_assert(offsetof(BufferIOFileClass, CachedFileSize) == 0x48);
static_assert(offsetof(BufferIOFileClass, FilePos) == 0x4c);
static_assert(offsetof(BufferIOFileClass, TrueFileStart) == 0x50);
static_assert(sizeof(CDFileClass) == 0x58);
static_assert(offsetof(CDFileClass, IsDisabled) == 0x54);
static_assert(sizeof(CCFileClass) == 0x6c);
static_assert(offsetof(CCFileClass, Buffer) == 0x58);
static_assert(offsetof(CCFileClass, Position) == 0x64);
static_assert(offsetof(CCFileClass, Availablility) == 0x68);
static_assert(sizeof(MixFileClass) == 0x28);
static_assert(offsetof(GenericNodeLayoutChecks, NextNode) == 4);
static_assert(offsetof(GenericNodeLayoutChecks, PrevNode) == 8);
static_assert(offsetof(MixFileClass, FileName) == 0x0c);
static_assert(offsetof(MixFileClass, IsDigest) == 0x10);
static_assert(offsetof(MixFileClass, IsEncrypted) == 0x11);
static_assert(offsetof(MixFileClass, IsAllocated) == 0x12);
static_assert(offsetof(MixFileClass, CountFiles) == 0x14);
static_assert(offsetof(MixFileClass, FileSize) == 0x18);
static_assert(offsetof(MixFileClass, FileStartOffset) == 0x1c);
static_assert(offsetof(MixFileClass, Headers) == 0x20);
static_assert(offsetof(MixFileClass, Data) == 0x24);
static_assert(sizeof(List<MixFileClass>) == 28);
static_assert(offsetof(GenericListLayoutChecks, FirstNode) == 4);
static_assert(offsetof(GenericListLayoutChecks, LastNode) == 16);
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#endif
}
