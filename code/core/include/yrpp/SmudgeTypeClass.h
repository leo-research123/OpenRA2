/*
    SmudgeTypes are initialized by INI files.
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ObjectTypeClass.h"
#include "api/type_drawing.hpp"

class SmudgeTypeClass : public ObjectTypeClass
{
public:
    /// VA: 0x006B57F0; local body in core/src/yrpp.
    virtual void ComputeCRC(CRCEngine& crc) const override;
    static const AbstractType AbsID = AbstractType::SmudgeType;

    // Array
    static DynamicVectorClass<SmudgeTypeClass*>& Array;
    static SmudgeTypeClass* YRPP_FASTCALL Find(const char* id);
    static int YRPP_FASTCALL FindIndex(const char* id);
    /// VA: 0x006B5910; local owning factory.
    static SmudgeTypeClass* YRPP_FASTCALL FindOrAllocate(const char* id);

    // IPersist
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override;

    // IPersistStream
    /// VA: 0x006B5850; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) override { JMP_STD(0x006B5850); }
    /// VA: 0x006B58B0; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm,BOOL fClearDirty) override { JMP_STD(0x006B58B0); }

    // Destructor
    /// Implementation/provenance: matching core/src/yrpp source.
    virtual ~SmudgeTypeClass();

    // AbstractClass
    virtual AbstractType WhatAmI() const override;
    virtual int Size() const override;
    virtual int GetArrayIndex() const override;

    // Local common + derived configuration path; dependencies are explicit.
    /// VA: 0x006B56D0.
    virtual bool LoadFromINI(CCINIClass* ini) override;

    // ObjectTypeClass
    /// VA: 0x006B5550; reference retained, not a local implementation.
    virtual bool SpawnAtMapCoords(CellStruct* pMapCoords, HouseClass* pOwner) override { JMP_THIS(0x006B5550); }
    /// VA: 0x006B55C0; reference retained, not a local implementation.
    virtual ObjectClass* CreateObject(HouseClass* pOwner) override { JMP_THIS(0x006B55C0); }

    // SmudgeTypeClass
    /// VA: 0x006B55F0
    virtual void DrawIt(const Point2D& Point, const RectangleStruct& Rect, int SmudgeData, int Height, const CellStruct& MapCoords);
    game::DrawingStatus DrawIt(const game::TypeDrawingContext&, const Point2D&,
        const RectangleStruct&, int data, int height, const CellStruct&) noexcept;
    /// VA: 0x006B55F0; explicit unresolved/original fallback.
    void DrawItOriginal(const Point2D& point, const RectangleStruct& clip, int data,
        int height, const CellStruct& cell) { JMP_THIS(0x006B55F0); }

    // Original 0x6B5490: loads theater art for an already populated Array;
    // this method does not read INI sections or instantiate types.
    /// VA: 0x006B5490.
    static void YRPP_FASTCALL LoadFromIniList(int idxTheatre);

    /// VA: 0x006B5F80
    bool CanPlaceHere(const CellStruct& origin, bool underBuildings) const;
    /// VA: 0x006B6080
    void Place(const CellStruct& origin) const;
    /// VA: 0x006B59A0
    static bool ScorchTheGround(const CoordStruct& coordinate, int width, int height, bool large);
    /// VA: 0x006B5C90
    static bool CraterTheGround(const CoordStruct& coordinate, int width, int height, bool large);

    // Constructor
    /// VA: 0x006B5260.
    SmudgeTypeClass(const char* id) noexcept;

protected:
    explicit __forceinline SmudgeTypeClass(noinit_t) noexcept
        : ObjectTypeClass(noinit_t())
    { }

    // Properties

public:

    int ArrayIndex;
    int Width;
    int Height;
    bool Crater;
    bool Burn;
};
