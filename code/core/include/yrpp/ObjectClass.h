/*
    Base class for all game objects.
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/AbstractClass.h"
#include "yrpp/Audio.h"
#include "yrpp/ObjectTypeClass.h"
#include "yrpp/Dir.h"
class TagClass;

struct SHPStruct;
class LightConvertClass;

// forward declarations
class AnimClass;
class BombClass;
class BuildingTypeClass;
class CellClass;
class InfantryTypeClass;
class TechnoClass;
class TechnoTypeClass;
class WarheadTypeClass;

class HouseTypeClass;

class LineTrail;
struct WeaponStruct;

class NOVTABLE ObjectClass : public AbstractClass
{
public:
    static const auto AbsDerivateID = AbstractFlags::Object;

    // global arrays
    /// Global VA: 0x00A8ECB8.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<ObjectClass*>, CurrentObjects, 0xA8ECB8u)
#else
    static DynamicVectorClass<ObjectClass*>& CurrentObjects;
#endif

    // Frame index LUT used by buildings for turret anim, projectiles etc.
    /// Global VA: 0x007F4890.
    DEFINE_ARRAY_REFERENCE(int, [40u], BodyShape, 0x7F4890u)

    // IPersistStream
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) override R0;

    // Destructor
    /// VA: unknown (legacy placeholder).
#if defined(RA2_YRPP_GAME)
    virtual ~ObjectClass() RX;
#else
    virtual ~ObjectClass();
#endif

    // AbstractClass
    /// VA: 0x005F3E70
#if defined(RA2_YRPP_GAME)
    void Update() override { JMP_THIS(0x5F3E70); }
#else
    void Update() override;
#endif
    /// VA: 0x005F5230
#if defined(RA2_YRPP_GAME)
    void PointerExpired(AbstractClass* object, bool removed) override { JMP_THIS(0x5F5230); }
#else
    void PointerExpired(AbstractClass* object, bool removed) override;
#endif
    /// VA: 0x005F6B60
#if defined(RA2_YRPP_GAME)
    virtual bool IsOnFloor() const override { JMP_THIS(0x5F6B60); }
#else
    bool IsOnFloor() const override;
#endif
    /// VA: 0x005F6B90.
#if defined(RA2_YRPP_GAME)
    virtual bool IsInAir() const override { JMP_THIS(0x5F6B90); }
#else
    bool IsInAir() const override;
#endif
    // ...and so on
    // FIXME other virtual function explicit addresses

    // ObjectClass
    /// VA: 0x005F3DB0
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) DirStruct* GetDirectionTo(DirStruct* out,AbstractClass* target) const { JMP_THIS(0x5F3DB0); }
#else
    DirStruct* GetDirectionTo(DirStruct* out,AbstractClass* target) const;
#endif
    /// VA: 0x005F6DA0
    virtual void AnimPointerExpired(AnimClass* anim) { if(Parachute==anim)Parachute=nullptr; }
    /// VA: unknown (legacy placeholder).
#if defined(RA2_YRPP_GAME)
    virtual bool IsSelectable() const R0;
#else
    virtual bool IsSelectable() const;
#endif
    /// VA: unknown (legacy placeholder).
    virtual VisualType VisualCharacter(VARIANT_BOOL SpecificOwner, HouseClass * WhoIsAsking) const RT(VisualType);
    /// VA: unknown (legacy placeholder).
#if defined(RA2_YRPP_GAME)
    virtual SHPStruct* GetImage() const R0;
#else
    virtual SHPStruct* GetImage() const;
#endif
    /// VA: unknown (legacy placeholder).
    virtual Action MouseOverCell(CellStruct const* pCell, bool checkFog = false, bool ignoreForce = false) const RT(Action);
    /// VA: unknown (legacy placeholder).
    virtual Action MouseOverObject(ObjectClass const* pObject, bool ignoreForce = false) const RT(Action);
    /// VA: 0x005F4260
#if defined(RA2_YRPP_GAME)
    virtual Layer InWhichLayer() const { JMP_THIS(0x5F4260); }
#else
    virtual Layer InWhichLayer() const;
#endif
    /// VA: 0x005F6C10
    virtual bool IsSurfaced() { return GetHeight()>-20; } // opposed to being submerged

/*
    Building returns if it is 1x1 and has UndeploysInto
    inf returns 0
    unit returns !NonVehicle
    Aircraft returns IsOnFloor()

    users include:
    452656 - is this building click-repairable
    440C26 - should this building get considered in BaseSpacing
    445A8E - -""-
    51E7D1 - can a VehicleThief be clicked to steal this unit
    51E4D9 - can an engi be clicked to enter this to fix/takeover
    51F0D3 - -""-
    51EA06 - can this building be C4'd?
    51E243 - can a VehicleThief steal this on his own decision
    4F93F3 - should this building's damage raise a BaseUnderAttack?
    442286 - -""-
    44296A - -""-
    741117 - can this be healed by a vehicle?
    6F8242 - can this aircraft be auto-target
    6F85BE - can this aircraft be auto-attacked
*/
    /// VA: unknown (legacy placeholder).
    virtual bool IsStrange() const R0;

    /// VA: unknown (legacy placeholder).
    virtual TechnoTypeClass* GetTechnoType() const R0;
    /// VA: unknown (legacy placeholder).
    virtual ObjectTypeClass* GetType() const R0;
    /// VA: unknown (legacy placeholder).
    virtual DWORD GetTypeOwners() const R0; // returns the data for IndexBitfield<HouseTypeClass*>
    /// VA: unknown (legacy placeholder).
