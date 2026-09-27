// Compile-only probe: these are the REAL project declarations, not duplicate
// interface definitions. The IR check asserts the dispatch slot and thiscall CC.
#include "yrpp/platform/ABI.h"
#include "support/yrpp_abi32.hpp"
#include "yrpp/Blitters/Blitter.h"
#include "yrpp/HouseTypeClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MPGameModeClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ObjectClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/InfantryClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/WaypointPathClass.h"
#include "yrpp/CommandClass.h"
#include "yrpp/ParticleSystemClass.h"
#include "yrpp/PowerClass.h"
#include "yrpp/FactoryClass.h"
#include "yrpp/SidebarClass.h"
#include "yrpp/SuperClass.h"
#include "yrpp/TeleportLocomotionClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/AlphaShapeClass.h"
#include <type_traits>

// Deprecated FoggedObjectClass: verify retained layout and virtual slots only.
// R0/RX/RT bodies are placeholders, not migrated behavior.
extern "C" __declspec(noinline) HRESULT RA2ABI_FoggedClassID(FoggedObjectClass* p,CLSID* id){return p->GetClassID(id);}
extern "C" __declspec(noinline) HRESULT RA2ABI_FoggedLoad(FoggedObjectClass* p,IStream* stream){return p->Load(stream);}
extern "C" __declspec(noinline) HRESULT RA2ABI_FoggedSave(FoggedObjectClass* p,IStream* stream,BOOL clear){return p->Save(stream,clear);}
extern "C" __declspec(noinline) void RA2ABI_FoggedDelete(FoggedObjectClass* p){delete p;}
extern "C" __declspec(noinline) AbstractType RA2ABI_FoggedWhat(FoggedObjectClass* p){return p->WhatAmI();}
extern "C" __declspec(noinline) int RA2ABI_FoggedSize(FoggedObjectClass* p){return p->Size();}
extern "C" __declspec(noinline) void RA2ABI_FoggedCRC(FoggedObjectClass* p,CRCEngine& crc){p->ComputeCRC(crc);}
extern "C" __declspec(noinline) CellStruct* RA2ABI_FoggedCell(FoggedObjectClass* p,CellStruct* out){return p->GetCell(out);}
extern "C" __declspec(noinline) CoordStruct* RA2ABI_FoggedCoords(FoggedObjectClass* p,CoordStruct* out){return p->GetCoords(out);}
extern "C" __declspec(noinline) HouseClass* RA2ABI_FoggedOwner(FoggedObjectClass* p){return p->GetOwningHouse();}
extern "C" __declspec(noinline) void RA2ABI_FoggedUpdate(FoggedObjectClass* p){p->Update();}
static_assert(sizeof(AlphaShapeClass)==0x40);
static_assert(offsetof(AlphaShapeClass,AttachedTo)==0x24 && offsetof(AlphaShapeClass,Rect)==0x28);
static_assert(offsetof(AlphaShapeClass,AlphaImage)==0x38 && offsetof(AlphaShapeClass,IsObjectGone)==0x3C);
extern "C" __declspec(noinline) void RA2ABI_AlphaExpired(AlphaShapeClass* p,AbstractClass* target,bool removed){p->PointerExpired(target,removed);}
extern "C" __declspec(noinline) void RA2ABI_AlphaCRC(AlphaShapeClass* p,CRCEngine& crc){p->ComputeCRC(crc);}
extern "C" __declspec(noinline) int RA2ABI_AlphaSize(AlphaShapeClass* p){return p->Size();}
extern "C" __declspec(noinline) void RA2ABI_AlphaDraw(const RectangleStruct& clip){AlphaShapeClass::DrawAll(clip);}
extern "C" __declspec(noinline) void RA2ABI_AlphaUpdateAll(){AlphaShapeClass::UpdateAll();}
static_assert(!std::is_abstract_v<HouseClass>);
static_assert(std::is_same_v<decltype(&HouseClass::FindConnectionPoint),
    HRESULT (YRPP_STDCALL HouseClass::*)(REFIID, IConnectionPoint**)>);
