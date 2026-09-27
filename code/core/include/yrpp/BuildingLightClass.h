#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/GeneralStructures.h"
#include "yrpp/ObjectClass.h"

class NOVTABLE BuildingLightClass : public ObjectClass
{
public:
    static const AbstractType AbsID = AbstractType::BuildingLight;

    // Static
    /// Global VA: 0x008B4190.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<BuildingLightClass*>, Array, 0x8B4190u)
#else
    static DynamicVectorClass<BuildingLightClass*>& Array;
#endif

    // IPersist
    /// VA: 0x00436910
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) R0;

    // IPersistStream
    /// VA: 0x004369C0
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm,BOOL fClearDirty) R0;

    // AbstractClass
    /// VA: 0x004370B0
    #if defined(RA2_YRPP_GAME)
    virtual AbstractType WhatAmI() const RT(AbstractType);
#else
    virtual AbstractType WhatAmI() const override;
#endif
    /// VA: 0x00436900
    #if defined(RA2_YRPP_GAME)
    virtual int	Size() const R0;
#else
    virtual int Size() const override;
#endif

    // Destructor
    /// VA: unknown (legacy placeholder).
    #if defined(RA2_YRPP_GAME)
    virtual ~BuildingLightClass() RX;
#else
    virtual ~BuildingLightClass();
#endif

#if defined(RA2_YRPP_GAME)
    // non-virtual
    /// VA: 0x00436BE0.
    void SetBehaviour(SpotlightBehaviour mode)
        { JMP_THIS(0x436BE0); }

    // Constructor
    /// VA: 0x00435820.
    BuildingLightClass(ObjectClass* pOwner) noexcept
        : BuildingLightClass(noinit_t())
    { JMP_THIS(0x435820); }

#else
    /// VA: 0x00436BE0
    void SetBehaviour(SpotlightBehaviour mode);
    /// VA: 0x00435820
    BuildingLightClass(ObjectClass* owner) noexcept;
    /// VA: 0x004361D0
    void Update() override;
    /// VA: 0x00435BE0
    void DrawIt(Point2D* location, RectangleStruct* bounds) const override;
    /// VA: 0x00436A00
    void PointerExpired(AbstractClass* object, bool removed) override;
    /// VA: 0x004369F0
    Layer InWhichLayer() const override;
    /// VA: 0x004369E0
    ObjectTypeClass* GetType() const override;
#endif

protected:
    explicit __forceinline BuildingLightClass(noinit_t) noexcept
        : ObjectClass(noinit_t())
    { }

    // Properties

public:

    double Speed;
    CoordStruct field_B8;
    CoordStruct field_C4;
    double Acceleration;
    bool Direction;
    SpotlightBehaviour BehaviourMode;
    ObjectClass * FollowingObject;
    TechnoClass * OwnerObject;
};
