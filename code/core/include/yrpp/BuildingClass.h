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

enum class BStateType : unsigned int {
    Construction = 0x0,
    Idle = 0x1,
    Active = 0x2,
    Full = 0x3,
    Aux1 = 0x4,
    Aux2 = 0x5,
    Count = 0x6,
    None = 0xFFFFFFFF,
};

class NOVTABLE BuildingClass : public TechnoClass {
public:
    using TechnoClass::GetRealFacing;
    using TechnoClass::TurretFacing;
    /// VA: 0x00453E20
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL Load(IStream* stream) override { JMP_STD(0x00453E20); }
#else
    HRESULT YRPP_STDCALL Load(IStream* stream) override;
#endif
    /// VA: 0x00454190
#if defined(RA2_YRPP_GAME)
    HRESULT YRPP_STDCALL Save(IStream* stream, BOOL clearDirty) override { JMP_STD(0x00454190); }
#else
    HRESULT YRPP_STDCALL Save(IStream* stream, BOOL clearDirty) override;
#endif
    /// VA: 0x00442C40
#if defined(RA2_YRPP_GAME)
    void Init() override { JMP_THIS(0x00442C40); }
#else
    void Init() override;
#endif
    /// VA: 0x00454260
#if defined(RA2_YRPP_GAME)
    void ComputeCRC(CRCEngine& crc) const override { JMP_THIS(0x00454260); }
#else
    void ComputeCRC(CRCEngine& crc) const override;
#endif
    /// VA: 0x004544A0
#if defined(RA2_YRPP_GAME)
    VisualType VisualCharacter(VARIANT_BOOL raw, HouseClass* asking) const override { JMP_THIS(0x004544A0); }
#else
    VisualType VisualCharacter(VARIANT_BOOL raw, HouseClass* asking) const override;
#endif
    /// VA: 0x0044F640
#if defined(RA2_YRPP_GAME)
    CoordStruct* GetExitCoords(CoordStruct* output, DWORD unused) const override { JMP_THIS(0x0044F640); }
#else
    CoordStruct* GetExitCoords(CoordStruct* output, DWORD unused) const override;
#endif
    /// VA: 0x0043D030
#if defined(RA2_YRPP_GAME)
    void DrawAgain(const Point2D& location, const RectangleStruct& bounds) const override { JMP_THIS(0x0043D030); }
#else
    void DrawAgain(const Point2D& location, const RectangleStruct& bounds) const override;
#endif
    /// VA: 0x00456750
#if defined(RA2_YRPP_GAME)
    void DrawRadialIndicator(DWORD drawLine) override { JMP_THIS(0x00456750); }
#else
    void DrawRadialIndicator(DWORD drawLine) override;
#endif
    /// VA: 0x00457C90
#if defined(RA2_YRPP_GAME)
    DamageState IronCurtain(int duration, HouseClass* source, bool forceShield) override { JMP_THIS(0x00457C90); }
#else
    DamageState IronCurtain(int duration, HouseClass* source, bool forceShield) override;
#endif
    /// VA: 0x00449A40
#if defined(RA2_YRPP_GAME)
    int Mission_AreaGuard() override { JMP_THIS(0x00449A40); }
#else
    int Mission_AreaGuard() override;
#endif
    /// VA: 0x0044C980
#if defined(RA2_YRPP_GAME)
    int Mission_Missile() override { JMP_THIS(0x0044C980); }
#else
    int Mission_Missile() override;
#endif
    /// VA: 0x00457770
#if defined(RA2_YRPP_GAME)
    bool IsReadyToCloak() const override { JMP_THIS(0x00457770); }
#else
    bool IsReadyToCloak() const override;
#endif
    /// VA: 0x004578C0
#if defined(RA2_YRPP_GAME)
    bool ShouldNotBeCloaked() const override { JMP_THIS(0x004578C0); }
#else
    bool ShouldNotBeCloaked() const override;
#endif
    /// VA: 0x00445E50
#if defined(RA2_YRPP_GAME)
    DirStruct* TurretFacing(DirStruct* output) const override { JMP_THIS(0x00445E50); }
#else
    DirStruct* TurretFacing(DirStruct* output) const override;
#endif
    /// VA: 0x0044D700
#if defined(RA2_YRPP_GAME)
    int GetPipFillLevel() const override { JMP_THIS(0x0044D700); }
#else
    int GetPipFillLevel() const override;
#endif
    /// VA: 0x0043E940
#if defined(RA2_YRPP_GAME)
    DirStruct* vt_entry_2C8(DirStruct* output, AbstractClass* target) const override { JMP_THIS(0x0043E940); }
#else
    DirStruct* vt_entry_2C8(DirStruct* output, AbstractClass* target) const override;
#endif
    /// VA: 0x00459870
#if defined(RA2_YRPP_GAME)
    int GetAntiAirValue() const override { JMP_THIS(0x00459870); }
#else
    int GetAntiAirValue() const override;
#endif
    /// VA: 0x00459880
#if defined(RA2_YRPP_GAME)
    int GetAntiArmorValue() const override { JMP_THIS(0x00459880); }
#else
    int GetAntiArmorValue() const override;
#endif
    /// VA: 0x00459890
#if defined(RA2_YRPP_GAME)
    int GetAntiInfantryValue() const override { JMP_THIS(0x00459890); }
#else
    int GetAntiInfantryValue() const override;
#endif
    /// VA: 0x00459DA0
#if defined(RA2_YRPP_GAME)
    ZGradient GetZGradient() const override { JMP_THIS(0x00459DA0); }
#else
    ZGradient GetZGradient() const override;
#endif
    /// VA: 0x00459DB0
#if defined(RA2_YRPP_GAME)
    CellStruct GetLastFlightMapCoords() const override { JMP_THIS(0x00459DB0); }
#else
    CellStruct GetLastFlightMapCoords() const override;
#endif
    /// VA: 0x0044D7D0
#if defined(RA2_YRPP_GAME)
    DirStruct* GetRealFacing(DirStruct* output) const override { JMP_THIS(0x0044D7D0); }
#else
    DirStruct* GetRealFacing(DirStruct* output) const override;
#endif
    /// VA: 0x0044EB10
#if defined(RA2_YRPP_GAME)
    InfantryTypeClass* GetCrew() const override { JMP_THIS(0x0044EB10); }
#else
    InfantryTypeClass* GetCrew() const override;
#endif
    /// VA: 0x00457020
#if defined(RA2_YRPP_GAME)
    bool IsRadarVisible(int* detection) const override { JMP_THIS(0x00457020); }
#else
    bool IsRadarVisible(int* detection) const override;
#endif
    /// VA: 0x00459C20
#if defined(RA2_YRPP_GAME)
    int VoiceDeploy() override { JMP_THIS(0x00459C20); }
#else
    int VoiceDeploy() override;
#endif
    /// VA: 0x00443B90
#if defined(RA2_YRPP_GAME)
    void SetTarget(AbstractClass* target) override { JMP_THIS(0x00443B90); }
#else
    void SetTarget(AbstractClass* target) override;
#endif