static_assert(offsetof(HouseClass,WaypointPath)==0x16064);
static_assert(sizeof(StorageClass)==0x10);
static_assert(std::is_same_v<decltype(&CellClass::ReduceTiberium), int (YRPP_THISCALL CellClass::*)(int)>);
static_assert(sizeof(ControlNode)==0x14);
static_assert(sizeof(CaptureManagerClass)==0x50);
static_assert(offsetof(CaptureManagerClass,ControlNodes)==0x24);
static_assert(offsetof(CaptureManagerClass,MaxControlNodes)==0x3C);
static_assert(offsetof(CaptureManagerClass,InfiniteMindControl)==0x40);
static_assert(offsetof(CaptureManagerClass,Owner)==0x48);
static_assert(offsetof(RulesClass,WarpIn)==0x338);
static_assert(offsetof(RulesClass,WarpOut)==0x33C);
static_assert(sizeof(TeleportLocomotionClass)==0x4C);
static_assert(offsetof(TeleportLocomotionClass,MovingDestination)==0x1C);
static_assert(offsetof(TeleportLocomotionClass,LastCoords)==0x28);
static_assert(offsetof(TeleportLocomotionClass,Moving)==0x34);
static_assert(offsetof(TeleportLocomotionClass,State)==0x38);
static_assert(offsetof(TeleportLocomotionClass,Timer)==0x3C);
static_assert(offsetof(TeleportLocomotionClass,Piggybackee)==0x48);
extern "C" __declspec(noinline) int RA2ABI_UnitHarvest(UnitClass* p){return p->Mission_Harvest();}
extern "C" __declspec(noinline) int RA2ABI_TypePipMax(TechnoTypeClass* p){return p->GetPipMax();}
extern "C" __declspec(noinline) void RA2ABI_RefineryOpening(BuildingClass* p,bool captured){p->Place(captured);}
extern "C" __declspec(noinline) CellStruct* RA2ABI_FootScanOre(FootClass* p,CellStruct* out,int range){return p->ScanForTiberium(out,range,0);}
extern "C" __declspec(noinline) CoordStruct RA2ABI_TeleportDestination(TeleportLocomotionClass* p){return p->Destination();}
static_assert(sizeof(PowerClass)==0x1544);
static_assert(offsetof(PowerClass,unknown_timer_1510)==0x1510 && offsetof(PowerClass,unknown_timer_1520)==0x1520);
static_assert(offsetof(PowerClass,unknown_152C)==0x152C && offsetof(PowerClass,unknown_1534)==0x1534);
static_assert(offsetof(PowerClass,PowerOutput)==0x153C && offsetof(PowerClass,PowerDrain)==0x1540);
static_assert(offsetof(BuildingClass,unknown_544)==0x544);
static_assert(offsetof(FactoryClass,Production)+offsetof(StageClass,Rate)==0x38);
static_assert(offsetof(SuperClass,RechargeTimer)==0x30 && offsetof(SuperClass,IsSuspended)==0x70);
static_assert(offsetof(DisplayClass,CurrentSWTypeIndex)==0x11B8);
static_assert(sizeof(RGBClass)==3 && sizeof(ParticleClass)==0x138 && sizeof(ParticleSystemClass)==0x100);
static_assert(sizeof(ParticleTypeClass)==0x318 && sizeof(ParticleSystemTypeClass)==0x310);
static_assert(offsetof(ParticleTypeClass,ColorList)==0x2B8 && offsetof(ParticleTypeClass,BehavesLike)==0x314);
static_assert(offsetof(ParticleClass,Color)==0xB0 && offsetof(ParticleClass,ColorIndex)==0xB4 && offsetof(ParticleClass,ColorAccum)==0xB8);
static_assert(offsetof(ParticleClass,GasVelocity)==0xCC && offsetof(ParticleClass,MovementDirection)==0x10C && offsetof(ParticleClass,PrecisePosition)==0x118);
static_assert(offsetof(ParticleClass,ParticleSystem)==0x124 && offsetof(ParticleClass,RemainingEC)==0x128 && offsetof(ParticleClass,IsToDie)==0x131);
static_assert(offsetof(ParticleSystemClass,Particles)==0xBC && offsetof(ParticleSystemClass,SpawnFrames)==0xE8 && offsetof(ParticleSystemClass,TimeToDie)==0xF8);
extern "C" __declspec(noinline) void RA2ABI_ParticleDraw(ParticleClass* p,Point2D* at,RectangleStruct* clip){p->DrawIt(at,clip);}
extern "C" __declspec(noinline) void RA2ABI_ParticleSystemDraw(ParticleSystemClass* p,Point2D* at,RectangleStruct* clip){p->DrawIt(at,clip);}
extern "C" __declspec(noinline) void RA2ABI_ParticleSystemUpdate(ParticleSystemClass* p){p->Update();}
extern "C" __declspec(noinline) void RA2ABI_ParticleSystemExpired(ParticleSystemClass* p,AbstractClass* target,bool removed){p->PointerExpired(target,removed);}
extern "C" __declspec(noinline) void RA2ABI_ParticleSystemUnInit(ParticleSystemClass* p){p->UnInit();}
static_assert(sizeof(CommandClass)==4);
static_assert(sizeof(IndexClass<int,CommandClass*>)==0x14);
static_assert(sizeof(IndexClass<int,CommandClass*>::NodeElement)==8);
static_assert(offsetof(TechnoClass,RearmTimer)==0x2EC);
static_assert(offsetof(TechnoClass,MindControlledBy)==0x2C0);
static_assert(offsetof(SessionClass,Config)+offsetof(GameModeOptionsClass,MCVRedeploy)==0xE8);
extern "C" __declspec(noinline) void RA2ABI_CommandExecute(CommandClass* p,WWKey key){p->Execute(key);}
extern "C" __declspec(noinline) bool RA2ABI_CommandCombination(CommandClass* p,WWKey key){return p->PreventCombinationOverride(key);}
extern "C" __declspec(noinline) bool RA2ABI_CommandCondition(CommandClass* p,WWKey key){return p->ExtraTriggerCondition(key);}
extern "C" __declspec(noinline) bool RA2ABI_TechnoClickedEvent(TechnoClass* p,EventType type){return p->ClickedEvent(type);}
extern "C" __declspec(noinline) Action RA2ABI_BuildingMouseObject(BuildingClass* p,ObjectClass* t,bool ignore){return p->MouseOverObject(t,ignore);}
extern "C" __declspec(noinline) Action RA2ABI_BuildingMouseCell(BuildingClass* p,CellStruct* t,bool fog,bool ignore){return p->MouseOverCell(t,fog,ignore);}
extern "C" __declspec(noinline) bool RA2ABI_BuildingObjectClick(BuildingClass* p,Action a,ObjectClass* t,bool ignore){return p->ObjectClickedAction(a,t,ignore);}
extern "C" __declspec(noinline) bool RA2ABI_BuildingCellClick(BuildingClass* p,Action a,CellStruct* t,CellStruct* f,bool ignore){return p->CellClickedAction(a,t,f,ignore);}
extern "C" __declspec(noinline) bool RA2ABI_BuildingControllable(BuildingClass* p){return p->IsControllable();}
extern "C" __declspec(noinline) bool RA2ABI_InfantryCanDeploy(InfantryClass* p){return p->CanDeploySlashUnload();}
static_assert(sizeof(TacticalSelectableStruct)==0xC && offsetof(TacticalClass,SelectableCount)==0xDB0);
static_assert(offsetof(ObjectClass,IsOnMap)==0x74 && offsetof(ObjectClass,InLimbo)==0x81 && offsetof(ObjectClass,IsAlive)==0x90);
static_assert(offsetof(TechnoClass,IsOwnedByCurrentPlayer)==0x41A && offsetof(TechnoClass,CloakState)==0x220);
static_assert(offsetof(TechnoTypeClass,Invisible)==0xC9A && offsetof(TerrainTypeClass,IsVeinhole)==0x2B4);
extern "C" __declspec(noinline) ObjectClass* RA2ABI_TacticalPick(TacticalClass* p,const Point2D& point){return p->GetSelectableObject(point);}
extern "C" __declspec(noinline) void RA2ABI_TacticalBuildings(TacticalClass* p,RectangleStruct bounds){p->AddBuildingsToSelectables(bounds);}
static_assert(offsetof(BuildingClass,unknown_rect_63C)==0x63C && offsetof(BuildingClass,unknown_coord_64C)==0x64C);
static_assert(offsetof(BuildingClass,unknown_int_658)==0x658 && offsetof(BuildingClass,unknown_65C)==0x65C);
static_assert(offsetof(BuildingTypeClass,unknown_1538)==0x1538 && offsetof(BuildingTypeClass,unknown_1544)==0x1544);
extern "C" __declspec(noinline) RectangleStruct* RA2ABI_BuildingRenderDimensions(BuildingClass* p,RectangleStruct* out){return p->GetRenderDimensions(out);}
static_assert(sizeof(WaypointClass)==0xC && sizeof(WaypointPathClass)==0x40);
static_assert(offsetof(WaypointPathClass,CurrentWaypointIndex)==0x24 && offsetof(WaypointPathClass,Waypoints)==0x28);
static_assert(std::is_same_v<decltype(&WaypointPathClass::GetWaypointAfter),WaypointClass* (WaypointPathClass::*)(const WaypointClass*) const>);
static_assert(offsetof(DisplayClass,DraggedWaypoint)==0x11BC && offsetof(DisplayClass,DraggedWaypointCoords)==0x11C0);
static_assert(offsetof(FootClass,LastMapCoords)==0x55C && offsetof(FootClass,unknown_abstract_array_588)==0x588);
static_assert(offsetof(FootClass,unknown_5A0)==0x5A0 && offsetof(FootClass,LastDestination)==0x5A8);
static_assert(offsetof(TechnoClass,CurrentWeaponNumber)==0x138 && offsetof(TechnoClass,CurrentGattlingStage)==0x140 && offsetof(TechnoClass,GattlingValue)==0x144);
static_assert(offsetof(TechnoClass,LastSightRange)==0x260);
static_assert(offsetof(HouseClass,OwnedInfantry)==0x2F4 && offsetof(HouseClass,HasBeenThieved)==0x244);
static_assert(offsetof(TechnoTypeClass,SensorsSight)==0x5F0 && offsetof(TechnoTypeClass,ThreatPosed)==0x670);
static_assert(offsetof(TechnoTypeClass,ResourceGatherer)==0x5EC && offsetof(TechnoTypeClass,ResourceDestination)==0x5ED);
// Event.Execute Idle (0x4C6CB0 case 0x6) directly touches these original fields.
static_assert(offsetof(TechnoTypeClass,BalloonHover)==0xD6A && offsetof(TechnoTypeClass,OpenTopped)==0x5E4);
static_assert(offsetof(UnitTypeClass,Harvester)==0xE0E);
static_assert(offsetof(FootClass,PlanningPathIdx)==0x520 && offsetof(FootClass,WaypointNearbyAccessibleCellDelta)==0x524);
static_assert(offsetof(FootClass,WaypointCell)==0x528 && offsetof(FootClass,NavQueue)==0x5AC && offsetof(FootClass,WaypointIndex)==0x686);
static_assert(offsetof(TechnoTypeClass,Drainable)==0x5EF && offsetof(TechnoTypeClass,NavalTargeting)==0x600 && offsetof(TechnoTypeClass,LandTargeting)==0x604);
static_assert(offsetof(TechnoTypeClass,SpeedType)==0x67C && offsetof(TechnoTypeClass,Unnatural)==0x694);
static_assert(offsetof(TechnoTypeClass,DeployFireWeapon)==0x6A8 && offsetof(TechnoTypeClass,Naval)==0xCCE);
static_assert(offsetof(TechnoTypeClass,OpenTransportWeapon)==0xD50 && offsetof(TechnoTypeClass,Underwater)==0xD69 && offsetof(TechnoTypeClass,Organic)==0xD97);
static_assert(offsetof(WeaponTypeClass,NeverUse)==0x136 && offsetof(WeaponTypeClass,DrainWeapon)==0x142 && offsetof(WeaponTypeClass,AreaFire)==0x150);
static_assert(offsetof(WarheadTypeClass,ElectricAssault)==0x158 && offsetof(WarheadTypeClass,Airstrike)==0x16C);
static_assert(offsetof(BuildingTypeClass,Overpowerable)==0x1575 && offsetof(BuildingTypeClass,CanC4)==0x1577 && offsetof(BuildingTypeClass,CanBeOccupied)==0x157B);
static_assert(offsetof(BuildingTypeClass,InfantryAbsorb)==0x16AF && offsetof(BuildingTypeClass,ExtraPowerBonus)==0xEE8);
static_assert(offsetof(BuildingClass,IsOverpowered)==0x661 && offsetof(BuildingClass,C4Applied)==0x6DF);
static_assert(offsetof(BuildingClass,C4AppliedBy)==0x540 && offsetof(BuildingClass,C4Timer)==0x528);
static_assert(offsetof(CellClass,Visibility)==0x120 && offsetof(CellClass,VisibilityChanged)==0x138);
static_assert(sizeof(InfantryClass)==0x6F0 && sizeof(FootClass)==0x6C0);
static_assert(offsetof(InfantryClass,Type)==0x6C0);
static_assert(offsetof(InfantryClass,SequenceAnim)==0x6C4);
static_assert(offsetof(TechnoClass,Animation)==0xF8);
static_assert(offsetof(TechnoClass,TargetingTimer)==0x180);
static_assert(offsetof(InfantryTypeClass,DeadBodies)==0xE50);
static_assert(offsetof(InfantryTypeClass,NotHuman)==0xEAD && offsetof(InfantryTypeClass,DeployedCrushable)==0xEC9);
static_assert(offsetof(TechnoClass,IsInPlayfield)==0x3D5 && offsetof(TechnoClass,DiscoveredByCurrentPlayer)==0x41B);
static_assert(offsetof(HouseClass,RecheckPower)==0x5778 && offsetof(HouseClass,RecheckRadar)==0x5779);
static_assert(offsetof(HouseClass,DiscoveredByPlayer)==0x1F4);
static_assert(offsetof(CellClass,CloakedByHouses)==0x78 && offsetof(CellClass,AltFlags)==0x12C);
static_assert(offsetof(TechnoClass,PrimaryFacing)==0x388);
static_assert(offsetof(InfantryTypeClass,Sequence)==0xE3C);
static_assert(offsetof(InfantryTypeClass,Occupier)==0xEB4 && offsetof(InfantryTypeClass,Crawls)==0xEBD);
static_assert(offsetof(InfantryTypeClass,Infiltrate)==0xEBE && offsetof(InfantryTypeClass,Agent)==0xEC4);
static_assert(offsetof(TechnoTypeClass,Spawned)==0xD54 && offsetof(TechnoTypeClass,MissileSpawn)==0xD68);
static_assert(offsetof(TechnoTypeClass,DeployFire)==0x6AC && offsetof(TechnoTypeClass,Storage)==0x800);
static_assert(offsetof(WarheadTypeClass,IsLocomotor)==0x15B);
static_assert(offsetof(BuildingTypeClass,Capturable)==0x1572 && offsetof(BuildingTypeClass,Spyable)==0x1576);
static_assert(offsetof(BuildingTypeClass,IsAnimDelayedFire)==0x16A7);
static_assert(offsetof(BuildingTypeClass,UnitRepair)==0x16A9 && offsetof(BuildingTypeClass,Grinding)==0x16AD);
static_assert(offsetof(TechnoClass,ShouldLoseTargetNow)==0x50C);
static_assert(offsetof(FootClass,Destination)==0x5A4 && offsetof(FootClass,NavQueue)==0x5AC);
static_assert(sizeof(SubSequenceStruct)==0x24 && sizeof(SequenceStruct)==0x5E8);
static_assert(sizeof(CellClass)==0x148);
static_assert(offsetof(CellClass,unknown_30)==0x30);
static_assert(offsetof(CellClass,FirstObject)==0xE4 && offsetof(CellClass,AltObject)==0xE8);
static_assert(offsetof(CellClass,OccupationFlags)==0x124 && offsetof(CellClass,AltOccupationFlags)==0x128);
#include "yrpp/RulesClass.h"
#define PROBE extern "C" __declspec(noinline)
PROBE int RA2ABI_TechnoSelectWeapon(TechnoClass* p,AbstractClass* t){return p->SelectWeapon(t);}
PROBE int RA2ABI_InfantrySelectWeapon(InfantryClass* p,AbstractClass* t){return p->SelectWeapon(t);}
PROBE int RA2ABI_TechnoNavalWeapon(TechnoClass* p,AbstractClass* t){return p->SelectNavalTargeting(t);}
PROBE int RA2ABI_TechnoWeaponRange(TechnoClass* p,int i){return p->GetWeaponRange(i);}
PROBE int RA2ABI_TechnoThreatValue(TechnoClass* p){return p->GetThreatValue();}
PROBE bool RA2ABI_TechnoClose3D(TechnoClass* p,const CoordStruct& c,int i){return p->IsCloseEnough3D(c,i);}
PROBE void RA2ABI_FootAddSensors(FootClass* p,CellStruct c){p->AddSensorsAt(c);}
PROBE void RA2ABI_FootRemoveSensors(FootClass* p,CellStruct c){p->RemoveSensorsAt(c);}
PROBE bool RA2ABI_FootIsLeavingMap(FootClass* p){return p->IsLeavingMap();}
PROBE CoordStruct* RA2ABI_FootPredicted(FootClass* p,CoordStruct* c){return p->vt_entry_4F0(c);}
PROBE CoordStruct* RA2ABI_ObjectTargetCoords(ObjectClass* p,CoordStruct* c){return p->GetTargetCoords(c);}
PROBE void RA2ABI_InfantryOccupy(InfantryClass* p,const CoordStruct* c) { p->MarkAllOccupationBits(*c); }
PROBE void RA2ABI_InfantryVacate(InfantryClass* p,const CoordStruct* c) { p->UnmarkAllOccupationBits(*c); }
PROBE void RA2ABI_CellExpired(CellClass* p,AbstractClass* o) { p->PointerExpired(o,true); }
PROBE void RA2ABI_RadioExpired(RadioClass* p,AbstractClass* o) { p->PointerExpired(o,true); }
PROBE void RA2ABI_InfantryExpired(InfantryClass* p,AbstractClass* o) { p->PointerExpired(o,true); }
PROBE void RA2ABI_BuildingExpired(BuildingClass* p,AbstractClass* o) { p->PointerExpired(o,true); }
PROBE void RA2ABI_TerrainExpired(TerrainClass* p,AbstractClass* o) { p->PointerExpired(o,true); }
PROBE void RA2ABI_TechnoAnimExpired(TechnoClass* p,AnimClass* o) { p->AnimPointerExpired(o); }
PROBE void RA2ABI_ObjectAnimExpired(ObjectClass* p,AnimClass* o) { p->AnimPointerExpired(o); }
PROBE void RA2ABI_FootExpired(FootClass* p,AbstractClass* o) { p->PointerExpired(o,true); }
PROBE void RA2ABI_TechnoExpired(TechnoClass* p,AbstractClass* o) { p->PointerExpired(o,true); }
PROBE bool RA2ABI_InfantryPlayAnim(InfantryClass* p,Sequence action) { return p->PlayAnim(action,false,false); }
PROBE void RA2ABI_InfantrySetTarget(InfantryClass* p,AbstractClass* o) { p->SetTarget(o); }
PROBE bool RA2ABI_ObjectMark(ObjectClass* p,MarkType mark) { return p->Mark(mark); }
PROBE bool RA2ABI_BuildingUnlimbo(BuildingClass* p,const CoordStruct& c,DirType d){return p->Unlimbo(c,d);}
PROBE bool RA2ABI_BuildingLimbo(BuildingClass* p){return p->Limbo();}
PROBE bool RA2ABI_BuildingMark(BuildingClass* p,MarkType m){return p->Mark(m);}
PROBE bool RA2ABI_TerrainUnlimbo(TerrainClass* p,const CoordStruct& c,DirType d){return p->Unlimbo(c,d);}
PROBE bool RA2ABI_TerrainLimbo(TerrainClass* p){return p->Limbo();}
PROBE bool RA2ABI_TerrainMark(TerrainClass* p,MarkType m){return p->Mark(m);}

