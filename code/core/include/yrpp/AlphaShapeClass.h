/*
    RadSites
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/FileSystem.h"
#include "yrpp/AbstractClass.h"

class ObjectClass;

class NOVTABLE AlphaShapeClass : public AbstractClass
{
public:
    static const AbstractType AbsID = AbstractType::AlphaShape;

    // Static
    /// Global VA: 0x0088A0F0.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<AlphaShapeClass*>, Array, 0x88A0F0u)
#else
    static DynamicVectorClass<AlphaShapeClass*>& Array;
#endif
    /// VA: 0x00421350
    static void YRPP_FASTCALL DrawAll(const RectangleStruct& clip);
    /// VA: 0x00420E90
    static void YRPP_CDECL UpdateAll();

    // IPersist
    /// VA: 0x00420D40
    HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override;

    // IPersistStream
    /// VA: 0x00420DE0
    HRESULT YRPP_STDCALL Load(IStream* pStm) override;
    /// VA: 0x00420E40
    HRESULT YRPP_STDCALL Save(IStream* pStm,BOOL fClearDirty) override;

    // Destructor
    /// VA: 0x00420C80
    ~AlphaShapeClass() override;

    // AbstractClass
    /// VA: 0x00420D80
    AbstractType WhatAmI() const override;
    /// VA: 0x00420D90
    int Size() const override;
    /// VA: 0x00420DA0
    void ComputeCRC(CRCEngine& crc) const override;
    /// VA: 0x00420E70
    void PointerExpired(AbstractClass* object, bool removed) override;

    // Constructor
    /// VA: 0x00420960
    AlphaShapeClass(ObjectClass* pObj, int nX, int nY) noexcept;
    /// VA: 0x00420AF0
    AlphaShapeClass() noexcept;

protected:
    /// VA: 0x00420C50
    explicit __forceinline AlphaShapeClass(noinit_t) noexcept
        : AbstractClass(noinit_t())
    { }

    // Properties

public:

    ObjectClass* AttachedTo;	//To which object is this AlphaShape attached?
    RectangleStruct Rect;
    SHPStruct* AlphaImage;
    bool IsObjectGone; // Owner expiration marks deletion; AttachedTo is retained until UpdateAll.
};