    /// VA: 0x00459ED0
#if defined(RA2_YRPP_GAME)
    const wchar_t* GetUIName() const override { JMP_THIS(0x00459ED0); }
#else
    const wchar_t* GetUIName() const override;
#endif

    /// VA: 0x00449440
#if defined(RA2_YRPP_GAME)
    Move IsCellOccupied(CellClass* cell, FacingType facing, int level, CellClass* source,
                        bool alternate) const override {
        JMP_THIS(0x449440);
    }
#else
    Move IsCellOccupied(CellClass* cell, FacingType facing, int level, CellClass* source,
                        bool alternate) const override;
#endif
    /// VA: 0x00440580
#if defined(RA2_YRPP_GAME)
    bool Unlimbo(const CoordStruct& where, DirType facing) override { JMP_THIS(0x00440580); }
#else
    bool Unlimbo(const CoordStruct& where, DirType facing) override;
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
    // Reads the map's records and creates the objects. Malformed records are skipped;
    // allocation failures and unreadable data throw, and earlier placements remain.
    /// VA: 0x0044F820
#if defined(RA2_YRPP_GAME)
    static void YRPP_FASTCALL ReadINI(CCINIClass* pINI) { JMP_STD(0x44F820); }
#else
    static void YRPP_FASTCALL ReadINI(CCINIClass* pINI);
#endif
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
    Action MouseOverObject(const ObjectClass* object, bool ignoreForce = false) const override;
    /// VA: 0x447540
    Action MouseOverCell(const CellStruct* cell, bool checkFog = false, bool ignoreForce = false) const override;
    /// VA: 0x443410
    bool ObjectClickedAction(Action action, ObjectClass* target, bool ignoreForce) override;
    /// VA: 0x4436F0
    bool CellClickedAction(Action action, CellStruct* cell, CellStruct* follow, bool ignoreForce) override;
    /// VA: 0x44F5C0
    bool IsControllable() const override;
    /// VA: 0x455DA0
    bool IsUnitFactory() const override;