PROBE bool RA2ABI_FootMark(FootClass* p,MarkType mark) { return p->Mark(mark); }
PROBE void RA2ABI_FootSetLocation(FootClass* p,const CoordStruct& c) { p->SetLocation(c); }
PROBE void RA2ABI_InfantrySetDestination(InfantryClass* p,AbstractClass* o) { p->SetDestination(o,true); }
PROBE bool RA2ABI_InfantryStopMoving(InfantryClass* p) { return p->StopMoving(); }
PROBE void RA2ABI_InfantryMovementBlocked(InfantryClass* p) { p->vt_entry_4F4(); }
PROBE bool RA2ABI_InfantryJumpJetToWalk(InfantryClass* p) { return p->vt_entry_4F8(); }
PROBE int RA2ABI_InfantryCurrentSpeed(InfantryClass* p) { return p->GetCurrentSpeed(); }
PROBE void RA2ABI_InfantryPerCell(InfantryClass* p) { p->UpdatePosition(PCPType::End); }
PROBE void RA2ABI_TechnoPerCell(TechnoClass* p) { p->UpdatePosition(PCPType::End); }
PROBE void RA2ABI_FootPerCell(FootClass* p) { p->UpdatePosition(PCPType::End); }
PROBE void RA2ABI_TechnoSensed(TechnoClass* p) { p->Sensed(); }
PROBE bool RA2ABI_TechnoDiscovered(TechnoClass* p,HouseClass* h) { return p->DiscoveredBy(h); }
PROBE void RA2ABI_InfantryDoing(InfantryClass* p) { p->Doing_AI(); }
PROBE void RA2ABI_InfantryMovement(InfantryClass* p) { p->Movement_AI(); }
PROBE void RA2ABI_TargetingDelay(TechnoClass* p) { p->ShortenTargetingDelay(); }
PROBE bool RA2ABI_InfantryIdle(InfantryClass* p) { return p->EnterIdleMode(false,true); }
PROBE void RA2ABI_ObjectFrame(ObjectClass* p) { p->Update(); }
PROBE void RA2ABI_MissionFrame(MissionClass* p) { p->Update(); }
PROBE void RA2ABI_FootFrame(FootClass* p) { p->Update(); }
PROBE void RA2ABI_InfantryFrame(InfantryClass* p) { p->Update(); }
PROBE void RA2ABI_TemporalFrame(TemporalClass* p) { p->Update(); }
PROBE int RA2ABI_ObjectZ(ObjectClass* p) { return p->GetZ(); }
PROBE BulletClass* RA2ABI_InfantryFire(InfantryClass* p,AbstractClass* t,int i) { return p->Fire(t,i); }
PROBE void RA2ABI_FootStun(FootClass* p) { p->Stun(); }
PROBE void RA2ABI_RadarTracking(TechnoClass* p,bool force) { p->RadarTrackingUpdate(force); }
PROBE void RA2ABI_LayerSubmit(ObjectClass* p) { DisplayClass::Submit(p); }
PROBE void RA2ABI_LayerRemove(ObjectClass* p) { DisplayClass::Remove(p); }
PROBE DirStruct* RA2ABI_ObjectDirection(ObjectClass* p,DirStruct* d,AbstractClass* t) { return p->GetDirectionTo(d,t); }
PROBE void RA2ABI_InfantryFear(InfantryClass* p) { p->Fear_AI(); }
PROBE void RA2ABI_InfantryFiring(InfantryClass* p) { p->Firing_AI(); }
static_assert(offsetof(AnimClass,RemainingIterations)==0x195);
static_assert(offsetof(AnimClass,Bounce)==0x128);
static_assert(sizeof(BounceClass)==0x50);
static_assert(offsetof(AnimClass,HasExtras)==0x194);
static_assert(offsetof(AnimTypeClass,MinZVel)==0x318);
static_assert(offsetof(AnimTypeClass,unknown_double_320)==0x320);
PROBE int RA2ABI_AnimExtras(AnimClass* p) { return p->AnimExtras(); }
PROBE void RA2ABI_FlamingAI(AnimClass* p) { p->FlamingGuyAI(); }
PROBE CoordStruct* RA2ABI_FlamingDestination(AnimClass* p,CoordStruct* out) { return p->NextFlamingGuyCoords(out); }
PROBE bool RA2ABI_FlamingCell(AnimClass* p,const CellStruct& cell) { return p->IsValidFlamingGuyCell(cell); }
static_assert(offsetof(AnimClass,Animation)==0xAC);
static_assert(offsetof(AnimClass,FlamingGuyCoords)==0x108);
static_assert(offsetof(AnimClass,FlamingGuyRetries)==0x114);
static_assert(offsetof(AnimClass,FlamingGuyExpire)==0x19A);
static_assert(offsetof(AnimTypeClass,RunningFrames)==0x350);
static_assert(offsetof(AnimTypeClass,IsFlamingGuy)==0x354);
static_assert(offsetof(TeamClass,IsLeavingMap)==0x82);
PROBE bool RA2ABI_FootIdle(FootClass* p) { return p->EnterIdleMode(false,true); }
PROBE bool RA2ABI_TechnoIdle(TechnoClass* p) { return p->EnterIdleMode(false,true); }
PROBE bool RA2ABI_InfantryReady(InfantryClass* p) { return p->ReadyToNextMission(); }
PROBE bool RA2ABI_FootHaveMega(FootClass* p) { return p->HaveMegaMission(); }
PROBE void RA2ABI_FootClearMega(FootClass* p) { p->ClearMegaMissionData(); }
PROBE bool RA2ABI_FootContinueMega(FootClass* p) { return p->ContinueMegaMission(); }
PROBE bool RA2ABI_FootRefreshMega(FootClass* p) { return p->RefreshMegaMission(); }
PROBE bool RA2ABI_FootMegaMove(FootClass* p) { return p->MegaMissionIsAttackMove(); }
PROBE bool RA2ABI_TechnoNotWarping(TechnoClass* p) { return p->IsNotWarpingIn(); }
PROBE int RA2ABI_FootMissionMove(FootClass* p) { return p->Mission_Move(); }
PROBE int RA2ABI_InfantryMissionMove(InfantryClass* p) { return p->Mission_Move(); }
PROBE void RA2ABI_FootUnInit(FootClass* p) { p->UnInit(); }
PROBE CellStruct* RA2ABI_ObjectMapCoords(ObjectClass* p,CellStruct* out) { return p->GetMapCoords(out); }
PROBE CellStruct* RA2ABI_ObjectDestinationCell(ObjectClass* p,CellStruct* out) { return p->GetMapCoordsAgain(out); }
PROBE CellClass* RA2ABI_ObjectCell(ObjectClass* p) { return p->GetCell(); }
PROBE CellClass* RA2ABI_ObjectCellAgain(ObjectClass* p) { return p->GetCellAgain(); }
PROBE void RA2ABI_DisplayLeftUp(DisplayClass* p,const CoordStruct& coords,const CellStruct& cell,ObjectClass* object,Action action,DWORD unknown) { p->LeftMouseButtonUp(coords,cell,object,action,unknown); }
PROBE void RA2ABI_DisplayRightUp(DisplayClass* p,DWORD unknown) { p->RightMouseButtonUp(unknown); }
static_assert(offsetof(FootClass,ShouldScanForTarget)==0x68A && offsetof(FootClass,unknown_bool_68B)==0x68B);
static_assert(offsetof(FootClass,IsWaitingBlockagePath)==0x6B7 && offsetof(FootClass,CurrentTunnelCoords)==0x568);
static_assert(offsetof(FootClass,PathWaitTimes)==0x64C);
static_assert(offsetof(BuildingTypeClass,OccupyHeight)==0xEF8);
static_assert(offsetof(BuildingTypeClass,RemoveOccupy)==0x1624 && offsetof(BuildingTypeClass,AddOccupy)==0x1664);
static_assert(offsetof(BuildingTypeClass,CanHideThings)==0x1766);
static_assert(offsetof(OverlayTypeClass,Tiberium)==0x2A9);
PROBE WeaponStruct* RA2ABI_TechnoWeapon(TechnoClass* p,int index) { return p->GetWeapon(index); }
PROBE TechnoTypeClass* RA2ABI_TechnoType(TechnoClass* p) { return p->GetTechnoType(); }
PROBE int RA2ABI_ObjectHeight(ObjectClass* p) { return p->GetHeight(); }
PROBE bool RA2ABI_FootInAir(FootClass* p) { return p->IsInAir(); }
PROBE Move RA2ABI_FootReach(FootClass* p,const CellClass* to,int& level,bool& bridge,const CellClass* from) { return p->CanReachCell(to,FacingType::East,level,bridge,from); }
PROBE Move RA2ABI_InfantryPassability(InfantryClass* p,CellClass* to,CellClass* from) { return p->IsCellOccupied(to,FacingType::East,0,from,false); }
PROBE bool RA2ABI_FootLeaveMap(FootClass* p) { return p->vt_entry_320(); }
PROBE bool RA2ABI_TechnoArmed(TechnoClass* p) { return p->IsArmed(); }
PROBE WeaponStruct* RA2ABI_TechnoTurretWeapon(TechnoClass* p) { return p->GetTurretWeapon(); }
PROBE bool RA2ABI_TechnoIronCurtain(TechnoClass* p) { return p->IsIronCurtained(); }
PROBE bool RA2ABI_TechnoDisguised(TechnoClass* p) { return p->IsDisguised(); }
PROBE bool RA2ABI_InfantryDisguisedAs(InfantryClass* p,HouseClass* h) { return p->IsDisguisedAs(h); }
static_assert(offsetof(TechnoClass,IronCurtainTimer)==0x18C && offsetof(TechnoClass,Disguised)==0x1D8);
static_assert(offsetof(TechnoClass,DisguisedAsHouse)==0x51C && offsetof(TechnoClass,IsTether)==0x418);
static_assert(offsetof(TechnoTypeClass,TurretCount)==0x808 && offsetof(TechnoTypeClass,IsGattling)==0xCD5);
static_assert(offsetof(InfantryTypeClass,C4)==0xEC2 && offsetof(InfantryTypeClass,Engineer)==0xEC3 && offsetof(InfantryTypeClass,VehicleThief)==0xEC6);
static_assert(offsetof(AircraftTypeClass,AirportBound)==0xE0D);
static_assert(offsetof(HouseClass,AirportDocks)==0x2D4);
static_assert(offsetof(TechnoClass,HijackerInfantryType)==0x338);
static_assert(offsetof(BuildingTypeClass,InvisibleInGame)==0x1701 && offsetof(BuildingTypeClass,LaserFence)==0x16BF);
static_assert(offsetof(BuildingTypeClass,FirestormWall)==0x16C0 && offsetof(BuildingTypeClass,Gate)==0x16B7 && offsetof(BuildingTypeClass,BridgeRepairHut)==0x16B6);
static_assert(offsetof(BuildingClass,LaserFenceFrame)==0x618 && offsetof(TechnoClass,UnloadTimer)==0x350);
static_assert(offsetof(CellClass,WallOwnerIndex)==0x50 && offsetof(CellClass,TubeIndex)==0x116);
static_assert(offsetof(CellClass,Level)==0x11B && offsetof(CellClass,SlopeIndex)==0x11C);
static_assert(offsetof(HouseClass,FirestormActive)==0x1FA);
#include "yrpp/TubeClass.h"
#include "yrpp/AStarClass.h"
static_assert(sizeof(TubeClass)==0x1C4 && offsetof(TubeClass,EnterCell)==0x24 && offsetof(TubeClass,ExitCell)==0x28);
static_assert(offsetof(TubeClass,ExitFace)==0x2C && offsetof(TubeClass,Faces)==0x30 && offsetof(TubeClass,FaceCount)==0x1C0);
static_assert(sizeof(AStarClass)==0xC80 && sizeof(PathFinderData)==0x20);
static_assert(offsetof(AStarClass,PathNodeBuffer)==0xC && offsetof(AStarClass,PathQueueBuffer)==0x10);
static_assert(offsetof(AStarClass,FindMode)==0x3C && offsetof(AStarClass,PassabilityData)==0xBC);
static_assert(offsetof(PathFinderData,Directions)==0xC && offsetof(PathFinderData,Levels)==0x14);
static_assert(offsetof(FootClass,ThreatAvoidanceCoefficient)==0x530 && offsetof(FootClass,CurrentMapCoords)==0x558);
static_assert(offsetof(MapClass,SubzoneTrackingCounts)==0x74 && offsetof(MapClass,LevelAndPassabilityStruct2pointer_70)==0x70);
static_assert(sizeof(CellLevelPassabilityStruct)==0x4 && sizeof(LevelAndPassabilityStruct2)==0xA);
static_assert(offsetof(LevelAndPassabilityStruct2,ZoneID)==0x6 && offsetof(LevelAndPassabilityStruct2,CellLevel)==0x8);
static_assert(sizeof(ZoneConnectionClass)==0x10 && offsetof(ZoneConnectionClass,ConnectionType)==0xC);
static_assert(sizeof(SubzoneTrackingStruct)==0x24 && offsetof(SubzoneTrackingStruct,ParentSubzoneID)==0x18);
static_assert(offsetof(SubzoneTrackingStruct,Passability)==0x1C && offsetof(SubzoneTrackingStruct,ThreatRegion)==0x20);
static_assert(sizeof(SubzoneConnectionStruct)==0x8 && offsetof(SubzoneConnectionStruct,IsCrossBlock)==0x4);
static_assert(offsetof(TeamTypeClass,AvoidThreats)==0xF2 && offsetof(HouseClass,ThreatPosedEstimates)==0x57E4);
static_assert(offsetof(HouseClass,ThreatPosedEstimates)+131*sizeof(unsigned int)==0x59F0);
PROBE double RA2ABI_AStarCost(AStarClass* p,CellClass** from,CellClass** to,FootClass* foot){return p->GetMovementCost(from,to,false,Move::OK,foot);}
PROBE PathFinderData* RA2ABI_AStarRegular(AStarClass* p,const CellStruct& from,const CellStruct& to,FootClass* f,int* dirs){return p->FindPathRegular(from,to,f,dirs,-1,false);}
PROBE FootClass* RA2ABI_AStarBlocker(const CellStruct& cell,int level){return AStarClass::FindMovingBlocker(cell,level);}
PROBE CellStruct* RA2ABI_AStarFollow(CellStruct* out,const CellStruct* from,int* dirs){return AStarClass::FollowPath(out,from,3,dirs);}
PROBE bool RA2ABI_AStarHierarchy(AStarClass* p,const CellStruct& a,const CellStruct& b,FootClass* f){return p->FindPathHierarchical(a,b,MovementZone::Infantry,f);}
PROBE PathFinderData* RA2ABI_AStarMain(AStarClass* p,CellStruct* a,CellStruct* b,FootClass* f,int* dirs){return p->FindPath(a,b,f,dirs,-1,MovementZone::None,0);}
PROBE int RA2ABI_AStarAttempt(AStarClass* p,CellStruct* a,CellStruct* b,FootClass* f){return p->AttemptPath(a,b,f,false,false);}
PROBE PathFinderData* RA2ABI_FootPath(FootClass* p,CellStruct* end,int* dirs){return p->FindPath(end,dirs,0,0,0,0);}
PROBE int RA2ABI_MapRegionThreat(HouseClass* h){return MapClass::RegionThreat(h,2,1,2);}
PROBE CellStruct* RA2ABI_MapBridgeCell(CellStruct* out,CellClass* cell){return MapClass::GetBridgeZoneConnectionCell(out,cell,true);}
PROBE CellStruct* RA2ABI_MapSpanEnd(CellStruct* out,const CellStruct& a,const CellStruct& b){return MapClass::FindBridgeSpanEndCell(out,a,b);}
PROBE CellStruct* RA2ABI_MapSubzoneEnd(MapClass* p,CellStruct* out,const CellStruct& a){return p->FindBridgeEndCellForSubzone(out,a,0,1);}
PROBE bool RA2ABI_MapReachable(MapClass* p,CellClass* cell,DynamicVectorClass<unsigned short>& out,FootClass* f){return p->BuildReachableSubzones(cell,0,out,f);}
PROBE int RA2ABI_MapZones(MapClass* p){return p->ResetAllZones();}
PROBE void RA2ABI_CellPassability(CellClass* p){p->RecalcPassability();}
PROBE int RA2ABI_TechnoOwnerIndex(TechnoClass* p) { return p->GetOwningHouseIndex(); }
PROBE HouseClass* RA2ABI_TechnoOwner(TechnoClass* p) { return p->GetOwningHouse(); }
PROBE unsigned char RA2ABI_InfantrySpot(const CoordStruct* c) { return CellClass::InfantrySubpositionIndex(*c); }
PROBE bool RA2ABI_ObjectOccupiesCells(ObjectClass* p) { return p->IsStandingStill(); }
PROBE bool RA2ABI_QueueMission(MissionClass* p,Mission mission,bool start) { return p->QueueMission(mission,start); }
PROBE bool RA2ABI_NextMission(MissionClass* p) { return p->NextMission(); }
PROBE void RA2ABI_ForceMission(MissionClass* p,Mission mission) { p->ForceMission(mission); }
PROBE void RA2ABI_OverrideMission(MissionClass* p,Mission mission) { p->Override_Mission(mission,nullptr,nullptr); }
PROBE bool RA2ABI_MissionOverridden(MissionClass* p) { return p->MissionIsOverriden(); }
PROBE bool RA2ABI_MissionReady(MissionClass* p) { return p->ReadyToNextMission(); }
static_assert(sizeof(WeaponTypeClass) == 0x160);
static_assert(offsetof(WeaponTypeClass, AmbientDamage) == 0x98);
static_assert(offsetof(WeaponTypeClass, Projectile) == 0xA0);
static_assert(offsetof(WeaponTypeClass, Speed) == 0xA8);
static_assert(offsetof(WeaponTypeClass, Warhead) == 0xAC);
static_assert(offsetof(WeaponTypeClass, Range) == 0xB4);
static_assert(offsetof(WeaponTypeClass, Report) == 0xBC);
static_assert(offsetof(WeaponTypeClass, DownReport) == 0xD8);
static_assert(offsetof(WeaponTypeClass, Anim) == 0xF4);
static_assert(offsetof(WeaponTypeClass, LaserDuration) == 0x14E);
static_assert(offsetof(WeaponTypeClass, IsMagBeam) == 0x15C);
static_assert(offsetof(BulletTypeClass, Floater) == 0x295);
static_assert(offsetof(BulletTypeClass, AA) == 0x2A4);
static_assert(offsetof(BulletTypeClass, AG) == 0x2A5);
static_assert(offsetof(BulletTypeClass, ROT) == 0x2DC);
static_assert(sizeof(BulletTypeClass) == 0x2F8);
static_assert(offsetof(BulletTypeClass, Color) == 0x2D4);
static_assert(std::is_same_v<decltype(BulletTypeClass::Color),int>);
static_assert(sizeof(WarheadTypeClass) == 0x1D0);
static_assert(offsetof(WarheadTypeClass, Verses) == 0xA0);
static_assert(offsetof(WarheadTypeClass, Particle) == 0x140);
static_assert(offsetof(WarheadTypeClass, Locomotor) == 0x15C);
static_assert(sizeof(LayerClass) == 0x18 && sizeof(LogicClass) == 0x18);
static_assert(offsetof(ObjectClass, IsInLogic) == 0x98);
static_assert(offsetof(RulesClass, Gravity) == 0x16B8);
PROBE void RA2ABI_WeaponCRC(WeaponTypeClass* p, CRCEngine* crc) { p->ComputeCRC(*crc); }
PROBE bool RA2ABI_WeaponINI(WeaponTypeClass* p, CCINIClass* ini) { return p->LoadFromINI(ini); }
PROBE void RA2ABI_WeaponSpeed(WeaponTypeClass* p) { p->CalculateSpeed(); }
PROBE ThreatType RA2ABI_WeaponThreats(WeaponTypeClass* p) { return p->AllowedThreats(); }
PROBE HRESULT RA2ABI_WeaponLoad(WeaponTypeClass* p,IStream* stream) { return p->Load(stream); }
PROBE HRESULT RA2ABI_WeaponSave(WeaponTypeClass* p,IStream* stream,BOOL clear) { return p->Save(stream,clear); }
PROBE bool RA2ABI_BulletINI(BulletTypeClass* p,CCINIClass* ini) { return p->LoadFromINI(ini); }
PROBE void RA2ABI_BulletCRC(BulletTypeClass* p,CRCEngine* crc) { p->ComputeCRC(*crc); }
PROBE bool RA2ABI_WarheadINI(WarheadTypeClass* p,CCINIClass* ini) { return p->LoadFromINI(ini); }
PROBE void RA2ABI_WarheadCRC(WarheadTypeClass* p,CRCEngine* crc) { p->ComputeCRC(*crc); }
PROBE bool RA2ABI_LayerAdd(LayerClass* p,ObjectClass* o,bool sorted) { return p->AddObject(o,sorted); }
PROBE bool RA2ABI_LogicAdd(LogicClass* p,ObjectClass* o,bool sorted) { return p->AddObject(o,sorted); }
PROBE void RA2ABI_LogicRemove(LogicClass* p,ObjectClass* o) { p->RemoveObject(o); }
PROBE void RA2ABI_LayerSort(LayerClass* p) { p->Sort(); }
PROBE const char* RA2ABI_Name(FileClass* p) { return p->GetFileName(); }
PROBE bool RA2ABI_Exists(FileClass* p) { return p->Exists(true); }
PROBE bool RA2ABI_Open(FileClass* p) { return p->Open(FileAccessMode::Read); }
PROBE bool RA2ABI_OpenEx(FileClass* p,const char* s) { return p->OpenEx(s); }
PROBE int RA2ABI_Read(FileClass* p,void* b,int n) { return p->ReadBytes(b,n); }
PROBE int RA2ABI_Seek(FileClass* p,int n) { return p->Seek(n); }
PROBE int RA2ABI_Size(FileClass* p) { return p->GetFileSize(); }
PROBE int RA2ABI_Write(FileClass* p,void* b,int n) { return p->WriteBytes(b,n); }
PROBE void RA2ABI_Close(FileClass* p) { p->Close(); }
PROBE DWORD RA2ABI_Time(FileClass* p) { return p->GetFileTime(); }
PROBE bool RA2ABI_SetTime(FileClass* p,DWORD n) { return p->SetFileTime(n); }
PROBE void RA2ABI_Error(FileClass* p,DWORD n) { p->CDCheck(n); }
PROBE void RA2ABI_Delete(FileClass* p) { delete p; }
PROBE void RA2ABI_Copy(Blitter* p,void* d,byte* s) { p->Blit_Copy(d,s,1,2,nullptr,nullptr,3,4); }
PROBE void RA2ABI_CopyTint(Blitter* p,void* d,byte* s) { p->Blit_Copy_Tinted(d,s,1,2,nullptr,nullptr,3,4,5); }
PROBE void RA2ABI_Move(Blitter* p,void* d,byte* s) { p->Blit_Move(d,s,1,2,nullptr,nullptr,3); }
PROBE void RA2ABI_MoveTint(Blitter* p,void* d,byte* s) { p->Blit_Move_Tinted(d,s,1,2,nullptr,nullptr,3,4); }
PROBE void RA2ABI_RLECopy(RLEBlitter* p,void* d,byte* s) { p->Blit_Copy(d,s,1,2,3,nullptr,nullptr,4,5,nullptr); }
PROBE void RA2ABI_RLETint(RLEBlitter* p,void* d,byte* s) { p->Blit_Copy_Tinted(d,s,1,2,3,nullptr,nullptr,4,5,nullptr,6); }
PROBE bool RA2ABI_ArrayCapacity(DynamicVectorClass<void*>* p,int n) { return p->SetCapacity(n); }
PROBE int RA2ABI_ArrayFind(DynamicVectorClass<void*>* p,void* value) { return p->FindItemIndex(value); }

