/*
    Base class for buildable objects
*/

#pragma once

#include "yrpp/platform/ABI.h"

#include "yrpp/Facing.h"

#include "yrpp/Matrix3D.h"
#include "yrpp/RadioClass.h"
#include "yrpp/RadBeam.h"
#include "yrpp/TechnoTypeClass.h"
#include "yrpp/CaptureManagerClass.h"
#include "yrpp/SlaveManagerClass.h"
#include "yrpp/TeamClass.h"
#include "yrpp/TemporalClass.h"
#include "yrpp/LaserDrawClass.h"
#include "yrpp/Helpers/Template.h"
#include "yrpp/StageClass.h"
#include "yrpp/PlanningTokenClass.h"

// forward declarations
class AirstrikeClass;
class AnimClass;
class BulletClass;
class BuildingClass;
class CellClass;
class HouseClass;
class FootClass;
class HouseClass;
class InfantryTypeClass;
class ObjectTypeClass;
class ParticleSystemClass;
class SpawnManagerClass;
class WaveClass;

class EventClass;

#include "yrpp/TransitionTimer.h"

struct VeterancyStruct
{
    VeterancyStruct() = default;

    explicit VeterancyStruct(double value) noexcept
    {
        this->Add(value);
    }

    void Add(int ownerCost, int victimCost) noexcept
    {
        this->Add(static_cast<double>(victimCost)
            / (ownerCost * RulesClass::Instance->VeteranRatio));
    }

    void Add(double value) noexcept
    {
        auto val = this->Veterancy + value;

        if (val > RulesClass::Instance->VeteranCap)
        {
            val = RulesClass::Instance->VeteranCap;
        }

        this->Veterancy = static_cast<float>(val);
    }

    Rank GetRemainingLevel() const noexcept
    {
        if (this->Veterancy >= 2.0f)
        {
            return Rank::Elite;
        }

        if (this->Veterancy >= 1.0f)
        {
            return Rank::Veteran;
        }

        return Rank::Rookie;
    }

    bool IsNegative() const noexcept
    {
        return this->Veterancy < 0.0f;
    }

    bool IsRookie() const noexcept
    {
        return this->Veterancy >= 0.0f && this->Veterancy < 1.0f;
    }

    bool IsVeteran() const noexcept
    {
        return this->Veterancy >= 1.0f && this->Veterancy < 2.0f;
    }

    bool IsElite() const noexcept
    {
        return this->Veterancy >= 2.0f;
    }

    void Reset() noexcept
    {
        this->Veterancy = 0.0f;
    }

    void SetRookie(bool notReally = true) noexcept
    {
        this->Veterancy = notReally ? -0.25f : 0.0f;
    }

    void SetVeteran(bool yesReally = true) noexcept
    {
        this->Veterancy = yesReally ? 1.0f : 0.0f;
    }

    void SetElite(bool yesReally = true) noexcept
    {
        this->Veterancy = yesReally ? 2.0f : 0.0f;
    }

    float Veterancy { 0.0f };
};

class PassengersClass
{
public:
    int NumPassengers;
    FootClass* FirstPassenger;

    /// VA: 0x004733A0.
    void AddPassenger(FootClass* pPassenger);

    /// VA: 0x004734B0.
#if defined(RA2_YRPP_GAME)
    void RemovePassenger(FootClass* pPassenger) { JMP_THIS(0x4734B0); }
#else
    void RemovePassenger(FootClass* pPassenger);
#endif

    /// VA: 0x00473450
    FootClass* GetFirstPassenger() const
    { return this->FirstPassenger; }

    /// VA: 0x00473430
#if defined(RA2_YRPP_GAME)
    FootClass* RemoveFirstPassenger()
    { JMP_THIS(0x473430); }
#else
    FootClass* RemoveFirstPassenger();
#endif

    /// VA: 0x00473460.
    int GetTotalSize() const;

    /// VA: 0x00473500.
    int IndexOf(FootClass* candidate) const;

    PassengersClass() : NumPassengers(0), FirstPassenger(nullptr) { };

    ~PassengersClass() { };
};

struct FlashData
{
    int DurationRemaining;
    bool FlashingNow;

    /// VA: 0x004CC770.
#if defined(RA2_YRPP_GAME)
    bool Update()
    { JMP_THIS(0x4CC770); }
#else
    bool Update();
#endif
};

struct RecoilData
{
    enum class RecoilState : unsigned int
    {
        Inactive = 0,
        Compressing = 1,
        Holding = 2,
        Recovering = 3,
    };

    TurretControl Turret;
    float TravelPerFrame;
    float TravelSoFar;
    RecoilState State;
    int TravelFramesLeft;

    /// VA: 0x0070ED10.
#if defined(RA2_YRPP_GAME)
    void Update()
    { JMP_THIS(0x70ED10); }
#else
    void Update();
#endif

    /// VA: 0x0070ECE0.
    void Fire()
    { JMP_THIS(0x70ECE0); }
};

class NOVTABLE TechnoClass : public RadioClass
{
public:
    /// VA: 0x006FFEC0
    Action MouseOverObject(ObjectClass const* object,bool ignoreForce=false) const override;
    /// VA: 0x006F9E50
#if defined(RA2_YRPP_GAME)
    void Update() override { JMP_THIS(0x6F9E50); }
#else
    void Update() override;
#endif
    /// VA: 0x006FC030
#if defined(RA2_YRPP_GAME)
    bool CanBeSelectedNow() const override { JMP_THIS(0x6FC030); }
#else
    bool CanBeSelectedNow() const override;
#endif
    /// VA: 0x0070E5A0
#if defined(RA2_YRPP_GAME)
    void UpdateInvulnerabilityTint() { JMP_THIS(0x70E5A0); }
#else
    void UpdateInvulnerabilityTint();
#endif
    /// VA: 0x0070E920
#if defined(RA2_YRPP_GAME)
    void UpdateAirstrikeTint() { JMP_THIS(0x70E920); }
#else
    void UpdateAirstrikeTint();
#endif
    /// VA: 0x0070F1D0
    void DrawBehindMark(Point2D* position,RectangleStruct* bounds) { JMP_THIS(0x70F1D0); }
    static const auto AbsDerivateID = AbstractFlags::Techno;
    /// VA: 0x6F4AB0
    RadioCommand ReceiveCommand(TechnoClass* sender,RadioCommand command,AbstractClass*& data) override;
    /// VA: 0x70D4A0
    void RemoveFromTargeting();

    /// Global VA: 0x00A8EC78.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(DynamicVectorClass<TechnoClass*>, Array, 0xA8EC78u)
#else
    static DynamicVectorClass<TechnoClass*>& Array;
#endif

    // Original global gate for the Tactical action-line pass; distinct from
    // GameOptionsClass::UnitActionLines.
    /// Global VA: 0x00843108.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(bool, ActionLines, 0x843108u)
#else
    static bool& ActionLines;
#endif

    // IPersistStream
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Load(IStream* pStm) R0;
    /// VA: unknown (legacy placeholder).
    virtual HRESULT YRPP_STDCALL Save(IStream* pStm, BOOL fClearDirty) R0;

    // Destructor
    /// VA: 0x006F4500
#if defined(RA2_YRPP_GAME)
    virtual ~TechnoClass() RX;
#else
    virtual ~TechnoClass();
#endif

    // AbstractClass
    /// VA: 0x006F4A70
    bool Mark(MarkType mark) override;
    /// VA: 0x006F9DB0
#if defined(RA2_YRPP_GAME)
    int GetOwningHouseIndex() const override { JMP_THIS(0x6F9DB0); }
#else
    int GetOwningHouseIndex() const override;
#endif
    /// VA: 0x006F9DC0
#if defined(RA2_YRPP_GAME)
    HouseClass* GetOwningHouse() const override { JMP_THIS(0x6F9DC0); }
#else
    HouseClass* GetOwningHouse() const override;
#endif

    /// VA: 0x007077C0
#if defined(RA2_YRPP_GAME)
    void PointerExpired(AbstractClass* object, bool removed) override { JMP_THIS(0x7077C0); }
#else
    void PointerExpired(AbstractClass* object, bool removed) override;
#endif