    /// VA: 0x455D50
#if defined(RA2_YRPP_GAME)
    void SetDestination(AbstractClass* target, bool immediate) override { JMP_THIS(0x455D50); }
#else
    void SetDestination(AbstractClass* target, bool immediate) override;
#endif

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

    /// Global VA: 0x008B41E0
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<BuildingClass*>, SecretLabArray, 0x008B41E0u)
#else
    static DynamicVectorClass<BuildingClass*>& SecretLabArray;
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
    /// VA: 0x44D5D0
#if defined(RA2_YRPP_GAME)
    bool DiscoveredBy(HouseClass* house) override { JMP_THIS(0x44D5D0); }
#else
    bool DiscoveredBy(HouseClass* house) override;
#endif
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
    /// VA: 0x00459E70
#if defined(RA2_YRPP_GAME)
    virtual int Size() const override { JMP_THIS(0x00459E70); }
#else
    virtual int Size() const override;
#endif

#if !defined(RA2_YRPP_GAME)
    ObjectTypeClass* GetType() const override;
    /// VA: 0x0043FB20
    void Update() override;
#endif
    using ObjectClass::GetCoords;
    using ObjectClass::GetDestination;
    using ObjectClass::GetDockCoords;
    /// VA: 0x00454DB0
    void UpdateCloak(bool ignore) override
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x00454DB0);
    }
#else
        ;
#endif
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
    /// VA: 0x00455F10
#if defined(RA2_YRPP_GAME)
    void UpdateBaseSpacing(bool rebuildOnly) { JMP_THIS(0x455F10); }
#else
    void UpdateBaseSpacing(bool rebuildOnly);
#endif
    /// VA: 0x004561F0
#if defined(RA2_YRPP_GAME)
    void RemoveBaseSpacing() { JMP_THIS(0x4561F0); }
#else
    void RemoveBaseSpacing();
#endif
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
    CoordStruct* vt_entry_300(CoordStruct* output, DWORD weapon) const override;
    /// VA: 0x453840
    CoordStruct* GetFLH(CoordStruct* output, int weapon, CoordStruct base) const override;
    /// VA: 0x458DB0
    bool IsArmed() const override;
    /// VA: 0x458E00
    int GetOccupyRangeBonus() const override;
    /// VA: 0x445F00
    AbstractClass* GreatestThreat(ThreatType threat, CoordStruct* origin, bool onlyEnemy) override;
    /// VA: 0x4496B0
    int Mission_Guard() override;
    /// VA: 0x0044E440
#if defined(RA2_YRPP_GAME)
    int Mission_Open() override { JMP_THIS(0x0044E440); }
#else
    int Mission_Open() override;
#endif
    /// VA: 0x44ACF0
    int Mission_Attack() override;
    /// VA: 0x44D880
    int Mission_Unload() override;
    /// VA: 0x449A50
#if defined(RA2_YRPP_GAME)
    int Mission_Construction() override { JMP_THIS(0x449A50); }
#else
    int Mission_Construction() override;
#endif
    /// VA: 0x44B780
#if defined(RA2_YRPP_GAME)
    int Mission_Repair() override { JMP_THIS(0x44B780); }
#else
    int Mission_Repair() override;
#endif
    /// VA: 0x454250
    bool ReadyToNextMission() const override;
    /// VA: 0x44D6A0
    bool EnterIdleMode(bool initial, bool resume) override;
    /// VA: 0x447F10
    FireError GetFireError(AbstractClass* target, int weapon, bool checkRange) const override;
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
#if defined(RA2_YRPP_GAME)
    void AddOverpowerer(InfantryClass* infantry) { JMP_THIS(0x452820); }
#else
    void AddOverpowerer(InfantryClass* infantry);
#endif
    // Extracted from Update's original charging-list phase, 0x4400F9..0x44019C.
    void UpdateOverpowerers();
    /// VA: 0x00459C00
#if defined(RA2_YRPP_GAME)
    bool CanBeSelectedNow() const override { JMP_THIS(0x459C00); }
#else
    bool CanBeSelectedNow() const override;