PROBE void RA2ABI_AbstractDelete(AbstractClass* p) { delete p; }
PROBE bool RA2ABI_CountryINI(HouseTypeClass* p, CCINIClass* ini) { return p->LoadFromINI(ini); }
PROBE bool RA2ABI_ModeReady(MPGameModeClass* p) { return p->UnfixAlliances(); }
PROBE bool RA2ABI_ModePrepare(MPGameModeClass* p) { return p->StartingPositionsToHouseBaseCells(1); }
PROBE bool RA2ABI_ModeStart(MPGameModeClass* p) { return p->StartingPositionsToHouseBaseCells2(true); }
PROBE void RA2ABI_TacticalDelete(TacticalClass* p) { delete p; }
PROBE void RA2ABI_VectorClear(DynamicVectorClass<void*>* p) { p->Clear(); }
#include "yrpp/DriveLocomotionClass.h"
PROBE DriveLocomotionClass* RA2ABI_CastDrive(ILocomotion* p) { return locomotion_cast<DriveLocomotionClass*>(p); }
#include "yrpp/HoverLocomotionClass.h"
PROBE HoverLocomotionClass* RA2ABI_CastHover(ILocomotion* p) { return locomotion_cast<HoverLocomotionClass*>(p); }
#include "yrpp/TunnelLocomotionClass.h"
PROBE TunnelLocomotionClass* RA2ABI_CastTunnel(ILocomotion* p) { return locomotion_cast<TunnelLocomotionClass*>(p); }
#include "yrpp/WalkLocomotionClass.h"
static_assert(!std::is_abstract_v<WalkLocomotionClass>);
static_assert(offsetof(LocomotionClass,Owner)==0x8 && offsetof(LocomotionClass,LinkedTo)==0xC);
static_assert(offsetof(LocomotionClass,Powered)==0x10 && offsetof(LocomotionClass,RefCount)==0x14);
static_assert(offsetof(WalkLocomotionClass,HeadToCoord)==0x28 && offsetof(WalkLocomotionClass,IsMoving)==0x34);
static_assert(offsetof(WalkLocomotionClass,InProcessing)==0x35 && offsetof(WalkLocomotionClass,IsReallyMoving)==0x36);
static_assert(offsetof(TechnoClass,EMPLockRemaining)==0x504);
static_assert(offsetof(TechnoTypeClass,ImmuneToVeins)==0xC91);
static_assert(offsetof(RulesClass,VeinAttack)==0xE8);
static_assert(offsetof(TechnoClass,BeingWarpedOut)==0x270 && offsetof(TechnoClass,WarpingOut)==0x271);
PROBE HRESULT RA2ABI_LocoQuery(ILocomotion* p,REFIID id,void** output){return p->QueryInterface(id,output);}
PROBE HRESULT RA2ABI_LocoLink(ILocomotion* p,FootClass* f){return p->Link_To_Object(f);}
PROBE bool RA2ABI_LocoMoving(ILocomotion* p){return p->Is_Moving();}
PROBE CoordStruct RA2ABI_LocoDestination(ILocomotion* p){return p->Destination();}
PROBE bool RA2ABI_LocoProcess(ILocomotion* p){return p->Process();}
PROBE void RA2ABI_LocoMove(ILocomotion* p,CoordStruct c){p->Move_To(c);}
PROBE void RA2ABI_LocoStop(ILocomotion* p){p->Stop_Moving();}
PROBE void RA2ABI_LocoMark(ILocomotion* p,MarkType m){p->Mark_All_Occupation_Bits(m);}
PROBE bool RA2ABI_LocoMovingHere(ILocomotion* p,CoordStruct c){return p->Is_Moving_Here(c);}
PROBE bool RA2ABI_LocoReallyMoving(ILocomotion* p){return p->Is_Really_Moving_Now();}
PROBE void RA2ABI_LocoLimbo(ILocomotion* p){p->Limbo();}
PROBE HRESULT RA2ABI_PiggyBegin(IPiggyback* p,ILocomotion* l){return p->Begin_Piggyback(l);}
PROBE HRESULT RA2ABI_PiggyEnd(IPiggyback* p,ILocomotion** l){return p->End_Piggyback(l);}
PROBE bool RA2ABI_PiggyReady(IPiggyback* p){return p->Is_Ok_To_End();}
PROBE HRESULT RA2ABI_PiggyClassID(IPiggyback* p,CLSID* id){return p->Piggyback_CLSID(id);}
PROBE bool RA2ABI_PiggyActive(IPiggyback* p){return p->Is_Piggybacking();}
PROBE bool RA2ABI_WalkReserve(WalkLocomotionClass* p,const CoordStruct& c){return p->Mark_Head_To(c);}
PROBE void RA2ABI_InfantryMovementStart(InfantryClass* p){p->vt_entry_548();}
PROBE void RA2ABI_InfantryMovementEnd(InfantryClass* p){p->vt_entry_54C();}
PROBE bool RA2ABI_TechnoEMP(TechnoClass* p){return p->IsUnderEMP();}
PROBE bool RA2ABI_TechnoWarped(TechnoClass* p){return p->IsBeingWarpedOut();}
PROBE bool RA2ABI_TechnoWarping(TechnoClass* p){return p->IsWarpingIn();}
PROBE WalkLocomotionClass* RA2ABI_CastWalk(ILocomotion* p) { return locomotion_cast<WalkLocomotionClass*>(p); }
#include "yrpp/DropPodLocomotionClass.h"
PROBE DropPodLocomotionClass* RA2ABI_CastDropPod(ILocomotion* p) { return locomotion_cast<DropPodLocomotionClass*>(p); }
#include "yrpp/FlyLocomotionClass.h"
PROBE FlyLocomotionClass* RA2ABI_CastFly(ILocomotion* p) { return locomotion_cast<FlyLocomotionClass*>(p); }
#include "yrpp/TeleportLocomotionClass.h"
PROBE TeleportLocomotionClass* RA2ABI_CastTeleport(ILocomotion* p) { return locomotion_cast<TeleportLocomotionClass*>(p); }
#include "yrpp/ShipLocomotionClass.h"
PROBE ShipLocomotionClass* RA2ABI_CastShip(ILocomotion* p) { return locomotion_cast<ShipLocomotionClass*>(p); }
#include "yrpp/JumpjetLocomotionClass.h"
PROBE JumpjetLocomotionClass* RA2ABI_CastJumpjet(ILocomotion* p) { return locomotion_cast<JumpjetLocomotionClass*>(p); }
#include "yrpp/RocketLocomotionClass.h"
PROBE RocketLocomotionClass* RA2ABI_CastRocket(ILocomotion* p) { return locomotion_cast<RocketLocomotionClass*>(p); }

