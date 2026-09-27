/*
    AbstractTypes are abstract template objects initialized by INI files
*/
#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractClass.h"
#include "yrpp/Memory.h"

// forward declarations
class CCINIClass;

// Macro for the static Array of every AbstractTypeClass!
#define ABSTRACTTYPE_ARRAY(class_name, address)	public:\
    DEFINE_REFERENCE(DynamicVectorClass<class_name*>, Array, address)\
    static __declspec(noinline) class_name* YRPP_FASTCALL Find(const char* pID)\
    {\
        for(auto pItem : Array)\
            if(!_strcmpi(pItem->ID, pID))\
                return pItem;\
        return nullptr;\
    }\
    static __declspec(noinline) int YRPP_FASTCALL FindIndex(const char* pID)\
    {\
        for(int i = 0; i < Array.Count; ++i)\
            if(!_strcmpi(Array[i]->ID, pID))\
                return i;\
        return -1;\
    }
//---

class NOVTABLE AbstractTypeClass : public AbstractClass
{
public:
    static const AbstractType AbsID = AbstractType::Abstract;

    // Static
    static DynamicVectorClass<AbstractTypeClass*>& Array;

    // Type arm of 7258D0: notify the actual type registry, then RulesClass.
    // An original host may additionally observe world-owned state. The observer
    // is borrowed, synchronous and must not throw or release this type.
    static void (*PointerExpirationObserver)(AbstractTypeClass*, bool) noexcept;
    void NotifyTypeExpired(bool removed = true);
    /// VA: 0x00410BE0
    virtual void ComputeCRC(CRCEngine& crc) const override;

    // Destructor
    /// VA: 0x004109C0
    virtual ~AbstractTypeClass();

    // AbstractTypeClass
    /// VA: 0x00410C20
    virtual void LoadTheaterSpecificArt(TheaterType th_type);
    /// VA: 0x00410A60
    virtual bool LoadFromINI(CCINIClass* pINI);
    /// VA: 0x00410B90
    virtual bool SaveToINI(CCINIClass* pINI);

    const char* get_ID() const;

    // Constructor
    /// VA: 0x00410800
    AbstractTypeClass(const char* pID) noexcept;

protected:
    // Target x86 base record fields; called by migrated derived stream methods.
    // No host vtable or native pointer bytes are copied to or from the record.
    bool EncodeTypeRecordBase(unsigned char* record, std::size_t size) const;
    bool DecodeTypeRecordBase(const unsigned char* record, std::size_t size);
    explicit __forceinline AbstractTypeClass(noinit_t) noexcept
        : AbstractClass(noinit_t())
    { }

    // Properties

public:

    char ID [0x18];
    PROTECTED_PROPERTY(BYTE, zero_3C);
    char UINameLabel [0x20];
    const wchar_t* UIName;
    char Name [0x31];
};
