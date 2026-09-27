/*
    [AITriggerTypes]
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/YRPPCore.h"
#include <cstdio>
#include "yrpp/HouseTypeClass.h"
#include "yrpp/TeamTypeClass.h"
#include "yrpp/TechnoTypeClass.h"
#include "yrpp/AbstractTypeClass.h"

// forward declarations
class TechnoTypeClass;
class TeamTypeClass;

enum class AITriggerConditionComparatorType : unsigned int
{
    Less = 0,
    LessOrEqual = 1,
    Equal = 2,
    GreaterOrEqual = 3,
    Greater = 4,
    NotEqual = 5
};

struct AITriggerConditionComparator
{
    int ComparatorOperand;
    AITriggerConditionComparatorType ComparatorType;
};

class AITriggerTypeClass : public AbstractTypeClass
{
public:
    /// VA: 0x0041E5E0; reference retained, not a local implementation.
    virtual void ComputeCRC(CRCEngine& crc) const override { JMP_THIS(0x0041E5E0); }
    /// VA: 0x0041F580
    virtual bool LoadFromINI(CCINIClass* pINI) override;
    /// VA: 0x0041FB10; reference retained, not a local implementation.
    virtual bool SaveToINI(CCINIClass* pINI) override { JMP_THIS(0x0041FB10); }

    static const AbstractType AbsID = AbstractType::AITriggerType;

    // Array
    static DynamicVectorClass<AITriggerTypeClass*>& Array;
    static AITriggerTypeClass* YRPP_FASTCALL Find(const char* id);
    static int YRPP_FASTCALL FindIndex(const char* id);

    // IPersist
    /// VA: 0x0041E500.
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID);

    // IPersistStream
    /// VA: 0x0041E540; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) { JMP_STD(0x0041E540); }
    /// VA: 0x0041E5C0; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) { JMP_STD(0x0041E5C0); }

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~AITriggerTypeClass();

    // AbstractClass
    /// VA: 0x0041FFD0.
    virtual AbstractType WhatAmI() const;
    /// VA: 0x0041FFE0.
    virtual int Size() const;

    /// VA: 0x0041F2E0.
    static void YRPP_FASTCALL LoadFromINIList(CCINIClass* pINI, int scope) noexcept;

    // Global == saving into AI.ini as opposed to map?
    // if !Global, [AITriggerTypesEnable] gets saved as well
    /// VA: 0x0041F490.
    static bool SaveToINIList(CCINIClass *pINI, bool Global)
        { JMP_STD(0x41F490); }

    // non-virtual

    // teams finished script, and
    /// VA: 0x0041FD60.
    void RegisterSuccess()
        { JMP_THIS(0x41FD60); }

    /// VA: 0x0041FE20.
    void RegisterFailure()
        { JMP_THIS(0x41FE20); }

    // the main condition
    /// VA: 0x0041E720.
    bool ConditionMet(HouseClass *CallingHouse, HouseClass *TargetHouse, bool EnoughBaseDefense) const
        { JMP_THIS(0x41E720); }

    // slaves
    /// VA: 0x0041EE90.
    bool OwnerHouseOwns(HouseClass *CallingHouse, HouseClass *TargetHouse) const
        { JMP_THIS(0x41EE90); }

    /// VA: 0x0041EC90.
    bool CivilianHouseOwns(HouseClass *CallingHouse, HouseClass *TargetHouse) const
        { JMP_THIS(0x41EC90); }

    /// VA: 0x0041EAF0.
    bool EnemyHouseOwns(HouseClass *CallingHouse, HouseClass *TargetHouse) const
        { JMP_THIS(0x41EAF0); }

    /// VA: 0x0041F0D0.
    bool IronCurtainCharged(HouseClass *CallingHouse, HouseClass *TargetHouse) const
        { JMP_THIS(0x41F0D0); }

    /// VA: 0x0041F180.
    bool ChronoSphereCharged(HouseClass *CallingHouse, HouseClass *TargetHouse) const
        { JMP_THIS(0x41F180); }

    /// VA: 0x0041F230.
    bool HouseCredits(HouseClass *CallingHouse, HouseClass *TargetHouse) const
        { JMP_THIS(0x41F230); }

    void FormatForSaving(char * buffer, size_t size) const {
        const char *Team1Name = "<none>";
        const char *Team2Name = "<none>";
        const char *HouseName = "<none>";
        const char *ConditionName = "<none>";

        TeamTypeClass *T = this->Team1;
        if(T) {
            Team1Name = T->get_ID();
        }
        T = this->Team2;
        if(T) {
            Team2Name = T->get_ID();
        }

        if(this->OwnerHouseType == AITriggerHouseType::Single) {
            auto const idxHouse = this->HouseIndex;
            if(idxHouse != -1) {
                HouseName = HouseTypeClass::Array.GetItem(idxHouse)->get_ID();
            }
        } else if(this->OwnerHouseType == AITriggerHouseType::Any) {
            HouseName = "<all>";
        }

        TechnoTypeClass *O = this->ConditionObject;
        if(O) {
            ConditionName = O->get_ID();
        }

        char ConditionString[68];
        int idx = 0;
        char * condStr = ConditionString;
        auto buf = reinterpret_cast<const byte*>(&this->Conditions);
        do {
            std::snprintf(condStr, 4, "%02x", *buf);
            ++buf;
            ++idx;
            condStr += 2;
        } while(idx < 0x20 );
        *condStr = '\0';

        std::snprintf(buffer, size, "%s = %s,%s,%s,%d,%d,%s,%s,%lf,%lf,%lf,%u,%d,%d,%u,%s,%u,%u,%u\n",
            this->ID,
            this->Name,
            Team1Name,
            HouseName,
            this->TechLevel,
            this->ConditionType,
            ConditionName,
            ConditionString,
            this->Weight_Current,
            this->Weight_Minimum,
            this->Weight_Maximum,
            this->IsForSkirmish,
            0,
            this->SideIndex,
            this->IsForBaseDefense,
            Team2Name,
            this->Enabled_Easy,
            this->Enabled_Normal,
            this->Enabled_Hard
        );

    }

    // Constructor
    /// VA: 0x0041E350.
    AITriggerTypeClass(const char* pID);

protected:
    explicit __forceinline AITriggerTypeClass(noinit_t) noexcept
        : AbstractTypeClass(noinit_t())
    { }

    // Properties

public:

    AITriggerCondition ConditionType;
    int              IsGlobal;
    AITriggerHouseType OwnerHouseType;
    bool             IsEnabled;
    int              HouseIndex;
    int              SideIndex;
    int              TechLevel;
    int              unknown_B4;
    double           Weight_Current;
    double           Weight_Minimum;
    double           Weight_Maximum;
    bool             IsForSkirmish;
    bool             IsForBaseDefense;
    bool             Enabled_Easy;
    bool             Enabled_Normal;
    bool             Enabled_Hard;
    TechnoTypeClass* ConditionObject;
    TeamTypeClass*   Team1;
    TeamTypeClass*   Team2;
    AITriggerConditionComparator Conditions [4]; // don't ask
    int              TimesExecuted;
    int              TimesCompleted;
    int              unknown_10C;

};