#include "yrpp/BeaconClass.h"
#include "yrpp/BeaconClass.h"
#include "yrpp/BeaconManagerClass.h"
#include "yrpp/BeaconManagerClass.h"
#include "yrpp/AlphaLightingRemapClass.h"
#include <type_traits>
using FlyDestination = CoordStruct (YRPP_STDCALL FlyLocomotionClass::*)();
static_assert(std::is_same_v<decltype(&FlyLocomotionClass::Destination), FlyDestination>);
PROBE HRESULT RA2ABI_QueryInterface(AbstractClass* p, REFIID iid, void** out) { return p->QueryInterface(iid, out); }
PROBE ULONG RA2ABI_AddRef(AbstractClass* p) { return p->AddRef(); }
PROBE ULONG RA2ABI_Release(AbstractClass* p) { return p->Release(); }
PROBE HRESULT RA2ABI_Load(AbstractClass* p, IStream* stream) { return p->Load(stream); }
PROBE HRESULT RA2ABI_Save(AbstractClass* p, IStream* stream, BOOL clear) { return p->Save(stream, clear); }
PROBE HRESULT RA2ABI_AbstractClassID(AbstractClass* p, CLSID* id) { return p->GetClassID(id); }
PROBE HRESULT RA2ABI_AbstractDirty(AbstractClass* p) { return p->IsDirty(); }
PROBE HRESULT RA2ABI_AbstractSizeMax(AbstractClass* p, ULARGE_INTEGER* out) { return p->GetSizeMax(out); }
PROBE AbstractType RA2ABI_RttiWhat(IRTTITypeInfo* p) { return p->What_Am_I(); }
PROBE int RA2ABI_RttiID(IRTTITypeInfo* p) { return p->Fetch_ID(); }
PROBE void RA2ABI_RttiCreate(IRTTITypeInfo* p) { p->Create_ID(); }
PROBE bool RA2ABI_NoticeSink(INoticeSink* p) { return p->INoticeSink_Unknown(1); }
PROBE void RA2ABI_NoticeSource(INoticeSource* p) { p->INoticeSource_Unknown(); }
PROBE void RA2ABI_AbstractInit(AbstractClass* p) { p->Init(); }
PROBE void RA2ABI_AbstractExpired(AbstractClass* p, AbstractClass* other) { p->PointerExpired(other, true); }
PROBE AbstractType RA2ABI_AbstractWhat(AbstractClass* p) { return p->WhatAmI(); }
PROBE int RA2ABI_AbstractSize(AbstractClass* p) { return p->Size(); }
PROBE void RA2ABI_AbstractCRC(AbstractClass* p, CRCEngine& crc) { p->ComputeCRC(crc); }
PROBE int RA2ABI_AbstractOwnerIndex(AbstractClass* p) { return p->GetOwningHouseIndex(); }
PROBE HouseClass* RA2ABI_AbstractOwner(AbstractClass* p) { return p->GetOwningHouse(); }
PROBE int RA2ABI_AbstractArrayIndex(AbstractClass* p) { return p->GetArrayIndex(); }
PROBE bool RA2ABI_AbstractDead(AbstractClass* p) { return p->IsDead(); }
PROBE CoordStruct* RA2ABI_AbstractCoords(AbstractClass* p, CoordStruct* out) { return p->GetCoords(out); }
PROBE CoordStruct* RA2ABI_AbstractDestination(AbstractClass* p, CoordStruct* out) { return p->GetDestination(out); }
PROBE bool RA2ABI_AbstractFloor(AbstractClass* p) { return p->IsOnFloor(); }
PROBE bool RA2ABI_AbstractAir(AbstractClass* p) { return p->IsInAir(); }
PROBE CoordStruct* RA2ABI_AbstractCenter(AbstractClass* p, CoordStruct* out) { return p->GetCenterCoords(out); }
PROBE void RA2ABI_AbstractUpdate(AbstractClass* p) { p->Update(); }
PROBE void RA2ABI_TypeArt(AbstractTypeClass* p) { p->LoadTheaterSpecificArt(TheaterType::Temperate); }
PROBE bool RA2ABI_TypeLoadINI(AbstractTypeClass* p, CCINIClass* ini) { return p->LoadFromINI(ini); }
PROBE bool RA2ABI_TypeSaveINI(AbstractTypeClass* p, CCINIClass* ini) { return p->SaveToINI(ini); }
PROBE const char* RA2ABI_RttiName(AbstractType type) { return AbstractClass::GetRTTIName(type); }
PROBE void RA2ABI_RemoveInactive() { AbstractClass::RemoveAllInactive(); }
PROBE AbstractClass* RA2ABI_TargetAbstract(TargetClass* p) { return p->As_Abstract(); }
PROBE HRESULT RA2ABI_SwizzleQuery(ISwizzle* p, REFIID iid, void** out) { return p->QueryInterface(iid,out); }
PROBE ULONG RA2ABI_SwizzleAdd(ISwizzle* p) { return p->AddRef(); }
PROBE ULONG RA2ABI_SwizzleRelease(ISwizzle* p) { return p->Release(); }
PROBE HRESULT RA2ABI_SwizzleReset(ISwizzle* p) { return p->Reset(); }
PROBE HRESULT RA2ABI_SwizzleRequest(ISwizzle* p, void** out) { return p->Swizzle(out); }
PROBE HRESULT RA2ABI_SwizzleID(ISwizzle* p, void* object, LONG* id) { return p->Fetch_Swizzle_ID(object,id); }
PROBE HRESULT RA2ABI_SwizzleAnnounce(ISwizzle* p, LONG id, void* object) { return p->Here_I_Am(id,object); }
PROBE HRESULT RA2ABI_SwizzleSave(ISwizzle* p, IStream* s, IUnknown* o) { return p->Save_Interface(s,o); }
PROBE HRESULT RA2ABI_SwizzleLoad(ISwizzle* p, IStream* s, GUID* iid, void** out) { return p->Load_Interface(s,iid,out); }
PROBE HRESULT RA2ABI_SwizzleSize(ISwizzle* p, int* out) { return p->Get_Save_Size(out); }
#include "yrpp/AircraftClass.h"
#include "yrpp/AirstrikeClass.h"
#include "yrpp/TeamClass.h"
static_assert(offsetof(TechnoClass, Airstrike) == 0x294);
static_assert(offsetof(TechnoClass, Target) == 0x2B4);
static_assert(offsetof(TechnoClass, Health) == 0x6C);
static_assert(offsetof(TechnoClass, IsAlive) == 0x90);
static_assert(offsetof(AirstrikeClass, Target) == 0x50);
static_assert(offsetof(AircraftClass, IsLocked) == 0x6D2);
static_assert(offsetof(MissionClass, CurrentMission) == 0xAC);
static_assert(offsetof(MissionClass, MissionStatus) == 0xBC);
static_assert(offsetof(TeamClass, QueuedFocus) == 0x3C);
static_assert(offsetof(TeamClass, Focus) == 0x40);
static_assert(offsetof(TeamClass, IsMoving) == 0x7F && offsetof(TeamClass, IsLeavingMap) == 0x82);
PROBE bool RA2ABI_RevertTargetMission(TechnoClass* p) { return p->Mission_Revert(); }
PROBE void RA2ABI_ClearAttackTarget(TechnoClass* p) { p->SetTarget(nullptr); }
PROBE AlphaLightingRemapClass* RA2ABI_AlphaAcquire(int count) { return AlphaLightingRemapClass::FindOrAllocate(count); }
PROBE void RA2ABI_AlphaRelease(AlphaLightingRemapClass* p) { AlphaLightingRemapClass::Release(p); }
PROBE int RA2ABI_AdjustForZ(int height) { return TacticalClass::AdjustForZ(height); }
PROBE void* RA2ABI_Allocate(std::size_t size) { return YRMemory::Allocate(size); }