    // ObjectClass
    /// VA: 0x00700600
#if defined(RA2_YRPP_GAME)
    Action MouseOverCell(const CellStruct* cell,bool checkFog=false,bool ignoreForce=false) const override { JMP_THIS(0x700600); }
#else
    Action MouseOverCell(const CellStruct* cell,bool checkFog=false,bool ignoreForce=false) const override;
#endif
    /// VA: 0x007010D0
#if defined(RA2_YRPP_GAME)
    bool IsActive() const override { JMP_THIS(0x7010D0); }
#else
    bool IsActive() const override;
#endif
    /// VA: 0x00700C40
#if defined(RA2_YRPP_GAME)
    bool IsControllable() const override { JMP_THIS(0x700C40); }
#else
    bool IsControllable() const override;
#endif
    /// VA: 0x0070C5F0
    bool IsNotWarping() const override { return !BeingWarpedOut && !WarpingOut; }
    /// VA: 0x0070ADC0
#if defined(RA2_YRPP_GAME)
    void See(DWORD incremental, DWORD dontMap) override { JMP_THIS(0x70ADC0); }
#else
    void See(DWORD incremental, DWORD dontMap) override;
#endif
    /// VA: 0x00703850
    void Reveal() override { Uncloak(false); }
    /// VA: 0x00702D40
    void RegisterDestruction(TechnoClass* destroyer) override;
    /// VA: 0x00703230
    void RegisterKill(HouseClass* destroyer) override;
    /// VA: 0x006F4A40
    void Undiscover() override;
    /// VA: 0x006F9DD0
#if defined(RA2_YRPP_GAME)
    void Flash(int duration) override { JMP_THIS(0x6F9DD0); }
#else
    void Flash(int duration) override;
#endif
    /// VA: 0x006F7970
#if defined(RA2_YRPP_GAME)
    bool IsCloseEnough3D(const CoordStruct& coords,int weapon) const override { JMP_THIS(0x6F7970); }
#else
    bool IsCloseEnough3D(const CoordStruct& coords,int weapon) const override;
#endif
    /// VA: 0x007012C0
#if defined(RA2_YRPP_GAME)
    int GetWeaponRange(int weapon) const override { JMP_THIS(0x7012C0); }
#else
    int GetWeaponRange(int weapon) const override;
#endif
    /// VA: 0x006F5090
#if defined(RA2_YRPP_GAME)
    void UpdatePosition(PCPType reason) override { JMP_THIS(0x6F5090); }
#else
    void UpdatePosition(PCPType reason) override;
#endif
    /// VA: 0x006F4960
#if defined(RA2_YRPP_GAME)
    bool DiscoveredBy(HouseClass* house) override { JMP_THIS(0x6F4960); }
#else
    bool DiscoveredBy(HouseClass* house) override;
#endif
    /// VA: 0x0041BF40
    bool IsIronCurtained() const override { return IronCurtainTimer.GetTimeLeft() > 0; }
    /// VA: 0x0041C010
    bool IsDisguised() const override { return Disguised; }
    /// VA: 0x0070C5B0
    bool IsBeingWarpedOut() const override { return BeingWarpedOut; }
    /// VA: 0x0070C5C0
    bool IsWarpingIn() const override { return WarpingOut; }
    /// VA: 0x0070C5D0
#if defined(RA2_YRPP_GAME)
    bool IsWarpingSomethingOut() const override { JMP_THIS(0x70C5D0); }
#else
    bool IsWarpingSomethingOut() const override;
#endif
    /// VA: 0x00710410
    void AnimPointerExpired(AnimClass* anim) override {
        if(BehindAnim==anim)BehindAnim=nullptr;
        if(DrainAnim==anim)DrainAnim=nullptr;
        if(MindControlRingAnim==anim)MindControlRingAnim=nullptr;
        if(DeployAnim==anim)DeployAnim=nullptr;
        ObjectClass::AnimPointerExpired(anim);
    }
    /// VA: 0x006F3270
    TechnoTypeClass* GetTechnoType() const override { return static_cast<TechnoTypeClass*>(GetType()); }
    /// VA: 0x006F60D0
#if defined(RA2_YRPP_GAME)
    void DrawBehind(Point2D* point,RectangleStruct* bounds) const override JMP_THIS(0x006F60D0);
#else
    void DrawBehind(Point2D* point,RectangleStruct* bounds) const override;
#endif
    /// VA: 0x006F5190
#if defined(RA2_YRPP_GAME)
    void DrawExtras(Point2D* point,RectangleStruct* bounds) const override JMP_THIS(0x006F5190);
#else
    void DrawExtras(Point2D* point,RectangleStruct* bounds) const override;
#endif
    /// VA: 0x006F6AC0.
    virtual bool Limbo() override;
    /// VA: 0x006F6CA0
    bool Unlimbo(const CoordStruct& where,DirType facing) override;

    /// VA: 0x00701900
    DamageState ReceiveDamage(int* damage,int distance,WarheadTypeClass* warhead,ObjectClass* source,
        bool ignoreDefenses,bool preventEscape,HouseClass* sourceHouse) override;