#endif
    /// VA: 0x00426410
    bool IsStandingStill() const override { return true; }  // original Occupies_Cells
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
    void DrawIt(Point2D* point, RectangleStruct* bounds) const override;
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
    void RadarTrackingStart() override;  // 456580, one record per foundation pixel.
    void RadarTrackingStop() override;   // 4565E0.
    void RadarTrackingFlash() override;  // 456640.
    /// VA: 0x00451330
#if defined(RA2_YRPP_GAME)
    int GetCrewCount() const override JMP_THIS(0x451330);
#else
    int GetCrewCount() const override;
#endif
    /// VA: unknown (legacy placeholder).
    virtual void Destroyed(ObjectClass* Killer) RX;
    /// VA: 0x00458A80
#if defined(RA2_YRPP_GAME)
    virtual bool ForceCreate(CoordStruct& coord, DWORD dwUnk = 0) { JMP_THIS(0x458A80); }
#else
    virtual bool ForceCreate(CoordStruct& coord, DWORD dwUnk = 0) noexcept;
#endif

    // BuildingClass
    /// VA: 0x44EFB0
    virtual CellStruct FindExitCell(FootClass* product, CellStruct preferred) const;
    /// VA: 0x447B20
#if defined(RA2_YRPP_GAME)
    CoordStruct* GetDockCoords(CoordStruct* output, TechnoClass* docker) const override { JMP_THIS(0x447B20); }
#else
    CoordStruct* GetDockCoords(CoordStruct* output, TechnoClass* docker) const override;
#endif
    /// VA: 0x447E90
#if defined(RA2_YRPP_GAME)
    CoordStruct* GetDestination(CoordStruct* output, TechnoClass* docker) const override { JMP_THIS(0x447E90); }
#else
    CoordStruct* GetDestination(CoordStruct* output, TechnoClass* docker) const override;
#endif
    /// VA: 0x443C60
    KickOutResult KickOutUnit(TechnoClass* product, CellStruct preferred) override;
    /// VA: 0x449540
    bool ClearFactoryBib();
    int MissionFactoryUnload();
    /// VA: 0x004500F0
#if defined(RA2_YRPP_GAME)
    void FactoryAI() { JMP_THIS(0x4500F0); }
#else
    void FactoryAI();
#endif
    /// VA: 0x43C2D0
    RadioCommand ReceiveCommand(TechnoClass* sender, RadioCommand command, AbstractClass*& data) override;
    /// VA: 0x447E00
#if defined(RA2_YRPP_GAME)
    virtual int DistanceToDockingCoord(ObjectClass* object) const { JMP_THIS(0x447E00); }
#else
    virtual int DistanceToDockingCoord(ObjectClass* object) const;
#endif
    /// VA: 0x445F80
    virtual void Place(bool captured);
    /// VA: 0x458A00
    bool IsCellImpassable(CellClass* cell) const;
    /// VA: 0x4456D0
#if defined(RA2_YRPP_GAME)
    virtual void UpdateConstructionOptions() { JMP_THIS(0x4456D0); }
#else
    virtual void UpdateConstructionOptions();
#endif
    /// VA: 0x0043DA80
    virtual void Draw(const Point2D& point, const RectangleStruct& rect);
    /// VA: unknown (legacy placeholder).
    /// VA: 0x43ED40
    virtual DirStruct FireAngleTo(AbstractClass* target) const;
    // Original Do_Destruction slot (legacy spelling retained).
    /// VA: 0x004415F0
#if defined(RA2_YRPP_GAME)
    virtual void Destory(TechnoClass* lastContact, TechnoClass* source, bool noSurvivor, const CellStruct* footprint)
        JMP_THIS(0x004415F0);
#else
    virtual void Destory(TechnoClass* lastContact, TechnoClass* source, bool noSurvivor, const CellStruct* footprint);
#endif
    /// VA: 0x00441F60
    void LeaveRubble();
    /// VA: unknown (legacy placeholder).
    virtual bool TogglePrimaryFactory() R0;

    /// VA: 0x448070
#if defined(RA2_YRPP_GAME)
    bool SetPrimary(bool overrideExisting) { JMP_THIS(0x448070); }
#else
    bool SetPrimary(bool overrideExisting);
#endif
    /// VA: unknown (legacy placeholder).
    virtual void SensorArrayActivate(CellStruct cell = CellStruct::Empty) RX;
    /// VA: unknown (legacy placeholder).
    virtual void SensorArrayDeactivate(CellStruct cell = CellStruct::Empty) RX;
    /// VA: unknown (legacy placeholder).
    virtual void DisguiseDetectorActivate(CellStruct cell = CellStruct::Empty) RX;
    /// VA: unknown (legacy placeholder).
    virtual void DisguiseDetectorDeactivate(CellStruct cell = CellStruct::Empty) RX;
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
    void UpdateAnimations() { JMP_THIS(0x4509D0); }