#include "yrpp/DriveLocomotionClass.h"
#include "yrpp/UnitClass.h"
static_assert(!std::is_abstract_v<DriveLocomotionClass>);
static_assert(sizeof(DriveLocomotionClass)==0x70 && offsetof(DriveLocomotionClass,Piggybackee)==0x68);
static_assert(offsetof(DriveLocomotionClass,HeadToCoord)==0x40 && offsetof(DriveLocomotionClass,SpeedAccum)==0x4C);
static_assert(offsetof(DriveLocomotionClass,movementspeed_50)==0x50 && offsetof(DriveLocomotionClass,TrackIndex)==0x5C);
static_assert(offsetof(DriveLocomotionClass,IsOnShortTrack)==0x60 && offsetof(DriveLocomotionClass,UnLocked)==0x65);
static_assert(sizeof(UnitClass)==0x8E8 && offsetof(UnitClass,Type)==0x6C4);
static_assert(offsetof(TechnoClass,UnlimboingInfantry)==0x1F8);
static_assert(offsetof(TechnoClass,BarrelFacing)==0x370 && offsetof(TechnoClass,SecondaryFacing)==0x3A0);
static_assert(offsetof(RulesClass,GuardAreaTargetingDelay)==0xE04);
static_assert(offsetof(FootClass,FrozenStill)==0x6B6);
static_assert(offsetof(FootClass,CurrentMapCoords)==0x558);
PROBE void RA2ABI_UnitUpdate(UnitClass* p){p->Update();}
PROBE DamageState RA2ABI_UnitDamage(UnitClass* p,int* damage,WarheadTypeClass* warhead,ObjectClass* source,HouseClass* house){return p->ReceiveDamage(damage,0,warhead,source,false,false,house);}
PROBE DamageState RA2ABI_AircraftDamage(AircraftClass* p,int* damage,WarheadTypeClass* warhead,ObjectClass* source,HouseClass* house){return p->ReceiveDamage(damage,0,warhead,source,false,false,house);}
PROBE bool RA2ABI_FootCrash(FootClass* p,ObjectClass* source){return p->Crash(source);}

