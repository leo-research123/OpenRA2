#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ObjectClass.h"

// forward declarations
class IsometricTileTypeClass;

class NOVTABLE IsometricTileClass : public ObjectClass {
public:
    static const AbstractType AbsID = AbstractType::Isotile;

// Array
/// Global VA: 0x0087F750.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<IsometricTileClass*>, Array, 0x87F750u)
#else
    static DynamicVectorClass<IsometricTileClass*>& Array;
#endif

    // IPersist
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override;

    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) override R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) override R0;

    // AbstractClass
    virtual AbstractType WhatAmI() const override;
    virtual int Size() const override;

    // ObjectClass
    virtual ObjectTypeClass* GetType() const override;
    /// VA: 0x00543A40.
    virtual bool Limbo() override
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x543A40);
    }
#else
        ;
#endif
    /// VA: 0x00543A10.
    virtual bool Unlimbo(const CoordStruct& Crd, DirType dFaceDir) override
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x543A10);
    }
#else
        ;
#endif
    /// VA: 0x00543330.
    virtual bool Mark(MarkType value) override
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x543330);
    }
#else
        ;
#endif
    /// VA: unknown (legacy placeholder).
    virtual void DrawIt(Point2D* pLocation, RectangleStruct* pBounds) const override RX;

    // Destructor
    /// VA: 0x00543B10.
    virtual ~IsometricTileClass()
#if defined(RA2_YRPP_GAME)
    {
#if defined(_MSC_VER) && defined(_M_IX86)
        EPILOG_THISCALL;
        _asm { pop edx }
        _asm {push 0} _asm {push edx} JMP(0x543B10);
#else
        JMP_THIS(0x543B10);
#endif
    }
#else
        ;
#endif

    // Constructor
    /// VA: 0x00543780.
    IsometricTileClass(int idxType, CellStruct const& location) noexcept
#if defined(RA2_YRPP_GAME)
        : IsometricTileClass(noinit_t()) {
        JMP_THIS(0x543780);
    }
#else
        ;
#endif

protected:
    explicit __forceinline IsometricTileClass(noinit_t) noexcept : ObjectClass(noinit_t()) {}

    // Properties

public:
    IsometricTileTypeClass* Type;
};
