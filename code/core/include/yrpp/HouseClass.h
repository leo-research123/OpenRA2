/*
    Players
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/Timer.h"
#include "yrpp/BasicStructures.h"
#include "yrpp/Facing.h"
#include "yrpp/AircraftTypeClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/HouseTypeClass.h"
#include "yrpp/InfantryTypeClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/SessionClass.h"
#include "yrpp/SideClass.h"
#include "yrpp/Helpers/Template.h"
#include "yrpp/UnitTypeClass.h"

// forward declarations
class AnimClass;
class BulletClass;
class CellClass;
class FactoryClass;
class BuildingClass;
class UnitClass;
class AirstrikeClass;
class ObjectClass;
class SuperClass;
class TagClass;
class WaypointPathClass;
class WaypointClass;

class UnitTrackerClass {
public:
    /// VA: 0x00748FD0.
#if defined(RA2_YRPP_GAME)
    UnitTrackerClass() JMP_THIS(0x748FD0);
#else
    UnitTrackerClass() noexcept : UnitTotals{}, UnitCount(0x200), InNetworkFormat(0) {}
#endif
    /// VA: 0x00749010
    ~UnitTrackerClass() = default;
    /// VA: 0x00749020.
    void IncrementUnitCount(int nUnit) JMP_THIS(0x749020);
    /// VA: 0x00749040.
    void DecrementUnitCount(int nUnit) JMP_THIS(0x749040);
    /// VA: 0x00749060.
#if defined(RA2_YRPP_GAME)
    void PopulateUnitCount(int nCount) JMP_THIS(0x749060);
#else
    void PopulateUnitCount(int nCount) noexcept;
#endif
    /// VA: 0x007490A0.
    int GetUnitCount() JMP_THIS(0x7490A0);
    /// VA: 0x007490C0.
    const int* GetArray() JMP_THIS(0x7490C0);
    /// VA: 0x007490D0.
    void ClearUnitCount() JMP_THIS(0x7490D0);
    /// VA: 0x00749100.
    void ToNetworkFormat() JMP_THIS(0x749100);
    /// VA: 0x00749150.
    void ToPCFormat() JMP_THIS(0x749150);

    int UnitTotals[0x200];
    int UnitCount;
    BOOL InNetworkFormat;
};

struct ZoneInfoStruct {
    int Aircraft;
    int Armor;
    int Infantry;
};

struct StartingTechnoStruct {
    TechnoTypeClass* Unit;
    CellStruct Cell;
};

// that's how WW calls it, seems to track levels of how much it hates other houses... typical ww style, with bugs
struct AngerStruct {
    HouseClass* House;
    int AngerLevel;

    // need to define a == operator so it can be used in array classes
    bool operator==(const AngerStruct& tAnger) const {
        return (House == tAnger.House && AngerLevel == tAnger.AngerLevel);
    }
};

struct ScoutStruct {
    HouseClass* House;
    bool IsPreferred;

    // need to define a == operator so it can be used in array classes
    bool operator==(const ScoutStruct& tScout) const {
        return (House == tScout.House && IsPreferred == tScout.IsPreferred);
    }
};

//--- BaseNodeClass
class BaseNodeClass {
public:
    // need to define a == operator so it can be used in array classes
    bool operator==(const BaseNodeClass& tBaseNode) const {
        return (BuildingTypeIndex == tBaseNode.BuildingTypeIndex) && (MapCoords == tBaseNode.MapCoords) &&
               (Placed == tBaseNode.Placed) && (Attempts == tBaseNode.Attempts);
    }

    int BuildingTypeIndex;
    CellStruct MapCoords;
    bool Placed;
    int Attempts;
};

//--- BaseClass - holds information about a player's base!
class HouseClass;  //forward declaration needed

class BaseClass {
public:
    /// VA: 0x0042E6F0.
#if defined(RA2_YRPP_GAME)
    BaseClass() { JMP_THIS(0x42E6F0); }
#else
    BaseClass() : BaseNodes{}, PercentBuilt(0), Cells_24{}, Cells_38{}, Center{}, SpacingBounds{}, WallBounds{}, Owner(nullptr) {}
#endif
    explicit BaseClass(noinit_t) : BaseNodes(), Cells_24(), Cells_38() {}
    ~BaseClass() {
        BaseNodes.Clear();
        Cells_38.~DynamicVectorClass();
        Cells_24.~DynamicVectorClass();
        BaseNodes.~DynamicVectorClass();
    }

    // VTable
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm) R0;
    /// VA: unknown (legacy placeholder).
    virtual void ComputeCRC(CRCEngine& crc) const RX;

    // virtual ~BaseClass() { /*???*/ }; // gcc demands a virtual since virtual funcs exist

    // Reads PercentBuilt and the numbered base nodes of a house section.
    /// VA: 0x0042EBE0
#if defined(RA2_YRPP_GAME)
    void LoadFromINI(CCINIClass* ini, const char* section) { JMP_THIS(0x42EBE0); }
#else
    void LoadFromINI(CCINIClass* ini, const char* section);
#endif

    /// VA: 0x0042F380.
    int FailedToPlaceNode(BaseNodeClass* Node)  // called after AI fails to place building, obviously
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x42F380);
    }
#else
        ;
#endif
    /// VA: 0x0042E820
#if defined(RA2_YRPP_GAME)
    BuildingClass* GetNodeBuilding(int index) const { JMP_THIS(0x42E820); }
#else
    BuildingClass* GetNodeBuilding(int index) const;
#endif
    /// VA: 0x0042E780
#if defined(RA2_YRPP_GAME)
    bool IsNodeBuilt(int index) const { JMP_THIS(0x42E780); }
#else
    bool IsNodeBuilt(int index) const;
#endif
    /// VA: 0x0042EB50
#if defined(RA2_YRPP_GAME)
    int FindNextNodeIndex(int typeIndex) const { JMP_THIS(0x42EB50); }
#else
    int FindNextNodeIndex(int typeIndex) const;
#endif
    /// VA: 0x0042EB20
#if defined(RA2_YRPP_GAME)
    BaseNodeClass* FindNextNode(int typeIndex) { JMP_THIS(0x42EB20); }
#else
    BaseNodeClass* FindNextNode(int typeIndex);
#endif

    // Properties
    DECLARE_PROPERTY(DynamicVectorClass<BaseNodeClass>, BaseNodes);
    int PercentBuilt;
    DECLARE_PROPERTY(DynamicVectorClass<CellStruct>, Cells_24);
    DECLARE_PROPERTY(DynamicVectorClass<CellStruct>, Cells_38);
    CellStruct Center;

    RectangleStruct SpacingBounds;
    RectangleStruct WallBounds;

    HouseClass* Owner;

#pragma warning(suppress : 4265)
};

// used for each of the 3 drop ships. has more functions that are not reproduced here yet
struct DropshipStruct {
    /// VA: 0x004B69B0.
#if defined(RA2_YRPP_GAME)
    DropshipStruct() JMP_THIS(0x4B69B0);
#else
    // 0x4B69B0 starts the timer on the current frame with the maximum delay.
    DropshipStruct() noexcept : Timer{}, unknown_C{}, align_D{}, Count{}, Types{}, TotalCost{} { Timer.Start(0x7FFFFFFF); }
#endif
    /// VA: 0x004B69D0.
#if defined(RA2_YRPP_GAME)
    ~DropshipStruct() JMP_THIS(0x4B69D0);
#else
    ~DropshipStruct() = default;
#endif

    DECLARE_PROPERTY(CDTimerClass, Timer);
    BYTE unknown_C;
    PROTECTED_PROPERTY(BYTE, align_D[3]);
    int Count;
    TechnoTypeClass* Types[5];
    int TotalCost;
};

//--- Here we go, finally...
class NOVTABLE HouseClass : public AbstractClass, public IHouse, public IPublicHouse, public IConnectionPointContainer {
public:
    /// VA: 0x00500200
#if defined(RA2_YRPP_GAME)
    CellStruct* WhereToGo(CellStruct* output, FootClass* object) { JMP_THIS(0x500200); }
#else
    CellStruct* WhereToGo(CellStruct* output, FootClass* object);
#endif
    static const AbstractType AbsID = AbstractType::House;

    /// VA: 0x004F8440
#if defined(RA2_YRPP_GAME)
    void Update() override { JMP_THIS(0x4F8440); }
#else
    void Update() override;
#endif

    // <Player @ A> and friends map to these constants
    enum { PlayerAtA = 4475, PlayerAtB, PlayerAtC, PlayerAtD, PlayerAtE, PlayerAtF, PlayerAtG, PlayerAtH };

    // Static
    /// Global VA: 0x00A80228.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<HouseClass*>, Array, 0xA80228u)
#else
    static DynamicVectorClass<HouseClass*>& Array;
#endif

    /// Global VA: 0x00A83D4C.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(HouseClass*, CurrentPlayer, 0xA83D4Cu)  // House of player at this computer.
#else
    static HouseClass*& CurrentPlayer;
#endif
    /// Global VA: 0x00AC1198.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(HouseClass*, Observer, 0xAC1198u);  // House of player that is observer.
#else
    static HouseClass*& Observer;
#endif

    // IConnectionPointContainer
    /// VA: 0x5024F0
    virtual HRESULT YRPP_STDCALL EnumConnectionPoints(IEnumConnectionPoints** ppEnum) override R0;
    /// VA: 0x502550
    virtual HRESULT YRPP_STDCALL FindConnectionPoint(REFIID riid, IConnectionPoint** ppCP) override R0;

    // IPublicHouse
    /// VA: 0x004F6A80
    virtual long YRPP_STDCALL Apparent_Category_Quantity(Category category) const override R0;
    /// VA: 0x004F6B50
    virtual long YRPP_STDCALL Apparent_Category_Power(Category category) const override R0;
    /// VA: 0x004F6D10
    virtual CellStruct YRPP_STDCALL Apparent_Base_Center() const RT(CellStruct);
    /// VA: 0x004F6910
    virtual bool YRPP_STDCALL Is_Powered() const R0;

