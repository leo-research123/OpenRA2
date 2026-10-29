/*
    Trees
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ObjectClass.h"
#include "yrpp/TerrainTypeClass.h"
#include "yrpp/StageClass.h"

#if defined(RA2_YRPP_GAME)
class NOVTABLE TerrainClass : public ObjectClass {
#else
class TerrainClass : public ObjectClass {
#endif
public:
    /// VA: 0x0071C110
#if defined(RA2_YRPP_GAME)
    void MarkAllOccupationBits(const CoordStruct& coords) override { JMP_THIS(0x71C110); }
#else
    void MarkAllOccupationBits(const CoordStruct& coords) override;
#endif
    /// VA: 0x0071C070
#if defined(RA2_YRPP_GAME)
    void UnmarkAllOccupationBits(const CoordStruct& coords) override { JMP_THIS(0x71C070); }
#else
    void UnmarkAllOccupationBits(const CoordStruct& coords) override;
#endif
    /// VA: 0x0071C4D0
#if defined(RA2_YRPP_GAME)
    Move IsCellOccupied(CellClass* destination, FacingType facing, int level, CellClass* source,
                        bool alt) const override { JMP_THIS(0x71C4D0); }
#else
    Move IsCellOccupied(CellClass* destination, FacingType facing, int level, CellClass* source,
                        bool alt) const override;
#endif
    /// VA: 0x0071D160
    RectangleStruct* GetRenderDimensions(RectangleStruct* output) override;
    /// VA: 0x0071D000
#if defined(RA2_YRPP_GAME)
    bool Unlimbo(const CoordStruct& where, DirType facing) override { JMP_THIS(0x0071D000); }
#else
    bool Unlimbo(const CoordStruct& where, DirType facing) override;
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
    // Reads the map's records and creates the objects. Malformed records are skipped;
    // allocation failures and unreadable data throw, and earlier placements remain.
    /// VA: 0x0071CA70
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL ReadINI(CCINIClass* pINI) { JMP_STD(0x71CA70); }
#else
    static void YRPP_FASTCALL ReadINI(CCINIClass* pINI);
#endif
    /// VA: 0x0071C1B0
    void DrawIt(Point2D* location, RectangleStruct* bounds) const override;

    /// VA: 0x71CF50
#if defined(RA2_YRPP_GAME)
    void ComputeCRC(CRCEngine& crc) const override { JMP_THIS(0x71CF50); }
#else
    void ComputeCRC(CRCEngine& crc) const override;
#endif
    /// VA: 0x71CC50
#if defined(RA2_YRPP_GAME)
    bool DrawIfVisible(RectangleStruct* bounds, bool forced, DWORD extrasOnly) const override { JMP_THIS(0x71CC50); }
#else
    bool DrawIfVisible(RectangleStruct* bounds, bool forced, DWORD extrasOnly) const override;
#endif
    /// VA: 0x71C360
#if defined(RA2_YRPP_GAME)
    void DrawAgain(const Point2D& point, const RectangleStruct& bounds) const override { JMP_THIS(0x71C360); }
#else
    void DrawAgain(const Point2D& point, const RectangleStruct& bounds) const override;
#endif
    /// VA: 0x71B920
#if defined(RA2_YRPP_GAME)
    DamageState ReceiveDamage(int* damage, int distance, WarheadTypeClass* warhead, ObjectClass* attacker, bool ignoreDefenses, bool preventEscape, HouseClass* house) override { JMP_THIS(0x71B920); }
#else
    DamageState ReceiveDamage(int* damage, int distance, WarheadTypeClass* warhead, ObjectClass* attacker, bool ignoreDefenses, bool preventEscape, HouseClass* house) override;
#endif
    /// VA: 0x71C5B0
#if defined(RA2_YRPP_GAME)
    bool Ignite() override { JMP_THIS(0x71C5B0); }
#else
    bool Ignite() override;
#endif
    /// VA: 0x71C6B0
#if defined(RA2_YRPP_GAME)
    void Extinguish() override { JMP_THIS(0x71C6B0); }
#else
    void Extinguish() override;
#endif
    /// VA: 0x71C8E0
#if defined(RA2_YRPP_GAME)
    void StartCrumbling() { JMP_THIS(0x71C8E0); }
#else
    void StartCrumbling();
#endif
    /// VA: 0x0071CB90
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL WriteINI(CCINIClass* ini) { JMP_STD(0x71CB90); }
#else
    static void YRPP_FASTCALL WriteINI(CCINIClass* ini);
#endif
    /// VA: 0x0071BDF0
#if defined(RA2_YRPP_GAME)
    TerrainClass() noexcept : TerrainClass(noinit_t()) { JMP_THIS(0x71BDF0); }
#else
    TerrainClass() noexcept;
#endif

    static const AbstractType AbsID = AbstractType::Terrain;

    // global array
    /// Global VA: 0x00A8E988.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<TerrainClass*>, Array, 0xA8E988u)
#else
    static DynamicVectorClass<TerrainClass*>& Array;
#endif

    // IPersist
    /// VA: 0x0071D310
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override;

    // IPersistStream
    /// VA: 0x0071CDA0
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL Load(IStream* stream) override { JMP_STD(0x71CDA0); }
#else
    HRESULT YRPP_STDCALL Load(IStream* stream) override;
#endif
    /// VA: 0x0071CF30
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL Save(IStream* stream, BOOL clearDirty) override { JMP_STD(0x71CF30); }
#else
    HRESULT YRPP_STDCALL Save(IStream* stream, BOOL clearDirty) override;
#endif

    // Destructor
    /// VA: 0x0071B7B0
#if defined(RA2_YRPP_GAME)
    virtual ~TerrainClass() { JMP_THIS(0x71B7B0); }
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
    /// VA: 0x0071D300
    virtual AbstractType WhatAmI() const override;
    /// VA: 0x0071D2F0
    virtual int Size() const override;
    /// VA: 0x0071D150
    virtual ObjectTypeClass* GetType() const override;

    /// VA: 0x0071C730
#if defined(RA2_YRPP_GAME)
    void Update() override { JMP_THIS(0x71C730); }
#else
    void Update() override;
#endif
    // Constructor, Destructor
    /// VA: 0x0071BB90
#if defined(RA2_YRPP_GAME)
    TerrainClass(TerrainTypeClass* tt, const CellStruct& coords) noexcept : TerrainClass(noinit_t()) { JMP_THIS(0x71BB90); }
#else
    TerrainClass(TerrainTypeClass* tt, const CellStruct& coords) noexcept;
#endif

protected:
    explicit __forceinline TerrainClass(noinit_t) noexcept : ObjectClass(noinit_t()) {}

    // Properties

public:
    StageClass Animation;
    TerrainTypeClass* Type;
    bool IsBurning;    // this terrain object has been ignited
    bool IsCrumbling;  // finish the animation and uninit
    RectangleStruct unknown_rect_D0;
};
