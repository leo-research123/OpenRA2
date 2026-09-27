#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/Interfaces.h"
#include "yrpp/GeneralDefinitions.h"
#include "yrpp/GeneralStructures.h"
#include "yrpp/ArrayClasses.h"
#include "yrpp/IndexClass.h"
#include "yrpp/GameStrings.h"
#include "yrpp/YRMath.h"
#include "yrpp/Dir.h"

// forward declarations
class TechnoClass;
class HouseClass;
class CRCEngine;

struct StorageClass
{
    /// VA: 0x006C9680
    float GetAmount(int index) const;

    /// VA: 0x006C9650
    float GetTotalAmount() const;

    /// VA: 0x006C9690
    float AddAmount(float amount, int index);

    /// VA: 0x006C96B0
    float RemoveAmount(float amount, int index);

    /// VA: 0x006C9600
    int GetTotalValue() const;

    float Tiberium1;
    float Tiberium2;
    float Tiberium3;
    float Tiberium4;
};
//---

// The AbstractClass is the base class of all game objects.
class NOVTABLE AbstractClass : public IPersistStream, public IRTTITypeInfo, public INoticeSink, public INoticeSource
{
public:
    static const AbstractType AbsID = AbstractType::Abstract;
    // Original mixed listener vectors B0F670 and B0F658 (not type-only arrays).
    static DynamicVectorClass<AbstractClass*>& TypeExpirationListeners;
    static DynamicVectorClass<AbstractClass*>& TriggerExpirationListeners;
    void NotifyTriggerNodeExpired(bool removed = true);
    // Native object-list arm of AnnounceExpiredPointer. Other original
    // manager/UI arms remain separate until their consumers are migrated.
    void NotifyObjectExpired(bool removed = true);
    static DynamicVectorClass<AbstractClass*>& TriggerInstanceExpirationListeners; // B0F708
    static DynamicVectorClass<AbstractClass*>& TagExpirationListeners; // B0F618
    static DynamicVectorClass<AbstractClass*>& PendingDeletes; // B0F698


    /// Global VA: 0x00B0F720.
    static DynamicVectorClass<AbstractClass*>& Array;
    /// Global VA: 0x00B0E840.
    static IndexClass<int, AbstractClass*>& TargetIndex;

    const char* GetRTTIName() const;
    /// VA: 0x0040DCB0
    static const char* YRPP_FASTCALL GetRTTIName(AbstractType abs);

    // IUnknown
    /// VA: 0x00410260
    virtual HRESULT YRPP_STDCALL QueryInterface(REFIID iid, void** ppvObject);
    /// VA: 0x00410300
    virtual ULONG YRPP_STDCALL AddRef();
    /// VA: 0x00410310
    virtual ULONG YRPP_STDCALL Release();
    // IPersist
    /// VA: implementation-defined (pure virtual).
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) = 0;

    // IPersistStream
    /// VA: 0x00410450
    virtual HRESULT YRPP_STDCALL IsDirty();
    // Pure virtual slot, with a qualified base implementation used by derived classes.
    /// VA: 0x00410380
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) = 0;
    /// VA: 0x00410320
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) = 0;

    /// VA: 0x004103E0
    virtual HRESULT YRPP_STDCALL GetSizeMax(ULARGE_INTEGER* pcbSize);

    // IRTTITypeInfo
    /// VA: 0x00410210
    virtual AbstractType YRPP_STDCALL What_Am_I() const;
    /// VA: 0x00410220
    virtual int YRPP_STDCALL Fetch_ID() const;
    /// VA: 0x00410230
    virtual void YRPP_STDCALL Create_ID();


    // INoticeSink
    /// VA: 0x00410580
    virtual bool YRPP_STDCALL INoticeSink_Unknown(DWORD dwUnknown);

    // INoticeSource
    /// VA: 0x00410590
    virtual void YRPP_STDCALL INoticeSource_Unknown();

    // Destructor
    /// VA: 0x004101F0
    virtual ~AbstractClass();

    // AbstractClass
    /// VA: 0x00410470
    virtual void Init();
    /// VA: 0x00410480
    virtual void PointerExpired(AbstractClass* pAbstract, bool removed);
    /// VA: implementation-defined (pure virtual).
    virtual AbstractType WhatAmI() const = 0;
    /// VA: implementation-defined (pure virtual).
    virtual int Size() const = 0;
    /// VA: 0x00410410
    virtual void ComputeCRC(CRCEngine& crc) const;
    /// VA: 0x00410490
    virtual int GetOwningHouseIndex() const;
    /// VA: 0x004104A0
    virtual HouseClass* GetOwningHouse() const;
    /// VA: 0x004104B0
    virtual int GetArrayIndex() const;
    /// VA: 0x00410440
    virtual bool IsDead() const;
    /// VA: 0x004104C0
    virtual CoordStruct* GetCoords(CoordStruct* pCrd) const;
    /// VA: 0x004104F0
    virtual CoordStruct* GetDestination(CoordStruct* pCrd, TechnoClass* pDocker = nullptr) const;
// where this is moving, or a building's dock for a techno. iow, a rendez-vous point
    /// VA: 0x00410520
    virtual bool IsOnFloor() const;
    /// VA: 0x00410530
    virtual bool IsInAir() const;
    /// VA: 0x00410540
    virtual CoordStruct* GetCenterCoords(CoordStruct* pCrd) const;
    /// VA: 0x00410570
    virtual void Update();
// non-virtual
    /// VA: 0x007258D0
    static void YRPP_FASTCALL AnnounceExpiredPointer(AbstractClass* pAbstract, bool removed = true)
        { JMP_THIS(0x7258D0); }

    /// VA: 0x00725C70
    static void YRPP_FASTCALL RemoveAllInactive();

    void AnnounceExpiredPointer(bool removed = true);

    CoordStruct GetCoords() const;

    CoordStruct GetDestination(TechnoClass* pDocker = nullptr) const;

    CoordStruct GetCenterCoords() const;

    /// VA: 0x005F3DB0
    DirStruct* GetTargetDirection(DirStruct* pDir, AbstractClass* pTarget) const;

    DirStruct GetTargetDirection(AbstractClass* pTarget) const;

    /// VA: 0x005F6440
    int DistanceFrom(AbstractClass *that) const;

    /// VA: 0x005F6360
    int DistanceFrom3D(AbstractClass *that) const;

    /// VA: 0x0070D4A0
    void BecomeUntargetable();

    // Operators
    bool operator < (const AbstractClass &rhs) const;

    // Native base metadata initialization, original 0x410170.
    /// VA: 0x00410170
    AbstractClass() noexcept;

protected:
    explicit __forceinline AbstractClass(noinit_t) noexcept
    { }

    // Properties

public:

    DWORD UniqueID; // generated by IRTTIInfo::Create_ID through an amazingly simple sequence of return ++ScenarioClass::Instance->UniqueID;
    AbstractFlags AbstractFlags;	// flags, see AbstractFlags enum in GeneralDefinitions.
    DWORD unknown_18;
    LONG RefCount;
    bool Dirty;		// for IPersistStream.
    PROTECTED_PROPERTY(BYTE, padding_21[0x3]);
};