    // TechnoClass
    /// VA: unknown (legacy placeholder).
    virtual bool IsUnitFactory() const R0;
    /// VA: 0x0070C5A0
    virtual bool IsCloakable() const { return Cloakable; }
    /// VA: 0x00703860
#if defined(RA2_YRPP_GAME)
    VisualType VisualCharacter(VARIANT_BOOL raw,HouseClass* asking) const override { JMP_THIS(0x703860); }
#else
    VisualType VisualCharacter(VARIANT_BOOL raw,HouseClass* asking) const override;
#endif
    /// VA: 0x6F3280
    virtual bool CanScatter() const {
        return GetCurrentMission()!=Mission::Sleep&&GetCurrentMission()!=Mission::Sticky
            &&GetCurrentMission()!=Mission::Unload&&!GetTechnoType()->IsTrain;
    }
    /// VA: unknown (legacy placeholder).
    virtual bool BelongsToATeam() const R0;
    /// VA: 0x0070BE80
#if defined(RA2_YRPP_GAME)
    virtual bool ShouldSelfHealOneStep() const { JMP_THIS(0x70BE80); }
#else
    virtual bool ShouldSelfHealOneStep() const;
#endif
    /// VA: 0x006F9E10
#if defined(RA2_YRPP_GAME)
    virtual bool IsVoxel() const { JMP_THIS(0x6F9E10); }
#else
    virtual bool IsVoxel() const;
#endif
    /// VA: unknown (legacy placeholder).
    virtual bool vt_entry_29C() R0;
    /// VA: 0x006FBDC0.
    virtual bool IsReadyToCloak() const JMP_THIS(0x6FBDC0);
    /// VA: 0x006FBC90.
    virtual bool ShouldNotBeCloaked() const JMP_THIS(0x6FBC90);
    /// VA: 0x004E0150
    virtual DirStruct* TurretFacing(DirStruct* pBuffer) const { *pBuffer=PrimaryFacing.Current();return pBuffer; }
    /// VA: 0x00701120
    virtual bool IsArmed() const { auto* weapon = GetTurretWeapon(); return weapon && weapon->WeaponType; }
    /// VA: 0x0070C620
#if defined(RA2_YRPP_GAME)
    virtual bool vt_entry_2B0() const { JMP_THIS(0x70C620); }
#else
    virtual bool vt_entry_2B0() const;
#endif
    /// VA: 0x00708BC0
#if defined(RA2_YRPP_GAME)
    virtual double GetStoragePercentage() const { JMP_THIS(0x708BC0); }
#else
    virtual double GetStoragePercentage() const;
#endif
    /// VA: unknown (legacy placeholder).
    virtual int GetPipFillLevel() const R0;
    /// VA: 0x0070ADA0
#if defined(RA2_YRPP_GAME)
    virtual int GetRefund() const { JMP_THIS(0x70ADA0); }
#else
    virtual int GetRefund() const;
#endif
    /// VA: 0x00708B40
#if defined(RA2_YRPP_GAME)
    virtual int GetThreatValue() const { JMP_THIS(0x708B40); }
#else
    virtual int GetThreatValue() const;
#endif
    /// VA: 0x00459D90
    virtual bool IsInSameZoneAs(AbstractClass* pTarget) { return true; } // Original base stub; Foot overrides.
    /// VA: unknown (legacy placeholder).
    virtual DWORD vt_entry_2C8(DWORD dwUnk, DWORD dwUnk2) R0;
    /// VA: unknown (legacy placeholder).
    virtual bool IsInSameZoneAsCoords(const CoordStruct& coord) R0;  // Are the coords reachable?
    /// VA: 0x006F3950.
    virtual int GetCrewCount() const JMP_THIS(0x6F3950);
    /// VA: unknown (legacy placeholder).
    virtual int GetAntiAirValue() const R0;
    /// VA: unknown (legacy placeholder).
    virtual int GetAntiArmorValue() const R0;
    /// VA: unknown (legacy placeholder).
    virtual int GetAntiInfantryValue() const R0;
    /// VA: 0x0070D980
    virtual void GotHijacked() { JMP_THIS(0x70D980); }
    /// VA: 0x006F3330
#if defined(RA2_YRPP_GAME)
    virtual int SelectWeapon(AbstractClass* pTarget) const { JMP_THIS(0x6F3330); }
#else
    virtual int SelectWeapon(AbstractClass* pTarget) const;
#endif
    /// VA: 0x006F3820
#if defined(RA2_YRPP_GAME)
    virtual int SelectNavalTargeting(AbstractClass* pTarget) const { JMP_THIS(0x6F3820); }
#else
    virtual int SelectNavalTargeting(AbstractClass* pTarget) const;
#endif
    /// VA: 0x00704350
#if defined(RA2_YRPP_GAME)
    virtual int GetZAdjustment() const { JMP_THIS(0x704350); }
#else
    virtual int GetZAdjustment() const;
#endif
    /// VA: 0x00459DA0
    virtual ZGradient GetZGradient() const RT(ZGradient);
    /// VA: unknown (legacy placeholder).
    virtual CellStruct GetLastFlightMapCoords() const RT(CellStruct);
    /// VA: unknown (legacy placeholder).
    virtual void SetLastFlightMapCoords(CellStruct coord) RX;
    /// VA: unknown (legacy placeholder).
    virtual CellStruct* vt_entry_2FC(CellStruct* Buffer, DWORD dwUnk2, DWORD dwUnk3) const R0;
    /// VA: 0x006F3D60
    virtual CoordStruct* vt_entry_300(CoordStruct* Buffer, DWORD weapon) const;
    /// VA: unknown (legacy placeholder).
    virtual FacingType DesiredLoadDir(FootClass* passenger, CellStruct& cell) const RT(FacingType);
    /// VA: 0x00708D70
    virtual DirStruct* GetRealFacing(DirStruct* pBuffer) const { return TurretFacing(pBuffer); }
    /// VA: 0x707D20
#if defined(RA2_YRPP_GAME)
    virtual InfantryTypeClass* GetCrew() const { JMP_THIS(0x707D20); }
#else
    virtual InfantryTypeClass* GetCrew() const;
#endif
    /// VA: 0x700D10
    virtual bool vt_entry_310() const R0;
    /// VA: 0x700D50
    #if defined(RA2_YRPP_GAME)
    virtual bool CanDeploySlashUnload() const { JMP_THIS(0x700D50); }
#else
    virtual bool CanDeploySlashUnload() const;
#endif
    /// VA: 0x006FCFA0
#if defined(RA2_YRPP_GAME)
    virtual int GetROF(int nWeapon) const { JMP_THIS(0x6FCFA0); }
#else
    virtual int GetROF(int weapon) const;
#endif
    // Parameter controls behaviour. Unlisted values behave same as 1.
    // -1 = returns -1 as range
    // 0  = uses GuardRange/weapon range as is
    // 1  = doubles range, if less than 0 increase to 0, if more than 4096 leptons cap it at that
    // 2  = doubles range, if less than 1792 leptons increase to 1792, if more than that return as is unless more than 4096 in which case cap at 4096.
    /// VA: 0x00707E60
    virtual int GetGuardRange(int control) const;
    /// VA: unknown (legacy placeholder).
    virtual bool vt_entry_320() const R0;
    /// VA: 0x0070D1D0
#if defined(RA2_YRPP_GAME)
    virtual bool IsRadarVisible(int* detection) const { JMP_THIS(0x70D1D0); }
#else
    virtual bool IsRadarVisible(int* detection) const;
#endif
    /// VA: 0x0070D420
#if defined(RA2_YRPP_GAME)
    virtual bool IsSensorVisibleToPlayer() const { JMP_THIS(0x70D420); }
#else
    virtual bool IsSensorVisibleToPlayer() const;
#endif
    /// VA: 0x0070D460
#if defined(RA2_YRPP_GAME)
    virtual bool IsSensorVisibleToHouse(HouseClass* house) const { JMP_THIS(0x70D460); }
#else
    virtual bool IsSensorVisibleToHouse(HouseClass* house) const;
#endif
    /// VA: unknown (legacy placeholder).
    virtual bool IsEngineer() const R0;
    /// VA: unknown (legacy placeholder).
    virtual void ProceedToNextPlanningWaypoint() RX;
    /// VA: unknown (legacy placeholder).
    virtual CellStruct* ScanForTiberium(CellStruct*, int range, DWORD dwUnk3) const R0;
    /// VA: unknown (legacy placeholder).
    virtual bool EnterGrinder() R0;
    /// VA: unknown (legacy placeholder).
    virtual bool EnterBioReactor() R0;
    /// VA: unknown (legacy placeholder).
    virtual bool EnterTankBunker() R0;
    /// VA: unknown (legacy placeholder).
    virtual bool EnterBattleBunker() R0;
    /// VA: unknown (legacy placeholder).
    virtual bool GarrisonStructure() R0;
    /// VA: unknown (legacy placeholder).
    virtual bool IsPowerOnline() const R0;
    /// VA: 0x00708D90
#if defined(RA2_YRPP_GAME)
    virtual void QueueVoice(int idxVoc) { JMP_THIS(0x708D90); }
#else
    virtual void QueueVoice(int idxVoc);
#endif
    /// VA: 0x00709020
    virtual int VoiceEnter();
    /// VA: 0x00709060
    virtual int VoiceHarvest();
    /// VA: unknown (legacy placeholder).
    virtual int VoiceSelect() R0;
    /// VA: 0x00708DC0
    virtual int VoiceCapture();
    /// VA: 0x00708FC0
#if defined(RA2_YRPP_GAME)
    virtual int VoiceMove() { JMP_THIS(0x708FC0); }
#else
    virtual int VoiceMove();
#endif
    /// VA: 0x00708E00
#if defined(RA2_YRPP_GAME)
    virtual int VoiceDeploy() { JMP_THIS(0x708E00); }
#else
    virtual int VoiceDeploy();
#endif
    /// VA: 0x007090A0
#if defined(RA2_YRPP_GAME)
    virtual int VoiceAttack(AbstractClass* pTarget) { JMP_THIS(0x7090A0); }
#else
    virtual int VoiceAttack(AbstractClass* pTarget);
#endif
    /// VA: 0x6FFE00
#if defined(RA2_YRPP_GAME)
    virtual bool ClickedEvent(EventType event) { JMP_THIS(0x6FFE00); }
#else
    virtual bool ClickedEvent(EventType event);
#endif