    // IHouse
    /// VA: 0x004F6E60
    virtual long YRPP_STDCALL ID_Number() const override R0;
    /// VA: 0x004F6950
    virtual BSTR YRPP_STDCALL Name() const override R0;
    /// VA: 0x004F6930
    virtual IApplication* YRPP_STDCALL Get_Application() override R0;
    /// VA: 0x4F6990
    long YRPP_STDCALL Available_Money() const override;
    /// VA: 0x004F69D0
    long YRPP_STDCALL Available_Storage() const override;
    /// VA: 0x004F6A00
    virtual long YRPP_STDCALL Power_Output() const override;
    /// VA: 0x004F6A10
    virtual long YRPP_STDCALL Power_Drain() const override;
    /// VA: 0x004F6A20
    virtual long YRPP_STDCALL Category_Quantity(Category category) const override R0;
    /// VA: 0x004F6AE0
    virtual long YRPP_STDCALL Category_Power(Category category) const override R0;
    /// VA: 0x004F6BC0
    virtual CellStruct YRPP_STDCALL Base_Center() const override RT(CellStruct);
    /// VA: 0x005013A0
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL Fire_Sale() const override { JMP_STD(0x5013A0); }
#else
    HRESULT YRPP_STDCALL Fire_Sale() const override;
#endif
    /// VA: 0x00501400
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL All_To_Hunt() override { JMP_STD(0x501400); }
#else
    HRESULT YRPP_STDCALL All_To_Hunt() override;
#endif

    // IUnknown
    /// VA: 0x004F6830
    virtual HRESULT YRPP_STDCALL QueryInterface(REFIID iid, void** ppvObject) R0;
    /// VA: 0x0050E340
    virtual ULONG YRPP_STDCALL AddRef() R0;
    /// VA: 0x0050E350
    virtual ULONG YRPP_STDCALL Release() R0;

    // IPersist
    /// VA: 0x005046F0
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) R0;

    // IPersistStream
    /// VA: 0x00503040
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) R0;
    /// VA: 0x00504080
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) R0;

    // Destructor
    /// VA: 0x004F7140
#if defined(RA2_YRPP_GAME)
    virtual ~HouseClass() RX;
#else
    virtual ~HouseClass();
#endif

    // AbstractClass
    /// VA: 0x004FB9B0
#if defined(RA2_YRPP_GAME)
    void PointerExpired(AbstractClass* object, bool removed) override { JMP_THIS(0x4FB9B0); }
#else
    // Native Tag, waypoint and factory arms; building/economy arms remain pending.
    void PointerExpired(AbstractClass* object, bool removed) override;
#endif
    /// VA: 0x0050E360
#if defined(RA2_YRPP_GAME)
    virtual AbstractType WhatAmI() const RT(AbstractType);
#else
    virtual AbstractType WhatAmI() const override { return AbsID; }
#endif
    /// VA: 0x00504730
#if defined(RA2_YRPP_GAME)
    virtual int Size() const R0;
#else
    virtual int Size() const override { return sizeof(*this); }
#endif

    bool MakeObserver() const {
        if (HouseClass::CurrentPlayer != this) return false;

        HouseClass::Observer = const_cast<HouseClass*>(this);
        return true;
    }

    /// VA: 0x004F9A10
    bool IsAlliedWith(int idxHouse) const {
        if (idxHouse == this->ArrayIndex) return true;
        if (idxHouse == -1) return false;
        return (this->Allies.data & (1u << (static_cast<unsigned>(idxHouse) & 31u))) != 0;
    }

    /// VA: 0x004F9A50
    bool IsAlliedWith(HouseClass const* pHouse) const {
        if (!pHouse) return false;
        if (this == pHouse || this->ArrayIndex == pHouse->ArrayIndex) return true;

        return pHouse->ArrayIndex != -1 &&
               (this->Allies.data & (1u << (static_cast<unsigned>(pHouse->ArrayIndex) & 31u))) != 0;
    }

    bool IsAlliedWith(ObjectClass const* pObject) const
    /// VA: 0x004F9A90
    {
        return pObject && this->IsAlliedWith(pObject->GetOwningHouse());
    }

    /// VA: 0x004F9AF0
    bool IsAlliedWith(AbstractClass const* pAbstract) const
    {
        auto* object = pAbstract && (pAbstract->AbstractFlags & ObjectClass::AbsDerivateID) != AbstractFlags::None
                           ? static_cast<const ObjectClass*>(pAbstract)
                           : nullptr;
        return this->IsAlliedWith(object);
    }

    inline bool IsMutualAlly(HouseClass const* pHouse) const {
        return pHouse == this ||
               (this->Allies.Contains(pHouse->ArrayIndex) && pHouse->Allies.Contains(this->ArrayIndex));
    }

    /// VA: 0x004F9B50.
    void MakeAlly(int iHouse, bool bAnnounce) { JMP_THIS(0x4F9B50); }
    /// VA: 0x004F9B70.
#if defined(RA2_YRPP_GAME)
    void MakeAlly(HouseClass* pWho, bool bAnnounce) { JMP_THIS(0x4F9B70); }
#else
    void MakeAlly(HouseClass* pWho, bool bAnnounce);
#endif
    /// VA: 0x004F9F90.
#if defined(RA2_YRPP_GAME)
    void MakeEnemy(HouseClass* pWho, bool bAnnounce) { JMP_THIS(0x4F9F90); }
#else
    void MakeEnemy(HouseClass* pWho, bool bAnnounce);
#endif

    /// VA: 0x00509400.
#if defined(RA2_YRPP_GAME)
    void AdjustThreats() { JMP_THIS(0x509400); }
#else
    void AdjustThreats();
#endif
    /// VA: 0x004FA2E0
#if defined(RA2_YRPP_GAME)
    void AdjustThreat(int region, int amount) { JMP_THIS(0x4FA2E0); }
#else
    void AdjustThreat(int region, int amount);
#endif
    /// VA: 0x00504790.
    void UpdateAngerNodes(int nScoreAdd, HouseClass* pHouse) { JMP_THIS(0x504790); }

    /// VA: 0x00501640.
#if defined(RA2_YRPP_GAME)
    static void AllyAIHouses() { JMP_STD(0x501640); }
#else
    static void AllyAIHouses();
#endif

    // no explosions, just poooof
    /// VA: 0x004FB920.
#if defined(RA2_YRPP_GAME)
    void SDDTORAllAndTriggers() { JMP_THIS(0x4FB920); }
#else
    void SDDTORAllAndTriggers();
#endif

    /// VA: 0x004FC0B0
#if defined(RA2_YRPP_GAME)
    void AcceptDefeat() { JMP_THIS(0x4FC0B0); }
#else
    void AcceptDefeat();
#endif

    // every matching object takes damage and explodes
    /// VA: 0x004FC6D0.
#if defined(RA2_YRPP_GAME)
    void DestroyAll() { JMP_THIS(0x4FC6D0); }
#else
    void DestroyAll() noexcept;
#endif
    /// VA: 0x004FC790.
#if defined(RA2_YRPP_GAME)
    void DestroyAllBuildings() { JMP_THIS(0x4FC790); }
#else
    void DestroyAllBuildings() noexcept;
#endif
    /// VA: 0x004FC820.
#if defined(RA2_YRPP_GAME)
    void DestroyAllNonBuildingsNonNaval() { JMP_THIS(0x4FC820); }
#else
    void DestroyAllNonBuildingsNonNaval() noexcept;
#endif
    /// VA: 0x004FC8D0.
#if defined(RA2_YRPP_GAME)
    void DestroyAllNonBuildingsNaval() { JMP_THIS(0x4FC8D0); }
#else
    void DestroyAllNonBuildingsNaval() noexcept;
#endif

    /// VA: 0x0050D320.
#if defined(RA2_YRPP_GAME)
    void RespawnStartingBuildings() { JMP_THIS(0x50D320); }
#else
    void RespawnStartingBuildings() noexcept;
#endif
    /// VA: 0x0050D6D0
#if defined(RA2_YRPP_GAME)
    void TeleportUnitsTo(CellStruct destination) { JMP_THIS(0x50D6D0); }
#else
    void TeleportUnitsTo(CellStruct destination) noexcept;
#endif
    /// VA: 0x0050D610
#if defined(RA2_YRPP_GAME)
    void RecordStartingUnits() { JMP_THIS(0x50D610); }
#else
    void RecordStartingUnits() noexcept;
#endif
    /// VA: 0x0050D440.
    void RespawnStartingForces() { JMP_THIS(0x50D440); }

    // flags the house to be defeated once its borrowed time expires,
    // unless it is already flagged to win, lose or die
    /// VA: 0x004FC980.
    bool FlagToDie() { JMP_THIS(0x4FC980); }

    /// VA: 0x004FC9E0.
#if defined(RA2_YRPP_GAME)
    BYTE Win(bool bSavourSomething) { JMP_THIS(0x4FC9E0); }
#else
    BYTE Win(bool bSavourSomething);
#endif
    /// VA: 0x004FCBD0.
#if defined(RA2_YRPP_GAME)
    BYTE Lose(bool bSavourSomething) { JMP_THIS(0x4FCBD0); }
#else
    BYTE Lose(bool bSavourSomething);
#endif

    // counts human-controlled houses other than this one that are not yet defeated
    /// VA: 0x005E2BA0.
    int CountOtherUndefeatedHumanHouses() const { JMP_THIS(0x5E2BA0); }

    /// VA: 0x004FB6B0.
    void RegisterJustBuilt(TechnoClass* pTechno) { JMP_THIS(0x4FB6B0); }

    /// VA: 0x00501540.
#if defined(RA2_YRPP_GAME)
    bool CanAlly(HouseClass* pOther) const { JMP_THIS(0x501540); }
#else
    bool CanAlly(HouseClass* pOther) const;
#endif

    /// VA: 0x004F9AF0.
    bool CanOverpower(TechnoClass* pTarget) const { JMP_THIS(0x4F9AF0); }

    /// VA: 0x0050E0E0
#if defined(RA2_YRPP_GAME)
    void LostPoweredCenter(TechnoTypeClass* type) { JMP_THIS(0x50E0E0); }
#else
    void LostPoweredCenter(TechnoTypeClass* type);
#endif
    /// VA: 0x0050E010
#if defined(RA2_YRPP_GAME)
    void RegisterPoweredCenter(TechnoTypeClass* type) { JMP_THIS(0x50E010); }
#else
    void RegisterPoweredCenter(TechnoTypeClass* type);
#endif
    /// VA: 0x0050E1B0
#if defined(RA2_YRPP_GAME)
    bool HasPoweredCenter(TechnoTypeClass* type) const { JMP_THIS(0x50E1B0); }
