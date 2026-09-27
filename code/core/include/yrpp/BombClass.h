#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractClass.h"
#include "yrpp/Audio.h"

// forward declarations
class ObjectClass;
class TechnoClass;
class HouseClass;

class NOVTABLE BombClass : public AbstractClass
{
public:
    static const AbstractType AbsID = AbstractType::Bomb;

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
    virtual ~BombClass() RX;

    // AbstractClass
    /// VA: unknown (legacy placeholder).
    virtual AbstractType WhatAmI() const RT(AbstractType);
    /// VA: unknown (legacy placeholder).
    virtual int	Size() const R0;

    /// VA: 0x00438720.
    void Detonate()
        { JMP_THIS(0x438720); }

    /// VA: 0x004389B0.
    void Disarm()
        { JMP_THIS(0x4389B0); }

    /// VA: 0x004389F0.
    BOOL IsDeathBomb() const
        { JMP_THIS(0x4389F0); }

    /// VA: 0x00438A00.
    int GetCurrentFlickerFrame() const // which frame of the ticking bomb to draw
        { JMP_THIS(0x438A00); }

    /// VA: 0x00438A70.
    bool TimeToExplode() const
        { JMP_THIS(0x438A70); }

    // Constructor
    // Bombs have a special constructor that just should not be called like this...
    // See BombListClass::Plant
    /// VA: 0x004385D0.
    BombClass() noexcept
        : AbstractClass(noinit_t())
    { JMP_THIS(0x4385D0); }

protected:
    explicit __forceinline BombClass(noinit_t) noexcept
        : AbstractClass(noinit_t())
    { }

    // Properties

public:

    TechnoClass* Owner;		//Most likely Ivan.
    HouseClass* OwnerHouse;
    ObjectClass* Target; // attaching to objects is possible, but it will never detonate
    BOOL DeathBomb; // unused - if so, [General]CanDetonateDeathBomb applies instead of CanDetonateTimeBomb
    int PlantingFrame;
    int DetonationFrame;
    AudioController Audio;
    int TickSound;
    BOOL ShouldPlayTickingSound; // seems so
    bool Harmless; // (mostly) set to 0 on plant, 1 on detonation/removal ?
};