#if defined(RA2_YRPP_GAME)
    virtual const wchar_t* GetUIName() const R0;
#else
    virtual const wchar_t* GetUIName() const;
#endif
    /// VA: unknown (legacy placeholder).
    virtual bool CanBeRepaired() const R0;
    /// VA: unknown (legacy placeholder).
    virtual bool CanBeSold() const R0;
    /// VA: unknown (legacy placeholder).
#if defined(RA2_YRPP_GAME)
    virtual bool IsActive() const R0;
#else
    virtual bool IsActive() const;
#endif

    // can the current player control this unit? (owned by him, not paralyzed, not spawned, not warping, not slaved...)
    /// VA: unknown (legacy placeholder).
    virtual bool IsControllable() const R0;

    // On non-buildings this is same as GetCenterCoord(), on buildings it returns the target coordinate that is affected by TargetCoordOffset.
    /// VA: 0x0041BDD0
    virtual CoordStruct* GetTargetCoords(CoordStruct* out) const { CoordStruct buffer;*out=*GetCoords(&buffer);return out; }
    // gets a building's free dock coordinates for a unit. falls back to this->GetCoords(pCrd);
    /// VA: unknown (legacy placeholder).
    virtual CoordStruct* GetDockCoords(CoordStruct* pCrd, TechnoClass* docker) const R0;
    /// VA: 0x0041BE00.
#if defined(RA2_YRPP_GAME)
    virtual CoordStruct* GetRenderCoords(CoordStruct* pCrd) const { JMP_THIS(0x41BE00); }
#else
    virtual CoordStruct* GetRenderCoords(CoordStruct* pCrd) const;
#endif
    /// VA: unknown (legacy placeholder).
    virtual CoordStruct* GetFLH(CoordStruct *pDest, int idxWeapon, CoordStruct BaseCoords) const R0;
    /// VA: unknown (legacy placeholder).
    virtual CoordStruct* GetExitCoords(CoordStruct* pCrd, DWORD dwUnk) const R0;
    /// VA: 0x005F6BD0.
#if defined(RA2_YRPP_GAME)
    virtual int GetYSort() const { JMP_THIS(0x5F6BD0); }
#else
    virtual int GetYSort() const;
#endif
    /// VA: unknown (legacy placeholder).
    virtual bool IsOnBridge(TechnoClass* pDocker = nullptr) const R0; // pDocker is passed to GetDestination
    /// VA: unknown (legacy placeholder).
    virtual bool IsStandingStill() const R0;
    /// VA: unknown (legacy placeholder).
    virtual bool IsDisguised() const R0;
    /// VA: unknown (legacy placeholder).
    virtual bool IsDisguisedAs(HouseClass *target) const R0; // only works correctly on infantry!
    /// VA: unknown (legacy placeholder).
    virtual ObjectTypeClass* GetDisguise(bool DisguisedAgainstAllies) const R0;
    /// VA: unknown (legacy placeholder).
    virtual HouseClass* GetDisguiseHouse(bool DisguisedAgainstAllies) const R0;

    // remove object from the map
    /// VA: 0x005F4D30
#if defined(RA2_YRPP_GAME)
    virtual bool Limbo() { JMP_THIS(0x5F4D30); }
