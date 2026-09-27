#pragma once

#include "yrpp/AbstractClass.h"

class BuildingClass;
class ObjectTypeClass;
class TerrainClass;

enum OverlayType : int;
enum SmudgeType : int;

// Deprecated memory-fog path: this project only preserves the original class layout/interfaces,
// using no-op YRpp implementations instead of porting the original logic.
// All 200 verified YR map entries (campaign, co-op, multiplayer and patches)
// use FogOfWar=no. R0/RX/RT are interface placeholders, not callable original behavior.
//  No native snapshot registries or lifecycle are implemented or scheduled for migration.
class NOVTABLE FoggedObjectClass : public AbstractClass
{
public:
    static const AbstractType AbsID = AbstractType::FoggedObject;

    // The nested name is also present in original RTTI at 0x008224A0.
    struct DrawRecord
    {
        ObjectTypeClass* TypeClass = nullptr; // 0x00
        int FrameNumber = 0;                 // 0x04
        BYTE HeightAdjust = 0;               // 0x08; followed by 0x03 padding
        int ZAdjust = 0;                     // 0x0C

        // Inlined in original VectorClass equality at 0x004D29B0:
        // height/depth adjustments do not participate in record identity.
        bool operator==(const DrawRecord& other) const noexcept {
            return TypeClass == other.TypeClass && FrameNumber == other.FrameNumber;
        }
        bool operator!=(const DrawRecord& other) const noexcept {
            return !(*this == other);
        }
    };

    /// Global VA: 0x008B3D10.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<FoggedObjectClass*>, Array, 0x008B3D10u)
#else
    static DynamicVectorClass<FoggedObjectClass*>& Array;
#endif
    /// Global VA: 0x008B3CC0.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE((IndexClass<int, FoggedObjectClass*>), FoggedObjectIndex, 0x008B3CC0u)
#else
    static IndexClass<int, FoggedObjectClass*>& FoggedObjectIndex;
#endif

    // IPersist / IPersistStream: stdcall, including this on the stack.
    /// VA: 0x004D27D0
    HRESULT YRPP_STDCALL GetClassID(CLSID* classID) override R0;
    /// VA: 0x004D2510
    HRESULT YRPP_STDCALL Load(IStream* stream) override R0;
    /// VA: 0x004D24A0
    HRESULT YRPP_STDCALL Save(IStream* stream, BOOL clearDirty) override R0;

    // Primary vtable 0x20 uses the compiler's deleting destructor at
    // 0x004D2910, which calls the following complete destructor.
    /// VA: 0x004D1650
    ~FoggedObjectClass() override RX;

    /// VA: 0x004D27B0
    AbstractType WhatAmI() const override RT(AbstractType);
    /// VA: 0x004D27C0
    int Size() const override R0;
    /// VA: 0x004D2810
    void ComputeCRC(CRCEngine& crc) const override RX;

    // Additional primary slot 0x60. YRpp's explicit return-buffer spelling
    // preserves the original thiscall Cell-by-value ABI: ECX=this, ret 0x04,
    // EAX=cell. This method is not AbstractClass::GetCoords (slot 0x48).
    /// VA: 0x004D28D0
    virtual CellStruct* GetCell(CellStruct* cell) R0;

    // Nonvirtual queries used by CellClass and target selection.
    /// VA: 0x004D2790
    const CellStruct* GetHeadRecordOccupyList() R0;
    /// VA: 0x004D28C0
    ObjectTypeClass* GetHeadRecordObjectType() R0;

    // Module-wide entry points grouped here; static functions add no vslots.
    // Original 0x004D1890 receives the clip reference in ECX (fastcall).
    /// VA: 0x004D1890
    static void YRPP_FASTCALL DrawAll(const RectangleStruct& clip) RX;
    /// VA: 0x004D2370
    static void YRPP_CDECL UpdateAll() RX;

    // All five ordinary constructors receive this in ECX. Overlay/smudge
    // each pop 0x0C bytes, building pops 0x08, terrain pops 0x04.
    /// VA: 0x004D08B0
    FoggedObjectClass() : AbstractClass(noinit_t()) RX;
    /// VA: 0x004D0980
    FoggedObjectClass(const CoordStruct& position, OverlayType type, int data)
        : AbstractClass(noinit_t()) RX;
    /// VA: 0x004D0C40
    FoggedObjectClass(const CoordStruct& position, SmudgeType type, int data)
        : AbstractClass(noinit_t()) RX;
    /// VA: 0x004D0EF0
    FoggedObjectClass(BuildingClass* object, bool canDraw)
        : AbstractClass(noinit_t()) RX;
    /// VA: 0x004D1370
    explicit FoggedObjectClass(TerrainClass* object) : AbstractClass(noinit_t()) RX;

    // Do not override PointerExpired, GetOwningHouse, GetCoords or Update:
    // the original table retains the AbstractClass implementations.
    // Field names describe recovered meaning; offsets below are Windows x86.
    OverlayType Overlay;                      // 0x24, -1 means none
    HouseClass* House;                        // 0x28
    int OverlayData;                          // 0x2C
    AbstractType RTTI;                        // 0x30, remembered object's kind
    CoordStruct Position;                     // 0x34
    RectangleStruct BoundingRect;             // 0x40, absolute tactical pixels
    int CellHeight;                           // 0x50
    SmudgeType Smudge;                         // 0x54, -1 means none
    int SmudgeData;                           // 0x58
    DynamicVectorClass<DrawRecord> Records;    // 0x5C, count at 0x6C
    bool CanDraw;                             // 0x74; followed by 0x03 padding
};