#else
    bool HasPoweredCenter(TechnoTypeClass* type) const;
#endif
    // Legacy name retained for source compatibility; this address only queries.
    /// VA: 0x0050E1B0.
    void GainedPoweredCenter(TechnoTypeClass* pTechnoType) { JMP_THIS(0x50E1B0); }

    /// VA: 0x0050D9C0
    bool DoInfantrySelfHeal() const { return this->InfantrySelfHeal > 0; }
    /// VA: 0x0050D9E0.
#if defined(RA2_YRPP_GAME)
    int GetInfSelfHealStep() const { JMP_THIS(0x50D9E0); }
#else
    int GetInfSelfHealStep() const noexcept;
#endif

    /// VA: 0x0050D9D0
    bool DoUnitsSelfHeal() const { return this->UnitsSelfHeal > 0; }
    /// VA: 0x0050D9F0.
#if defined(RA2_YRPP_GAME)
    int GetUnitSelfHealStep() const { JMP_THIS(0x50D9F0); }
#else
    int GetUnitSelfHealStep() const noexcept;
#endif

    /// VA: 0x00508C30.
#if defined(RA2_YRPP_GAME)
    void UpdatePower() { JMP_THIS(0x508C30); }
#else
    void UpdatePower();
#endif
    /// VA: 0x00508DF0
    void UpdateRadarAvailability();
    // No exceptions may escape this state-transition boundary.
    /// VA: 0x00508F60
#if defined(RA2_YRPP_GAME)
    void UpdateSpySat() noexcept { JMP_THIS(0x508F60); }
#else
    void UpdateSpySat() noexcept;
#endif
    /// VA: 0x0050AF10
    void UpdateSuperWeapons();
    // Native entry for the tech-tree block of House AI (0x4F9265..0x4F9302),
    // not a separate original function. No object or virtual-table changes.
    void UpdateTechTree() noexcept;
    /// VA: 0x0050C0A0
    double GetBuildTimeMultiplier(TechnoTypeClass* type) const;
    /// VA: 0x00500910
    int CountFactories(AbstractType type, bool naval) const;
    /// VA: 0x0050BC90.
#if defined(RA2_YRPP_GAME)
    void CreatePowerOutage(int duration) { JMP_THIS(0x50BC90); }
#else
    void CreatePowerOutage(int duration);
#endif
    /// VA: 0x004FCE30.
#if defined(RA2_YRPP_GAME)
    double GetPowerPercentage() const { JMP_THIS(0x4FCE30); }
#else
    double GetPowerPercentage() const;
#endif

    bool HasFullPower() const { return this->PowerOutput >= this->PowerDrain || !this->PowerDrain; }

    bool HasLowPower() const { return this->PowerOutput < this->PowerDrain && this->PowerDrain; }

    /// VA: 0x0050BCD0.
#if defined(RA2_YRPP_GAME)
    void CreateRadarOutage(int duration) { JMP_THIS(0x50BCD0); }
#else
    void CreateRadarOutage(int duration);
#endif

    // won't work if has spysat
    /// VA: 0x0050BD10.
#if defined(RA2_YRPP_GAME)
    void ReshroudMap() { JMP_THIS(0x50BD10); }
#else
    void ReshroudMap();
#endif

    /// VA: 0x0050C8C0.
#if defined(RA2_YRPP_GAME)
    void Cheer() { JMP_THIS(0x50C8C0); }
#else
    void Cheer() noexcept;
#endif

    /// VA: 0x004F93E0.
    void BuildingUnderAttack(BuildingClass* pBld);

    /// VA: 0x004F9790.
#if defined(RA2_YRPP_GAME)
    void TakeMoney(int amount) { JMP_THIS(0x4F9790); }
#else
    void TakeMoney(int amount);
#endif
    /// VA: 0x004F9950.
#if defined(RA2_YRPP_GAME)
    void GiveMoney(int amount) { JMP_THIS(0x4F9950); }
#else
    void GiveMoney(int amount);
#endif

    inline bool CanTransactMoney(int amount) const { return amount > 0 || this->Available_Money() >= -amount; }

    inline void TransactMoney(int amount) {
        if (amount > 0) {
            this->GiveMoney(amount);
        } else {
            this->TakeMoney(-amount);
        }
    }

    /// VA: 0x004F9610.
    void GiveTiberium(float amount, int type);
    /// VA: 0x004F9970.
    void UpdateAllSilos(int prevStorage, int prevTotalStorage) { JMP_THIS(0x4F9970); }
    /// VA: 0x004F6E70.
    double GetStoragePercentage() { JMP_THIS(0x4F6E70); }

    // no LostThreatNode() , this gets called also when node building dies! BUG
    /// VA: 0x00509130.
    void AcquiredThreatNode() { JMP_THIS(0x509130); }

    // these are for mostly for map actions - HouseClass* foo = IsMP() ? Find_YesMP() : Find_NoMP();
    /// VA: 0x00510F60.
    static bool YRPP_FASTCALL Index_IsMP(int idx) {
        // JMP_STD(0x510F60);
        return idx >= PlayerAtA && idx <= PlayerAtH;
    }
    /// VA: 0x00502D30.
    static HouseClass* YRPP_FASTCALL FindByCountryIndex(int HouseType)  // find first house of this houseType
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x502D30);
    }
#else
        ;
#endif
    /// VA: 0x00510ED0.
    static HouseClass* YRPP_FASTCALL
    FindByIndex(int idxHouse)  // PlayerAtA..PlayerAtH through ScenarioClass::HouseIndices
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x510ED0);
    }
#else
        ;
#endif
    // Index in Array of the house whose PlainName equals name (case-sensitive), or -1.
    /// VA: 0x0050C170.
    static signed int YRPP_FASTCALL FindIndexByName(const char* name)
#if defined(RA2_YRPP_GAME)
    {
        JMP_STD(0x50C170);
    }
#else
        ;
#endif

    /// VA: 0x00510FB0.
    static int YRPP_FASTCALL GetPlayerAtFromString(const char* name) { JMP_STD(0x510FB0); }
    /// VA: 0x00510ED0.
    static HouseClass* YRPP_FASTCALL FindByPlayerAt(int at) { JMP_STD(0x510ED0); }

    // gets the first house of a type with this name
    static HouseClass* FindByCountryName(const char* name) {
        auto idx = HouseTypeClass::FindIndexOfName(name);
        return FindByCountryIndex(idx);
    }

    // gets the first house of a type with name Neutral
    static HouseClass* FindNeutral() { return FindByCountryName(GameStrings::Neutral); }

    // gets the first house of a type with name Special
    static HouseClass* FindSpecial() { return FindByCountryName(GameStrings::Special); }

    // gets the first house of a side with this name
    static HouseClass* FindBySideIndex(int index) {
        for (auto pHouse : Array) {
            if (pHouse->Type->SideIndex == index) {
                return pHouse;
            }
        }
        return nullptr;
    }

    // gets the first house of a type with this name
    static HouseClass* FindBySideName(const char* name) {
        auto idx = SideClass::FindIndex(name);
        return FindBySideIndex(idx);
    }

    // gets the first house of a type from the Civilian side
    static HouseClass* FindCivilianSide() { return FindBySideName(GameStrings::Civilian); }

    // Creates the houses of the scenario's [Houses] list, or one per country
    // when there is none, and reads each from the scenario INI.
    /// VA: 0x005009B0.
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL LoadFromINIList(CCINIClass* pINI) { JMP_STD(0x5009B0); }
#else
    static void YRPP_FASTCALL LoadFromINIList(CCINIClass* pINI);
#endif
    // Reads this house's section of the scenario INI.
    /// VA: 0x00500B40
#if defined(RA2_YRPP_GAME)
    void LoadFromINI(CCINIClass* pINI) { JMP_THIS(0x500B40); }
#else
    void LoadFromINI(CCINIClass* pINI);
#endif

    int GetSpawnPosition() const {
        const int currentIndex = this->ArrayIndex;
        const int* houseIndices = ScenarioClass::Instance->HouseIndices;

        for (int i = 0; i < 8; i++) {
            if (houseIndices[i] == currentIndex) return i;
        }
        return -1;
    }

    /// VA: 0x005023B0.
    WaypointClass* GetPlanningWaypointAt(CellStruct* coords)
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x5023B0);
    }
#else
        ;
#endif
    /// VA: 0x005090F0
#if defined(RA2_YRPP_GAME)
    bool CanAddPlanningWaypoint() const { JMP_THIS(0x5090F0); }
#else
    bool CanAddPlanningWaypoint() const;
#endif
    /// VA: 0x00502460.
    bool GetPlanningWaypointProperties(WaypointClass* wpt, int& idxPath, BYTE& idxWP)
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x502460);
    }
#else
        ;
#endif

    // calls WaypointPathClass::WaypointPathClass() if needed
    /// VA: 0x00504740.
    WaypointPathClass* EnsurePlanningPathExists(int idx) noexcept
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x504740);
    }
#else
        ;
#endif

    // call after the availability of a factory has changed.
    /// VA: 0x00509140.
#if defined(RA2_YRPP_GAME)
    void Update_FactoriesQueues(AbstractType factoryOf, bool isNaval, BuildCat buildCat) const { JMP_THIS(0x509140); }
#else
    void Update_FactoriesQueues(AbstractType factoryOf, bool isNaval, BuildCat buildCat) const;
#endif

    // returns the factory owned by this house, having pItem in production right now, not queued
    /// VA: 0x004F83C0.
    FactoryClass* GetFactoryProducing(TechnoTypeClass const* pItem) const { JMP_THIS(0x4F83C0); }

    // finds a buildingtype from the given array that this house can build
    // this checks whether the Owner=, Required/ForbiddenHouses= , AIBasePlanningSide= match and if SuperWeapon= (not SW2=) is not forbidden
    /// VA: 0x505310
#if defined(RA2_YRPP_GAME)
    UnitTypeClass* FirstOwnedUnitFromArray(const DynamicVectorClass<UnitTypeClass*>& items) {
#if defined(__clang__) && defined(_M_IX86)
        __asm {
            mov ecx, this
            mov esp, ebp
            pop ebp
            mov eax, 0x505310
            jmp eax
        }
#else
        JMP_THIS(0x505310);
#endif
    }