#else
    virtual bool Limbo();
#endif

    // place the object on the map
    /// VA: 0x005F4EC0
#if defined(RA2_YRPP_GAME)
    virtual bool Unlimbo(const CoordStruct& Crd, DirType dFaceDir) { JMP_THIS(0x5F4EC0); }
#else
    virtual bool Unlimbo(const CoordStruct& Crd, DirType dFaceDir);
#endif

    // cleanup things (lose line trail, deselect, etc). Permanently: destroyed/removed/gone opposed to just going out of sight.
    /// VA: 0x005F5280
#if defined(RA2_YRPP_GAME)
    virtual void Disappear(bool permanently) { JMP_THIS(0x5F5280); }
#else
    virtual void Disappear(bool permanently);
#endif

    /// VA: unknown (legacy placeholder).
    virtual void RegisterDestruction(TechnoClass *Destroyer) RX;

    // maybe Object instead of Techno? Raises Map Events, grants veterancy, increments house kill counters
    /// VA: unknown (legacy placeholder).
    virtual void RegisterKill(HouseClass *Destroyer) RX; // ++destroyer's kill counters , etc

    /// VA: 0x005F5940.
#if defined(RA2_YRPP_GAME)
    virtual bool SpawnParachuted(const CoordStruct& coords) { JMP_THIS(0x5F5940); }
#else
    virtual bool SpawnParachuted(const CoordStruct& coords);
#endif
    /// VA: 0x005F4160
    virtual void DropAsBomb() { JMP_THIS(0x5F4160); }
    /// VA: unknown (legacy placeholder).
    virtual void MarkAllOccupationBits(const CoordStruct& coords) RX;
    /// VA: unknown (legacy placeholder).
    virtual void UnmarkAllOccupationBits(const CoordStruct& coords) RX;
    /// VA: 0x005F65F0
#if defined(RA2_YRPP_GAME)
    virtual void UnInit() { JMP_THIS(0x5F65F0); }
#else
    virtual void UnInit();
#endif
    /// VA: unknown (legacy placeholder).
    virtual void Reveal() RX; // uncloak when object is bumped, damaged, detected, ...
    /// VA: unknown (legacy placeholder).
    virtual KickOutResult KickOutUnit(TechnoClass* pTechno, CellStruct Cell) RT(KickOutResult);
    /// VA: 0x005F4B10
    // forced redraw; extrasOnly is consumed by the building override.
    virtual bool DrawIfVisible(RectangleStruct* bounds, bool forced, DWORD extrasOnly) const;
    /// VA: 0x005F5B90
    virtual CellStruct const* GetFoundationData(bool includeBib = false) const;
    /// VA: unknown (legacy placeholder).
    virtual void DrawBehind(Point2D* pLocation, RectangleStruct* pBounds) const RX;
    /// VA: unknown (legacy placeholder).
    virtual void DrawExtras(Point2D* pLocation, RectangleStruct* pBounds) const RX; // draws ivan bomb, health bar, talk bubble, etc
    /// VA: unknown (legacy placeholder).
    virtual void DrawIt(Point2D* pLocation, RectangleStruct* pBounds) const RX;
    /// VA: unknown (legacy placeholder).
    virtual void DrawAgain(const Point2D& location, const RectangleStruct& bounds) const RX; // just forwards the call to Draw
    /// VA: unknown (legacy placeholder).
    virtual void Undiscover() RX;
    /// VA: unknown (legacy placeholder).
    virtual void See(DWORD dwUnk, DWORD dwUnk2) RX;
    /// VA: 0x005F5850
    virtual bool Mark(MarkType value);
    /// VA: unknown (legacy placeholder).
    virtual RectangleStruct* GetDimensions(RectangleStruct* pRect) const R0;
    /// VA: unknown (legacy placeholder).
    virtual RectangleStruct* GetRenderDimensions(RectangleStruct* pRect) R0;
    /// VA: unknown (legacy placeholder).
    virtual void DrawRadialIndicator(DWORD dwUnk) RX;
    /// VA: 0x005F4D10
    virtual void MarkForRedraw();
    /// VA: 0x005F6C30
#if defined(RA2_YRPP_GAME)
    virtual bool CanBeSelected() const R0;
#else
    virtual bool CanBeSelected() const;
#endif
    /// VA: 0x005F6C70
#if defined(RA2_YRPP_GAME)
    virtual bool CanBeSelectedNow() const R0;
