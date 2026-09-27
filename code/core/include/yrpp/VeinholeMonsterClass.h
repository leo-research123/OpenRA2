#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/FootClass.h"
#include "yrpp/PriorityQueueClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/CRT.h"

class VeinholeLogic
{
public:

    //74DFAF
    void Construct(int nCount = RulesClass::Instance->MaxVeinholeGrowth)
    {
        Nodes = (PriorityQueueClassNode*)YRMemory::Allocate(sizeof(PriorityQueueClassNode) * nCount);
        CellIndexesWithVeins = (bool*)YRMemory::Allocate(sizeof(bool) * nCount);
        CRT::_memset(CellIndexesWithVeins, 0, sizeof(bool) * nCount); //0x7D75E0 reserve ?
        Queue = GameCreate<PriorityQueueClass<PriorityQueueClassNode>>(nCount);
    }

    //74E8A0 Delete SpreadData
    void Destruct()
    {
        if (Queue)
        {
            GameDelete(Queue);
            Queue = nullptr;
        }

        if (Nodes)
        {
            YRMemory::Deallocate(Nodes);
            Nodes = nullptr;
        }

        if (CellIndexesWithVeins)
        {
            YRMemory::Deallocate(CellIndexesWithVeins);
            CellIndexesWithVeins = nullptr;
        }

        NextFreeNodeIndex = 0;
    }

    int NextFreeNodeIndex;
    PriorityQueueClass<PriorityQueueClassNode>* Queue;
    PriorityQueueClassNode* Nodes;
    CDTimerClass VeinTimer;
    bool* CellIndexesWithVeins;
};

class NOVTABLE VeinholeMonsterClass : public ObjectClass
{
public:
    static const AbstractType AbsID = AbstractType::VeinholeMonster;

    /// Global VA: 0x00A83DC8.
    DEFINE_REFERENCE(bool*, IsCurrentPosAffected, 0xA83DC8u)
    /// Global VA: 0x00B1D2EC.
    DEFINE_REFERENCE(SHPStruct*, VeinSHPData, 0xB1D2ECu)
    /// Global VA: 0x00B1D290.
    DEFINE_REFERENCE(DynamicVectorClass<VeinholeMonsterClass*>, Array, 0xB1D290u)