#else
    UnitTypeClass* FirstOwnedUnitFromArray(const DynamicVectorClass<UnitTypeClass*>& items);
#endif
    /// VA: 0x005051E0.
#if defined(RA2_YRPP_GAME)
    BuildingTypeClass* FirstBuildableFromArray(DynamicVectorClass<BuildingTypeClass*> const& items) {
        JMP_THIS(0x5051E0);
    }
#else
    BuildingTypeClass* FirstBuildableFromArray(DynamicVectorClass<BuildingTypeClass*> const& items);
#endif

    // are all prereqs for Techno listed in vectorBuildings[0..vectorLength]. Yes, the length is needed (the vector is used for breadth-first search)
    /// VA: 0x00505360
#if defined(RA2_YRPP_GAME) && defined(__clang__) && defined(_M_IX86)
    // clang-cl spills implicit bool results between separate asm blocks, even
    // after JMP_THIS restores the caller's frame. Keep this tail jump atomic.
    bool AllPrerequisitesAvailable(TechnoTypeClass const* pItem,
                                   DynamicVectorClass<BuildingTypeClass*> const& vectorBuildings, int vectorLength) {
        __asm {
            mov ecx, this
            mov esp, ebp
            pop ebp
            mov eax, 0x505360
            jmp eax
        }
    }
#elif defined(RA2_YRPP_GAME)
    bool AllPrerequisitesAvailable(TechnoTypeClass const* pItem,
                                   DynamicVectorClass<BuildingTypeClass*> const& vectorBuildings, int vectorLength) {
        JMP_THIS(0x505360);
    }
#else
    bool AllPrerequisitesAvailable(TechnoTypeClass const* pItem,
                                   DynamicVectorClass<BuildingTypeClass*> const& vectorBuildings, int vectorLength);
#endif

    // Whether any human player controls this house.
    /// VA: 0x0050B730
#if defined(RA2_YRPP_GAME)
    bool IsControlledByHuman() const {  // { JMP_THIS(0x50B730); }
        bool result = this->IsHumanPlayer;
        if (SessionClass::Instance.GameMode == GameMode::Campaign) {
            result = result || this->IsInPlayerControl;
        }
        return result;
    }
#else
    bool IsControlledByHuman() const;
#endif

    // Campaign control includes every human or explicitly player-controlled house.
    /// VA: 0x0050B6F0
#if defined(RA2_YRPP_GAME)
    bool IsControlledByCurrentPlayer() const {
        if (SessionClass::Instance.GameMode == GameMode::Campaign) return IsHumanPlayer || IsInPlayerControl;
        return IsCurrentPlayer();
    }
#else
    bool IsControlledByCurrentPlayer() const;
#endif

    /// VA: 0x65E660
#if defined(RA2_YRPP_GAME)
    void YRPP_FASTCALL SendParadropPlanes(int aircraftTypeIdx, int aircraftCount, Mission mission,
                                          AbstractClass* pTarget, AbstractClass* pDestination, int infantryTypeIdx,
                                          int infantryCount) {
        JMP_STD(0x65E660);
    }
#else
    void YRPP_FASTCALL SendParadropPlanes(int aircraftTypeIdx, int aircraftCount, Mission mission,
                                         AbstractClass* pTarget, AbstractClass* pDestination, int infantryTypeIdx,
                                         int infantryCount);
#endif

    /// VA: 0x65E850
#if defined(RA2_YRPP_GAME)
    int YRPP_FASTCALL SendAirstrikePlanes(int aircraftTypeIdx, int aircraftCount, Mission mission,
                                          AbstractClass* target, AbstractClass* destination, int infantryTypeIdx,
                                          int infantryCount, AirstrikeClass* sender) {
        JMP_STD(0x65E850);
    }
#else
    int YRPP_FASTCALL SendAirstrikePlanes(int aircraftTypeIdx, int aircraftCount, Mission mission,
                                          AbstractClass* target, AbstractClass* destination, int infantryTypeIdx,
                                          int infantryCount, AirstrikeClass* sender);
#endif

    /// VA: 0x65EAB0
#if defined(RA2_YRPP_GAME)
    void YRPP_FASTCALL SendSpyPlanes(int aircraftTypeIdx, int aircraftCount, Mission mission, AbstractClass* pTarget,
                                     AbstractClass* pDestination) {
        JMP_STD(0x65EAB0);
    }
#else
    void YRPP_FASTCALL SendSpyPlanes(int aircraftTypeIdx, int aircraftCount, Mission mission, AbstractClass* pTarget,
                                    AbstractClass* pDestination);
#endif

    // registering in prereq counters (all technoes get logged, but only buildings get checked on validation... wtf)
    /// VA: 0x00504790
    void RegisterDamage(int amount, HouseClass* source);

    /// VA: 0x00502A80.
    void RegisterGain(TechnoClass* pTechno, bool ownerChange);

    /// VA: 0x005025F0.
    void RegisterLoss(TechnoClass* pTechno, bool keepTiberium);

    /// VA: 0x4FFA50
#if defined(RA2_YRPP_GAME)
    void AddFactory(TechnoClass* object) { JMP_THIS(0x4FFA50); }
#else
    void AddFactory(TechnoClass* object);
#endif
    /// VA: 0x4FF980
#if defined(RA2_YRPP_GAME)
    void RemoveFactory(TechnoClass* object) { JMP_THIS(0x4FF980); }
#else
    void RemoveFactory(TechnoClass* object);
#endif

    /// VA: 0x005018C0
    void AddPowerDrain(int amount);
    /// VA: 0x4FA350
    int BeginProduction(AbstractType type, int index, bool naval, bool resume = false);
    /// VA: 0x4FA910
    int SuspendProduction(AbstractType type, int index, bool naval);
    /// VA: 0x4FB0E0
    bool PlaceObject(AbstractType type, int index, bool naval, const CellStruct& cell);
    /// VA: 0x4FB6B0
    void JustBuilt(TechnoClass* object);
    /// VA: 0x4FB840
#if defined(RA2_YRPP_GAME)
    bool EnterPlacement(BuildingClass* builder, BuildingClass* product) { JMP_THIS(0x4FB840); }
#else
    bool EnterPlacement(BuildingClass* builder, BuildingClass* product);
#endif
    /// VA: 0x004FAA10
#if defined(RA2_YRPP_GAME)
    void AbandonProduction(AbstractType type, int index, bool naval, bool all) { JMP_THIS(0x4FAA10); }
#else
    void AbandonProduction(AbstractType type, int index, bool naval, bool all);
#endif
    /// VA: 0x0050B370
#if defined(RA2_YRPP_GAME)
#if defined(RA2_YRPP_GAME)
    bool HasReachedBuildLimit(TechnoTypeClass* type) { JMP_THIS(0x50B370); }
#else
    bool HasReachedBuildLimit(TechnoTypeClass* type);
#endif
#else
    bool HasReachedBuildLimit(TechnoTypeClass* type);
#endif

    /// VA: 0x004FD060
    #if defined(RA2_YRPP_GAME)
    BuildingClass* FindBuildingOfType(int idx, int sector = -1) const { JMP_THIS(0x4FD060); }
#else
    BuildingClass* FindBuildingOfType(int idx, int sector = -1) const;
#endif

    /// VA: 0x0043B5E0
    #if defined(RA2_YRPP_GAME)
    AnimClass* YRPP_FASTCALL PsiWarn(CellClass* pTarget, BulletClass* Bullet, char* AnimName) JMP_THIS(0x43B5E0);
#else
    AnimClass* YRPP_FASTCALL PsiWarn(CellClass* pTarget, BulletClass* Bullet, char* AnimName);
#endif

    /// VA: 0x00509E00
    #if defined(RA2_YRPP_GAME)
    bool Fire_LightningStorm(SuperClass* pSuper) { JMP_THIS(0x509E00); }
#else
    bool Fire_LightningStorm(SuperClass* pSuper);
#endif

    /// VA: 0x00509CD0
    #if defined(RA2_YRPP_GAME)
    bool Fire_ParaDrop(SuperClass* pSuper) { JMP_THIS(0x509CD0); }
#else
    bool Fire_ParaDrop(SuperClass* pSuper);
#endif

    /// VA: 0x0050A150
    #if defined(RA2_YRPP_GAME)
    bool Fire_PsyDom(SuperClass* pSuper) { JMP_THIS(0x50A150); }
#else
    bool Fire_PsyDom(SuperClass* pSuper);
#endif

    /// VA: 0x00509F60
    #if defined(RA2_YRPP_GAME)
    bool Fire_GenMutator(SuperClass* pSuper) { JMP_THIS(0x509F60); }
#else
    bool Fire_GenMutator(SuperClass* pSuper);
