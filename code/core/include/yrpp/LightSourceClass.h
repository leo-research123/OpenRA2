/*
    LightSource - used for light posts and radiation
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractClass.h"

class NOVTABLE LightSourceClass : public AbstractClass
{
public:
    static const AbstractType AbsID = AbstractType::LightSource;
    /// Global VA: 0x00ABCA10.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<LightSourceClass*>, Array, 0xABCA10)
#else
    static DynamicVectorClass<LightSourceClass*>& Array;
#endif
    // IPersist
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) R0;

    // IPersistStream
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm,BOOL fClearDirty) R0;

    // Destructor
    /// VA: unknown (legacy placeholder).
    #if defined(RA2_YRPP_GAME)
    virtual ~LightSourceClass() RX;
#else
    virtual ~LightSourceClass();
#endif

    // AbstractClass
    /// VA: unknown (legacy placeholder).
    #if defined(RA2_YRPP_GAME)
    virtual AbstractType WhatAmI() const RT(AbstractType);
#else
    virtual AbstractType WhatAmI() const override;
#endif
    /// VA: unknown (legacy placeholder).
    #if defined(RA2_YRPP_GAME)
    virtual int Size() const R0;
#else
    virtual int Size() const override;
#endif

#if defined(RA2_YRPP_GAME)
    // non-virtual
    /// VA: 0x00554A60.
    void Activate(DWORD dwZero = 0)	//Start lighting
        { JMP_THIS(0x554A60); }

    /// VA: 0x00554A80.
    void Deactivate(DWORD dwZero = 0)	//Stop lighting
        { JMP_THIS(0x554A80); }

    /// VA: 0x00554AA0.
    void ChangeLevels(int nIntensity, TintStruct Tint, char mode)
        { JMP_THIS(0x554AA0); }

    /// VA: 0x00554D50.
    static void YRPP_FASTCALL UpdateLightConverts(int value, bool force = false)
        { JMP_STD(0x554D50); }

    // Constructor
    /// VA: 0x00554760.
    LightSourceClass(
        int X, int Y, int Z, int nVisibility, int nIntensity, int Red, int Green, int Blue) noexcept
        : LightSourceClass(noinit_t())
    { JMP_THIS(0x554760); }

    /// VA: 0x00554760.
    LightSourceClass(
        CoordStruct Crd, int nVisibility, int nIntensity, TintStruct Tint) noexcept
        : LightSourceClass(noinit_t())
    { JMP_THIS(0x554760); }

#else
    /// VA: 0x00554A60
    void Activate(DWORD deferred = 0);
    /// VA: 0x00554A80
    void Deactivate(DWORD deferred = 0);
    /// VA: 0x00554AA0
    void ChangeLevels(int intensity, TintStruct tint, char deferred);
    /// VA: 0x00554D50
    static void YRPP_FASTCALL UpdateLightConverts(int value, bool force = false);
    /// VA: 0x00554760
    LightSourceClass(int x,int y,int z,int visibility,int intensity,int red,int green,int blue) noexcept;
    /// VA: 0x00554760
    LightSourceClass(CoordStruct location,int visibility,int intensity,TintStruct tint) noexcept;
#endif

protected:
    explicit __forceinline LightSourceClass(noinit_t) noexcept
        : AbstractClass(noinit_t())
    { }

    // Properties

public:

    int LightIntensity;
    TintStruct LightTint;
    int DetailLevel;
    CoordStruct Location;
    int LightVisibility;
    bool Activated;
};