    // IPersist
    /// VA: 0x0074F2D0.
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) override JMP_THIS(0x74F2D0);

    // IPersistStream
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) R0; //none
    /// VA: 0x0074EEE0.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) override JMP_THIS(0x74EEE0);
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL GetSizeMax(ULARGE_INTEGER* pcbSize) R0;

    // Destructor
    /// VA: 0x0074C9F0.
    virtual ~VeinholeMonsterClass() JMP_THIS(0x74C9F0);

    // AbstractClass
    /// VA: 0x0074F310.
    virtual AbstractType WhatAmI() const override JMP_THIS(0x74F310);
    /// VA: 0x0074F320.
    virtual int Size() const override JMP_THIS(0x74F320);
    /// VA: 0x0074CE50.
    virtual void Update() override JMP_THIS(0x74CE50);

    // ObjectClass
    /// VA: 0x0074D490.
    virtual void DrawIt(Point2D* pLocation, RectangleStruct* pBounds) const override
        JMP_THIS(0x74D490); //114

    /// VA: 0x0074D5D0.
    virtual DamageState ReceiveDamage(
        int* pDamage,
        int DistanceFromEpicenter,
        WarheadTypeClass* pWH,
        ObjectClass* Attacker,
        bool IgnoreDefenses,
        bool PreventPassengerEscape,
        HouseClass* pAttackingHouse
    ) override
        JMP_THIS(0x74D5D0);

    /// VA: 0x0074CDB0.
    static VeinholeMonsterClass* YRPP_FASTCALL GetVeinholeMonsterAt(CellStruct* pCell)
        JMP_STD(0x74CDB0);

    /// VA: 0x0074CD60.
    static VeinholeMonsterClass* YRPP_FASTCALL GetVeinholeMonsterFrom(CellStruct* pCell)
        JMP_STD(0x74CD60);

    /// VA: 0x0074EF10.
    void RemoveFrom(CellClass* pCell) const
        JMP_THIS(0x74EF10);

    /// VA: 0x0074EA30.
    void ClearVector() const
        JMP_THIS(0x74EA30);

    void ClearGrowthData()
    {
        GrowthLogic.Destruct();
    }

    /// VA: 0x0074E930.
    void Recalculate() const
        JMP_THIS(0x74E930);

    /// VA: 0x0074E6B0.
    void RecalculateSpread() const
        JMP_THIS(0x74E6B0);

    /// VA: 0x0074E1C0.
    void Func_74E1C0_RecalculateCellVector() const
        JMP_THIS(0x74E1C0);

    /// VA: 0x0074DC00.
    void Func_74DC00() const
        JMP_THIS(0x74DC00);

    /// VA: 0x0074D7C0.
    void UpdateGrowth() const
        JMP_THIS(0x74D7C0);

    /// VA: 0x0074E100.
    static void YRPP_FASTCALL ClearVeinGrowthData()
    {
        JMP_STD(0x74E100);

        /*
        // pop back ?
        for (int i = Array.Count - 1; i >= 0; --i)
        {
            auto pVeinholes = Array.GetItem(i);
            pVeinholes->ClearGrowthData();
        }

        if (IsCurrentPosAffected())
        {
            YRMemory::Deallocate(Make_Pointer<bool>(0xA83DC8u));
            IsCurrentPosAffected = nullptr;
        }*/
    }

    // called 687A80
    /// VA: 0x0074DE90.
    static void YRPP_FASTCALL InitVeinGrowthData(bool bAllocate = true)
        JMP_STD(0x74DE90);

    /// VA: 0x0074D670.
    static bool YRPP_FASTCALL IsCellEligibleForVeinHole(CellStruct& nWhere)
        JMP_STD(0x74D670);

    /// VA: 0x0074D450.
    static void YRPP_FASTCALL TheaterInit(TheaterType nType)
        JMP_STD(0x74D450);

    /// VA: 0x0074EF00.
    static TerrainTypeClass* YRPP_FASTCALL GetTerrainType()
        JMP_STD(0x74EF00);

    /// VA: 0x0074ED60.
    static HRESULT YRPP_FASTCALL SaveVector(void* stream, DynamicVectorClass<VeinholeMonsterClass*>* a2)
        JMP_STD(0x74ED60);

    /// VA: 0x0074EA70.
    static HRESULT YRPP_FASTCALL LoadVector(LPSTREAM a1)
        JMP_STD(0x74EA70);

    /// VA: 0x0074EA30.
    static void YRPP_FASTCALL DestroyAll()
        JMP_STD(0x74EA30);

    /// VA: 0x0074D430.
    static void YRPP_FASTCALL DrawAll()
        JMP_STD(0x74D430);

    /// VA: 0x0074D760.
    static void YRPP_FASTCALL DeleteAll()
        JMP_STD(0x74D760);

    /// VA: 0x0074E880.
    static void YRPP_FASTCALL DeleteVeinholeGrowthData()
        JMP_STD(0x74E880);

    /// VA: 0x0074D450.
    static void YRPP_FASTCALL LoadVeinholeArt(int idxTheatre)
        JMP_STD(0x74D450);

    /// VA: 0x0074CDF0.
    static void YRPP_CDECL UpdateAllVeinholes()
        JMP_STD(0x74CDF0);

    static void YRPP_FASTCALL UpdateAll()
    {
        for (auto const& pVeins : Array)
        {
            if (!pVeins->InLimbo)
                pVeins->Update();
        }
    }

    /// VA: 0x0074C5B0.
    VeinholeMonsterClass(CellStruct* pWhere) noexcept
        : VeinholeMonsterClass(noinit_t())
    {
        JMP_THIS(0x74C5B0);
    }

protected:
    explicit __forceinline VeinholeMonsterClass(noinit_t) noexcept
        : ObjectClass(noinit_t()) { }
public:

    DECLARE_PROPERTY(VeinholeLogic, GrowthLogic);
    int CurrentState;
    int NextState;
    int MonsterFrameIdx;
    char IsAnimationUpToDate;
    CDTimerClass UpdateAnimationFrameTimer;
    int AnimationUpdatePeriod;
    int MonsterFrameIdxChange;
    CDTimerClass UpdateStateTimer;
    CellStruct Position;
    int MonsterFrameToDraw;
    char IsDead;
    char DontPuffGas;
    int VeinCount;
};

static_assert(sizeof(VeinholeMonsterClass) == 0x108); //264
