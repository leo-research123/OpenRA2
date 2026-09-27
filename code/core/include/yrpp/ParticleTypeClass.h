/*
    ParticleTypes are initialized by INI files.
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ObjectTypeClass.h"
#include "yrpp/Drawing.h"

// forward declarations
class WarheadTypeClass;
class RGBClass;

class ParticleTypeClass : public ObjectTypeClass
{
public:
    /// VA: 0x006458B0; reference retained, not a local implementation.
    virtual void PointerExpired(AbstractClass* pAbstract, bool removed) override;
    /// VA: 0x006454E0; reference retained, not a local implementation.
    virtual void ComputeCRC(CRCEngine& crc) const override { JMP_THIS(0x006454E0); }
    /// VA: 0x00644F50
#if defined(RA2_YRPP_GAME)
    virtual bool LoadFromINI(CCINIClass* pINI) override { JMP_THIS(0x00644F50); }
#else
    bool LoadFromINI(CCINIClass* ini) override;
#endif

    static const AbstractType AbsID = AbstractType::ParticleType;

    // Array
    static DynamicVectorClass<ParticleTypeClass*>& Array;
    static ParticleTypeClass* YRPP_FASTCALL Find(const char* id);
    static int YRPP_FASTCALL FindIndex(const char* id);

    // IPersist
    /// VA: 0x00645620.
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID);

    // IPersistStream
    /// VA: 0x00645660; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) { JMP_STD(0x00645660); }
    /// VA: 0x006457A0; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) { JMP_STD(0x006457A0); }

    // AbstractClass
    /// VA: 0x00645920.
    virtual AbstractType WhatAmI() const;
    /// VA: 0x00645910.
    virtual int Size() const;

    // ObjectTypeClass
    /// VA: 0x00645930; local body in core/src/yrpp.
    virtual bool SpawnAtMapCoords(CellStruct* mcoords, HouseClass* owner);
    /// VA: 0x00645940; local body in core/src/yrpp.
    virtual ObjectClass* CreateObject(HouseClass* owner);

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~ParticleTypeClass();

    // Constructor
    /// VA: 0x00644BE0.
    ParticleTypeClass(const char* pID);

protected:
    explicit __forceinline ParticleTypeClass(noinit_t) noexcept
        : ObjectTypeClass(noinit_t())
    { }

    // Properties

public:

    CoordStruct NextParticleOffset;
    int    XVelocity;
    int    YVelocity;
    int    MinZVelocity;
    int    ZVelocityRange;
    double ColorSpeed;
    TypeList<RGBClass> ColorList; // Original inline three-byte RGB values, not pointers.
    ColorStruct StartColor1;
    ColorStruct StartColor2;
    int    MaxDC;
    int    MaxEC;
    WarheadTypeClass* Warhead;
    int    Damage;
    int    StartFrame;
    int    NumLoopFrames;
    int    Translucency;
    int    WindEffect;
    float  Velocity;
    float  Deacc;
    int    Radius;
    bool   DeleteOnStateLimit;
    BYTE   EndStateAI;
    BYTE   StartStateAI;
    BYTE   StateAIAdvance;
    BYTE   FinalDamageState;
    BYTE   Translucent25State;
    BYTE   Translucent50State;
    bool   Normalized;
    int NextParticle; // ParticleTypeClass array index, -1 if not set.
    BehavesLike BehavesLike;

};