#else
    virtual bool CanBeSelectedNow() const;
#endif
    /// VA: unknown (legacy placeholder).
    virtual bool CellClickedAction(Action action, CellStruct* pCell, CellStruct* pCell1, bool bUnk) R0;
    /// VA: unknown (legacy placeholder).
    virtual bool ObjectClickedAction(Action action, ObjectClass* pTarget, bool bUnk) R0;
    /// VA: unknown (legacy placeholder).
    virtual void Flash(int Duration) RX;
    /// VA: 0x005F4520
#if defined(RA2_YRPP_GAME)
    virtual bool Select() R0;
#else
    virtual bool Select();
#endif
    /// VA: unknown (legacy placeholder).
#if defined(RA2_YRPP_GAME)
    virtual void Deselect() RX;
#else
    virtual void Deselect();
#endif
    /// VA: unknown (legacy placeholder).
    virtual DamageState IronCurtain(int nDuration, HouseClass *pSource, bool ForceShield) RT(DamageState);
    /// VA: unknown (legacy placeholder).
    virtual void StopAirstrikeTimer() RX;
    /// VA: unknown (legacy placeholder).
    virtual void StartAirstrikeTimer(int Duration) RX;
    /// VA: unknown (legacy placeholder).
    virtual bool IsIronCurtained() const R0;
    /// VA: unknown (legacy placeholder).
    virtual bool IsCloseEnough3D(const CoordStruct& coords, int weapon) const R0;
    /// VA: unknown (legacy placeholder).
    virtual int GetWeaponRange(int idxWeapon) const R0;
    /// VA: 0x005F5390
    virtual DamageState ReceiveDamage(int* pDamage, int DistanceFromEpicenter, WarheadTypeClass* pWH,
        ObjectClass* Attacker, bool IgnoreDefenses, bool PreventPassengerEscape, HouseClass* pAttackingHouse);
    /// VA: unknown (legacy placeholder).
    virtual void Destroy() RX;
    /// VA: unknown (legacy placeholder).
    virtual void Scatter(const CoordStruct &crd, bool ignoreMission, bool ignoreDestination) RX;
    /// VA: unknown (legacy placeholder).
    virtual bool Ignite() R0;
    /// VA: unknown (legacy placeholder).
    virtual void Extinguish() RX;
    /// VA: unknown (legacy placeholder).
    virtual DWORD GetPointsValue() const R0;
    /// VA: unknown (legacy placeholder).
    virtual Mission GetCurrentMission() const RT(Mission);
    /// VA: unknown (legacy placeholder).
    virtual void RestoreMission(Mission mission) RX;
    /// VA: unknown (legacy placeholder).
    virtual void UpdatePosition(PCPType how) RX;
    /// VA: unknown (legacy placeholder).
    virtual BuildingClass* FindFactory(bool allowOccupied, bool requirePower) const R0;
    /// VA: unknown (legacy placeholder).
    /// VA: 0x5F5320
    virtual RadioCommand ReceiveCommand(TechnoClass* pSender, RadioCommand command, AbstractClass* &pInOut);
    /// VA: 0x005F5930
    virtual bool DiscoveredBy(HouseClass* house) { return house != nullptr; }
    /// VA: unknown (legacy placeholder).
    virtual void SetRepairState(int state) RX; // 0 - off, 1 - on, -1 - toggle
    /// VA: unknown (legacy placeholder).
    virtual void Sell(int control) RX; // -1 = Always sell, 0 = Sell only if already being sold/unpacking, 1 = Same as -1 except not if dying from DelayKill, Other = only plays click sound
    /// VA: 0x005F6B50
    virtual void AssignPlanningPath(signed int idxPath, signed char idxWP) RX;
    /// VA: unknown (legacy placeholder).
    virtual void MoveToDirection(FacingType facing) RX; // Vestigial, never called by the game.
    /// VA: unknown (legacy placeholder).
    virtual Move IsCellOccupied(CellClass *pDestCell, FacingType facing, int level, CellClass* pSourceCell, bool alt) const RT(Move);
    /// VA: unknown (legacy placeholder).
    // Original Can_Reach: slot 0x1B0. Height and bridge are in/out parameters,
    // not DWORD values. Object's base implementation permits the step.
    virtual Move CanReachCell(const CellClass* destination, FacingType facing,
        int& level, bool& bridge, const CellClass* source) const { return Move::OK; }
    /// VA: 0x005F6940
    virtual void SetLocation(const CoordStruct& crd);

