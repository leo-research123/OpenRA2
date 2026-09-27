#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ObjectClass.h"

// forward declarations
class IsometricTileTypeClass;

class NOVTABLE IsometricTileClass : public ObjectClass
{
public:
    static const AbstractType AbsID = AbstractType::Isotile;

    // Array
    /// Global VA: 0x0087F750.
    DEFINE_REFERENCE(DynamicVectorClass<IsometricTileClass*>, Array, 0x87F750u)

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
    /// VA: unknown (legacy placeholder).
    virtual bool Limbo() override R0;
    /// VA: unknown (legacy placeholder).
    virtual bool Unlimbo(const CoordStruct& Crd, DirType dFaceDir) override R0;
    /// VA: unknown (legacy placeholder).
    virtual void DrawIt(Point2D* pLocation, RectangleStruct* pBounds) const override RX;

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~IsometricTileClass() RX;

    // Constructor
    /// VA: 0x00543780.
    IsometricTileClass(int idxType, CellStruct const& location) noexcept
        : IsometricTileClass(noinit_t())
    { JMP_THIS(0x543780); }

protected:
    explicit __forceinline IsometricTileClass(noinit_t) noexcept
        : ObjectClass(noinit_t())
    { }

    // Properties

public:
    IsometricTileTypeClass* Type;
};
