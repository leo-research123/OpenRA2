#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/GeneralDefinitions.h"

class TechnoClass;

class NOVTABLE WaveClass : public ObjectClass
{
public:
    static const AbstractType AbsID = AbstractType::Wave;

    /// Global VA: 0x00A8EC38.
    DEFINE_REFERENCE(DynamicVectorClass<WaveClass*>, Array, 0xA8EC38u)

    // IPersistStream
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm,BOOL fClearDirty) R0;

    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) R0;

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~WaveClass() RX;

    // AbstractClass
    /// VA: unknown (legacy placeholder).
    virtual AbstractType WhatAmI() const RT(AbstractType);
    /// VA: unknown (legacy placeholder).
    virtual int Size() const R0;

    /// VA: 0x00762070.
    void Draw_Magnetic(const CoordStruct& xyzFrom, const CoordStruct& xyzTo)
        { JMP_THIS(0x762070); }

    /// VA: 0x00761640.
    void Draw_NonMagnetic(const CoordStruct& xyzFrom, const CoordStruct& xyzTo)
        { JMP_THIS(0x761640); }

    /// VA: 0x00762AF0.
    void Update_Wave()
        { JMP_THIS(0x762AF0); }

    // ambient
    /// VA: 0x0075F330.
    void DamageArea(const CoordStruct& location) const
        { JMP_THIS(0x75F330); }

    // Constructor
    /// VA: 0x0075E950.
    WaveClass(
        const CoordStruct& From, const CoordStruct& To, TechnoClass *Owner,
        WaveType mode, AbstractClass *Target) : WaveClass(noinit_t())
    { JMP_THIS(0x75E950); }

protected:
    explicit __forceinline WaveClass(noinit_t)
        : ObjectClass(noinit_t())
    { }

    // Properties

public:
    AbstractClass* Target;
    WaveType Type;
    CoordStruct LimboCoords;
    CoordStruct Pos0;
    Point2D WaveStartMiddle;
    Point2D WaveEndMiddle;
    Point2D WaveEndSide1;
    Point2D WaveEndSide2;
    Point2D WaveStartSide1;
    Point2D WaveStartSide2;
    CoordStruct WaveEndSide1Coord;
    CoordStruct WaveEndSide2Coord;
    CoordStruct WaveStartSide1Coord;
    CoordStruct WaveStartSide2Coord;
    bool IsTraveling;
    bool ShouldEnd;
    BYTE field_12E;
    BYTE field_12F;
    int WaveEC; // for sonic/magna only
    int WaveCount;
    double MatrixScale1;
    double MatrixScale2;
    int PointData_Counter;
    DWORD PointData_Pointer;
    Point2D SonicPoints[6];
    Point2D MagPoints[4];
    int PointData2_X;
    int PointData2_Y;
    DWORD PointData2_Pointer;
    int PitchData[8];
    int FacingIndex;
    int LaserEC; // for lasers only, ctor = 160, per frame -= 6, 32 == dtor
    TechnoClass* Owner;
    FacingClass Facing;
    DynamicVectorClass<CellClass*> Cells;
    int ColorData[14];
};