// these two work through the object's Location
    /// VA: 0x0041BEA0
#if defined(RA2_YRPP_GAME)
    virtual CellStruct* GetMapCoords(CellStruct* pUCell) const { JMP_THIS(0x41BEA0); }
#else
    virtual CellStruct* GetMapCoords(CellStruct* pUCell) const;
#endif
    /// VA: 0x005F6960
#if defined(RA2_YRPP_GAME)
    virtual CellClass* GetCell() const { JMP_THIS(0x5F6960); }
#else
    virtual CellClass* GetCell() const;
#endif

// These two query virtual GetDestination(nullptr), not Location or GetCoords.
    /// VA: 0x005F69C0
#if defined(RA2_YRPP_GAME)
    virtual CellStruct* GetMapCoordsAgain(CellStruct* pUCell) const { JMP_THIS(0x5F69C0); }
#else
    virtual CellStruct* GetMapCoordsAgain(CellStruct* pUCell) const;
#endif
    /// VA: 0x005F6A10
#if defined(RA2_YRPP_GAME)
    virtual CellClass* GetCellAgain() const { JMP_THIS(0x5F6A10); }
#else
    virtual CellClass* GetCellAgain() const;
#endif

    /// VA: 0x005F5F40
#if defined(RA2_YRPP_GAME)
    virtual int GetHeight() const { JMP_THIS(0x5F5F40); }
#else
    virtual int GetHeight() const;
#endif
    /// VA: 0x005F5FA0
    virtual void SetHeight(DWORD height);
    /// VA: 0x005F5F30
    virtual int GetZ() const { return Location.Z; }
    /// VA: unknown (legacy placeholder).
    virtual bool IsBeingWarpedOut() const R0;
    /// VA: unknown (legacy placeholder).
    virtual bool IsWarpingIn() const R0;
    /// VA: unknown (legacy placeholder).
    virtual bool IsWarpingSomethingOut() const R0;
    /// VA: unknown (legacy placeholder).
    virtual bool IsNotWarping() const R0;
    /// VA: unknown (legacy placeholder).
    virtual LightConvertClass *GetRemapColour() const R0;

    // technically it takes an ecx<this> , but it's not used and ecx is immediately overwritten on entry
    // draws the mind control line when unit is selected
    static void DrawALinkTo(int src_X, int src_Y, int src_Z, int dst_X, int dst_Y, int dst_Z, ColorStruct color)
        { PUSH_VAR32(color); PUSH_VAR32(dst_Z); PUSH_VAR32(dst_Y); PUSH_VAR32(dst_X);
            PUSH_VAR32(src_Z); PUSH_VAR32(src_Y); PUSH_VAR32(src_X); CALL(0x704E40); }

    /// VA: 0x005F5C60
    double GetHealthPercentage() const
        { return static_cast<double>(this->Health) / this->GetType()->Strength; }

    /// VA: 0x005F5C80.
#if defined(RA2_YRPP_GAME)
    void SetHealthPercentage(double percentage)
        { JMP_THIS(0x5F5C80); }
#else
    void SetHealthPercentage(double percentage);
#endif

    /// VA: 0x005F5CD0.
#if defined(RA2_YRPP_GAME)
    bool IsRedHP() const
        { JMP_THIS(0x5F5CD0); }
#else
    bool IsRedHP() const;
#endif

    /// VA: 0x005F5D20.
#if defined(RA2_YRPP_GAME)
    bool IsYellowHP() const
        { JMP_THIS(0x5F5D20); }
#else
    bool IsYellowHP() const;
#endif

    /// VA: 0x005F5D90.
#if defined(RA2_YRPP_GAME)
    bool IsGreenHP() const
        { JMP_THIS(0x5F5D90); }
#else
    bool IsGreenHP() const;
#endif

    /// VA: 0x005F5DD0.
#if defined(RA2_YRPP_GAME)
    HealthState GetHealthStatus() const
        { JMP_THIS(0x5F5DD0); }
#else
    HealthState GetHealthStatus() const;
#endif

    /// VA: 0x005F5B50.
    bool AttachTrigger(TagClass* pTag)
#if defined(RA2_YRPP_GAME)
        { JMP_THIS(0x5F5B50); }
#else
        ;