#endif

    /// VA: 0x0053A130
    bool IonSensitivesShouldBeOffline() const { return false; }  // YR removed the TS ion-storm outage.

    const char* get_ID() const { return this->Type->get_ID(); }

    int FindSuperWeaponIndex(SuperWeaponType type) const;

    SuperClass* FindSuperWeapon(SuperWeaponType type) const;

    // I don't want to talk about these
    // read the code <_<

    //  Count owned now
    int CountOwnedNow(TechnoTypeClass const* pItem) const;

    int CountOwnedNow(BuildingTypeClass const* const pItem) const {
        return this->OwnedBuildingTypes.GetItemCount(pItem->ArrayIndex);
    }

    int CountOwnedNow(AircraftTypeClass const* const pItem) const {
        return this->OwnedAircraftTypes.GetItemCount(pItem->ArrayIndex);
    }

    int CountOwnedNow(InfantryTypeClass const* const pItem) const {
        return this->OwnedInfantryTypes.GetItemCount(pItem->ArrayIndex);
    }

    int CountOwnedNow(UnitTypeClass const* const pItem) const {
        return this->OwnedUnitTypes.GetItemCount(pItem->ArrayIndex);
    }

    // Count owned and present
    int CountOwnedAndPresent(TechnoTypeClass const* pItem) const;

    int CountOwnedAndPresent(BuildingTypeClass const* const pItem) const {
        return this->ActiveBuildingTypes.GetItemCount(pItem->ArrayIndex);
    }

    int CountOwnedAndPresent(AircraftTypeClass const* const pItem) const {
        return this->ActiveAircraftTypes.GetItemCount(pItem->ArrayIndex);
    }

    int CountOwnedAndPresent(InfantryTypeClass const* const pItem) const {
        return this->ActiveInfantryTypes.GetItemCount(pItem->ArrayIndex);
    }

    int CountOwnedAndPresent(UnitTypeClass const* const pItem) const {
        return this->ActiveUnitTypes.GetItemCount(pItem->ArrayIndex);
    }

    // Count owned ever
    int CountOwnedEver(TechnoTypeClass const* pItem) const;

    int CountOwnedEver(BuildingTypeClass const* const pItem) const {
        return this->FactoryProducedBuildingTypes.GetItemCount(pItem->ArrayIndex);
    }

    int CountOwnedEver(AircraftTypeClass const* const pItem) const {
        return this->FactoryProducedAircraftTypes.GetItemCount(pItem->ArrayIndex);
    }

    int CountOwnedEver(InfantryTypeClass const* const pItem) const {
        return this->FactoryProducedInfantryTypes.GetItemCount(pItem->ArrayIndex);
    }

    int CountOwnedEver(UnitTypeClass const* const pItem) const {
        return this->FactoryProducedUnitTypes.GetItemCount(pItem->ArrayIndex);
    }

    bool HasFromSecretLab(const TechnoTypeClass* const pItem) const {
        for (const auto& pLab : this->SecretLabs) {
            if (pLab->GetSecretProduction() == pItem) {
                return true;
            }
        }
        return false;
    }

    bool HasAllStolenTech(const TechnoTypeClass* const pItem) const {
        if (pItem->RequiresStolenAlliedTech && !this->Side0TechInfiltrated) {
            return false;
        }
        if (pItem->RequiresStolenSovietTech && !this->Side1TechInfiltrated) {
            return false;
        }
        if (pItem->RequiresStolenThirdTech && !this->Side2TechInfiltrated) {
            return false;
        }
        return true;
    }

    bool HasFactoryForObject(const TechnoTypeClass* const pItem) const {
        auto const abs = pItem->WhatAmI();
        auto const naval = pItem->Naval;
        for (auto const& pBld : this->Buildings) {
            auto pType = pBld->Type;
            if (pType->Factory == abs && pType->Naval == naval) {
                return true;
            }
        }
        return false;
    }

    /// VA: 0x00509610
#if defined(RA2_YRPP_GAME)
    bool CanBuildTeam(TeamTypeClass* team) const { JMP_THIS(0x509610); }
#else
    bool CanBuildTeam(TeamTypeClass* team) const;
#endif
    /// VA: 0x005095D0
#if defined(RA2_YRPP_GAME)
    int CountTeams(TeamTypeClass* team) const { JMP_THIS(0x5095D0); }
#else
    int CountTeams(TeamTypeClass* team) const;
#endif

    bool CanExpectToBuild(const TechnoTypeClass* pItem) const;

    bool CanExpectToBuild(const TechnoTypeClass* pItem, int idxParent) const;

    bool InOwners(const TechnoTypeClass* const pItem) const {
        auto const idxParentCountry = this->Type->FindParentCountryIndex();
        return pItem->InOwners(1u << idxParentCountry);
    }

    bool InRequiredHouses(const TechnoTypeClass* const pItem) const {
        return pItem->InRequiredHouses(1u << this->Type->ArrayIndex2);
    }

    bool InForbiddenHouses(const TechnoTypeClass* const pItem) const {
        return pItem->InForbiddenHouses(1u << this->Type->ArrayIndex2);
    }

    /// VA: 0x004F7870.
#if defined(RA2_YRPP_GAME)
    CanBuildResult CanBuild(TechnoTypeClass const* pItem, bool buildLimitOnly, bool allowIfInProduction) const {
        JMP_THIS(0x4F7870);
    }
#else
    CanBuildResult CanBuild(TechnoTypeClass const* pItem, bool buildLimitOnly, bool allowIfInProduction) const;
#endif

    /// VA: 0x4FD500
#if defined(RA2_YRPP_GAME)
    int AI_StrategicUpdate() { JMP_THIS(0x4FD500); }
#else
    int AI_StrategicUpdate();
#endif

    /// VA: 0x4F6540
#if defined(RA2_YRPP_GAME)
    bool AI_IsEconomyRecoverable() { JMP_THIS(0x4F6540); }
#else
    bool AI_IsEconomyRecoverable();
#endif

    /// VA: 0x4FD9A0
#if defined(RA2_YRPP_GAME)
    int AI_EvaluateEconomy() { JMP_THIS(0x4FD9A0); }
#else
    int AI_EvaluateEconomy();
#endif

    // Emergency recovery stops locally on native service/allocation failure.
    /// VA: 0x4FDD10
#if defined(RA2_YRPP_GAME)
    bool AI_RecoverEconomy(int priority) noexcept { JMP_THIS(0x4FDD10); }
#else
    bool AI_RecoverEconomy(int priority) noexcept;
#endif

    /// VA: 0x004FE3E0.
    int AI_BaseConstructionUpdate()
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x4FE3E0);
    }
#else
        ;
#endif
    /// VA: 0x00509700
#if defined(RA2_YRPP_GAME)
    void AI_ProductionCompleted(AbstractType kind) { JMP_THIS(0x509700); }
#else
    void AI_ProductionCompleted(AbstractType kind);
#endif
    /// VA: 0x004FBD80
#if defined(RA2_YRPP_GAME)
    TechnoTypeClass* GetProductionType(AbstractType kind, bool naval) const { JMP_THIS(0x4FBD80); }
#else
    TechnoTypeClass* GetProductionType(AbstractType kind, bool naval) const;
#endif

    /// VA: 0x004FEA60.
#if defined(RA2_YRPP_GAME)
    int AI_VehicleConstructionUpdate() { JMP_THIS(0x4FEA60); }
#else
    int AI_VehicleConstructionUpdate();
#endif

    /// VA: 0x004FEEE0
#if defined(RA2_YRPP_GAME)
    int AI_InfantryConstructionUpdate() { JMP_THIS(0x4FEEE0); }
#else
    int AI_InfantryConstructionUpdate();
#endif

    /// VA: 0x004FF210
#if defined(RA2_YRPP_GAME)
    int AI_AircraftConstructionUpdate() { JMP_THIS(0x4FF210); }
#else
    int AI_AircraftConstructionUpdate();
#endif

    /// VA: 0x004FD150
#if defined(RA2_YRPP_GAME)
    void RecalculateBaseCenter() { JMP_THIS(0x4FD150); }
#else
    void RecalculateBaseCenter();
#endif
    /// VA: 0x00500300
#if defined(RA2_YRPP_GAME)
#if defined(__clang__) && defined(_M_IX86)
    TechnoClass* FindNearbyEnemy(const CoordStruct& at) {
        using Entry = TechnoClass*(YRPP_THISCALL*)(HouseClass*, const CoordStruct&);
        return reinterpret_cast<Entry>(0x500300)(this, at);
    }
#else
    TechnoClass* FindNearbyEnemy(const CoordStruct& at) { JMP_THIS(0x500300); }
#endif
#else
    TechnoClass* FindNearbyEnemy(const CoordStruct& at);
#endif

    /// VA: 0x004FFB20
#if defined(RA2_YRPP_GAME)
    int WhichZone(const CoordStruct& coordinate) const { JMP_THIS(0x4FFB20); }
#else
    int WhichZone(const CoordStruct& coordinate) const;
#endif
    /// VA: 0x0050CAD0
#if defined(RA2_YRPP_GAME)
    bool CanRebuildBaseNode(const BaseNodeClass* node) const { JMP_THIS(0x50CAD0); }
#else
    bool CanRebuildBaseNode(const BaseNodeClass* node) const;
#endif
    /// VA: 0x0050A490
#if defined(RA2_YRPP_GAME)
    void InvalidateBaseNodePosition(BuildingClass* building) { JMP_THIS(0x50A490); }
#else
    void InvalidateBaseNodePosition(BuildingClass* building);
#endif

    /// VA: 0x005098F0
    #if defined(RA2_YRPP_GAME)
    void AI_TryFireSW() { JMP_THIS(0x5098F0); }
#else
    void AI_TryFireSW();
#endif

    /// VA: 0x4FAE50
#if defined(RA2_YRPP_GAME)
    bool Fire_SW(int idx, const CellStruct& coords) { JMP_THIS(0x4FAE50); }
#else
    bool Fire_SW(int idx, const CellStruct& coords);
#endif

    /// VA: 0x4FAF00
#if defined(RA2_YRPP_GAME)
    void NotifySuperWeaponFired(SuperClass* super, const CellStruct& cell) { JMP_THIS(0x4FAF00); }
#else
    void NotifySuperWeaponFired(SuperClass* super, const CellStruct& cell);
#endif

    /// VA: 0x0050D170
    #if defined(RA2_YRPP_GAME)
    CellStruct* PickTargetByType(CellStruct& outBuffer, QuarryType targetType) const { JMP_THIS(0x50D170); }
#else
    CellStruct* PickTargetByType(CellStruct& outBuffer, QuarryType targetType) const;
#endif

    CellStruct PickTargetByType(QuarryType targetType) const {
        CellStruct outBuffer;
        this->PickTargetByType(outBuffer, targetType);
        return outBuffer;
    }

    /// VA: 0x0050CBF0
    #if defined(RA2_YRPP_GAME)
    CellStruct* PickIonCannonTarget(CellStruct& outBuffer) const { JMP_THIS(0x50CBF0); }
#else
    CellStruct* PickIonCannonTarget(CellStruct& outBuffer) const;
#endif

    CellStruct PickIonCannonTarget() const {
        CellStruct outBuffer;
        this->PickIonCannonTarget(outBuffer);
        return outBuffer;
    }

    bool IsIonCannonEligibleTarget(const TechnoClass* pTechno) const;

    /// VA: 0x004FBE40.
    /// VA: 0x4FBE40
#if defined(RA2_YRPP_GAME)
    void UpdateFlagCoords(UnitClass* NewCarrier, DWORD dwUnk) { JMP_THIS(0x4FBE40); }
#else
    void UpdateFlagCoords(UnitClass* NewCarrier, DWORD dwUnk);
#endif

    /// VA: 0x004FBF60.
    void DroppedFlag(CellStruct* Where, UnitClass* Who) { JMP_THIS(0x4FBF60); }

    /// VA: 0x004FC060.
    /// VA: 0x4FC060
