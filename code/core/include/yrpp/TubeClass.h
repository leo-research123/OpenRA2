#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractClass.h"

class NOVTABLE TubeClass : public AbstractClass
{
public:
    static const AbstractType AbsID = AbstractType::Tube;

    /// Global VA: 0x008B4138.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<TubeClass*>, Array, 0x8B4138u)
#else
    static DynamicVectorClass<TubeClass*>& Array;
#endif

    // IPersist
    /// VA: 0x007286D0
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override { JMP_STD(0x7286D0); }
#else
    HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override;
#endif

    // IPersistStream
    /// VA: 0x007281A0
    HRESULT YRPP_STDCALL Load(IStream* pStm) override { JMP_STD(0x7281A0); }
    /// VA: 0x007281E0
    HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) override { JMP_STD(0x7281E0); }

    // AbstractClass
    /// VA: 0x007286C0
    AbstractType WhatAmI() const override { return AbsID; }
    /// VA: 0x007286B0
    int Size() const override { return 0x1C4; }

    // Destructor
    /// VA: 0x00728710
#if defined(RA2_YRPP_GAME)
    virtual ~TubeClass() RX;
#else
    ~TubeClass() override;
#endif

    // Constructor
    /// VA: 0x00727FD0.
#if defined(RA2_YRPP_GAME)
    TubeClass(CellStruct* a2, int a3) noexcept
        : TubeClass(noinit_t())
    { JMP_THIS(0x727FD0); }
#else
    TubeClass(CellStruct* cell, int facing) noexcept;
#endif

protected:
    explicit __forceinline TubeClass(noinit_t) noexcept
        : AbstractClass(noinit_t())
    { }

public:
    CellStruct EnterCell;
    CellStruct ExitCell;
    int ExitFace;
    int Faces[100];
    int FaceCount;
};
