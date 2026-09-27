/*
    Trees
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ObjectClass.h"
#include "yrpp/TerrainTypeClass.h"
#include "yrpp/StageClass.h"

class NOVTABLE TerrainClass : public ObjectClass
{
public:
    /// VA: 0x0071D160
    RectangleStruct* GetRenderDimensions(RectangleStruct* output) override;
    /// VA: 0x0071D000
#if defined(RA2_YRPP_GAME)
    bool Unlimbo(const CoordStruct& where,DirType facing) override { JMP_THIS(0x0071D000); }
#else
    bool Unlimbo(const CoordStruct& where,DirType facing) override;
#endif
    /// VA: 0x0071C930
#if defined(RA2_YRPP_GAME)
    bool Limbo() override { JMP_THIS(0x0071C930); }
#else
    bool Limbo() override;
#endif
    /// VA: 0x0071BFB0
#if defined(RA2_YRPP_GAME)
    bool Mark(MarkType value) override { JMP_THIS(0x0071BFB0); }
#else
    bool Mark(MarkType value) override;
#endif
    // Native map-reader status overload; no exceptions escape. Rejected records
    // are counted; a failure may retain earlier placements. Signature differs from the original entry.
    /// VA: 0x71CA70
    static bool ReadINI(CCINIClass& ini,unsigned int& rejectedRecords) noexcept;
    // Original entries needed by map display; references only, no new ABI slots.
    // Update: animation, crumbling and Tiberium spawning.
    /// VA: 0x0071C730.
    /// VA: 0x0071C1B0
    void DrawIt(Point2D* location, RectangleStruct* bounds) const override;

    static const AbstractType AbsID = AbstractType::Terrain;

    // global array
    /// Global VA: 0x00A8E988.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<TerrainClass*>, Array, 0xA8E988u)
#else
    static DynamicVectorClass<TerrainClass*>& Array;
#endif

    // IPersist
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override;

    // IPersistStream
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) override R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) override R0;

    // Destructor
    /// VA: unknown (legacy placeholder).
#if defined(RA2_YRPP_GAME)
    virtual ~TerrainClass() RX;
#else
    virtual ~TerrainClass();
#endif

    // identification
    /// VA: 0x0071CFD0
#if defined(RA2_YRPP_GAME)
    void PointerExpired(AbstractClass* object, bool removed) override { JMP_THIS(0x71CFD0); }
#else
    void PointerExpired(AbstractClass* object, bool removed) override;
#endif
    virtual AbstractType WhatAmI() const override;
    virtual int Size() const override;
    virtual ObjectTypeClass* GetType() const override;

#if !defined(RA2_YRPP_GAME)
    void Update() override;
#endif
    // Constructor, Destructor
    /// VA: 0x0071BB90.
#if defined(RA2_YRPP_GAME)
    TerrainClass(TerrainTypeClass* tt, CellStruct coords) noexcept
        : TerrainClass(noinit_t())
    { JMP_THIS(0x71BB90); }
#else
    TerrainClass(TerrainTypeClass* tt, CellStruct coords) noexcept;
#endif

protected:
    explicit __forceinline TerrainClass(noinit_t) noexcept
        : ObjectClass(noinit_t())
    { }

    // Properties

public:

    StageClass Animation;
    TerrainTypeClass* Type;
    bool IsBurning; // this terrain object has been ignited
    bool IsCrumbling; // finish the animation and uninit
    RectangleStruct unknown_rect_D0;

};