#else
    void UpdateAnimations();
#endif
    // Native power/placement synchronization does not advance Animation_AI.
    // Exceptions cannot cross this boundary.
    void SynchronizePowerAnimations() noexcept;
    // Map loading prepares visuals without registering powered-unit centers.
    // Exceptions cannot cross this boundary.
    void InitializePowerAnimations() noexcept;

private:
    void SynchronizePowerAnimations(bool updatePowered) noexcept;
    /// VA: 0x0044E3A0
#if defined(RA2_YRPP_GAME)
    bool IsBlockedByOccupier() const { JMP_THIS(0x0044E3A0); }
#else
    bool IsBlockedByOccupier() const;
#endif

public:
    /// VA: 0x004545D0
#if defined(RA2_YRPP_GAME)
    void EnablePowerDependentAnims() noexcept { JMP_THIS(0x004545D0); }
#else
    void EnablePowerDependentAnims() noexcept;
#endif
    /// VA: 0x004547C0
#if defined(RA2_YRPP_GAME)
    void DisablePowerDependentAnims() noexcept { JMP_THIS(0x004547C0); }
#else
    void DisablePowerDependentAnims() noexcept;
#endif
    // Animation branches of UpdatePowered (0x004549B0). The caller dispatches
    // controller and locomotor release before these animation effects.
    // No exceptions may cross these native animation boundaries.
    void UpdatePowerAnimations() noexcept;
    /// VA: 0x004549B0
#if defined(RA2_YRPP_GAME)
    void UpdatePowered() { JMP_THIS(0x4549B0); }
#else
    void UpdatePowered();
#endif

    /// VA: 0x0043EF90.
#if defined(RA2_YRPP_GAME)
    int GetCurrentFrame() { JMP_THIS(0x43EF90); }
#else
    int GetCurrentFrame();
#endif

    /// VA: 0x00457A10.
    bool IsAllFogged() const { JMP_THIS(0x457A10); }

    /// VA: 0x443860
#if defined(RA2_YRPP_GAME)
    void SetRallypoint(CellStruct* pTarget, bool bPlayEVA) { JMP_THIS(0x443860); }
#else
    void SetRallypoint(CellStruct* pTarget, bool bPlayEVA);
#endif

    /// VA: 0x00457AA0.
    void FreezeInFog(DynamicVectorClass<FoggedObjectClass*>* pFoggedArray, CellClass* pCell, bool Visible) {
        JMP_THIS(0x457AA0);
    }

    // power up
    /// VA: 0x00452260
#if defined(RA2_YRPP_GAME)
    void GoOnline() { JMP_THIS(0x452260); }
#else
    void GoOnline();
#endif
    /// VA: 0x00452360
#if defined(RA2_YRPP_GAME)
    void GoOffline() { JMP_THIS(0x452360); }
#else
    void GoOffline();
#endif

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
    DWORD GetFWFlags() const { JMP_THIS(0x455B90); }

    // Intentionally not implemented in the native core: laser fences are unused
    // by the 38 official RA2/YR campaign maps and both tutorial maps.
    // Retain the original entry for compatibility.
    /// VA: 0x004533A0
    void CreateEndPost(bool arg) { JMP_THIS(0x4533A0); }

    // kick out content
    /// VA: 0x4593A0
#if defined(RA2_YRPP_GAME)
    void UnloadBunker() { JMP_THIS(0x4593A0); }
#else
    void UnloadBunker();
#endif

    // content is dead - chronosphered away or died inside
    /// VA: 0x459470
#if defined(RA2_YRPP_GAME)
    void ClearBunker() { JMP_THIS(0x459470); }
#else
    void ClearBunker();
#endif

    // kick out content, remove anims, etc... don't ask me what's different from kick out
    /// VA: 0x4595C0
#if defined(RA2_YRPP_GAME)
    void EmptyBunker() { JMP_THIS(0x4595C0); }
#else
    void EmptyBunker();
#endif

    // called after destruction - CrateBeneath, resetting foundation'ed cells
    /// VA: 0x00441F60.
    void AfterDestruction() { JMP_THIS(0x441F60); }

    // Emits smoke at the configured refinery chimney offsets.
    /// VA: 0x00459900
    void UpdateRefinerySmokeSystems() override;

    // destroys the specific animation (active, turret, special, etc)
    // No exceptions may escape upgrade removal.
    /// VA: 0x451690