PROBE InfantryTypeClass* RA2ABI_TechnoCrew(TechnoClass* p){return p->GetCrew();}
PROBE DirStruct* RA2ABI_UnitTurretFacing(UnitClass* p,DirStruct* result){return p->TurretFacing(result);}
PROBE int RA2ABI_FootDrawingDepth(FootClass* p){return p->GetZAdjustment();}
PROBE bool RA2ABI_UnitReady(UnitClass* p){return p->ReadyToNextMission();}
PROBE Move RA2ABI_UnitPassability(UnitClass* p,CellClass* c,FacingType f,int z){return p->IsCellOccupied(c,f,z,nullptr,true);}
PROBE void RA2ABI_UnitDestination(UnitClass* p,AbstractClass* t){p->SetDestination(t,true);}
PROBE void RA2ABI_UnitOccupy(UnitClass* p,const CoordStruct& c){p->MarkAllOccupationBits(c);}
PROBE void RA2ABI_UnitVacate(UnitClass* p,const CoordStruct& c){p->UnmarkAllOccupationBits(c);}
PROBE bool RA2ABI_UnitStanding(UnitClass* p){return p->IsStandingStill();}
PROBE void RA2ABI_TechnoRock(TechnoClass* p){p->vt_entry_41C();}
PROBE Point2D RA2ABI_DriveSmooth(DriveLocomotionClass* p,const Point2D& c,int& facing){return p->Smooth_Turn(c,facing);}

PROBE int RA2ABI_UnitUnload(UnitClass* p){return p->Mission_Unload();}
PROBE bool RA2ABI_UnitCanDeploy(UnitClass* p){return p->CanDeploySlashUnload();}
PROBE FacingType RA2ABI_UnitLoadDirection(UnitClass* p,FootClass* passenger,CellStruct* cell){return p->DesiredLoadDir(passenger,*cell);}