#if defined(RA2_YRPP_GAME)
    char PickedUpFlag(UnitClass* Who, DWORD dwUnk) { JMP_THIS(0x4FC060); }
#else
    char PickedUpFlag(UnitClass* Who, DWORD dwUnk);
#endif

    /// VA: 0x00500510.
    FactoryClass* GetPrimaryFactory(AbstractType absID, bool naval, BuildCat buildCat) const;

    // zone: 0 = core, 1 = north, 2 = east, 3 = south, 4 = west
    /// VA: 0x00501AC0.
#if defined(RA2_YRPP_GAME)
    CellStruct* PickRandomCellInZone(CellStruct& outBuffer, int zone) const { JMP_THIS(0x501AC0); }
#else
    CellStruct* PickRandomCellInZone(CellStruct& outBuffer, int zone) const;
#endif

    CellStruct PickRandomCellInZone(int zone) const {
        CellStruct outBuffer;
        this->PickRandomCellInZone(outBuffer, zone);
        return outBuffer;
    }

    /// VA: 0x00500850.
#if defined(RA2_YRPP_GAME)
    void SetPrimaryFactory(FactoryClass* pFactory, AbstractType absID, bool naval, BuildCat buildCat) {
        JMP_THIS(0x500850);
    }
#else
    void SetPrimaryFactory(FactoryClass* pFactory, AbstractType absID, bool naval, BuildCat buildCat);
#endif

    // Sets the AI difficulty and the multipliers and delays derived from it;
    // the original returns the previous difficulty.
    // The Rules team delay for a difficulty; the original indexes the list
    // without a bound, a missing entry reads as 0 here.
    static int TeamDelayFor(int difficulty);
    /// VA: 0x004F6EC0.
#if defined(RA2_YRPP_GAME)
    void AssignHandicap(int difficulty) { JMP_THIS(0x4F6EC0); }
#else
    void AssignHandicap(int difficulty);
#endif
    /// VA: 0x004FCE00
    void InitializeForMultiplayer(int color, int country, int credits);

    // Derives Color from the main shade of the color scheme selected by ColorSchemeIndex. A negative or missing
    // scheme falls back to scheme 5 (white). A scheme without a palette is a native load failure and throws.
    /// VA: 0x0050B840
#if defined(RA2_YRPP_GAME)
    void UpdateColor() { JMP_THIS(0x50B840); }
#else
    void UpdateColor();
#endif
    // Derives LaserColor from Color: normalizes it to length 240, drops channels below 96 and normalizes again.
    /// VA: 0x0050BA00
#if defined(RA2_YRPP_GAME)
    void UpdateLaserColor() { JMP_THIS(0x50BA00); }
#else
    void UpdateLaserColor();
#endif

    /// VA: 0x50DF30
#if defined(RA2_YRPP_GAME)
    CoordStruct* GetBaseCenterCoords(CoordStruct* output) const { JMP_THIS(0x50DF30); }
#else
    CoordStruct* GetBaseCenterCoords(CoordStruct* output) const;
#endif
    /// VA: 0x504860
#if defined(RA2_YRPP_GAME)
    void MarkScouted(HouseClass* house) { JMP_THIS(0x504860); }
#else
    void MarkScouted(HouseClass* house);
#endif
    /// VA: 0x5054B0
#if defined(RA2_YRPP_GAME)
    void GenerateBasePlan() { JMP_THIS(0x5054B0); }
#else
    void GenerateBasePlan() noexcept;
#endif
    /// VA: 0x505180
#if defined(RA2_YRPP_GAME)
    void EnsureBasePlan() { JMP_THIS(0x505180); }
#else
    void EnsureBasePlan() noexcept;
#endif
    /// VA: 0x50C210
#if defined(RA2_YRPP_GAME)
    void InitializeBase() { JMP_THIS(0x50C210); }
#else
    void InitializeBase() noexcept;
#endif
    /// VA: 0x50C920
#if defined(RA2_YRPP_GAME)
    void GatherUnitsAtBase() { JMP_THIS(0x50C920); }
#else
    void GatherUnitsAtBase() noexcept;
#endif

    /// VA: 0x508150
#if defined(RA2_YRPP_GAME) && defined(__clang__) && defined(_M_IX86)
    void PredictEnemyForces() {
        __asm {
            mov ecx, this
            mov esp, ebp
            pop ebp
            mov eax, 0x508150
            jmp eax
        }
    }
#elif defined(RA2_YRPP_GAME)
    void PredictEnemyForces() { JMP_THIS(0x508150); }
#else
    void PredictEnemyForces() noexcept;
#endif
    // Original sret-style boundary: output must be an empty vector; returns &output.
    /// VA: 0x507b80
#if defined(RA2_YRPP_GAME) && defined(__clang__) && defined(_M_IX86)
    DynamicVectorClass<BuildingTypeClass*>* GetAntiAirDefenses(DynamicVectorClass<BuildingTypeClass*>& output, const DynamicVectorClass<BuildingTypeClass*>& owned) {
        __asm {
            mov ecx, this
            mov esp, ebp
            pop ebp
            mov eax, 0x507b80
            jmp eax
        }
    }
#elif defined(RA2_YRPP_GAME)
    DynamicVectorClass<BuildingTypeClass*>* GetAntiAirDefenses(DynamicVectorClass<BuildingTypeClass*>& output, const DynamicVectorClass<BuildingTypeClass*>& owned) { JMP_THIS(0x507b80); }
#else
    DynamicVectorClass<BuildingTypeClass*>* GetAntiAirDefenses(DynamicVectorClass<BuildingTypeClass*>& output, const DynamicVectorClass<BuildingTypeClass*>& owned) noexcept;
#endif
    /// VA: 0x507d70
#if defined(RA2_YRPP_GAME) && defined(__clang__) && defined(_M_IX86)
    DynamicVectorClass<BuildingTypeClass*>* GetAntiArmorDefenses(DynamicVectorClass<BuildingTypeClass*>& output, const DynamicVectorClass<BuildingTypeClass*>& owned) {
        __asm {
            mov ecx, this
            mov esp, ebp
            pop ebp
            mov eax, 0x507d70
            jmp eax
        }
    }
#elif defined(RA2_YRPP_GAME)
    DynamicVectorClass<BuildingTypeClass*>* GetAntiArmorDefenses(DynamicVectorClass<BuildingTypeClass*>& output, const DynamicVectorClass<BuildingTypeClass*>& owned) { JMP_THIS(0x507d70); }
#else
    DynamicVectorClass<BuildingTypeClass*>* GetAntiArmorDefenses(DynamicVectorClass<BuildingTypeClass*>& output, const DynamicVectorClass<BuildingTypeClass*>& owned) noexcept;
#endif
    /// VA: 0x507f60
#if defined(RA2_YRPP_GAME) && defined(__clang__) && defined(_M_IX86)
    DynamicVectorClass<BuildingTypeClass*>* GetAntiInfantryDefenses(DynamicVectorClass<BuildingTypeClass*>& output, const DynamicVectorClass<BuildingTypeClass*>& owned) {
        __asm {
            mov ecx, this
            mov esp, ebp
            pop ebp
            mov eax, 0x507f60
            jmp eax
        }
    }
#elif defined(RA2_YRPP_GAME)
    DynamicVectorClass<BuildingTypeClass*>* GetAntiInfantryDefenses(DynamicVectorClass<BuildingTypeClass*>& output, const DynamicVectorClass<BuildingTypeClass*>& owned) { JMP_THIS(0x507f60); }
#else
    DynamicVectorClass<BuildingTypeClass*>* GetAntiInfantryDefenses(DynamicVectorClass<BuildingTypeClass*>& output, const DynamicVectorClass<BuildingTypeClass*>& owned) noexcept;
#endif
    /// VA: 0x506ef0
#if defined(RA2_YRPP_GAME) && defined(__clang__) && defined(_M_IX86)
    bool ExpandBaseDefense(int index, DynamicVectorClass<CellStruct>* wallCells) {
        __asm {
            mov ecx, this
            mov esp, ebp
            pop ebp
            mov eax, 0x506ef0
            jmp eax
        }
    }
#elif defined(RA2_YRPP_GAME)
    bool ExpandBaseDefense(int index, DynamicVectorClass<CellStruct>* wallCells) { JMP_THIS(0x506ef0); }
#else
    bool ExpandBaseDefense(int index, DynamicVectorClass<CellStruct>* wallCells) noexcept;
#endif
    /// VA: 0x50c340
#if defined(RA2_YRPP_GAME) && defined(__clang__) && defined(_M_IX86)
    bool AddBaseProtectionWalls(int index) {
        __asm {
            mov ecx, this
            mov esp, ebp
            pop ebp
            mov eax, 0x50c340
            jmp eax
        }
    }
#elif defined(RA2_YRPP_GAME)
    bool AddBaseProtectionWalls(int index) { JMP_THIS(0x50c340); }
#else
    bool AddBaseProtectionWalls(int index) noexcept;
#endif
    /// VA: 0x5082c0
#if defined(RA2_YRPP_GAME) && defined(__clang__) && defined(_M_IX86)
    void AddBasePerimeterWalls() {
        __asm {
            mov ecx, this
            mov esp, ebp
            pop ebp
            mov eax, 0x5082c0
            jmp eax
        }
    }
#elif defined(RA2_YRPP_GAME)
    void AddBasePerimeterWalls() { JMP_THIS(0x5082c0); }
#else
    void AddBasePerimeterWalls() noexcept;
#endif
    /// VA: 0x506D50
#if defined(RA2_YRPP_GAME)
    static void YRPP_STDCALL AccumulateBaseDefense(const BuildingClass* building, int value, int* scores, const RectangleStruct* bounds) {
        JMP_STD(0x506D50);
    }
#else
    static void YRPP_STDCALL AccumulateBaseDefense(const BuildingClass* building, int value, int* scores, const RectangleStruct* bounds) noexcept;
#endif
    /// VA: 0x505FD0
#if defined(RA2_YRPP_GAME) && defined(_M_IX86)
    static int YRPP_FASTCALL ScoreBaseDefenseCell(const HouseClass* house, const CellStruct* cell, int index, int zone) {
        __asm {
            mov ecx, house
            mov edx, cell
            mov esp, ebp
            pop ebp
            mov eax, 0x505FD0
            jmp eax
        }
    }