#if defined(RA2_YRPP_GAME)
    bool RemoveUpgrade() noexcept { JMP_THIS(0x451690); }
#else
    bool RemoveUpgrade() noexcept;
#endif
    /// VA: 0x00451E40.
#if defined(RA2_YRPP_GAME)
    void DestroyNthAnim(BuildingAnimSlot Slot) { JMP_THIS(0x451E40); }
#else
    void DestroyNthAnim(BuildingAnimSlot Slot);
#endif

    // the game picks the slot's normal/damaged/garrisoned name from these two states
    /// VA: 0x00451750.
#if defined(RA2_YRPP_GAME)
    void PlayNthAnim(BuildingAnimSlot Slot, bool Damaged, bool Garrisoned, int effectDelay = 0) { JMP_THIS(0x451750); }
#else
    void PlayNthAnim(BuildingAnimSlot Slot, bool Damaged, bool Garrisoned, int effectDelay = 0);
#endif

    // derives the two states the way the game's own callers do. mind that a bool
    // passed here is an effectDelay, not a state -- use the four-argument form.
    void PlayNthAnim(BuildingAnimSlot Slot, int effectDelay = 0) {
        this->PlayNthAnim(Slot, !this->IsGreenHP(), this->GetOccupantCount() > 0, effectDelay);
    }

    /// VA: 0x004517D0
    void SetAnimCoords();

    /// VA: 0x00451890.
#if defined(RA2_YRPP_GAME)
    void PlayAnim(const char* animName, BuildingAnimSlot Slot, bool Damaged, bool Garrisoned, int effectDelay = 0) {
        JMP_THIS(0x451890);
    }
#else
    void PlayAnim(const char* animName, BuildingAnimSlot Slot, bool Damaged, bool Garrisoned, int effectDelay = 0);
#endif

    // changes between building's damaged and undamaged animations.
    /// VA: 0x00451EE0.
#if defined(RA2_YRPP_GAME)
    void ToggleDamagedAnims(bool isDamaged) { JMP_THIS(0x451EE0); }
#else
    void ToggleDamagedAnims(bool isDamaged);
#endif

    // when the building is switched off
    /// VA: 0x00452480
#if defined(RA2_YRPP_GAME)
    void DisableStuff() { JMP_THIS(0x452480); }
#else
    void DisableStuff();
#endif

    // when the building is switched on
    /// VA: 0x00452410
#if defined(RA2_YRPP_GAME)
    void EnableStuff() { JMP_THIS(0x452410); }
#else
    void EnableStuff();
#endif

    /// VA: 0x00452000
    void UpdateAnimAppearance();

// when the building is warped
/// VA: 0x004521C0.
#if defined(RA2_YRPP_GAME)
    void DisableTemporal() { JMP_THIS(0x4521C0); }
#else
    void DisableTemporal();
#endif

// when the building warped back in
/// VA: 0x00452210.
#if defined(RA2_YRPP_GAME)
    void EnableTemporal() { JMP_THIS(0x452210); }
#else
    void EnableTemporal();
#endif

    // returns Type->SuperWeapon, if its AuxBuilding is satisfied
    /// VA: 0x00457630.
#if defined(RA2_YRPP_GAME)
    int FirstActiveSWIdx() const { JMP_THIS(0x457630); }
#else
    int FirstActiveSWIdx() const;
#endif

    /// VA: 0x0043EF90.
#if defined(RA2_YRPP_GAME)
    int GetShapeNumber() const { JMP_THIS(0x43EF90); }
#else
    int GetShapeNumber() const;
#endif

    /// VA: 0x00447780.
#if defined(RA2_YRPP_GAME)
    void BeginMode(BStateType bType) { JMP_THIS(0x447780); }
#else
    void BeginMode(BStateType bType);
#endif

    // returns Type->SuperWeapon2, if its AuxBuilding is satisfied
    /// VA: 0x00457690.
#if defined(RA2_YRPP_GAME)
    int SecondActiveSWIdx() const { JMP_THIS(0x457690); }
#else
    int SecondActiveSWIdx() const;
#endif

    /// VA: 0x0044ABD0
#if defined(RA2_YRPP_GAME)
    void FireLaser(CoordStruct Coords) { JMP_THIS(0x44ABD0); }