    // depending on the mission you click, cells/Target are not always needed
    /// VA: 0x006FFBE0
#if defined(RA2_YRPP_GAME)
    virtual bool ClickedMission(Mission mission, AbstractClass* target, AbstractClass* destination, CellClass* follow) { JMP_THIS(0x6FFBE0); }
#else
    virtual bool ClickedMission(Mission mission, AbstractClass* target, AbstractClass* destination, CellClass* follow);
#endif
    /// VA: 0x0070EFD0
    virtual bool IsUnderEMP() const { return static_cast<int>(EMPLockRemaining) > 0; }
    /// VA: 0x00459E40
    virtual bool IsParalyzed() const { return false; }
    /// VA: unknown (legacy placeholder).
    virtual bool CanCheer() const R0;
    /// VA: unknown (legacy placeholder).
    virtual void Cheer(bool Force) RX;
    /// VA: 0x0070EFE0
    virtual int GetDefaultSpeed() const { auto* type = GetTechnoType(); return type ? type->Speed : 0; }
    /// VA: 0x0070D670
    virtual void DecreaseAmmo() { if(Ammo>0)--Ammo; }
    /// VA: unknown (legacy placeholder).
    /// VA: 0x710670
    virtual void AddPassenger(FootClass* pPassenger);
    /// VA: 0x0070EF00
#if defined(RA2_YRPP_GAME)
    virtual bool CanDisguiseAs(AbstractClass* pTarget) const { JMP_THIS(0x70EF00); }
#else
    virtual bool CanDisguiseAs(AbstractClass* pTarget) const;
#endif
    /// VA: 0x00709820
    virtual bool TargetAndEstimateDamage(CoordStruct& coord, ThreatType threat);
    /// VA: 0x006FCD40
    virtual void Stun();
    /// VA: unknown (legacy placeholder).
    virtual bool TriggersCellInset(AbstractClass* pTarget) R0;
    /// VA: 0x006F77B0
#if defined(RA2_YRPP_GAME)
    virtual bool IsCloseEnough(AbstractClass* pTarget, int idxWeapon) const { JMP_THIS(0x6F77B0); }
#else
    virtual bool IsCloseEnough(AbstractClass* target,int weapon) const;
#endif
    /// VA: 0x006F7220
#if defined(RA2_YRPP_GAME)
    bool IsCloseEnough(const CoordStruct& source,AbstractClass* target,const WeaponTypeClass* weapon) const { JMP_THIS(0x6F7220); }
#else
    bool IsCloseEnough(const CoordStruct& source,AbstractClass* target,const WeaponTypeClass* weapon) const;
#endif
    /// VA: 0x006F7780
    virtual bool IsCloseEnoughToAttack(AbstractClass* pTarget) const { return IsCloseEnough(pTarget,SelectWeapon(pTarget)); }
    /// VA: 0x006F7930
#if defined(RA2_YRPP_GAME)
    virtual bool IsCloseEnoughToAttackCoords(const CoordStruct& coords) const { JMP_THIS(0x6F7930); }
#else
    virtual bool IsCloseEnoughToAttackCoords(const CoordStruct& coords) const;
#endif
    /// VA: 0x006F78D0
#if defined(RA2_YRPP_GAME)
    virtual bool InAuxiliarySearchRange(AbstractClass* pTarget) const { JMP_THIS(0x6F78D0); }
#else
    virtual bool InAuxiliarySearchRange(AbstractClass* pTarget) const;
#endif
    /// VA: implementation-defined (pure virtual).
    virtual void Destroyed(ObjectClass* Killer) = 0;
    /// VA: 0x006FC090
    virtual FireError GetFireErrorWithoutRange(AbstractClass* pTarget, int nWeaponIndex) const { return GetFireError(pTarget,nWeaponIndex,false); }
    /// VA: 0x006FC0B0
#if defined(RA2_YRPP_GAME)
    virtual FireError GetFireError(AbstractClass* pTarget, int nWeaponIndex, bool checkRange) const { JMP_THIS(0x6FC0B0); }
#else
    virtual FireError GetFireError(AbstractClass* target, int weapon, bool checkRange) const;
#endif
    /// VA: 0x00703B10
#if defined(RA2_YRPP_GAME)
    bool IsUnderBridge() const { JMP_THIS(0x703B10); }
#else
    bool IsUnderBridge() const;
#endif
    /// VA: 0x006F8DF0.
    virtual AbstractClass* GreatestThreat(ThreatType threat, CoordStruct* pCoord, bool onlyTargetHouseEnemy);
    /// VA: 0x006FCDB0.
#if defined(RA2_YRPP_GAME)
    virtual void SetTarget(AbstractClass* pTarget) JMP_THIS(0x6FCDB0);
#else
    virtual void SetTarget(AbstractClass* pTarget);
#endif
    /// VA: 0x006FDD50
#if defined(RA2_YRPP_GAME)
    virtual BulletClass* Fire(AbstractClass* pTarget, int nWeaponIndex) { JMP_THIS(0x6FDD50); }
#else
    virtual BulletClass* Fire(AbstractClass* pTarget, int nWeaponIndex);
#endif
    /// VA: 0x006F3AD0
    CoordStruct* GetFLH(CoordStruct* output,int weapon,CoordStruct base) const override;
    /// VA: 0x0070BCB0
    CoordStruct* PredictTargetCoords(CoordStruct* output) const;
    /// VA: 0x006FDB80
    int EstimateDamage(TechnoClass* target,WeaponTypeClass* weapon) const;
    /// VA: 0x0070FD70
    void StartDrain(TechnoClass* target) { JMP_THIS(0x70FD70); }
    /// VA: 0x0070C690
    CoordStruct* RailgunBeamDamage(CoordStruct* out,const CoordStruct& from,AbstractClass* target,WeaponTypeClass* weapon) { JMP_THIS(0x70C690); }
    /// VA: 0x006FD570
    void FireElectricBolt(AbstractClass* target) { JMP_THIS(0x6FD570); }
    /// VA: 0x006FD620
    void FireRadiationBeam(AbstractClass* target,bool temporal) { JMP_THIS(0x6FD620); }
    /// VA: 0x006FD800
    void FireRadiationEruption(short spread) { JMP_THIS(0x6FD800); }
    /// VA: 0x7013A0
    void Override_Mission(Mission mission,AbstractClass* target,AbstractClass* destination) override;
    /// VA: 0x7013E0
    bool Mission_Revert() override;
    /// VA: 0x70F850
    virtual void Guard(); // clears target and destination and puts in guard mission
    /// VA: unknown (legacy placeholder).
    /// VA: 0x7014A0
    virtual bool SetOwningHouse(HouseClass* pHouse, bool announce = true);
    /// VA: unknown (legacy placeholder).
    virtual void vt_entry_3D8(const CoordStruct* impact, float amplitude, bool direct) RX;
    /// VA: unknown (legacy placeholder).
    virtual bool Crash(ObjectClass* Killer) R0;
    /// VA: unknown (legacy placeholder).
    virtual bool IsAreaFire() const R0;
    /// VA: 0x0070DD70
    virtual int IsNotSprayAttack() const { return !GetTechnoType()->SprayAttack; }
    /// VA: 0x0070DD90
    virtual int GetSecondaryWeaponIndex() const { return 1; }
    /// VA: unknown (legacy placeholder).
    virtual int IsNotSprayAttack2() const R0;
    /// VA: 0x0070E120
    virtual WeaponStruct* GetDeployWeapon() const { return GetWeapon(IsNotSprayAttack()); }
    /// VA: 0x0070E1A0.
#if defined(RA2_YRPP_GAME)
    virtual WeaponStruct* GetTurretWeapon() const JMP_THIS(0x70E1A0);
#else
    virtual WeaponStruct* GetTurretWeapon() const;
#endif
    /// VA: 0x0070E140
#if defined(RA2_YRPP_GAME)
    virtual WeaponStruct* GetWeapon(int nWeaponIndex) const { JMP_THIS(0x70E140); }
#else
    virtual WeaponStruct* GetWeapon(int nWeaponIndex) const;
#endif
    /// VA: 0x0041BFA0
    virtual bool HasTurret() const { return GetTechnoType()->Turret; }
    /// VA: 0x0041BFB0
    virtual bool CanOccupyFire() const { return false; }
    /// VA: unknown (legacy placeholder).
    virtual int GetOccupyRangeBonus() const R0;
    /// VA: 0x0041BFD0
    virtual int GetOccupantCount() const { return 0; }
    /// VA: 0x00701410
    virtual void OnFinishRepair();
    /// VA: 0x006FB740
#if defined(RA2_YRPP_GAME)
    virtual void UpdateCloak(bool bUnk = 1) { JMP_THIS(0x6FB740); }
#else
    virtual void UpdateCloak(bool bUnk = 1);
#endif
    /// VA: unknown (legacy placeholder).
    virtual void CreateGap() RX;
    /// VA: 0x006FB470
    virtual void DestroyGap() RX;
    /// VA: 0x0070B570
#if defined(RA2_YRPP_GAME)
    virtual void vt_entry_41C() { JMP_THIS(0x70B570); }
#else
    virtual void vt_entry_41C(); // Original Rocking_AI.
#endif
    // Original Try_To_Cloak: generator coverage and reacquisition of attackers.
    /// VA: 0x006F4EB0
#if defined(RA2_YRPP_GAME)
    virtual void Sensed() { JMP_THIS(0x6F4EB0); }
#else
    virtual void Sensed();
#endif
    /// VA: unknown (legacy placeholder).
    virtual void Reload() RX;
    /// VA: unknown (legacy placeholder).
    virtual void vt_entry_428() RX;
    // Returns target's coordinates if on attack mission & have target, otherwise own coordinates.
    /// VA: unknown (legacy placeholder).
    virtual CoordStruct* GetAttackCoordinates(CoordStruct* pCrd) const R0;
    /// VA: 0x00705D50
    virtual bool IsNotWarpingIn() const { return !IsWarpingIn(); }
    /// VA: unknown (legacy placeholder).
    virtual bool vt_entry_434(DWORD dwUnk) const R0;
    /// VA: unknown (legacy placeholder).
    virtual void DrawActionLines(bool Force, DWORD dwUnk2) RX;
    /// VA: 0x007049C0
    void DrawActionLine(CoordStruct from, CoordStruct to, ColorStruct color, bool dashed, bool shadow);
    /// VA: 0x00704E40
    void DrawMindControlLine(CoordStruct from, CoordStruct to, ColorStruct color);
    /// Global VA: 0x00B0EA80.
#if defined(RA2_YRPP_GAME)
    DEFINE_REFERENCE(CDTimerClass, ActionLineTimer, 0xB0EA80u)
#else
    static CDTimerClass& ActionLineTimer;
#endif
    /// VA: 0x0070ED80
    virtual DWORD GetDisguiseFlags(DWORD existingFlags) const;
    /// VA: 0x0070EE30
    virtual bool IsClearlyVisibleTo(HouseClass* house) const;
    /// VA: 0x00706640
    virtual void DrawVoxel(const VoxelStruct& Voxel, DWORD frame, int cacheKey,
        const IndexClass<VoxelIndexKey, VoxelCacheStruct*>& VoxelIndex, const RectangleStruct& Rect, const Point2D& Location,
        const Matrix3D& Matrix, int Intensity, DWORD tint, DWORD excludedFlags);
    /// VA: unknown (legacy placeholder).
    virtual void vt_entry_448(DWORD dwUnk, DWORD dwUnk2) RX;
    /// VA: 0x006F64A0
#if defined(RA2_YRPP_GAME)
    virtual void DrawHealthBar(Point2D* pLocation, RectangleStruct* pBounds, bool bUnk3) const JMP_THIS(0x006F64A0);
#else
    // Native building branch; the original pip-scale extras remain separate.
    virtual void DrawHealthBar(Point2D* pLocation, RectangleStruct* pBounds, bool bUnk3) const;
#endif
    /// VA: 0x00709A90
#if defined(RA2_YRPP_GAME)
    virtual void DrawPipScalePips(Point2D* pLocation, Point2D* pOriginalLocation, RectangleStruct* pBounds) const JMP_THIS(0x00709A90);
#else
    virtual void DrawPipScalePips(Point2D* pLocation, Point2D* pOriginalLocation, RectangleStruct* pBounds) const;
#endif
    /// VA: 0x0070A990
    virtual void DrawVeterancyPips(Point2D* pLocation, RectangleStruct* pBounds) const;
    /// VA: 0x0070AA60
#if defined(RA2_YRPP_GAME)
    virtual void DrawExtraInfo(Point2D const& location, Point2D const& originalLocation, RectangleStruct const& bounds) const JMP_THIS(0x0070AA60);
#else
    virtual void DrawExtraInfo(Point2D const& location, Point2D const& originalLocation, RectangleStruct const& bounds) const;
#endif
    /// VA: 0x007036C0
    virtual void Uncloak(bool silent);
    /// VA: 0x00703770
    virtual void Cloak(bool silent);
    /// VA: 0x70D190
    virtual int GetFlashingIntensity(int currentIntensity) const {
        return (Flashing.DurationRemaining & 2) ? (currentIntensity <= 1500 ? 2000 : 500) : currentIntensity;
    }
    /// VA: unknown (legacy placeholder).
    virtual void UpdateRefinerySmokeSystems() RX;
    /// VA: 0x0070E280.
    virtual DWORD DisguiseAs(AbstractClass* pTarget) JMP_THIS(0x70E280);
    /// VA: unknown (legacy placeholder).
    virtual void ClearDisguise() RX;
    /// VA: 0x007099E0
#if defined(RA2_YRPP_GAME)
    virtual bool IsItTimeForIdleActionYet() const { JMP_THIS(0x7099E0); }
#else
    virtual bool IsItTimeForIdleActionYet() const;
#endif
    /// VA: 0x0041C040
    virtual bool UpdateIdleAction() R0;
    /// VA: unknown (legacy placeholder).
    virtual void vt_entry_47C(AbstractClass* follow) RX;
    /// VA: unknown (legacy placeholder).
    virtual void SetDestination(AbstractClass* pDest, bool bUnk) RX;
    /// VA: 0x00709A40
#if defined(RA2_YRPP_GAME)
    virtual bool EnterIdleMode(bool initial, bool resume) { JMP_THIS(0x709A40); }
#else
    virtual bool EnterIdleMode(bool initial, bool resume);
#endif
    /// VA: 0x0070AF50
#if defined(RA2_YRPP_GAME)
    virtual void UpdateSight(bool incremental, int unused, bool useHouse, HouseClass* house, int sightOverride) { JMP_THIS(0x70AF50); }
#else
    virtual void UpdateSight(bool incremental, int unused, bool useHouse, HouseClass* house, int sightOverride);
#endif
    /// VA: 0x0070B1D0
    virtual void vt_entry_48C(bool keep, int unknown, bool useHouse, HouseClass* house);
    /// VA: unknown (legacy placeholder).
    virtual bool ForceCreate(CoordStruct& coord, DWORD dwUnk = 0) R0;
    /// VA: 0x0070CC90.
    virtual void RadarTrackingStart();
    /// VA: 0x0070CCC0.
    virtual void RadarTrackingStop();
    /// VA: 0x0070CCF0.
    virtual void RadarTrackingFlash();
    /// VA: 0x0070D990
#if defined(RA2_YRPP_GAME)
    virtual void RadarTrackingUpdate(bool force) { JMP_THIS(0x70D990); }
#else
    virtual void RadarTrackingUpdate(bool force);
#endif
    /// VA: 0x0070F000
#if defined(RA2_YRPP_GAME)
    virtual Mission RespondMegaEventMission(EventClass* event) { JMP_THIS(0x70F000); }
#else
    virtual Mission RespondMegaEventMission(EventClass* event);
#endif
    /// VA: unknown (legacy placeholder).
    virtual void ClearMegaMissionData() RX;
    /// VA: unknown (legacy placeholder).
    virtual bool HaveMegaMission() const R0;
    /// VA: unknown (legacy placeholder).
    virtual bool HaveAttackMoveTarget() const R0;
    /// VA: unknown (legacy placeholder).
    virtual Mission GetMegaMission() const RT(Mission);
    /// VA: unknown (legacy placeholder).
    virtual CoordStruct* GetAttackMoveCoords(CoordStruct* pBuffer) R0;
    /// VA: 0x0070F070
#if defined(RA2_YRPP_GAME)
    virtual bool CanUseWaypoint() const { JMP_THIS(0x70F070); }
#else
    virtual bool CanUseWaypoint() const;
#endif
    /// VA: 0x0070F090
    virtual bool CanAttackOnTheMove() const { return GetTechnoType()->CanAttackMove(); }
    /// VA: 0x0070F0E0
    virtual bool MegaMissionIsAttackMove() const R0;
    /// VA: 0x0070F0F0
    virtual bool ContinueMegaMission() R0;
    /// VA: 0x0070F100
    virtual void UpdateAttackMove() RX;
    /// VA: 0x0070F110
    virtual bool RefreshMegaMission() R0;

