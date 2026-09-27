/*
    Buildings
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/TechnoClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/BuildingLightClass.h"
#include "yrpp/StageClass.h"

class FactoryClass;
class InfantryClass;
class LightSourceClass;
class FoggedObjectClass;

enum class BStateType : unsigned int
{
    Construction = 0x0,
    Idle = 0x1,
    Active = 0x2,
    Full = 0x3,
    Aux1 = 0x4,
    Aux2 = 0x5,
    Count = 0x6,
    None = 0xFFFFFFFF,
};

class NOVTABLE BuildingClass : public TechnoClass
{
public:
    /// VA: 0x00459ED0
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIName() const override { JMP_THIS(0x00459ED0); }
#else
    const wchar_t* GetUIName() const override;
#endif

    /// VA: 0x00449440
#if defined(RA2_YRPP_GAME)
    Move IsCellOccupied(CellClass* cell, FacingType facing, int level,
                        CellClass* source, bool alternate) const override { JMP_THIS(0x449440); }
#else
    Move IsCellOccupied(CellClass* cell, FacingType facing, int level,
                        CellClass* source, bool alternate) const override;
#endif
    /// VA: 0x00440580
#if defined(RA2_YRPP_GAME)
    bool Unlimbo(const CoordStruct& where,DirType facing) override { JMP_THIS(0x00440580); }
#else
    bool Unlimbo(const CoordStruct& where,DirType facing) override;
#endif
    /// VA: 0x00445880
#if defined(RA2_YRPP_GAME)
    bool Limbo() override { JMP_THIS(0x00445880); }
#else
    bool Limbo() override;
#endif
    /// VA: 0x0043F180
#if defined(RA2_YRPP_GAME)
    bool Mark(MarkType value) override { JMP_THIS(0x0043F180); }
#else
    bool Mark(MarkType value) override;
#endif
    // Native map-reader status overload; no exceptions escape. Rejected records
    // are counted; a failure may retain earlier placements. Signature differs from the original entry.
    /// VA: 0x44F820
    static bool ReadINI(CCINIClass& ini,unsigned int& rejectedRecords,int firstHouse = 0) noexcept;
    // Original entries needed by map display; references only, no new ABI slots.
    // Update.
    /// VA: 0x0043FB20.
    // Unlimbo.
    /// VA: 0x00440580.
    /// VA: 0x0043CEA0
    bool DrawIfVisible(RectangleStruct* bounds, bool forced, DWORD extrasOnly) const override;
    // Mark.
    /// VA: 0x0043F180

    static const AbstractType AbsID = AbstractType::Building;

    /// VA: 0x447210
    Action MouseOverObject(const ObjectClass* object,bool ignoreForce=false) const override;
    /// VA: 0x447540
    Action MouseOverCell(const CellStruct* cell,bool checkFog=false,bool ignoreForce=false) const override;
    /// VA: 0x443410
    bool ObjectClickedAction(Action action,ObjectClass* target,bool ignoreForce) override;
    /// VA: 0x4436F0
    bool CellClickedAction(Action action,CellStruct* cell,CellStruct* follow,bool ignoreForce) override;
    /// VA: 0x44F5C0
    bool IsControllable() const override;
    /// VA: 0x455DA0
    bool IsUnitFactory() const override;

    /// VA: 0x455C20
#if defined(RA2_YRPP_GAME)
    RectangleStruct* GetRenderDimensions(RectangleStruct* output) override JMP_THIS(0x455C20);
#else
    RectangleStruct* GetRenderDimensions(RectangleStruct* output) override;
#endif

    // Static
    /// Global VA: 0x00A8EB40.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<BuildingClass*>, Array, 0xA8EB40u)
#else
    static DynamicVectorClass<BuildingClass*>& Array;
#endif

    // IPersist
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL GetClassID(CLSID* pClassID) R0;

    // IPersistStream
    // Destructor
    /// VA: unknown (legacy placeholder).
#if defined(RA2_YRPP_GAME)
    virtual ~BuildingClass() RX;
#else
    virtual ~BuildingClass();
#endif

    // AbstractClass
    /// VA: 0x0044E8F0
#if defined(RA2_YRPP_GAME)
    void PointerExpired(AbstractClass* object, bool removed) override { JMP_THIS(0x44E8F0); }
#else
    void PointerExpired(AbstractClass* object, bool removed) override;
#endif
    /// VA: 0x00451B40
#if defined(RA2_YRPP_GAME)
    void DetachAnim(AnimClass* anim) { JMP_THIS(0x451B40); }
#else
    void DetachAnim(AnimClass* anim);
#endif
    /// VA: unknown (legacy placeholder).
#if defined(RA2_YRPP_GAME)
    virtual AbstractType WhatAmI() const RT(AbstractType);
#else
    virtual AbstractType WhatAmI() const override;
#endif
    /// VA: unknown (legacy placeholder).
#if defined(RA2_YRPP_GAME)
    virtual int	Size() const R0;
#else
    virtual int Size() const override;
#endif

#if !defined(RA2_YRPP_GAME)
    ObjectTypeClass* GetType() const override;
    /// VA: 0x0043FB20
    void Update() override;
#endif
    using ObjectClass::GetCoords;
    /// VA: 0x00446FF0
    void SetRepairState(int state) override;
    /// VA: 0x00452630
    bool CanBeRepaired() const override;
    /// VA: 0x004494C0
    bool CanBeSold() const override;
    /// VA: 0x00447110
    void Sell(int control) override;
    /// VA: 0x00449C30
    int Mission_Selling() override;
    // Player-initiated repair portion of the original Repair_AI entry.
    /// VA: 0x00450630
    void UpdateRepair();
    /// VA: 0x458200
    void UpdateGarrison();
    /// VA: 0x458330
    void UpdateGarrisonAnimations();
    /// VA: 0x4526F0
    WeaponStruct* GetWeapon(int index) const override;
    /// VA: 0x453A70
    CoordStruct* vt_entry_300(CoordStruct* output,DWORD weapon) const override;
    /// VA: 0x453840
    CoordStruct* GetFLH(CoordStruct* output,int weapon,CoordStruct base) const override;
    /// VA: 0x458DB0
    bool IsArmed() const override;
    /// VA: 0x458E00
    int GetOccupyRangeBonus() const override;
    /// VA: 0x445F00
    AbstractClass* GreatestThreat(ThreatType threat,CoordStruct* origin,bool onlyEnemy) override;
    /// VA: 0x4496B0
    int Mission_Guard() override;
    /// VA: 0x44ACF0
    int Mission_Attack() override;
    /// VA: 0x44D880
    int Mission_Unload() override;
    /// VA: 0x454250
    bool ReadyToNextMission() const override;
    /// VA: 0x44D6A0
    bool EnterIdleMode(bool initial,bool resume) override;
    /// VA: 0x447F10
    FireError GetFireError(AbstractClass* target,int weapon,bool checkRange) const override;
    using ObjectClass::GetCenterCoords;
    using ObjectClass::GetRenderCoords;
    /// VA: 0x004500A0
#if defined(RA2_YRPP_GAME)
    CoordStruct* GetTargetCoords(CoordStruct* output) const override { JMP_THIS(0x4500A0); }
#else
    CoordStruct* GetTargetCoords(CoordStruct* output) const override;
#endif
    /// VA: 0x00447AC0
#if defined(RA2_YRPP_GAME)
    CoordStruct* GetCoords(CoordStruct* output) const override JMP_THIS(0x447AC0);
#else
    CoordStruct* GetCoords(CoordStruct* output) const override;
#endif
    /// VA: 0x00459EF0
#if defined(RA2_YRPP_GAME)
    CoordStruct* GetRenderCoords(CoordStruct* output) const override JMP_THIS(0x459EF0);
#else
    CoordStruct* GetRenderCoords(CoordStruct* output) const override;
#endif
    // ObjectClass
    /// VA: 0x00452820
    void AddOverpowerer(InfantryClass* infantry) { JMP_THIS(0x452820); }
    /// VA: 0x00459C00
#if defined(RA2_YRPP_GAME)
    bool CanBeSelectedNow() const override { JMP_THIS(0x459C00); }
#else
    bool CanBeSelectedNow() const override;
#endif
    /// VA: 0x00426410
    bool IsStandingStill() const override { return true; } // original Occupies_Cells
    /// VA: 0x00453D60
#if defined(RA2_YRPP_GAME)
    void MarkAllOccupationBits(const CoordStruct& coords) override { JMP_THIS(0x453D60); }
#else
    void MarkAllOccupationBits(const CoordStruct& coords) override;
#endif
    /// VA: 0x00453DC0
#if defined(RA2_YRPP_GAME)
    void UnmarkAllOccupationBits(const CoordStruct& coords) override { JMP_THIS(0x453DC0); }
#else
    void UnmarkAllOccupationBits(const CoordStruct& coords) override;
#endif
    /// VA: 0x0043D290
    void DrawIt(Point2D* point,RectangleStruct* bounds) const override;
    /// VA: 0x0043E900
    int GetZAdjustment() const override;
    /// VA: 0x004513D0
#if defined(RA2_YRPP_GAME)
    SHPStruct* GetImage() const override JMP_THIS(0x004513D0);
#else
    SHPStruct* GetImage() const override;
#endif
    /// VA: 0x00449410
#if defined(RA2_YRPP_GAME)
    int GetYSort() const override { JMP_THIS(0x00449410); }
#else
    int GetYSort() const override;
#endif
    /// VA: 0x004581F0
#if defined(RA2_YRPP_GAME)
    int GetOccupantCount() const override { JMP_THIS(0x004581F0); }
#else
    int GetOccupantCount() const override;
#endif
    // MissionClass
    // TechnoClass
    /// VA: 0x004527D0
#if defined(RA2_YRPP_GAME)
    bool HasTurret() const override JMP_THIS(0x004527D0);
#else
    bool HasTurret() const override;
#endif
    /// VA: 0x004555D0
#if defined(RA2_YRPP_GAME)
    bool IsPowerOnline() const override JMP_THIS(0x004555D0);
#else
    bool IsPowerOnline() const override;
#endif
    /// VA: 0x00458DD0
#if defined(RA2_YRPP_GAME)
    bool CanOccupyFire() const override JMP_THIS(0x00458DD0);
#else
    bool CanOccupyFire() const override;
#endif
    void RadarTrackingStart() override; // 456580, one record per foundation pixel.
    void RadarTrackingStop() override; // 4565E0.
    void RadarTrackingFlash() override; // 456640.
    /// VA: 0x00451330
#if defined(RA2_YRPP_GAME)
    int GetCrewCount() const override JMP_THIS(0x451330);
#else
    int GetCrewCount() const override;
#endif
    /// VA: unknown (legacy placeholder).
    virtual void Destroyed(ObjectClass* Killer) RX;
    /// VA: unknown (legacy placeholder).
    virtual bool ForceCreate(CoordStruct& coord, DWORD dwUnk = 0) R0;

    // BuildingClass
    /// VA: 0x44EFB0
    virtual CellStruct FindExitCell(FootClass* product,CellStruct preferred) const;
    /// VA: 0x443C60
    KickOutResult KickOutUnit(TechnoClass* product,CellStruct preferred) override;
    /// VA: 0x449540
    bool ClearFactoryBib();
    int MissionFactoryUnload();
    /// VA: 0x43C2D0
    RadioCommand ReceiveCommand(TechnoClass* sender,RadioCommand command,AbstractClass*& data) override;
    /// VA: unknown (legacy placeholder).
    virtual int DistanceToDockingCoord(ObjectClass* pObj) const R0;
    /// VA: 0x445F80
    virtual void Place(bool captured);
    /// VA: 0x458A00
    bool IsCellImpassable(CellClass* cell) const;
    /// VA: unknown (legacy placeholder).
    virtual void UpdateConstructionOptions() RX;
    /// VA: 0x0043DA80
    virtual void Draw(const Point2D& point, const RectangleStruct& rect);
    /// VA: unknown (legacy placeholder).
    /// VA: 0x43ED40
    virtual DirStruct FireAngleTo(AbstractClass* target) const;
    // Original Do_Destruction slot (legacy spelling retained).
    /// VA: 0x004415F0
#if defined(RA2_YRPP_GAME)
    virtual void Destory(TechnoClass* lastContact, TechnoClass* source, bool noSurvivor, const CellStruct* footprint) JMP_THIS(0x004415F0);
#else
    virtual void Destory(TechnoClass* lastContact, TechnoClass* source, bool noSurvivor, const CellStruct* footprint);
#endif
    /// VA: 0x00441F60
    void LeaveRubble();
    /// VA: unknown (legacy placeholder).
    virtual bool TogglePrimaryFactory() R0;
    /// VA: unknown (legacy placeholder).
    virtual void SensorArrayActivate(CellStruct cell=CellStruct::Empty) RX;
    /// VA: unknown (legacy placeholder).
    virtual void SensorArrayDeactivate(CellStruct cell=CellStruct::Empty) RX;
    /// VA: unknown (legacy placeholder).
    virtual void DisguiseDetectorActivate(CellStruct cell=CellStruct::Empty) RX;
    /// VA: unknown (legacy placeholder).
    virtual void DisguiseDetectorDeactivate(CellStruct cell=CellStruct::Empty) RX;
    /// VA: unknown (legacy placeholder).
    virtual int AlwaysZero() R0;

    // non-vt

    /// VA: 0x0043C0D0
#if defined(RA2_YRPP_GAME)
    void CreateDamageFires() noexcept { JMP_THIS(0x0043C0D0); }
#else
    // Allocation failure skips that fire; exceptions cannot escape.
    void CreateDamageFires() noexcept;
#endif
    // Health-transition/cleanup subset of Update, also used when leaving the map.
    void UpdateDamageFires() noexcept;

    /// VA: 0x004509D0.
#if defined(RA2_YRPP_GAME)
    void UpdateAnimations()
        { JMP_THIS(0x4509D0); }
#else
    void UpdateAnimations();
#endif

    /// VA: 0x0043EF90.
#if defined(RA2_YRPP_GAME)
    int GetCurrentFrame()
        { JMP_THIS(0x43EF90); }
#else
    int GetCurrentFrame();
#endif

    /// VA: 0x00457A10.
    bool IsAllFogged() const
        { JMP_THIS(0x457A10); }

    /// VA: 0x00443860.
    void SetRallypoint(CellStruct* pTarget, bool bPlayEVA)
        { JMP_THIS(0x443860); }

    /// VA: 0x00457AA0.
    void FreezeInFog(DynamicVectorClass<FoggedObjectClass*>* pFoggedArray, CellClass* pCell, bool Visible)
        { JMP_THIS(0x457AA0); }

    // power up
    /// VA: 0x00452260.
    void GoOnline()
        { JMP_THIS(0x452260); }
    /// VA: 0x00452360.
    void GoOffline()
        { JMP_THIS(0x452360); }

    /// VA: 0x0044E7B0.
#if defined(RA2_YRPP_GAME)
    int GetPowerOutput() const { JMP_THIS(0x44E7B0); }
#else
    int GetPowerOutput() const;
#endif
    /// VA: 0x0044E880.
#if defined(RA2_YRPP_GAME)
    int GetPowerDrain() const { JMP_THIS(0x44E880); }
#else
    int GetPowerDrain() const;
#endif

    // Firewall aka FirestormWall
    // depending on what facings of this building
    // are connected to another FWall,
    // returns the index of the image file
    // to draw.
    /// VA: 0x00455B90.
    DWORD GetFWFlags() const
        { JMP_THIS(0x455B90); }

    /// VA: 0x004533A0.
    void CreateEndPost(bool arg)
        { JMP_THIS(0x4533A0); }

    // kick out content
    /// VA: 0x004593A0.
    void UnloadBunker()
        { JMP_THIS(0x4593A0); }

    // content is dead - chronosphered away or died inside
    /// VA: 0x00459470.
    void ClearBunker()
        { JMP_THIS(0x459470); }

    // kick out content, remove anims, etc... don't ask me what's different from kick out
    /// VA: 0x004595C0.
    void EmptyBunker()
        { JMP_THIS(0x4595C0); }

    // called after destruction - CrateBeneath, resetting foundation'ed cells
    /// VA: 0x00441F60.
    void AfterDestruction()
        { JMP_THIS(0x441F60); }

    // Emits smoke at the configured refinery chimney offsets.
    /// VA: 0x00459900
    void UpdateRefinerySmokeSystems() override;

    // destroys the specific animation (active, turret, special, etc)
    /// VA: 0x00451E40.
#if defined(RA2_YRPP_GAME)
    void DestroyNthAnim(BuildingAnimSlot Slot)
        { JMP_THIS(0x451E40); }
#else
    void DestroyNthAnim(BuildingAnimSlot Slot);
#endif

    // the game picks the slot's normal/damaged/garrisoned name from these two states
    /// VA: 0x00451750.
#if defined(RA2_YRPP_GAME)
    void PlayNthAnim(BuildingAnimSlot Slot, bool Damaged, bool Garrisoned, int effectDelay = 0)
        { JMP_THIS(0x451750); }
#else
    void PlayNthAnim(BuildingAnimSlot Slot, bool Damaged, bool Garrisoned, int effectDelay = 0);
#endif

    // derives the two states the way the game's own callers do. mind that a bool
    // passed here is an effectDelay, not a state -- use the four-argument form.
    void PlayNthAnim(BuildingAnimSlot Slot, int effectDelay = 0)
        { this->PlayNthAnim(Slot, !this->IsGreenHP(), this->GetOccupantCount() > 0, effectDelay); }

    /// VA: 0x004517D0
    void SetAnimCoords();

    /// VA: 0x00451890.
#if defined(RA2_YRPP_GAME)
    void PlayAnim(const char* animName, BuildingAnimSlot Slot, bool Damaged, bool Garrisoned, int effectDelay = 0)
        { JMP_THIS(0x451890); }
#else
    void PlayAnim(const char* animName, BuildingAnimSlot Slot, bool Damaged, bool Garrisoned, int effectDelay = 0);
#endif

    // changes between building's damaged and undamaged animations.
    /// VA: 0x00451EE0.
#if defined(RA2_YRPP_GAME)
    void ToggleDamagedAnims(bool isDamaged)
        { JMP_THIS(0x451EE0); }
#else
    void ToggleDamagedAnims(bool isDamaged);
#endif

    // when the building is switched off
    /// VA: 0x00452480.
    void DisableStuff()
        { JMP_THIS(0x452480); }

    // when the building is switched on
    /// VA: 0x00452410.
    void EnableStuff()
        { JMP_THIS(0x452410); }

    /// VA: 0x00452000
    void UpdateAnimAppearance();

    // when the building is warped
    /// VA: 0x004521C0.
    void DisableTemporal()
        { JMP_THIS(0x4521C0); }

    // when the building warped back in
    /// VA: 0x00452210.
    void EnableTemporal()
        { JMP_THIS(0x452210); }

    // returns Type->SuperWeapon, if its AuxBuilding is satisfied
    /// VA: 0x00457630.
#if defined(RA2_YRPP_GAME)
    int FirstActiveSWIdx() const
        { JMP_THIS(0x457630); }
#else
    int FirstActiveSWIdx() const;
#endif

    /// VA: 0x0043EF90.
#if defined(RA2_YRPP_GAME)
    int GetShapeNumber() const
        { JMP_THIS(0x43EF90); }
#else
    int GetShapeNumber() const;
#endif

    /// VA: 0x00447780.
#if defined(RA2_YRPP_GAME)
    void BeginMode(BStateType bType)
        { JMP_THIS(0x447780); }
#else
    void BeginMode(BStateType bType);
#endif

    // returns Type->SuperWeapon2, if its AuxBuilding is satisfied
    /// VA: 0x00457690.
#if defined(RA2_YRPP_GAME)
    int SecondActiveSWIdx() const
        { JMP_THIS(0x457690); }
#else
    int SecondActiveSWIdx() const;
#endif

    /// VA: 0x0044ABD0.
    void FireLaser(CoordStruct Coords)
        { JMP_THIS(0x44ABD0); }

    /// VA: 0x0070FEC0.
#if defined(RA2_YRPP_GAME)
    bool IsBeingDrained() const
        { JMP_THIS(0x70FEC0); }
#else
    bool IsBeingDrained() const;
#endif

    /// VA: 0x00458E50.
    bool UpdateBunker()
        { JMP_THIS(0x458E50); }

    /// VA: 0x004585C0.
    void KillOccupants(TechnoClass* pAssaulter);
    /// VA: 0x00457DE0
    void UnloadOccupants(bool scatter,bool force);

    // returns false if this is a gate that needs time to open, true otherwise
    /// VA: 0x00452540.
    bool MakeTraversable()
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x452540); }
#else
        ;
#endif

    /// VA: 0x00457A10.
    bool CheckFog()
        { JMP_THIS(0x457A10); }

    /// VA: 0x00453BF0
#if defined(RA2_YRPP_GAME)
    CoordStruct* GetVoxelFireCoords(CoordStruct* output,int weapon,bool justFired) const noexcept
        { JMP_THIS(0x00453BF0); }
#else
    CoordStruct* GetVoxelFireCoords(CoordStruct* output,int weapon,bool justFired) const noexcept;
#endif

    /// VA: 0x00458810
#if defined(RA2_YRPP_GAME)
    Matrix3D* GetVoxelBarrelOffsetMatrix(Matrix3D& ret)
        { JMP_THIS(0x458810); }
#else
    Matrix3D* GetVoxelBarrelOffsetMatrix(Matrix3D& ret);
#endif

    // returns false if this is a gate that is closed, true otherwise
    /// VA: 0x004525F0.
#if defined(RA2_YRPP_GAME)
    bool IsTraversable() const
        { JMP_THIS(0x4525F0); }
#else
    bool IsTraversable() const;
#endif

    /// VA: 0x0043E7B0
#if defined(RA2_YRPP_GAME)
    void DrawInfoTipAndSpiedSelection(Point2D* pLocation, RectangleStruct* pRect) const
        { JMP_THIS(0x43E7B0); }
#else
    void DrawInfoTipAndSpiedSelection(Point2D* pLocation, RectangleStruct* pRect) const;
#endif

    /// VA: 0x00452670
#if defined(RA2_YRPP_GAME)
    bool CanUpgrade(const BuildingTypeClass* upgrade, const HouseClass* owner) const
        { JMP_THIS(0x452670); }
#else
    bool CanUpgrade(const BuildingTypeClass* upgrade, const HouseClass* owner) const noexcept;
#endif

    // helpers
    bool HasSuperWeapon(int index) const {
        if(this->Type->HasSuperWeapon(index)) {
            return true;
        }
        for(auto pType : this->Upgrades) {
            if(pType && pType->HasSuperWeapon(index)) {
                return true;
            }
        }
        return false;
    }

    TechnoTypeClass* GetSecretProduction() const;

    AnimClass*& GetAnim(BuildingAnimSlot slot) {
        return this->Anims[static_cast<int>(slot)];
    }

    AnimClass* const& GetAnim(BuildingAnimSlot slot) const {
        return this->Anims[static_cast<int>(slot)];
    }

    bool& GetAnimState(BuildingAnimSlot slot) {
        return this->AnimStates[static_cast<int>(slot)];
    }

    bool const& GetAnimState(BuildingAnimSlot slot) const {
        return this->AnimStates[static_cast<int>(slot)];
    }

    /// VA: 0x00457CE0.
    bool CanBeOccupiedBy(InfantryClass* pInfantry) const;

    /// VA: 0x004571E0
    void SpiedBy(HouseClass* house) { JMP_THIS(0x4571E0); }
    /// VA: 0x00457620
    bool IsStrange() const override { return Type->IsVehicle(); }
    /// VA: 0x004576F0
    void GotHijacked() override;
    /// VA: 0x00456E00
    void Flash(int duration) override;
    /// VA: 0x00456F80
    int GetFlashingIntensity(int currentIntensity) const override;
    /// VA: 0x0044EBF0
    void Disappear(bool permanently) override;
    /// VA: 0x00448260
    bool SetOwningHouse(HouseClass* house,bool announce=true) override;
    /// VA: 0x00442230
    DamageState ReceiveDamage(int* damage,int distance,WarheadTypeClass* warhead,ObjectClass* source,
        bool ignoreDefenses,bool preventEscape,HouseClass* sourceHouse) override;

    /// VA: 0x004566B0.
    int GetRadialIndicatorRange() const
        { JMP_THIS(0x4566B0); }

    // Constructor
    /// VA: 0x0043B740.
#if defined(RA2_YRPP_GAME)
    BuildingClass(BuildingTypeClass* pType, HouseClass* pOwner) noexcept
        : BuildingClass(noinit_t())
    { JMP_THIS(0x43B740); }
#else
    BuildingClass(BuildingTypeClass* pType, HouseClass* pOwner) noexcept;
#endif

protected:
    explicit __forceinline BuildingClass(noinit_t) noexcept
        : TechnoClass(noinit_t())
    { }

    // Properties

public:

    BuildingTypeClass* Type;
    FactoryClass* Factory;
    CDTimerClass C4Timer;
    int BState;
    int QueueBState;
    DWORD OwnerCountryIndex;
    InfantryClass* C4AppliedBy;
    DWORD unknown_544;
    AnimClass* FirestormAnim; //pointer
    AnimClass* PsiWarnAnim; //pointer
    CDTimerClass FactoryRetryTimer;

// see eBuildingAnims above for slot index meanings
    AnimClass * Anims [0x15];
    bool AnimStates [0x15]; // one flag for each of the above anims (whether the anim was enabled when power went offline?)

protected:
    char align_5C5[3];
public:

    AnimClass * DamageFireAnims [0x8];

    bool RequiresDamageFires; // current low-health fire state; Update acts on transitions
    //5E8 - 5F8 ????????
    BuildingTypeClass * Upgrades [0x3];

    int FiringSWType; // type # of sw being launched
    DWORD unknown_5FC;
    BuildingLightClass* Spotlight;
    RateTimer GateTimer;
    LightSourceClass * LightSource; // tiled light , LightIntensity > 0
    DWORD LaserFenceFrame; // 0-7 for active directionals, 8/12 for offline ones, check ntfnce.shp or whatever
    DWORD FirestormWallFrame; // anim data for firestorm active animations
    StageClass RepairProgress; // for hospital, armory, unitrepair etc
    RectangleStruct unknown_rect_63C;
    CoordStruct unknown_coord_64C;
    int unknown_int_658;
    DWORD unknown_65C;
    bool HasPower;
    bool IsOverpowered;

    // each powered unit controller building gets this set on power activation and unset on power outage
    bool RegisteredAsPoweredUnitSource;

    DWORD SupportingPrisms;
    bool HasExtraPowerBonus;
    bool HasExtraPowerDrain;
    DynamicVectorClass<InfantryClass*> Overpowerers;
    DynamicVectorClass<InfantryClass*> Occupants;
    int FiringOccupantIndex; // which occupant should get XP, which weapon should be fired (see 6FF074)

    AudioController Audio7;
    AudioController Audio8;

    bool WasOnline; // the the last state when Update()ing. if this changed since the last Update(), UpdatePowered is called.
    bool ShowRealName; // is also NOMINAL under [Structures]
    bool BeingProduced; // is also AI_REBUILDABLE under [Structures]
    bool ShouldRebuild; // is also AI_REPAIRABLE under [Structures]
    bool HasEngineer; // used to pass the NeedsEngineer check
    CDTimerClass CashProductionTimer;
    bool AI_Sellable; // AI_SELLABLE under [Structures]
    bool IsReadyToCommence;
    bool NeedsRepairs; // AI handholder for repair logic,
    bool C4Applied;
    bool NoCrew;
    bool unknown_bool_6E1;
    bool unknown_bool_6E2;
    bool HasBeenCaptured; // has this building changed ownership at least once? affects crew and repair.
    bool ActuallyPlacedOnMap;
    bool unknown_bool_6E5;
    bool IsDamaged; // AI handholder for repair logic,
    bool IsFogged;
    bool IsBeingRepaired; // show animooted repair wrench
    bool HasBuildUp;
    bool StuffEnabled; // status set by EnableStuff() and DisableStuff()
    char HasCloakingData; // some fugly buffers
    byte CloakRadius; // from Type->CloakRadiusInCells
    char Translucency;
    DWORD StorageFilledSlots; // the old "silo needed" logic
    TechnoTypeClass * SecretProduction; // randomly assigned secret lab bonus, used if SecretInfantry, SecretUnit, and SecretBuilding are null
    ColorStruct ColorAdd;
    int unknown_int_6FC;
    short unknown_short_700;
    BYTE UpgradeLevel; // Original CanUpgrade compares this byte as signed.
    char GateStage;
    PrismChargeState PrismStage;
    CoordStruct PrismTargetCoords;
    DWORD DelayBeforeFiring;

    TankBunkerState TankBunkerState; // used in UpdateBunker and friends
};
