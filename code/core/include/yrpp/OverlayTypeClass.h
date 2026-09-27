/*
    OverlayTypes are initialized by INI files.
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ObjectTypeClass.h"
#include "api/type_drawing.hpp"

// forward declarations
class AnimTypeClass;

class OverlayTypeClass : public ObjectTypeClass
{
public:
    /// VA: 0x005FEA50; local body in core/src/yrpp.
    virtual void ComputeCRC(CRCEngine& crc) const override;
    static const AbstractType AbsID = AbstractType::OverlayType;

    // Array
    static DynamicVectorClass<OverlayTypeClass*>& Array;
    static OverlayTypeClass* YRPP_FASTCALL Find(const char* id);
    static int YRPP_FASTCALL FindIndex(const char* id);
    /// VA: 0x005FEC70; local owning factory.
    static OverlayTypeClass* YRPP_FASTCALL FindOrAllocate(const char* id);
    /// VA: 0x005FE620
    static void YRPP_FASTCALL LoadFromIniList(int theater) noexcept;

    // IPersist
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override;

    // IPersistStream
    /// VA: 0x005FEAF0; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) override { JMP_STD(0x005FEAF0); }
    /// VA: 0x005FEC10; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) override { JMP_STD(0x005FEC10); }

    // Destructor
    /// Implementation/provenance: matching core/src/yrpp source.
    virtual ~OverlayTypeClass();

    // AbstractClass
    virtual AbstractType WhatAmI() const override;
    virtual int Size() const override;
    virtual int GetArrayIndex() const override;

    // Local common + derived configuration path; dependencies are explicit.
    /// VA: 0x005FE770.
    virtual bool LoadFromINI(CCINIClass* ini) override;

    // ObjectTypeClass
    /// VA: 0x005FEA30; local body in core/src/yrpp.
    virtual CoordStruct* vt_entry_6C(CoordStruct* dest, CoordStruct* source) const override;


    /// VA: 0x005FE530; reference retained, not a local implementation.
    virtual bool SpawnAtMapCoords(CellStruct* pMapCoords,HouseClass* pOwner) override { JMP_THIS(0x005FE530); }
    /// VA: 0x005FE570; reference retained, not a local implementation.
    virtual ObjectClass* CreateObject(HouseClass* pOwner) override { JMP_THIS(0x005FE570); }

    // Original demand-loader override, backed by native scoped file services.
    /// VA: 0x005FEDE0.
    virtual SHPStruct* GetImage() const override;
    SHPStruct* GetImageOriginal() const { JMP_THIS(0x005FEDE0); }
// OverlayTypeClass
    /// VA: 0x005FDCC0
    static Point2D* YRPP_FASTCALL GetDrawOffset(Point2D* output, int overlayIndex);
    /// Implementation/provenance: matching core/src/yrpp source.
    virtual void Draw(Point2D* pClientCoords, RectangleStruct* pClipRect, int nFrame);
    game::DrawingStatus Draw(const game::TypeDrawingContext&, const Point2D&,
        const RectangleStruct&, int frame) noexcept;
    /// VA: 0x005FE5A0; explicit unresolved/original fallback.
    void DrawOriginal(Point2D* point, RectangleStruct* clip, int frame) { JMP_THIS(0x005FE5A0); }

    // Constructor
    /// VA: 0x005FE250.
    OverlayTypeClass(const char* id) noexcept;

protected:
    explicit __forceinline OverlayTypeClass(noinit_t) noexcept
        : ObjectTypeClass(noinit_t())
    { }

    // Properties

public:

    int                ArrayIndex;
    LandType           LandType;
    AnimTypeClass*     CellAnim;
    int                DamageLevels;
    int                Strength;
    bool               Wall;
    bool               Tiberium;
    bool               Crate;
    bool               CrateTrigger;
    bool               NoUseTileLandType;
    bool               IsVeinholeMonster;
    bool               IsVeins;
    bool               ImageLoaded;	//not INI
    bool               Explodes;
    bool               ChainReaction;
    bool               Overrides;
    bool               DrawFlat;
    bool               IsRubble;
    bool               IsARock;
    ColorStruct RadarColor;

};