    // non-virtual

    /// VA: 0x0070F770
#if defined(RA2_YRPP_GAME)
    __declspec(noinline) void ShortenTargetingDelay() { JMP_THIS(0x70F770); }
#else
    void ShortenTargetingDelay();
#endif

    // (re-)starts the reload timer
    /// VA: 0x006FB080.
    void StartReloading()
    { JMP_THIS(0x6FB080); }

    /// VA: 0x006F79A0.
    double ShouldSuppress(CellStruct* coords) const;

    // smooth operator
    const char* get_ID() const
    {
        auto const pType = this->GetType();
        return pType ? pType->get_ID() : nullptr;
    }

    /// VA: 0x006F47A0.
#if defined(RA2_YRPP_GAME)
    int TimeToBuild() const { JMP_THIS(0x6F47A0); }
#else
    int TimeToBuild() const;
#endif

    /// VA: 0x007105E0.
    bool IsMindControlled() const { return MindControlledBy || MindControlledByAUnit; }

    /// VA: 0x0053C450.
    bool CanBePermaMindControlled() const
    { JMP_THIS(0x53C450); }

    /// VA: 0x006FD210.
    LaserDrawClass* CreateLaser(ObjectClass* pTarget, int idxWeapon, WeaponTypeClass* pWeapon, const CoordStruct& Coords)
    { JMP_THIS(0x6FD210); }

