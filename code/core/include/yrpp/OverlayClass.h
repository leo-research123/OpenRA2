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

class NOVTABLE OverlayClass : public ObjectClass
{
public:
    // Native packed-map reader; resets rejection count and contains exceptions.
    /// VA: 0x005FD2E0
    static bool ReadINI(CCINIClass& ini,unsigned int& rejectedRecords) noexcept;

    static const AbstractType AbsID = AbstractType::Overlay;

    // Static
    /// Global VA: 0x00A8EC50.
    DEFINE_REFERENCE(DynamicVectorClass<OverlayClass*>, Array, 0xA8EC50u)

    // IPersist
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override;

    // IPersistStream
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) override R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) override R0;

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~OverlayClass() RX;

    // AbstractClass
    virtual AbstractType WhatAmI() const override;
    virtual int Size() const override;
    virtual ObjectTypeClass* GetType() const override;

    // Gets overlay's tiberium type
    /// VA: 0x005FDD20.
    static int YRPP_FASTCALL GetTiberiumType(int overlayTypeIndex)
        { JMP_THIS(0x5FDD20); }

    // Constructor
    /// VA: 0x005FC380.
    OverlayClass(OverlayTypeClass* pType, const CellStruct& mapCoord, int flag) noexcept : OverlayClass(noinit_t())
        { JMP_THIS(0x5FC380); }

    // Properties
protected:
    explicit OverlayClass(noinit_t) noexcept : ObjectClass(noinit_t())
        {}

public:

    OverlayTypeClass* Type;
};
