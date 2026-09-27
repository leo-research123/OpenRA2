/*
    Tiberiums are initialized by INI files.
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractTypeClass.h"
#include "yrpp/PriorityQueueClass.h"
#include "yrpp/Timer.h"

// forward declarations
class AnimTypeClass;
class OverlayTypeClass;

class TiberiumLogic
{
public:
    void Construct(int nCount = PriorityQueueClassNode::SurfaceDataCount())
    {
        Nodes = (PriorityQueueClassNode*)YRMemory::Allocate(sizeof(PriorityQueueClassNode) * nCount);
        CellIndexesWithTiberium = (bool*)YRMemory::Allocate(sizeof(bool) * nCount);

        Queue = GameCreate<PriorityQueueClass<PriorityQueueClassNode>>(nCount);
    }

    void Destruct()
    {
        GameDelete(Queue);
        Queue = nullptr;

        if (Nodes)
        {
            YRMemory::Deallocate(Nodes);
            Nodes = nullptr;
        }

        if (CellIndexesWithTiberium)
        {
            YRMemory::Deallocate(CellIndexesWithTiberium);
            CellIndexesWithTiberium = nullptr;
        }
    }

    int Count;
    PriorityQueueClass<PriorityQueueClassNode>* Queue;
    bool* CellIndexesWithTiberium;
    PriorityQueueClassNode* Nodes;
    CDTimerClass Timer;
};

class TiberiumClass : public AbstractTypeClass
{
public:
    // Original queue entry references; these are logical queues, not tile caches.
    // Dispatch SpreadLogic timers for all Tiberium types.
    /// VA: 0x007221B0.
    // Recreate SpreadLogic queue storage for all types.
    /// VA: 0x00722240.
    // Process one type's spread queue.
    /// VA: 0x00722440.
    // Rebuild one type's spread queue from map cells.
    /// VA: 0x007228B0.
    // Register a cell for spreading after mineral mutation.
    /// VA: 0x00722AF0.
    // Dispatch GrowthLogic timers for all Tiberium types.
    /// VA: 0x00722C40.
    // Recreate GrowthLogic queue storage for all types.
    /// VA: 0x00722D00.
    // Process one type's growth queue.
    /// VA: 0x00722F00.
    // Rebuild one type's growth queue from map cells.
    /// VA: 0x007233A0.

    /// VA: 0x00722140; reference retained, not a local implementation.
    virtual void PointerExpired(AbstractClass* pAbstract, bool removed) override;
    /// VA: 0x00721DC0; reference retained, not a local implementation.
    virtual void ComputeCRC(CRCEngine& crc) const override { JMP_THIS(0x00721DC0); }
    /// VA: 0x00723700; reference retained, not a local implementation.
    virtual int GetArrayIndex() const override;
    /// VA: 0x00721A50; reference retained, not a local implementation.
#if defined(RA2_YRPP_GAME)
    virtual bool LoadFromINI(CCINIClass* pINI) override { JMP_THIS(0x00721A50); }
#else
    virtual bool LoadFromINI(CCINIClass*) override;
#endif

    static const AbstractType AbsID = AbstractType::Tiberium;

    // Array
    static DynamicVectorClass<TiberiumClass*>& Array;
    static TiberiumClass* YRPP_FASTCALL Find(const char* id);
    static int YRPP_FASTCALL FindIndex(const char* id);

    // IPersist
    /// VA: 0x00721E40.
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID);

    // IPersistStream
    /// VA: 0x00721E80; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) { JMP_STD(0x00721E80); }
    /// VA: 0x007220D0; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) { JMP_STD(0x007220D0); }
    /// VA: 0x007220A0; reference retained, not a local implementation.
    virtual HRESULT YRPP_STDCALL GetSizeMax(ULARGE_INTEGER* pcbSize) { JMP_STD(0x007220A0); }

    // Destructor
    /// VA: unknown (legacy placeholder).
    virtual ~TiberiumClass();

    // AbstractClass
    /// VA: 0x007236F0.
    virtual AbstractType WhatAmI() const;
    /// VA: 0x007236E0.
    virtual int Size() const;

    // TiberiumClass

    /// VA: 0x007235A0.
#if defined(RA2_YRPP_GAME)
    void RegisterForGrowth(CellStruct* cell) { JMP_THIS(0x7235A0); }
#else
    void RegisterForGrowth(CellStruct* cell);
#endif

    /// VA: 0x00722C40
    static void UpdateGrowth();
    /// VA: 0x007221B0
    static void UpdateSpread();
    /// VA: 0x00722F00
    void Grow();
    /// VA: 0x00722440
    void SpreadCells();
    /// VA: 0x007233A0
    void RebuildGrowth();
    /// VA: 0x007228B0
    void RebuildSpread();
    /// VA: 0x00722AF0
    void RegisterForSpread(CellStruct* cell);

    // Static helpers

    /// VA: 0x005FDD20
    static int FindIndex(int idxOverlayType);

    static TiberiumClass* Find(int idxOverlayType) {
        int idx = FindIndex(idxOverlayType);
        return Array.GetItemOrDefault(idx);
    }

    // Constructor
    /// VA: 0x007216C0.
    TiberiumClass(const char* pID);

protected:
    explicit __forceinline TiberiumClass(noinit_t)
        : AbstractTypeClass(noinit_t())
    { }

    // Properties

public:

    int ArrayIndex;
    int Spread;
    double SpreadPercentage;
    int Growth;
    double GrowthPercentage;
    int Value;
    int Power;
    int Color;
    DECLARE_PROPERTY(TypeList<AnimTypeClass*>, Debris);
    OverlayTypeClass* Image;
    int NumFrames;
    int NumImages;
    int NumSlopes;
    DECLARE_PROPERTY(TiberiumLogic, SpreadLogic);
    DECLARE_PROPERTY(TiberiumLogic, GrowthLogic);
};