    /*
     *  Cell->AddThreat(this->Owner, -this->ThreatPosed);
     *  this->ThreatPosed = 0;
     *  int Threat = this->CalculateThreat(); // this is another gem of a function, to be revealed another time...
     *  this->ThreatPosed = Threat;
     *  Cell->AddThreat(this->Owner, Threat);
     */
    /// VA: 0x0070F6E0.
#if defined(RA2_YRPP_GAME)
    void UpdateThreatInCell(CellClass* cell) { JMP_THIS(0x70F6E0); }
#else
    void UpdateThreatInCell(CellClass* cell);
#endif
    /// VA: 0x0070F670
#if defined(RA2_YRPP_GAME)
    void AddThreatToCell(CellClass* cell) { JMP_THIS(0x70F670); }
#else
    void AddThreatToCell(CellClass* cell);
#endif
    /// VA: 0x0070F6A0
#if defined(RA2_YRPP_GAME)
    void RemoveThreatFromCell(CellClass* cell) { JMP_THIS(0x70F6A0); }
#else
    void RemoveThreatFromCell(CellClass* cell);
#endif

    // CanTargetWhatAmI is a bitfield, if(!(CanTargetWhatAmI & (1 << tgt->WhatAmI())) { fail; }

    // slave of the next one
    /// VA: 0x006F7CA0.
    bool CanAutoTargetObject(
        ThreatType targetFlags,
        int canTargetWhatAmI,
        int wantedDistance,
        TechnoClass* pTarget,
        int* pThreatPosed,
        DWORD dwUnk,
        const CoordStruct* pSourceCoords) const;

    // called by AITeam Attack Target Type and autoscan
    /// VA: 0x006F8960.
    bool TryAutoTargetObject(
        ThreatType targetFlags,
        int canTargetWhatAmI,
        CellStruct* pCoords,
        int range,
        TechnoClass** target,
        int* pThreatPosed,
        int zone);

    /// VA: 0x006F8C10
    int GetWallThreat(const CellStruct* cell) const;
    /// VA: 0x00709550
    void SelectDistributedTarget();

    /// VA: 0x0070FBE0.
    void Reactivate()
    { JMP_THIS(0x70FBE0); }

    /// VA: 0x0070FC90.
    void Deactivate()
    { JMP_THIS(0x70FC90); }

    // this should be the transport, but it's unused
    // marks passenger as "InOpenTopped" for targeting, range scanning and other purposes
    /// VA: 0x00710470.
    void EnteredOpenTopped(TechnoClass* pWho)
    { JMP_THIS(0x710470); }

    // this should be the transport, but it's unused
    // reverses the above
    /// VA: 0x007104A0.
    void ExitedOpenTopped(TechnoClass* pWho)
    { JMP_THIS(0x7104A0); }

    // called when the source unit dies - passengers are about to get kicked out, this basically calls ->ExitedOpenTransport on each passenger
    /// VA: 0x007104C0.
    void MarkPassengersAsExited()
    { JMP_THIS(0x7104C0); }

    /// VA: 0x004598A0.
    bool IsAbsorbAllowed() const
    { JMP_THIS(0x4598A0); }

    // for gattlings
    /// VA: 0x0070DDD0.
    void SetCurrentWeaponStage(int idx)
    { if(idx >= 0) CurrentGattlingStage = idx; }

    /// VA: 0x0070C610.
    void SetArchiveTarget(AbstractClass* pTarget) { ArchiveTarget = pTarget; }

    /// VA: 0x00706BD0.
    void DrawVoxelShadow(VoxelStruct* vxl, int shadow_index, VoxelIndexKey vxl_index_key, IndexClass<ShadowVoxelIndexKey, VoxelCacheStruct*>* shadow_cache,
        RectangleStruct* bound, Point2D* a3, Matrix3D* matrix, bool again, Surface* surface, Point2D shadow_point);

    /// VA: 0x00705E00
    void DrawObject(SHPStruct* pSHP, int nFrame, Point2D* pLocation, RectangleStruct* pBounds,
        int, int, int nZAdjust, ZGradient eZGradientDescIdx, int, int nBrightness, int TintColor,
        SHPStruct* pZShape, int nZFrame, int nZOffsetX, int nZOffsetY, int);

    /// VA: 0x0070CD10.
    double ThreatCoeffients(const ObjectClass* pTarget, const CoordStruct* pLocation) const;

    /// VA: 0x0070DE00.
    int sub_70DE00(int State)
    { if(State >= 0) GattlingValue = State; return State; }

    /// VA: 0x006386E0.
#if defined(RA2_YRPP_GAME)
    void YRPP_FASTCALL ClearPlanningTokens(EventClass* pEvent) { Game::PlanningManager_ClearToken(this,pEvent); }
#else
    void YRPP_FASTCALL ClearPlanningTokens(EventClass* pEvent);
#endif

    /// VA: 0x00710550
#if defined(RA2_YRPP_GAME)
    void SetTargetForPassengers(AbstractClass* pTarget)
    { JMP_THIS(0x710550); }
#else
    void SetTargetForPassengers(AbstractClass* target);
#endif

    /// VA: 0x00707CB0
#if defined(RA2_YRPP_GAME)
    void KillPassengers(TechnoClass* pSource)
    { JMP_THIS(0x707CB0); }
#else
    void KillPassengers(TechnoClass* source);
#endif

    // returns the house that created this object (factoring in Mind Control)
    /// VA: 0x0070F820.
    HouseClass* GetOriginalOwner() const
    { JMP_THIS(0x70F820); }

    // returns the house that controls this techno (replaces the ID with player's ID if needed)
    /// VA: 0x006339B0.
    int GetControllingHouse() const
    { JMP_THIS(0x6339B0); }

    /// VA: 0x0070D690.
#if defined(RA2_YRPP_GAME)
    void FireDeathWeapon(int additionalDamage)
    { JMP_THIS(0x70D690); }
#else
    void FireDeathWeapon(int additionalDamage);
#endif

    /// VA: 0x0070D0D0.
    bool HasAbility(Ability ability) const
    {
        if (!Veterancy.IsVeteran() && !Veterancy.IsElite()) return false;
        auto* type = GetTechnoType();
        return (Veterancy.IsVeteran() && type->VeteranAbilities[ability])
            || (Veterancy.IsElite() && (type->VeteranAbilities[ability] || type->EliteAbilities[ability]));
    }

    /// VA: 0x00734270
#if defined(RA2_YRPP_GAME)
    void ClearSidebarTabObject() const
    { JMP_THIS(0x734270); }
#else
    void ClearSidebarTabObject() const noexcept;
#endif

    /// VA: 0x00705D70.
    LightConvertClass* GetDrawer() const
    { JMP_THIS(0x705D70); }

    /// VA: 0x0070E360.
    int GetEffectTintIntensity(int currentIntensity) const
    { return GetAirstrikeTintIntensity(GetInvulnerabilityTintIntensity(currentIntensity)); }

    /// VA: 0x0070E380.
    int GetInvulnerabilityTintIntensity(int currentIntensity) const;

    /// VA: 0x0070E4B0.
    int GetAirstrikeTintIntensity(int currentIntensity) const;

    /// VA: 0x006F3970.
#if defined(RA2_YRPP_GAME)
    int CombatDamage(int nWeaponIndex) const
    { JMP_THIS(0x6F3970); }
#else
    int CombatDamage(int nWeaponIndex) const;
#endif

