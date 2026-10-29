/*
    Overlays (mainly Ore and Gems)
*/

#pragma once

/*

;GEF in case it wasn't intuitively obvious from the above, this list mirrors an object enumeration
;in overlay.hh. If you want to add something to this list, make sure you add to the enumeration
;or get a programmer to do it for you

 * Ironically, the list they had in overlay.hh doesn't seem to have been updated that much since TS.

 */

#define OVERLAY_GASAND 0x00
#define OVERLAY_GAWALL 0x02
#define OVERLAY_NAWALL 0x1A

#define OVERLAY_VEINS 0x7E
#define OVERLAY_VEINHOLE 0xA7
#define OVERLAY_VEINHOLEDUMMY 0xB2

#define OVERLAY_BRIDGEHEAD11 0x18
#define OVERLAY_BRIDGEHEAD12 0x19

#define OVERLAY_BRIDGEHEAD21 0xED
#define OVERLAY_BRIDGEHEAD22 0xEE

#define OVERLAY_LOBRIDGE1 0x7A
#define OVERLAY_LOBRIDGE2 0x7B
#define OVERLAY_LOBRIDGE3 0x7C
#define OVERLAY_LOBRIDGE4 0x7D

#include "yrpp/platform/ABI.h"

#include "yrpp/ObjectClass.h"
#include "yrpp/OverlayTypeClass.h"

class NOVTABLE OverlayClass : public ObjectClass {
public:
    // Reads the packed [OverlayPack] and [OverlayDataPack] sections. Overlay
    // indexes without a type are skipped; an undecodable pack or an allocation
    // failure throws, and earlier placements remain.
    /// VA: 0x005FD2E0
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL ReadINI(CCINIClass* pINI) { JMP_STD(0x5FD2E0); }
#else
    static void YRPP_FASTCALL ReadINI(CCINIClass* pINI);
#endif

    static const AbstractType AbsID = AbstractType::Overlay;

    // Static
    /// Global VA: 0x00A8EC50.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<OverlayClass*>, Array, 0xA8EC50u)
#else
    static DynamicVectorClass<OverlayClass*>& Array;
#endif

    // Clears every vein cell and lays the full ones again, last first, so the
    // map's vein frames follow their connections.
    /// VA: 0x005FDDF0
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL RebuildVeins() { JMP_STD(0x5FDDF0); }
#else
    static void YRPP_FASTCALL RebuildVeins();
#endif

    // IPersist
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override;

    // IPersistStream
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) override R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) override R0;

    // Destructor
    /// VA: 0x005FDF70
#if defined(RA2_YRPP_GAME)
    virtual ~OverlayClass() RX;
#else
    ~OverlayClass() override;
#endif

    // AbstractClass
    virtual AbstractType WhatAmI() const override;
    virtual int Size() const override;
    virtual ObjectTypeClass* GetType() const override;

    /// VA: 0x005FD270
    bool Unlimbo(const CoordStruct& where, DirType facing) override
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x5FD270);
    }
#else
        ;
#endif
    /// VA: 0x005FC570
    bool Mark(MarkType mark) override
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x5FC570);
    }
#else
        ;
#endif

    // Gets overlay's tiberium type
    /// VA: 0x005FDD20.
    static int YRPP_FASTCALL GetTiberiumType(int overlayTypeIndex) { JMP_THIS(0x5FDD20); }

    // Constructor
    /// VA: 0x005FC380.
#if defined(RA2_YRPP_GAME)
    OverlayClass(OverlayTypeClass* pType, const CellStruct& mapCoord, int flag) noexcept : OverlayClass(noinit_t()) {
        JMP_THIS(0x5FC380);
    }
#else
    OverlayClass(OverlayTypeClass* pType, const CellStruct& mapCoord, int flag) noexcept;
#endif

    // Properties
protected:
    explicit OverlayClass(noinit_t) noexcept : ObjectClass(noinit_t()) {}

public:
    OverlayTypeClass* Type;
};