#else
    void FireLaser(CoordStruct Coords);
#endif

    /// VA: 0x004503F0
#if defined(RA2_YRPP_GAME)
    void UpdateDelayedFire() { JMP_THIS(0x4503F0); }
#else
    void UpdateDelayedFire();
#endif

    /// VA: 0x0070FEC0.
#if defined(RA2_YRPP_GAME)
    bool IsBeingDrained() const { JMP_THIS(0x70FEC0); }
#else
    bool IsBeingDrained() const;
#endif

    /// VA: 0x458E50
#if defined(RA2_YRPP_GAME)
    bool UpdateBunker() { JMP_THIS(0x458E50); }
#else
    bool UpdateBunker();
#endif

    /// VA: 0x004585C0.
    void KillOccupants(TechnoClass* pAssaulter);
    /// VA: 0x00457DE0
    void UnloadOccupants(bool scatter, bool force);

    // returns false if this is a gate that needs time to open, true otherwise
    /// VA: 0x00452540.
    bool MakeTraversable()
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x452540);
    }
#else
        ;
#endif

    /// VA: 0x00457A10.
    bool CheckFog() { JMP_THIS(0x457A10); }

    /// VA: 0x00453BF0
#if defined(RA2_YRPP_GAME)
    CoordStruct* GetVoxelFireCoords(CoordStruct* output, int weapon, bool justFired) const noexcept {
        JMP_THIS(0x00453BF0);
    }
#else
    CoordStruct* GetVoxelFireCoords(CoordStruct* output, int weapon, bool justFired) const noexcept;
#endif

    /// VA: 0x00458810
#if defined(RA2_YRPP_GAME)
    Matrix3D* GetVoxelBarrelOffsetMatrix(Matrix3D& ret) { JMP_THIS(0x458810); }
#else
    Matrix3D* GetVoxelBarrelOffsetMatrix(Matrix3D& ret);
#endif

    // returns false if this is a gate that is closed, true otherwise
    /// VA: 0x004525F0.
#if defined(RA2_YRPP_GAME)
    bool IsTraversable() const { JMP_THIS(0x4525F0); }
#else
    bool IsTraversable() const;
#endif

    /// VA: 0x0043E7B0
#if defined(RA2_YRPP_GAME)
    void DrawInfoTipAndSpiedSelection(Point2D* pLocation, RectangleStruct* pRect) const { JMP_THIS(0x43E7B0); }
#else
    void DrawInfoTipAndSpiedSelection(Point2D* pLocation, RectangleStruct* pRect) const;
#endif

    /// VA: 0x00452670
#if defined(RA2_YRPP_GAME)
    bool CanUpgrade(const BuildingTypeClass* upgrade, const HouseClass* owner) const { JMP_THIS(0x452670); }
#else
    bool CanUpgrade(const BuildingTypeClass* upgrade, const HouseClass* owner) const noexcept;
#endif

    // helpers
    /// VA: 0x00451400
    bool Upgrade()
#if defined(RA2_YRPP_GAME)
    {
        JMP_THIS(0x00451400);
    }
#else
        ;
#endif
    bool HasSuperWeapon(int index) const {
        if (this->Type->HasSuperWeapon(index)) {
            return true;
        }
        for (auto pType : this->Upgrades) {
            if (pType && pType->HasSuperWeapon(index)) {
                return true;
            }
        }
        return false;
    }

    /// VA: 0x00459840
    TechnoTypeClass* GetSecretProduction() const;

    // Reconstruction-only slot accessors, not original executable entry points.
    // These overloads expose existing array elements; mutable overloads allow writes.
    // They add no instance storage or vtable slots.
    AnimClass*& GetAnim(BuildingAnimSlot slot) { return this->Anims[static_cast<int>(slot)]; }

    AnimClass* const& GetAnim(BuildingAnimSlot slot) const { return this->Anims[static_cast<int>(slot)]; }

    bool& GetAnimState(BuildingAnimSlot slot) { return this->AnimStates[static_cast<int>(slot)]; }

    bool const& GetAnimState(BuildingAnimSlot slot) const { return this->AnimStates[static_cast<int>(slot)]; }

    /// VA: 0x00457CE0.
    bool CanBeOccupiedBy(InfantryClass* pInfantry) const;

    /// VA: 0x004571E0
#if defined(RA2_YRPP_GAME)
    void SpiedBy(HouseClass* house) { JMP_THIS(0x4571E0); }
