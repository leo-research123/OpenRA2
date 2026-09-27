/*
    AnimTypes are initialized by INI files.
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/ObjectTypeClass.h"

// forward declarations
class OverlayTypeClass;
class ParticleTypeClass;
class WarheadTypeClass;

class AnimTypeClass : public ObjectTypeClass
{
public:
    /// VA: 0x00428800; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) override { JMP_STD(0x00428800); }
    /// VA: 0x00428970; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) override { JMP_STD(0x00428970); }
    /// VA: 0x00428C10; reference retained, not a local implementation.
    virtual void PointerExpired(AbstractClass* pAbstract, bool removed) override;
    /// VA: 0x004289D0; reference retained, not a local implementation.
    virtual void ComputeCRC(CRCEngine& crc) const override { JMP_THIS(0x004289D0); }
    /// VA: 0x00428E60; reference retained, not a local implementation.
    virtual int GetArrayIndex() const override;
    /// VA: 0x00427D00; reference retained, not a local implementation.
#if defined(RA2_YRPP_GAME)
    virtual bool LoadFromINI(CCINIClass* pINI) override { JMP_THIS(0x00427D00); }
#else
    virtual bool LoadFromINI(CCINIClass*) override;
#endif

    static const AbstractType AbsID = AbstractType::AnimType;

    // Array
    // Existing type objects are owned by their original type lifecycle.
    static DynamicVectorClass<AnimTypeClass*>& Array;
    static AnimTypeClass* YRPP_FASTCALL Find(const char* id);
    static int YRPP_FASTCALL FindIndex(const char* id);
    /// VA: 0x00428B80.
    static AnimTypeClass* YRPP_FASTCALL FindOrAllocate(const char* id);
    // IPersist
    /// VA: 0x00428990.
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID);

    // AbstractClass
    /// VA: 0x00428E50.
    virtual AbstractType WhatAmI() const;
    /// VA: 0x00428E70.
    virtual int	Size() const;

    // ObjectTypeClass
    /// VA: 0x00428E80; local body in core/src/yrpp.
    virtual bool SpawnAtMapCoords(CellStruct* pMapCoords,HouseClass* pOwner);
    /// VA: 0x00428E90; local body in core/src/yrpp.
    virtual ObjectClass* CreateObject(HouseClass* owner); // ! this just returns NULL instead of creating the anim, fucking slackers

    // Actual vtable slot 156 is GetImage, not a new LoadImage slot.
    /// VA: 0x00428C30; demand-load/resource dependencies remain original.
#if defined(RA2_YRPP_GAME)
    virtual SHPStruct* GetImage() const override { JMP_THIS(0x00428C30); }
#else
    virtual SHPStruct* GetImage() const override;
#endif
    // Source compatibility helper only; it must not occupy a virtual slot.
    SHPStruct* LoadImage() { return GetImage(); }
    // Actual slot 160 takes one four-byte theater argument (427B50: retn 4).
    // The prior no-argument placeholder had a mismatched original ABI.
    /// VA: 0x00427B50; art/filename/frame dependencies remain original.
#if defined(RA2_YRPP_GAME)
    virtual void Load2DArt(TheaterType theater) { JMP_THIS(0x00427B50); }
#else
    virtual void Load2DArt(TheaterType theater);
#endif

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~AnimTypeClass();

    // Constructor
    /// VA: 0x00427530.
    AnimTypeClass(const char* pID);

protected:
    explicit __forceinline AnimTypeClass(noinit_t) noexcept
        : ObjectTypeClass(noinit_t())
    { }

    // Properties

public:

    int ArrayIndex;
    int MiddleFrameIndex;
    int MiddleFrameWidth;
    int MiddleFrameHeight;
    BYTE unknown_2A4;
    double Damage;
    int Rate;
    int Start;
    int LoopStart;
    int LoopEnd;
    int End;
    int LoopCount;
    AnimTypeClass* Next;
    int SpawnsParticle; // index of that ParticleTypeClass
    int NumParticles;
    int DetailLevel;
    int TranslucencyDetailLevel;
    RandomStruct RandomLoopDelay;
    RandomStruct RandomRate;
    int Translucency;
    AnimTypeClass* Spawns;
    int SpawnCount;
    int Report;		//VocClass index
    int StopSound;		//VocClass index
    AnimTypeClass* BounceAnim;
    AnimTypeClass* ExpireAnim;
    AnimTypeClass* TrailerAnim;
    int TrailerSeperation;	//MISTYPE BY WESTWOOD!
    double Elasticity;
    double MinZVel;
    double unknown_double_320;
    double MaxXYVel;
    WarheadTypeClass* Warhead;
    int DamageRadius;
    OverlayTypeClass* TiberiumSpawnType;
    int TiberiumSpreadRadius;
    int YSortAdjust;
    int YDrawOffset;
    int ZAdjust;
    int MakeInfantry;
    int RunningFrames;
    bool IsFlamingGuy;
    bool IsVeins;
    bool IsMeteor;
    bool TiberiumChainReaction;
    bool IsTiberium;
    bool HideIfNoOre;
    bool Bouncer;
    bool Tiled;
    bool ShouldUseCellDrawer;
    bool UseNormalLight;
    bool DemandLoad; // not loaded from ini anymore
    bool FreeLoad;  // not loaded from ini anymore
    bool IsAnimatedTiberium;
    bool AltPalette;
    bool Normalized;
    Layer Layer;
    bool DoubleThick;
    bool Flat;
    bool Translucent;
    bool Scorch;
    bool Flamer;
    bool Crater;
    bool ForceBigCraters;
    bool Sticky;
    bool PingPong;
    bool Reverse;
    bool Shadow;
    bool PsiWarning;
    bool ShouldFogRemove;
};