    /// VA: 0x0070E1A0.
#if defined(RA2_YRPP_GAME)
    WeaponStruct* GetPrimaryWeapon() const
    { JMP_THIS(0x70E1A0); }
#else
    WeaponStruct* GetPrimaryWeapon() const { return TechnoClass::GetTurretWeapon(); }
#endif

    // TODO(RADAR-PLAN-EXEC): native next-node execution is deferred by the
    // user. This remains a jump/unsupported entry, not a restored method.
    /// VA: 0x006385C0
    bool TryNextPlanningTokenNode()
    { JMP_THIS(0x6385C0); }

    int GetIonCannonValue(AIDifficulty difficulty) const;

    int GetIonCannonValue(AIDifficulty difficulty, int maxHealth) const
    {
        // what TS does
        if (maxHealth > 0 && this->Health > maxHealth)
        {
            return (this->WhatAmI() == AbstractType::Building) ? 3 : 1;
        }

        return this->GetIonCannonValue(difficulty);
    }

    DirStruct TurretFacing() const
    {
        DirStruct ret;
        this->TurretFacing(&ret);
        return ret;
    }

    DirStruct GetRealFacing() const
    {
        DirStruct ret;
        this->GetRealFacing(&ret);
        return ret;
    }

    // Invokes AI response on their 'base' being attacked. Used by buildings, ToProtect=true technos and Whiner=true team members.
    /// VA: 0x00708080.
    void BaseIsAttacked(TechnoClass* pEnemy)
    { JMP_THIS(0x708080); }

    /// VA: 0x007087C0.
    bool CanRetaliateToAttacker(ObjectClass* pAttacker, WarheadTypeClass* pWH);

    /// VA: 0x0070DE70.
#if defined(RA2_YRPP_GAME)
    void GattlingRateUp(int value)
    { JMP_THIS(0x70DE70); }
#else
    void GattlingRateUp(int value);
#endif

    /// VA: 0x0070E000.
#if defined(RA2_YRPP_GAME)
    void GattlingRateDown(int value)
    { JMP_THIS(0x70E000); }
#else
    void GattlingRateDown(int value);
#endif

    /// VA: 0x0070FEE0.
    void ReleaseLocomotor(bool setTarget)
    { JMP_THIS(0x70FEE0); }

    // changes locomotor to the given one, Magnetron style
    // mind that this locks up the source too, Magnetron style
    /// VA: 0x00710000.
    void ImbueLocomotor(FootClass* target, CLSID clsid)
    { JMP_THIS(0x710000); }

    /// VA: 0x00703590.
    CellStruct* NearbyLocation(CellStruct* pCell, AbstractClass* pDest);

    /// VA: 0x007091D0.
#if defined(RA2_YRPP_GAME)
    bool CanPassiveAcquireTargets()
        { JMP_THIS(0x7091D0); }
#else
    bool CanPassiveAcquireTargets();
#endif

    /// VA: 0x0070F7E0.
    bool TargetingTimerFinished()
        { return TargetingTimer.GetTimeLeft()==0; }

    /// VA: 0x00709290.
#if defined(RA2_YRPP_GAME)
    bool CanOpportunityFire()
    { JMP_THIS(0x709290); }
#else
    bool CanOpportunityFire();
#endif

    // Constructor
    /// VA: 0x0070D7E0
    bool UpdateEnterQueue();
    /// VA: 0x70D8F0
    bool ApproachEnterQueue();
    /// VA: 0x006F2B40.
#if defined(RA2_YRPP_GAME)
    TechnoClass(HouseClass* pOwner) noexcept
        : TechnoClass(noinit_t())
    {
        JMP_THIS(0x6F2B40);
    }
#else
    TechnoClass(HouseClass* pOwner) noexcept;
#endif

protected:
    explicit __forceinline TechnoClass(noinit_t) noexcept
        : RadioClass(noinit_t())
    { }

    // Properties
public:
    DECLARE_PROPERTY(FlashData, Flashing);
    DECLARE_PROPERTY(StageClass, Animation); // how the unit animates
    DECLARE_PROPERTY(PassengersClass, Passengers);
    TechnoClass*     Transporter; // unit carrying me
    int              LastFireBulletFrame;
    int              CurrentTurretNumber; // for IFV/gattling/charge turrets
    int              unknown_int_128;
    AnimClass*       BehindAnim;
    AnimClass*       DeployAnim;
    bool             InAir;
    int              CurrentWeaponNumber; // for IFV/gattling
    Rank             CurrentRanking; // only used for promotion detection
    int              CurrentGattlingStage;
    int              GattlingValue; // sum of RateUps and RateDowns
    int              TurretAnimFrame;
    HouseClass* InitialOwner; // only set in ctor
    DECLARE_PROPERTY(VeterancyStruct, Veterancy);
    DWORD            align_154;
    double           ArmorMultiplier;
    double           FirepowerMultiplier;
    DECLARE_PROPERTY(CDTimerClass, IdleActionTimer); // MOO
    DECLARE_PROPERTY(CDTimerClass, RadarFlashTimer);
    DECLARE_PROPERTY(CDTimerClass, TargetingTimer); //Duration = 45 on init!
    DECLARE_PROPERTY(CDTimerClass, IronCurtainTimer);
    DECLARE_PROPERTY(CDTimerClass, IronTintTimer); // how often to alternate the effect color
    int              IronTintStage; // ^
    DECLARE_PROPERTY(CDTimerClass, AirstrikeTimer);
    DECLARE_PROPERTY(CDTimerClass, AirstrikeTintTimer); // tracks alternation of the effect color
    DWORD            AirstrikeTintStage; //  ^
    int              ForceShielded;	//0 or 1, NOT a bool - is this under ForceShield as opposed to IC?
    bool             Deactivated; //Robot Tanks without power for instance
    TechnoClass*     DrainTarget; // eg Disk -> PowerPlant, this points to PowerPlant
    TechnoClass*     DrainingMe;  // eg Disk -> PowerPlant, this points to Disk
    AnimClass*       DrainAnim;
    bool             Disguised;
    DWORD            DisguiseCreationFrame;
    DECLARE_PROPERTY(CDTimerClass, InfantryBlinkTimer); // Rules->InfantryBlinkDisguiseTime , detects mirage firing per description
    DECLARE_PROPERTY(CDTimerClass, DisguiseBlinkTimer); // disguise disruption timer
    bool             UnlimboingInfantry;
    DECLARE_PROPERTY(CDTimerClass, ReloadTimer);
    Point2D          RadarPosition;

    // WARNING! this is actually an index of HouseTypeClass es, but it's being changed to fix typical WW bugs.
    DECLARE_PROPERTY(IndexBitfield<HouseClass*>, DisplayProductionTo); // each bit corresponds to one player on the map, telling us whether that player has (1) or hasn't (0) spied this building, and the game should display what's being produced inside it to that player. The bits are arranged by player ID, i.e. bit 0 refers to house #0 in HouseClass::Array, 1 to 1, etc.; query like ((1 << somePlayer->ArrayIndex) & someFactory->DisplayProductionToHouses) != 0

    int              Group; //0-9, assigned by CTRL+Number, these kinds // also set by aimd TeamType->Group !
    AbstractClass*   ArchiveTarget; // Set when told to guard a unit or such, or to distinguish undeploy and selling. Also used by rally points as well as harvesters for remembering ore fields etc.
    HouseClass*      Owner;
    CloakState       CloakState;
    DECLARE_PROPERTY(StageClass, CloakProgress); // phase from [opaque] -> [fading] -> [transparent] , [General]CloakingStages= long
    DECLARE_PROPERTY(CDTimerClass, CloakDelayTimer); // delay before cloaking again
    float            WarpFactor; // don't ask! set to 0 in CTOR, never modified, only used as ((this->Fetch_ID) + this->WarpFactor) % 400 for something in cloak ripple
    bool             unknown_bool_250;
    CoordStruct      LastSightCoords;
    int              LastSightRange;
    int              LastSightHeight;
    bool             GapSuperCharged; // GapGenerator, when SuperGapRadiusInCells != GapRadiusInCells, you can deploy the gap to boost radius
    bool             GeneratingGap; // is currently generating gap
    int              GapRadius;
    bool             BeingWarpedOut; // is being warped by CLEG
    bool             WarpingOut; // phasing in after chrono-jump
    bool             unknown_bool_272;
    BYTE             unused_273;
    TemporalClass*   TemporalImUsing; // CLEG attacking Power Plant : CLEG's this
    TemporalClass*   TemporalTargetingMe; 	// CLEG attacking Power Plant : PowerPlant's this
    bool             IsImmobilized; // by chrono aftereffects
    DWORD            unknown_280;
    int              ChronoLockRemaining; // countdown after chronosphere warps things around
    CoordStruct      ChronoDestCoords; // teleport loco and chsphere set this
    AirstrikeClass*  Airstrike; //Boris
    bool             Berzerk;
    int            BerzerkDurationLeft;
    DWORD            SprayOffsetIndex; // hardcoded array of xyz offsets for sprayattack, 0 - 7, see 6FE0AD
    bool             Uncrushable; // DeployedCrushable fiddles this, otherwise all 0