#else
    void SpiedBy(HouseClass* house);
#endif
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
    bool SetOwningHouse(HouseClass* house, bool announce = true) override;
    /// VA: 0x00442230
    DamageState ReceiveDamage(int* damage, int distance, WarheadTypeClass* warhead, ObjectClass* source,
                              bool ignoreDefenses, bool preventEscape, HouseClass* sourceHouse) override;

    /// VA: 0x004566B0
#if defined(RA2_YRPP_GAME)
    int GetRadialIndicatorRange() const { JMP_THIS(0x004566B0); }
#else
    int GetRadialIndicatorRange() const;
#endif

    // Constructor
    /// VA: 0x0043B740.
#if defined(RA2_YRPP_GAME)
    BuildingClass(BuildingTypeClass* pType, HouseClass* pOwner) noexcept : BuildingClass(noinit_t()) {
        JMP_THIS(0x43B740);
    }
#else
    BuildingClass(BuildingTypeClass* pType, HouseClass* pOwner) noexcept;
#endif

protected:
    explicit __forceinline BuildingClass(noinit_t) noexcept : TechnoClass(noinit_t()) {}

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
    AnimClass* FirestormAnim;  //pointer
    AnimClass* PsiWarnAnim;    //pointer
    CDTimerClass FactoryRetryTimer;

    // see eBuildingAnims above for slot index meanings
    AnimClass* Anims[0x15];
    bool AnimStates
        [0x15];  // one flag for each of the above anims (whether the anim was enabled when power went offline?)

protected:
    char align_5C5[3];

public:
    AnimClass* DamageFireAnims[0x8];

    bool RequiresDamageFires;  // current low-health fire state; Update acts on transitions
    //5E8 - 5F8 ????????
    BuildingTypeClass* Upgrades[0x3];

    int FiringSWType;  // type # of sw being launched
    DWORD unknown_5FC;
    BuildingLightClass* Spotlight;
    RateTimer GateTimer;
    LightSourceClass* LightSource;  // tiled light , LightIntensity > 0
    DWORD LaserFenceFrame;          // 0-7 for active directionals, 8/12 for offline ones, check ntfnce.shp or whatever
    DWORD FirestormWallFrame;       // anim data for firestorm active animations
    StageClass RepairProgress;      // for hospital, armory, unitrepair etc
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
    int FiringOccupantIndex;  // which occupant should get XP, which weapon should be fired (see 6FF074)

    AudioController Audio7;
    AudioController Audio8;

    bool
        WasOnline;  // the the last state when Update()ing. if this changed since the last Update(), UpdatePowered is called.
    bool ShowRealName;   // is also NOMINAL under [Structures]
    bool BeingProduced;  // is also AI_REBUILDABLE under [Structures]
    bool ShouldRebuild;  // is also AI_REPAIRABLE under [Structures]
    bool HasEngineer;    // used to pass the NeedsEngineer check
    CDTimerClass CashProductionTimer;
    bool AI_Sellable;  // AI_SELLABLE under [Structures]
    bool IsReadyToCommence;
    bool NeedsRepairs;  // AI handholder for repair logic,
    bool C4Applied;
    bool NoCrew;
    bool unknown_bool_6E1;
    bool unknown_bool_6E2;
    bool HasBeenCaptured;  // has this building changed ownership at least once? affects crew and repair.
    bool ActuallyPlacedOnMap;
    bool unknown_bool_6E5;
    bool IsDamaged;  // AI handholder for repair logic,
    bool IsFogged;
    bool IsBeingRepaired;  // show animooted repair wrench
    bool HasBuildUp;
    bool StuffEnabled;     // status set by EnableStuff() and DisableStuff()
    char HasCloakingData;  // some fugly buffers
    byte CloakRadius;      // from Type->CloakRadiusInCells
    char Translucency;
    DWORD StorageFilledSlots;  // the old "silo needed" logic
    TechnoTypeClass*
        SecretProduction;  // randomly assigned secret lab bonus, used if SecretInfantry, SecretUnit, and SecretBuilding are null
    ColorStruct ColorAdd;
    int unknown_int_6FC;
    short unknown_short_700;
    BYTE UpgradeLevel;  // Original CanUpgrade compares this byte as signed.
    char GateStage;
    PrismChargeState PrismStage;
    CoordStruct PrismTargetCoords;
    DWORD DelayBeforeFiring;

    TankBunkerState TankBunkerState;  // used in UpdateBunker and friends
};