#endif

    /// VA: 0x005F5B4C.
    void ReplaceTag(TagClass* pTag)
        { JMP_THIS(0x5F5B4C); }

    /// VA: 0x005F5F00.
    int GetCellLevel() const;

    /// VA: 0x005F6CD0.
    bool IsCrushable(TechnoClass* pCrusher)
        { JMP_THIS(0x5F6CD0); }

    CellStruct GetMapCoords() const {
        CellStruct ret;
        this->GetMapCoords(&ret);
        return ret;
    }

    CellStruct GetMapCoordsAgain() const {
        CellStruct ret;
        this->GetMapCoordsAgain(&ret);
        return ret;
    }

    // On non-buildings this is same as GetCenterCoord(), on buildings it returns the target coordinate that is affected by TargetCoordOffset.
    CoordStruct GetTargetCoords() const
    {
        CoordStruct ret;
        this->GetTargetCoords(&ret);
        return ret;
    }

    CoordStruct GetRenderCoords() const {
        CoordStruct ret;
        this->GetRenderCoords(&ret);
        return ret;
    }

    CoordStruct GetDockCoords(TechnoClass* docker) const
    {
        CoordStruct ret;
        this->GetDockCoords(&ret, docker);
        return ret;
    }

    CoordStruct GetFLH(int idxWeapon, const CoordStruct& base) const {
        CoordStruct ret;
        this->GetFLH(&ret, idxWeapon, base);
        return ret;
    }

#if !defined(RA2_YRPP_GAME)
    using AbstractClass::GetCoords;
    using AbstractClass::GetCenterCoords;
    CoordStruct* GetCoords(CoordStruct*) const override;
    CoordStruct* GetCenterCoords(CoordStruct*) const override;
    bool IsDead() const override;
#endif
    // Constructor NEVER CALL IT DIRECTLY
    /*ObjectClass()  noexcept
        { JMP_THIS(0x5F3900); }*/

protected:
#if !defined(RA2_YRPP_GAME)
    ObjectClass() noexcept;
#endif
    explicit __forceinline ObjectClass(noinit_t)  noexcept
        : AbstractClass(noinit_t())
    { }

    // Properties
public:
    DWORD              unknown_24;
    DWORD              unknown_28;
    int                FallRate;     // how fast is it falling down? only works if FallingDown is set below, and actually positive numbers will move the thing UPWARDS
    ObjectClass*       NextObject;   // Next Object in the same cell or transport. This is a linked list of Objects.
    TagClass*          AttachedTag;  // Should be TagClass , TODO: change when implemented
    BombClass*         AttachedBomb; // Ivan's little friends.
    DECLARE_PROPERTY(AudioController, AmbientSoundController); // the "mofo" struct, evil evil stuff
    DECLARE_PROPERTY(AudioController, CustomSoundController);  // the "mofo" struct, evil evil stuff
    int                CustomSound;
    bool               BombVisible;    // In range of player's bomb seeing units, so should draw it
    PROTECTED_PROPERTY(BYTE, align_69[0x3]);
    int                Health;         // The current Health.
    int                EstimatedHealth;// used for auto-targeting threat estimation
    bool               IsOnMap;        // has this object been placed on the map?
    PROTECTED_PROPERTY(BYTE, align_75[0x3]);
    DWORD              unknown_78;
    DWORD              unknown_7C;
    bool               NeedsRedraw;
    bool               InLimbo;        // act as if it doesn't exist - e.g., post mortem state before being deleted
    bool               InOpenToppedTransport;
    bool               IsSelected;     // Has the player selected this Object?
    bool               HasParachute;   // Is this Object parachuting?
    PROTECTED_PROPERTY(BYTE, align_85[0x3]);
    AnimClass*         Parachute;      // Current parachute Anim.
    bool               OnBridge;
    bool               IsFallingDown;
    bool               WasFallingDown; // last falling state when FootClass::Update executed. used to find out whether it changed.
    bool               IsABomb;        // if set, will explode after FallingDown brings it to contact with the ground
    bool               IsAlive;        // Self-explanatory.
    PROTECTED_PROPERTY(BYTE, align_91[0x3]);
    Layer              LastLayer;
    bool               IsInLogic;      // has this object been added to the logic collection?
    bool               IsVisible;      // was this object in viewport when drawn?
    PROTECTED_PROPERTY(BYTE, align_99[0x2]);
    CoordStruct        Location;       // Absolute current 3D location (in leptons)
    LineTrail*         LineTrailer;
};