    // unless source is Pushy=
    // abs_Infantry source links with abs_Unit target and vice versa - can't attack others until current target flips
    // no checking whether source is Infantry, but no update for other types either
    // old Brute hack
    FootClass*       DirectRockerLinkedUnit;
    FootClass*       LocomotorTarget; // mag->LocoTarget = victim
    FootClass*       LocomotorSource; // victim->LocoSource = mag
    AbstractClass*   Target; //if attacking
    AbstractClass*   LastTarget;
    CaptureManagerClass* CaptureManager; //for Yuris
    TechnoClass*     MindControlledBy;
    bool             MindControlledByAUnit;
    AnimClass*       MindControlRingAnim;
    HouseClass*      MindControlledByHouse; //used for a TAction
    SpawnManagerClass* SpawnManager;
    TechnoClass*     SpawnOwner; // on DMISL , points to DRED and such
    SlaveManagerClass*   SlaveManager;
    TechnoClass*     SlaveOwner; // on SLAV, points to YAREFN
    HouseClass*      OriginallyOwnedByHouse; //used for mind control

    // units point to the Building bunkering them, building points to Foot contained within
    TechnoClass*     BunkerLinkedItem;

    float            PitchAngle; // not exactly, and it doesn't affect the drawing, only internal state of a dropship
    DECLARE_PROPERTY(CDTimerClass, RearmTimer); // Originally named Arm in RA1, but this is more descriptive name.
    int              ChargeTurretDelay;         // Set to same duration (frames) as RearmTimer when weapon is fired. Only used by IsChargeTurret to calculate timespan during which to display turret animation.
    int              Ammo;
    int              Value; // set to actual cost when this gets queued in factory, updated only in building's 42C

    ParticleSystemClass* FireParticleSystem;
    ParticleSystemClass* SparkParticleSystem;
    ParticleSystemClass* NaturalParticleSystem;
    ParticleSystemClass* DamageParticleSystem;
    ParticleSystemClass* RailgunParticleSystem;
    ParticleSystemClass* unk1ParticleSystem;
    ParticleSystemClass* unk2ParticleSystem;
    ParticleSystemClass* FiringParticleSystem;

    WaveClass* Wave; //Beams

    // rocking effect
    float            AngleRotatedSideways; // in this frame, in radians - if abs() exceeds pi/2, it dies
    float            AngleRotatedForwards; // same

    // set these and leave the previous two alone!
    // if these are set, the unit will roll up to pi/4, by this step each frame, and balance back
    float            RockingSidewaysPerFrame; // left to right - positive pushes left side up
    float            RockingForwardsPerFrame; // back to front - positive pushes ass up

    int              HijackerInfantryType; // mutant hijacker

    DECLARE_PROPERTY(StorageClass, Tiberium);
    DWORD            unknown_34C;

    DECLARE_PROPERTY(TransitionTimer, UnloadTimer); // times the deploy, unload, etc. cycles

    DECLARE_PROPERTY(FacingClass, BarrelFacing);
    DECLARE_PROPERTY(FacingClass, PrimaryFacing);
    DECLARE_PROPERTY(FacingClass, SecondaryFacing);
    int              CurrentBurstIndex;
    DECLARE_PROPERTY(CDTimerClass, TargetLaserTimer);
    short            unknown_short_3C8;
    WORD             unknown_3CA;
    bool             CountedAsOwned; // is this techno contained in OwningPlayer->Owned... counts?
    bool             IsSinking;
    bool             WasSinkingAlready; // if(IsSinking && !WasSinkingAlready) { play SinkingSound; WasSinkingAlready = 1; }
    bool             unknown_bool_3CF;
    bool             IsUseless; // Units that are considered to have fulfilled their purpose and useless. Harvesters that cannot do anything without player input are considered this. AI will sell these units on Service Depots.
    bool             HasBeenAttacked; // ReceiveDamage when not HouseClass_IsAlly
    bool             Cloakable;
    bool             IsPrimaryFactory; // doubleclicking a warfac/barracks sets it as primary
    bool             IsALoaner;
    bool             IsInPlayfield;
    DECLARE_PROPERTY(RecoilData, TurretRecoil);
    DECLARE_PROPERTY(RecoilData, BarrelRecoil);
    bool             IsTether;
    bool             IsAlternativeTether;
    bool             IsOwnedByCurrentPlayer; // Returns true if owned by the player on this computer
    bool             DiscoveredByCurrentPlayer;
    bool             DiscoveredByComputer;
    bool             unknown_bool_41D;
    bool             unknown_bool_41E;
    bool             unknown_bool_41F;
    char             SightIncrease; // used for LeptonsPerSightIncrease
    bool             RecruitableA; // these two are like Lenny and Carl, weird purpose and never seen separate
    bool             RecruitableB; // they're usually set on preplaced objects in maps
    bool             IsRadarTracked;
    bool             IsOnCarryall;
    bool             IsCrashing;
    bool             WasCrashingAlready;
    bool             IsBeingManipulated;
    TechnoClass*     BeingManipulatedBy; // set when something is being molested by a locomotor such as magnetron
    // the pointee will be marked as the killer of whatever the victim falls onto
    HouseClass*      ChronoWarpedByHouse;
    bool             unknown_bool_430;
    bool             IsMouseHovering;
    bool             ShouldBeReselectOnUnlimbo;
    TeamClass*       OldTeam;
    bool             CountedAsOwnedSpecial; // for absorbers, infantry uses this to manually control OwnedInfantry count
    bool             Absorbed; // in UnitAbsorb/InfantryAbsorb or smth, lousy memory
    bool             unknown_bool_43A;
    DWORD            unknown_43C;
    DECLARE_PROPERTY(DynamicVectorClass<int>, CurrentTargetThreatValues);
    DECLARE_PROPERTY(DynamicVectorClass<AbstractClass*>, CurrentTargets);

    // if DistributedFire=yes, this is used to determine which possible targets should be ignored in the latest threat scan
    DECLARE_PROPERTY(DynamicVectorClass<AbstractClass*>, AttackedTargets);

    DECLARE_PROPERTY(AudioController, TurretRotateSoundController);

    BOOL            IsTurretRotateSoundPlaying;
    BOOL            TurretIsRotating;

    DECLARE_PROPERTY(AudioController, GattlingSoundController);

    bool             IsGattlingSoundPlaying;
    DWORD            unknown_4BC; // Set to 0 and loaded but never read/used

    DECLARE_PROPERTY(AudioController, UnusedGattlingSoundController); // Called to stop but never actually used to play anything

    bool             IsUnusedGattlingSoundPlaying; // Set to 0 and loaded but never read/used
    DWORD            unknown_4D8; // Set to 0 and loaded but never read/used

    DECLARE_PROPERTY(AudioController, QueuedVoiceSoundController); // Used by select/move/attack voices.

    DWORD            QueuedVoiceIndex;
    DWORD            unknown_4F4;
    bool             unknown_bool_4F8;
    DWORD            unknown_4FC;	//gets initialized with the current Frame, but this is NOT a TimerStruct!
    TechnoClass*     QueueUpToEnter;
    DWORD            EMPLockRemaining;
    DWORD            ThreatPosed; // calculated to include cargo etc
    DWORD            ShouldLoseTargetNow;
    RadBeam*         FiringRadBeam;
    PlanningTokenClass* PlanningToken;
    ObjectTypeClass* Disguise;
    HouseClass*      DisguisedAsHouse;
};

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(TechnoClass) == 0x520);
#endif