#else
    static int YRPP_FASTCALL ScoreBaseDefenseCell(const HouseClass* house, const CellStruct* cell, int index, int zone);
#endif

    // Original fastcall scoring contract: callbacks must not propagate exceptions.
    using BaseCellScore = int (YRPP_FASTCALL*)(const HouseClass*, const CellStruct*, int, int);
    /// VA: 0x00505F80
    static int YRPP_FASTCALL ScoreBaseCell(const HouseClass* house, const CellStruct* cell, int index, int zone);
    /// VA: 0x005060B0
#if defined(RA2_YRPP_GAME) && defined(__clang__) && defined(_M_IX86)
    CellStruct* FindBaseBuildingSite(CellStruct* output, BuildingTypeClass* type, BaseCellScore score, int zone) {
        __asm {
            mov ecx, this
            mov esp, ebp
            pop ebp
            mov eax, 0x5060B0
            jmp eax
        }
    }
#elif defined(RA2_YRPP_GAME)
    CellStruct* FindBaseBuildingSite(CellStruct* output, BuildingTypeClass* type, BaseCellScore score, int zone) { JMP_THIS(0x5060B0); }
#else
    CellStruct* FindBaseBuildingSite(CellStruct* output, BuildingTypeClass* type, BaseCellScore score, int zone);
#endif
    /// VA: 0x00506B90
#if defined(RA2_YRPP_GAME) && defined(__clang__) && defined(_M_IX86)
    CellStruct* FindBaseUpgradeSite(CellStruct* output, BuildingTypeClass* type) {
        __asm {
            mov ecx, this
            mov esp, ebp
            pop ebp
            mov eax, 0x506B90
            jmp eax
        }
    }
#elif defined(RA2_YRPP_GAME)
    CellStruct* FindBaseUpgradeSite(CellStruct* output, BuildingTypeClass* type) { JMP_THIS(0x506B90); }
#else
    CellStruct* FindBaseUpgradeSite(CellStruct* output, BuildingTypeClass* type);
#endif
    /// VA: 0x0050B760
#if defined(RA2_YRPP_GAME) && defined(__clang__) && defined(_M_IX86)
    bool IsNearBase(BuildingTypeClass* type, const CellStruct& cell) const {
        __asm {
            mov ecx, this
            mov esp, ebp
            pop ebp
            mov eax, 0x50B760
            jmp eax
        }
    }
#elif defined(RA2_YRPP_GAME)
    bool IsNearBase(BuildingTypeClass* type, const CellStruct& cell) const { JMP_THIS(0x50B760); }
#else
    bool IsNearBase(BuildingTypeClass* type, const CellStruct& cell) const;
#endif

    /// VA: 0x50DEF0
    const CellStruct& GetBaseCenter() const {
        if (this->BaseCenter != CellStruct::Empty) {
            return this->BaseCenter;
        } else {
            return this->BaseSpawnCell;
        }
    }

    unsigned int GetAIDifficultyIndex() const { return static_cast<unsigned int>(this->AIDifficulty); }

    /*!
        At the moment, this function is really just a more intuitively named mask for
        this->Type->MultiplayPassive, but it might be expanded into something more
        complicated later.

        Primarily used to check if something is owned by the neutral house.
        \return true if house is passive in multiplayer, false if not.
        \author Renegade
        \date 01.03.10
    */
    bool IsNeutral() const { return this->Type->MultiplayPassive; }

    // Whether this house is equal to CurrentPlayer
    bool IsCurrentPlayer() const { return this == CurrentPlayer; }

    // Whether this house is equal to Observer
    bool IsObserver() const { return this == Observer; }

    bool inline IsInitiallyObserver() const { return this->IsHumanPlayer && (this->GetSpawnPosition() == -1); }

    // Whether CurrentPlayer is equal to Observer
    static bool IsCurrentPlayerObserver() { return CurrentPlayer && CurrentPlayer->IsObserver(); }

    /// VA: 0x0050BF60.
#if defined(RA2_YRPP_GAME)
    void CalculateCostMultipliers() { JMP_THIS(0x50BF60); }
#else
    void CalculateCostMultipliers();
#endif

    /// VA: 0x0050BD30.
#if defined(RA2_YRPP_GAME)
    double GetArmorMultiplier(TechnoTypeClass* pType) { JMP_THIS(0x50BD30); }
#else
    double GetArmorMultiplier(TechnoTypeClass* pType);
#endif

    /// VA: 0x004FCDC0.
    void ForceEnd() { JMP_THIS(0x4FCDC0); }

    /// VA: 0x004FF550.
    void RemoveTracking(TechnoClass* pTechno);

    /// VA: 0x004FF700.
    void AddTracking(TechnoClass* pTechno);

    /// VA: 0x004F9750
#if defined(RA2_YRPP_GAME)
    double GetWeedStoragePercentage() { JMP_THIS(0x4F9750); }
#else
    double GetWeedStoragePercentage();
#endif

    /// VA: 0x0050B1D0.
#if defined(RA2_YRPP_GAME)
    bool AISupers() { JMP_THIS(0x50B1D0); }
#else
    bool AISupers();
#endif

    /// VA: 0x004FCE80.
    void SellWall(CellStruct& cell, bool skipSound)
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x4FCE80);
    }
#else
        ;
#endif

    // Constructor
    /// VA: 0x004F54A0.
#if defined(RA2_YRPP_GAME)
    HouseClass(HouseTypeClass* pCountry) noexcept : HouseClass(noinit_t()) { JMP_THIS(0x4F54A0); }
#else
    HouseClass(HouseTypeClass* pCountry) noexcept;
#endif

protected:
    explicit __forceinline HouseClass(noinit_t) noexcept : AbstractClass(noinit_t()), Base(noinit_t()) {}

    // Properties