PROBE void RA2ABI_PowerUpdate(PowerClass* p,const int& key,const Point2D& point){p->Update(key,point);}
PROBE void RA2ABI_PowerDraw(PowerClass* p){p->Draw(1);}
PROBE int RA2ABI_TechnoTypeBuildSpeed(TechnoTypeClass* p){return p->GetBuildSpeed();}
PROBE long RA2ABI_HousePowerOutput(IHouse* p){return p->Power_Output();}
PROBE long RA2ABI_HousePowerDrain(IHouse* p){return p->Power_Drain();}
// Original IConnectionPointContainer vtable: 0x7EA7F4, House subobject +0x2C.
PROBE HRESULT RA2ABI_ConnectionEnum(IConnectionPointContainer* p,IEnumConnectionPoints** out){return p->EnumConnectionPoints(out);}
PROBE HRESULT RA2ABI_ConnectionFind(IConnectionPointContainer* p,REFIID iid,IConnectionPoint** out){return p->FindConnectionPoint(iid,out);}
PROBE HRESULT RA2ABI_HouseConnectionEnum(HouseClass* p,IEnumConnectionPoints** out){return p->EnumConnectionPoints(out);}
PROBE HRESULT RA2ABI_HouseConnectionFind(HouseClass* p,REFIID iid,IConnectionPoint** out){return p->FindConnectionPoint(iid,out);}

// Factory production / native original-class integration.
PROBE void RA2ABI_FactoryUpdate(FactoryClass* p){p->Update();}
PROBE DWORD RA2ABI_TypeOwners(TechnoTypeClass* p){return p->GetOwners();}
PROBE SHPStruct* RA2ABI_TypeCameo(TechnoTypeClass* p){return p->GetCameo();}
PROBE KickOutResult RA2ABI_BuildingExitProduct(BuildingClass* p,TechnoClass* unit,CellStruct at){return p->KickOutUnit(unit,at);}
PROBE CellStruct RA2ABI_BuildingExitCell(BuildingClass* p,FootClass* unit,CellStruct at){return p->FindExitCell(unit,at);}
PROBE void RA2ABI_UnitArrival(UnitClass* p){p->UpdatePosition(PCPType::End);}
PROBE RadioCommand RA2ABI_BuildingRadio(BuildingClass* p,TechnoClass* sender,AbstractClass*& data){return p->ReceiveCommand(sender,RadioCommand::NotifyUnloaded,data);}
PROBE int RA2ABI_HouseBegin(HouseClass* p,int index){return p->BeginProduction(AbstractType::UnitType,index,false,false);}
PROBE int RA2ABI_HouseSuspend(HouseClass* p,int index){return p->SuspendProduction(AbstractType::UnitType,index,false);}
PROBE bool RA2ABI_HousePlace(HouseClass* p,int index,const CellStruct& at){return p->PlaceObject(AbstractType::UnitType,index,false,at);}
PROBE TechnoTypeClass* RA2ABI_TypeByIndex(int index){return TechnoTypeClass::GetByTypeAndIndex(AbstractType::UnitType,index);}
PROBE bool RA2ABI_CameoSort(int left,int right){return BuildType::SortsBefore(AbstractType::UnitType,left,AbstractType::UnitType,right);}

PROBE bool RA2ABI_SidebarUnlink(int index,FactoryClass* p){return SidebarClass::UnlinkFactory(AbstractType::UnitType,index,p);}

// Original Mirage dispatch slots, Unit vtable 0x7F5C70.
PROBE bool RA2ABI_UnitDisguisedAs(UnitClass* p,HouseClass* h){return p->IsDisguisedAs(h);}
PROBE ObjectTypeClass* RA2ABI_UnitDisguise(UnitClass* p){return p->GetDisguise(true);}
PROBE HouseClass* RA2ABI_UnitDisguiseHouse(UnitClass* p){return p->GetDisguiseHouse(true);}
PROBE void RA2ABI_UnitClearDisguise(UnitClass* p){p->ClearDisguise();}
PROBE DWORD RA2ABI_TechnoDisguiseFlags(TechnoClass* p,DWORD flags){return p->GetDisguiseFlags(flags);}
PROBE bool RA2ABI_TechnoClearlyVisible(TechnoClass* p,HouseClass* h){return p->IsClearlyVisibleTo(h);}
PROBE BulletClass* RA2ABI_UnitFire(UnitClass* p,AbstractClass* target){return p->Fire(target,0);}
PROBE AbstractClass* RA2ABI_UnitGreatestThreat(UnitClass* p,CoordStruct* origin){return p->GreatestThreat(ThreatType(1),origin,false);}

// Campaign defensive-building virtual dispatch: gamemd Building vtable 0x7E3EBC.
PROBE bool RA2ABI_BuildingIdle(BuildingClass* p){return p->EnterIdleMode(false,false);}
PROBE bool RA2ABI_BuildingReady(BuildingClass* p){return p->ReadyToNextMission();}
PROBE FireError RA2ABI_BuildingFireError(BuildingClass* p,AbstractClass* target){return p->GetFireError(target,0,true);}
PROBE DirStruct RA2ABI_BuildingFireAngle(BuildingClass* p,AbstractClass* target){return p->FireAngleTo(target);}
static_assert(offsetof(BuildingClass,IsReadyToCommence)==0x6DD);
static_assert(offsetof(TechnoClass,TurretAnimFrame)==0x148);
static_assert(offsetof(BuildingTypeClass,TurretAnimIsVoxel)==0x16C5);
static_assert(offsetof(BuildingTypeClass,BuildingAnim)+9*sizeof(BuildingAnimStruct)+offsetof(BuildingAnimStruct,Position)==0x11E0);

#include "yrpp/AITriggerTypeClass.h"
#include "yrpp/TagTypeClass.h"
#include "yrpp/TriggerTypeClass.h"
static_assert(sizeof(TeamTypeClass)==0xF8 && offsetof(TeamTypeClass,IsGlobal)==0xE8);
static_assert(sizeof(TaskForceClass)==0xD4 && offsetof(TaskForceClass,IsGlobal)==0xA0);
static_assert(offsetof(TaskForceClass,Entries)==0xA4 && sizeof(TaskForceClass::IsGlobal)==4);
static_assert(sizeof(AITriggerTypeClass)==0x110 && offsetof(AITriggerTypeClass,IsGlobal)==0x9C);
static_assert(offsetof(TechnoTypeClass,Naval)==0xCCE && offsetof(TechnoTypeClass,Passengers)==0x5E0);
extern "C" __declspec(noinline) void RA2ABI_TeamList(CCINIClass* ini,int scope){TeamTypeClass::LoadFromINIList(ini,scope);}
extern "C" __declspec(noinline) void RA2ABI_ScriptList(CCINIClass* ini,int scope){ScriptTypeClass::LoadFromINIList(ini,scope);}
extern "C" __declspec(noinline) void RA2ABI_TaskForceList(CCINIClass* ini,int scope){TaskForceClass::LoadFromINIList(ini,scope);}
extern "C" __declspec(noinline) void RA2ABI_AITriggerList(CCINIClass* ini,int scope){AITriggerTypeClass::LoadFromINIList(ini,scope);}
extern "C" __declspec(noinline) void RA2ABI_TriggerList(CCINIClass* ini){TriggerTypeClass::LoadFromINIList(ini);}
extern "C" __declspec(noinline) void RA2ABI_TagList(CCINIClass* ini){TagTypeClass::LoadFromINIList(ini);}
extern "C" __declspec(noinline) void RA2ABI_TeamProcess(TeamTypeClass* type){type->ProcessTaskForce();}
extern "C" __declspec(noinline) int RA2ABI_TaskForceTech(TaskForceClass* type){return type->GetRequiredTechLevel();}

static_assert(sizeof(ScriptTypeClass)==0x234 && offsetof(ScriptTypeClass,IsGlobal)==0x9C && sizeof(ScriptTypeClass::IsGlobal)==4);

PROBE CoordStruct RA2ABI_FlyDestination(FlyLocomotionClass* p) { return p->Destination(); }
PROBE int RA2ABI_FlyLandingAltitude(IFlyControl* p) { return p->Landing_Altitude(); }
static_assert(sizeof(FlyLocomotionClass)==0x60 && offsetof(FlyLocomotionClass,MovingDestination)==0x1C);
static_assert(sizeof(AircraftClass)==0x6D8 && offsetof(AircraftClass,Type)==0x6C4);

// Paradrop entry points and fields calibrated against the fixed YR executable.
PROBE int RA2ABI_AircraftParaApproach(AircraftClass* p){return p->Mission_ParaDropApproach();}
PROBE int RA2ABI_AircraftParaOverfly(AircraftClass* p){return p->Mission_ParaDropOverfly();}
PROBE int RA2ABI_AircraftRetreat(AircraftClass* p){return p->Mission_Retreat();}
PROBE bool RA2ABI_ObjectParachuted(ObjectClass* p,const CoordStruct& c){return p->SpawnParachuted(c);}
PROBE bool RA2ABI_InfantryParachuted(InfantryClass* p,const CoordStruct& c){return p->SpawnParachuted(c);}
PROBE CoordStruct* RA2ABI_AnimCoords(AnimClass* p,CoordStruct* c){return p->GetCoords(c);}
static_assert(offsetof(AircraftClass,IsLocked)==0x6D2 && offsetof(AircraftClass,NumParadropsLeft)==0x6D3);
static_assert(offsetof(ObjectClass,HasParachute)==0x84);
static_assert(offsetof(AnimClass,OwnerObject)==0xCC && offsetof(AnimClass,LightConvert)==0xD4 && offsetof(AnimClass,TintColor)==0xFC);
static_assert(offsetof(CellClass,Intensity_Normal)==0x10A);
static_assert(offsetof(HouseClass,ActiveUnitTypes)==0x5564 && offsetof(HouseClass,ActiveInfantryTypes)==0x5578);
