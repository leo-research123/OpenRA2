#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractClass.h"
#include "yrpp/Audio.h"

// forward declarations
class ObjectClass;
class TechnoClass;
class HouseClass;

class NOVTABLE BombClass : public AbstractClass {
public:
    static const AbstractType AbsID = AbstractType::Bomb;

    /// Global VA: 0x0089C668.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<BombClass*>, Array, 0x89C668u)
#else
    static DynamicVectorClass<BombClass*>& Array;
#endif

    // IPersist
    /// VA: 0x00438B00
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override { JMP_STD(0x438B00); }
#else
    HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override;
#endif

    // IPersistStream
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) R0;

    // Destructor
    /// VA: 0x00438670
#if defined(RA2_YRPP_GAME)
    virtual ~BombClass() RX;
#else
    ~BombClass() override;
#endif

    // AbstractClass
    /// VA: 0x004393E0
    AbstractType WhatAmI() const override { return AbsID; }
    /// VA: 0x004393D0
    int Size() const override { return sizeof(BombClass); }

    /// VA: 0x00438720
#if defined(RA2_YRPP_GAME)
    void Detonate() { JMP_THIS(0x438720); }
#else
    void Detonate();
#endif

    /// VA: 0x004389B0
#if defined(RA2_YRPP_GAME)
    void Disarm() { JMP_THIS(0x4389B0); }
#else
    void Disarm();
#endif

    /// VA: 0x004389F0.
    BOOL IsDeathBomb() const
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x4389F0);
    }
#else
    {
        return DeathBomb;
    }
#endif

    /// VA: 0x00438A00
#if defined(RA2_YRPP_GAME)
    int GetCurrentFlickerFrame() const { JMP_THIS(0x438A00); }
#else
    int GetCurrentFlickerFrame() const;
#endif

    /// VA: 0x00438A70
#if defined(RA2_YRPP_GAME)
    bool TimeToExplode() const { JMP_THIS(0x438A70); }
#else
    bool TimeToExplode() const;
#endif

    // Constructor
    // Bombs have a special constructor that just should not be called like this...
    // See BombListClass::Plant
    // Allocation/registration failure is fatal locally; exceptions cannot escape.
    /// VA: 0x004385D0
#if defined(RA2_YRPP_GAME)
    BombClass() noexcept : AbstractClass(noinit_t()) { JMP_THIS(0x4385D0); }
#else
    BombClass() noexcept;
#endif

protected:
    explicit __forceinline BombClass(noinit_t) noexcept : AbstractClass(noinit_t()) {}

    // Properties

public:
    TechnoClass* Owner;  //Most likely Ivan.
    HouseClass* OwnerHouse;
    ObjectClass* Target;  // attaching to objects is possible, but it will never detonate
    BOOL DeathBomb;       // unused - if so, [General]CanDetonateDeathBomb applies instead of CanDetonateTimeBomb
    int PlantingFrame;
    int DetonationFrame;
#if defined(RA2_YRPP_GAME)
    AudioController Audio;
#else
    // Preserve the original member layout and explicit destruction order.
    union {
        AudioController Audio;
    };
#endif
    int TickSound;
    BOOL ShouldPlayTickingSound;  // seems so
    bool Harmless;                // (mostly) set to 0 on plant, 1 on detonation/removal ?
};