public:
    int ArrayIndex;
    HouseTypeClass* Type;
    DECLARE_PROPERTY(DynamicVectorClass<TagClass*>, RelatedTags);
    DECLARE_PROPERTY(DynamicVectorClass<BuildingClass*>, ConYards);
    DECLARE_PROPERTY(DynamicVectorClass<BuildingClass*>, Buildings);
    DECLARE_PROPERTY(DynamicVectorClass<BuildingClass*>, UnitRepairStations);
    DECLARE_PROPERTY(DynamicVectorClass<BuildingClass*>, Grinders);
    DECLARE_PROPERTY(DynamicVectorClass<BuildingClass*>, Absorbers);
    DECLARE_PROPERTY(DynamicVectorClass<BuildingClass*>, Bunkers);
    DECLARE_PROPERTY(DynamicVectorClass<BuildingClass*>, Occupiables);
    DECLARE_PROPERTY(DynamicVectorClass<BuildingClass*>, CloningVats);
    DECLARE_PROPERTY(DynamicVectorClass<BuildingClass*>, SecretLabs);
    DECLARE_PROPERTY(DynamicVectorClass<BuildingClass*>, PsychicDetectionBuildings);
    DECLARE_PROPERTY(DynamicVectorClass<BuildingClass*>, FactoryPlants);
    int CountResourceGatherers;
    int CountResourceDestinations;
    int CountWarfactories;
    int InfantrySelfHeal;
    int UnitsSelfHeal;
    DECLARE_PROPERTY(DynamicVectorClass<StartingTechnoStruct*>, StartingUnits);
    AIDifficulty
        AIDifficulty;  // be advised that it's reverse, Hard == 0 and Easy == 2. I'm sure Westwood has a good reason for this. Yep.
    double FirepowerMultiplier;    // used
    double GroundspeedMultiplier;  // unused ...
    double AirspeedMultiplier;
    double ArmorMultiplier;
    double ROFMultiplier;
    double CostMultiplier;
    double BuildTimeMultiplier;  // ... unused ends
    double RepairDelay;
    double BuildDelay;
    int IQLevel;
    int TechLevel;
    IndexBitfield<HouseClass*> AltAllies;  // ask question, receive brain damage
    int StartingCredits;                   // not sure how these are used // actual credits = this * 100
    Edge StartingEdge;
    DWORD AIState_1E4;
    int SideIndex;
    bool IsHumanPlayer;      // Is controlled by a human player.
    bool IsInPlayerControl;  // Is controlled by current player.
    bool Production;         // AI production has begun.
    bool AutocreateAllowed;
    bool NodeLogic_1F0;
    bool ShipYardConst_1F1;
    bool AITriggersActive;
    bool AutoBaseBuilding;
    bool DiscoveredByPlayer;
    bool Defeated;
    bool IsGameOver;
    bool IsWinner;
    bool IsLoser;
    bool CiviliansEvacuated;  // used by the CivEvac triggers
    bool FirestormActive;
    bool HasThreatNode;
    bool RecheckTechTree;
    int IPAddress;
    int TournamentTeamID;
    bool LostConnection;
    int SelectedPathIndex;
    WaypointPathClass* PlanningPaths[12];  // 12 paths for "planning mode"
    char Visionary;                        //??? exe says so
    bool MapIsClear;
    bool IsTiberiumShort;
    bool HasBeenSpied;
    bool HasBeenThieved;  // Something of this house has been entered by a Thief/VehicleThief
    bool Repairing;       // BuildingClass::Repair, handholder for hurr durf AI
    bool IsBuiltSomething;
    bool IsResigner;
    bool IsGiverUpper;
    bool AllToHunt;
    bool IsParanoid;
    bool IsToLook;
    int IQLevel2;  // no idea why we got this twice
    AIMode AIMode;
    DECLARE_PROPERTY(DynamicVectorClass<SuperClass*>, Supers);
    int LastBuiltBuildingType;
    int LastBuiltInfantryType;
    int LastBuiltAircraftType;
    int LastBuiltVehicleType;
    int AllowWinBlocks;                           // some ra1 residue map trigger-fu, should die a painful death
    DECLARE_PROPERTY(CDTimerClass, RepairTimer);  // for AI
    DECLARE_PROPERTY(CDTimerClass, AlertTimer);
    DECLARE_PROPERTY(CDTimerClass, BorrowedTime);
    DECLARE_PROPERTY(CDTimerClass, PowerBlackoutTimer);
    DECLARE_PROPERTY(CDTimerClass, RadarBlackoutTimer);
    bool Side2TechInfiltrated;  // asswards! whether this player has infiltrated stuff
    bool Side1TechInfiltrated;  // which is listed in [AI]->BuildTech
    bool Side0TechInfiltrated;  // and has the appropriate AIBasePlanningSide
    bool BarracksInfiltrated;
    bool WarFactoryInfiltrated;

    // these four are unused horrors
    // checking prerequisites:
    /*
        if(1 << this->Country->IndexInArray & item->RequiredHouses
            || (item->WhatAmI == abs_InfantryType && (item->RequiredHouses & this->InfantryAltOwner))
            || (item->WhatAmI == abs_UnitType && (item->RequiredHouses & this->UnitAltOwner))
            || (item->WhatAmI == abs_AircraftType && (item->RequiredHouses & this->AircraftAltOwner))
            || (item->WhatAmI == abs_BuildingType && (item->RequiredHouses & this->BuildingAltOwner))
        )
            { can build }
        */
    DWORD InfantryAltOwner;
    DWORD UnitAltOwner;
    DWORD AircraftAltOwner;
    DWORD BuildingAltOwner;

    int AirportDocks;
    int PoweredUnitCenters;
    int CreditsSpent;
    int HarvestedCredits;
    int StolenBuildingsCredits;
    int OwnedUnits;
    int OwnedNavy;
    int OwnedBuildings;
    int OwnedInfantry;
    int OwnedAircraft;
    DECLARE_PROPERTY(StorageClass, OwnedTiberium);
    int Balance;
    int TotalStorage;  // capacity of all building Storage
    DECLARE_PROPERTY(StorageClass, OwnedWeed);
    DWORD unknown_324;
    DECLARE_PROPERTY(UnitTrackerClass, BuiltAircraftTypes);
    DECLARE_PROPERTY(UnitTrackerClass, BuiltInfantryTypes);
    DECLARE_PROPERTY(UnitTrackerClass, BuiltUnitTypes);
    DECLARE_PROPERTY(UnitTrackerClass, BuiltBuildingTypes);
    DECLARE_PROPERTY(UnitTrackerClass, KilledAircraftTypes);
    DECLARE_PROPERTY(UnitTrackerClass, KilledInfantryTypes);
    DECLARE_PROPERTY(UnitTrackerClass, KilledUnitTypes);
    DECLARE_PROPERTY(UnitTrackerClass, KilledBuildingTypes);
    DECLARE_PROPERTY(UnitTrackerClass, CapturedBuildings);
    DECLARE_PROPERTY(UnitTrackerClass, CollectedCrates);  // YES, THIS IS HOW WW WASTES TONS OF RAM
    int NumAirpads;
    int NumBarracks;
    int NumWarFactories;
    int NumConYards;
    int NumShipyards;
    int NumOrePurifiers;
    float CostInfantryMult;
    float CostUnitsMult;
    float CostAircraftMult;
    float CostBuildingsMult;
    float CostDefensesMult;
    int PowerOutput;
    int PowerDrain;
    FactoryClass* Primary_ForAircraft;
    FactoryClass* Primary_ForInfantry;
    FactoryClass* Primary_ForVehicles;
    FactoryClass* Primary_ForShips;
    FactoryClass* Primary_ForBuildings;
    FactoryClass* Primary_Unused1;
    FactoryClass* Primary_Unused2;
    FactoryClass* Primary_Unused3;
    FactoryClass* Primary_ForDefenses;
    BYTE AircraftType_53D0;
    BYTE InfantryType_53D1;
    BYTE VehicleType_53D2;
    BYTE ShipType_53D3;
    BYTE BuildingType_53D4;
    BYTE unknown_53D5;
    BYTE unknown_53D6;
    BYTE unknown_53D7;
    BYTE DefenseType_53D8;
    BYTE unknown_53D9;
    BYTE unknown_53DA;
    BYTE unknown_53DB;
    UnitClass* OurFlagCarrier;
    CellStruct OurFlagCoords;
    // for endgame score screen
    int KilledUnitsOfHouses[20];  // 20 Houses only!
    int TotalKilledUnits;
    int KilledBuildingsOfHouses[20];  // 20 Houses only!
    int TotalKilledBuildings;
    int WhoLastHurtMe;
    CellStruct BaseSpawnCell;
    CellStruct BaseCenter;  // set by map action 137 and 138
    int Radius;
    DECLARE_PROPERTY_ARRAY(ZoneInfoStruct, ZoneInfos, 5);
    int LATime;
    int LAEnemy;
    BuildingClass* ToCapture;
    //	IndexBitfield<HouseTypeClass *> RadarVisibleTo; // these house types(!?!, fuck you WW) can see my radar
    IndexBitfield<HouseClass*>
        RadarVisibleTo;              // this crap is being rewritten to use house indices instead of house types
    int PointTotal;                  // Running score, based on units destroyed and units lost.
    QuarryType PreferredTargetType;  // Set via map action 35. The preferred object type to attack.
    CellStruct
        PreferredTargetCell;  // Set via map action 135 and 136. Used to override firing location of targettable SWs.
    CellStruct PreferredDefensiveCell;  // Set via map action 140 and 141, or when an AIDefendAgainst SW is launched.
    CellStruct
        PreferredDefensiveCell2;  // No known function sets this to a real value, but it would take precedence over the other.
    int PreferredDefensiveCellStartTime;  // The frame the PreferredDefensiveCell was set. Used to fire the Force Shield.

    // Used for: Counting objects ever owned
    // altered on each object's loss or gain
    // BuildLimit > 0 validation uses this
    DECLARE_PROPERTY(CounterClass, OwnedBuildingTypes);
    DECLARE_PROPERTY(CounterClass, OwnedUnitTypes);
    DECLARE_PROPERTY(CounterClass, OwnedInfantryTypes);
    DECLARE_PROPERTY(CounterClass, OwnedAircraftTypes);

    // Used for: Counting objects currently owned and on the map
    // altered on each object's loss or gain
    // AITriggerType condition uses this
    // original PrereqOverride check uses this
    // original Prerequisite check uses this
    // AuxBuilding check uses this
    DECLARE_PROPERTY(CounterClass, ActiveBuildingTypes);
    DECLARE_PROPERTY(CounterClass, ActiveUnitTypes);
    DECLARE_PROPERTY(CounterClass, ActiveInfantryTypes);
    DECLARE_PROPERTY(CounterClass, ActiveAircraftTypes);

    // Used for: Counting objects produced from Factory
    // not altered when things get taken over or removed
    // BuildLimit < 0 validation uses this
    DECLARE_PROPERTY(CounterClass, FactoryProducedBuildingTypes);
    DECLARE_PROPERTY(CounterClass, FactoryProducedUnitTypes);
    DECLARE_PROPERTY(CounterClass, FactoryProducedInfantryTypes);
    DECLARE_PROPERTY(CounterClass, FactoryProducedAircraftTypes);

    DECLARE_PROPERTY(CDTimerClass, AttackTimer);
    int InitialAttackDelay;  // both unused
    int EnemyHouseIndex;
    DECLARE_PROPERTY(DynamicVectorClass<AngerStruct>, AngerNodes);  //arghghghgh bugged
    DECLARE_PROPERTY(DynamicVectorClass<ScoutStruct>, ScoutNodes);  // filled with data which is never used, jood gob WW
    DECLARE_PROPERTY(CDTimerClass, AITimer);
    DECLARE_PROPERTY(CDTimerClass, Unknown_Timer_5640);
    int ProducingBuildingTypeIndex;
    int ProducingUnitTypeIndex;
    int ProducingInfantryTypeIndex;
    int ProducingAircraftTypeIndex;
    int RatioAITriggerTeam;
    int RatioTeamAircraft;
    int RatioTeamInfantry;
    int RatioTeamBuildings;
    int BaseDefenseTeamCount;
    DECLARE_PROPERTY_ARRAY(DropshipStruct, DropshipData, 3);
    int CurrentDropshipIndex;
    byte HasCloakingRanges;  // don't ask
    ColorStruct Color;
    ColorStruct LaserColor;
    DECLARE_PROPERTY(BaseClass, Base);
    bool RecheckPower;
    bool RecheckRadar;
    bool SpySatActive;
    bool IsBeingDrained;
    Edge Edge;
    CellStruct EMPTarget;
    CellStruct NukeTarget;
    IndexBitfield<HouseClass*> Allies;  // flags, one bit per HouseClass instance
    //                                 //-> 32 players possible here
    DECLARE_PROPERTY(CDTimerClass, DamageDelayTimer);
    DECLARE_PROPERTY(CDTimerClass, TeamDelayTimer);  // for AI attacks
    DECLARE_PROPERTY(CDTimerClass, TriggerDelayTimer);
    DECLARE_PROPERTY(CDTimerClass, SpeakAttackDelayTimer);
    DECLARE_PROPERTY(CDTimerClass, SpeakPowerDelayTimer);
    DECLARE_PROPERTY(CDTimerClass, SpeakMoneyDelayTimer);
    DECLARE_PROPERTY(CDTimerClass, SpeakMaxedDelayTimer);
    IAIHouse* AIGeneral;

    unsigned int ThreatPosedEstimates[130][130];  // BLARGH

    char PlainName
        [21];  // this defaults to the owner country's name in SP or <human player><computer player> in MP. Used as owner for preplaced map objects
    char UINameString[33];  // this contains the UIName= text from the INI! or
    wchar_t UIName
        [21];  // this contains the CSF string from UIName= above, or a copy of the country's UIName if not defined. Take note that this is shorter than the country's UIName can be...
    int ColorSchemeIndex;
    union {
        int StartingPoint;
        CellStruct StartingCell;  // Could it really be a CellStruct ? - Saved for backwards compatibility
    };
    IndexBitfield<HouseClass*> StartingAllies;
    union {
        DWORD unknown_16060;  // Retained for the original INI compatibility path.
        int* BaseDefenseScores;  // Temporary score field during 0x506EF0.
    };
    DECLARE_PROPERTY(DynamicVectorClass<IConnectionPoint*>, WaypointPath);
    DWORD unknown_1607C;
    DWORD unknown_16080;
    DWORD unknown_16084;
    double unused_16088;
    double unused_16090;
    DWORD padding_16098;
    float PredictionEnemyArmor;  // defaults to 0.33, AIForcePredictionFudge'd later
    float PredictionEnemyAir;
    float PredictionEnemyInfantry;
    int TotalOwnedInfantryCost;
    int TotalOwnedVehicleCost;
    int TotalOwnedAircraftCost;
    int PowerSurplus;
};
