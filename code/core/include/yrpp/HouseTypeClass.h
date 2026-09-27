/*
    ObjectTypes are initialized by INI files.
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractTypeClass.h"
#include "yrpp/Helpers/String.h"

class AircraftTypeClass;
class InfantryTypeClass;
class UnitTypeClass;

class HouseTypeClass : public AbstractTypeClass
{
public:
    /// VA: 0x00512570; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL GetSizeMax(ULARGE_INTEGER* pcbSize) override { JMP_STD(0x00512570); }
    /// VA: 0x00512170; reference retained, not a local implementation.
    virtual void ComputeCRC(CRCEngine& crc) const override { JMP_THIS(0x00512170); }
    /// VA: 0x00512730; reference retained, not a local implementation.
    virtual int GetArrayIndex() const override;
    /// VA: 0x511850
    bool LoadFromINI(CCINIClass* pINI) override;

    static const AbstractType AbsID = AbstractType::HouseType;
    static const int TempObserverID = -3;

    // Array
    static DynamicVectorClass<HouseTypeClass*>& Array;
    static HouseTypeClass* YRPP_FASTCALL Find(const char* id);
    static int YRPP_FASTCALL FindIndex(const char* id);

    // IPersist
    /// VA: 0x00512640.
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID);

    // IPersistStream
    /// VA: 0x00512290; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) { JMP_STD(0x00512290); }
    /// VA: 0x00512480; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm,BOOL fClearDirty) { JMP_STD(0x00512480); }

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~HouseTypeClass();

    // AbstractClass
    /// VA: 0x00512710.
    virtual AbstractType WhatAmI() const;
    /// VA: 0x00512720.
    virtual int	Size() const;

    // helpers
    HouseTypeClass* FindParentCountry() const {
        return HouseTypeClass::Find(this->ParentCountry);
    }

    int FindParentCountryIndex() const {
        return HouseTypeClass::FindIndexOfName(this->ParentCountry);
    }

    /// VA: 0x005117D0.
    static signed int YRPP_FASTCALL FindIndexOfName(const char *name);

    // Constructor
    /// VA: 0x005113F0.
    HouseTypeClass(const char* pID);

protected:
    explicit __forceinline HouseTypeClass(noinit_t) noexcept
        : AbstractTypeClass(noinit_t())
    { }

    // Properties

public:

    FixedString<25> ParentCountry;
    PROTECTED_PROPERTY(BYTE, align_B1[3]);
    int            ArrayIndex;
    int            ArrayIndex2; //dunno why
    int            SideIndex;
    int            ColorSchemeIndex;
    PROTECTED_PROPERTY(DWORD, align_C4);

    // are these unused TS leftovers?
    double         FirepowerMult;
    double         GroundspeedMult;
    double         AirspeedMult;
    double         ArmorMult;
    double         ROFMult;
    double         CostMult;
    double         BuildtimeMult;
    //---

    float          ArmorInfantryMult;
    float          ArmorUnitsMult;
    float          ArmorAircraftMult;
    float          ArmorBuildingsMult;
    float          ArmorDefensesMult;

    float          CostInfantryMult;
    float          CostUnitsMult;
    float          CostAircraftMult;
    float          CostBuildingsMult;
    float          CostDefensesMult;

    float          SpeedInfantryMult;
    float          SpeedUnitsMult;
    float          SpeedAircraftMult;

    float          BuildtimeInfantryMult;
    float          BuildtimeUnitsMult;
    float          BuildtimeAircraftMult;
    float          BuildtimeBuildingsMult;
    float          BuildtimeDefensesMult;

    float          IncomeMult;

    TypeList<InfantryTypeClass*> VeteranInfantry;
    TypeList<UnitTypeClass*> VeteranUnits;
    TypeList<AircraftTypeClass*> VeteranAircraft;

    char Suffix [4];

    char           Prefix;
    bool           Multiplay;
    bool           MultiplayPassive;
    bool           WallOwner;
    bool           SmartAI; //"smart"?
    PROTECTED_PROPERTY(BYTE, padding_1A9[7]);
};
