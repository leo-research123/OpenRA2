// YRpp RulesClass::PointerGotInvalid, calibrated to fixed YR 678850.
#include "yrpp/RulesClass.h"

namespace {
template<typename T>
void remove_reference(TypeList<T*>& list, AbstractClass* invalid) {
    // Original type objects place AbstractClass at offset zero. No object
    // dereference or ownership transfer is involved in this identity lookup.
    list.Remove(reinterpret_cast<T*>(invalid));
}
}

void RulesClass::PointerGotInvalid(AbstractClass* invalid, bool removed) {
    // 678850 ignores removed, clears selected scalar references, then removes
    // the first matching element from each listed container in this order.
    (void)removed;
    if (static_cast<void*>(LargeVisceroid) == static_cast<void*>(invalid)) LargeVisceroid = nullptr;
    if (static_cast<void*>(SmallVisceroid) == static_cast<void*>(invalid)) SmallVisceroid = nullptr;
    if (static_cast<void*>(VeinAttack) == static_cast<void*>(invalid)) VeinAttack = nullptr;
    if (static_cast<void*>(Wake) == static_cast<void*>(invalid)) Wake = nullptr;
    if (static_cast<void*>(OreTwinkle) == static_cast<void*>(invalid)) OreTwinkle = nullptr;
    if (static_cast<void*>(FlamingInfantry) == static_cast<void*>(invalid)) FlamingInfantry = nullptr;
    if (static_cast<void*>(NukeTakeOff) == static_cast<void*>(invalid)) NukeTakeOff = nullptr;
    if (static_cast<void*>(InfantryHeadPop) == static_cast<void*>(invalid)) InfantryHeadPop = nullptr;
    if (static_cast<void*>(InfantryNuked) == static_cast<void*>(invalid)) InfantryNuked = nullptr;
    if (static_cast<void*>(InfantryVirus) == static_cast<void*>(invalid)) InfantryVirus = nullptr;
    if (static_cast<void*>(InfantryBrute) == static_cast<void*>(invalid)) InfantryBrute = nullptr;
    if (static_cast<void*>(InfantryMutate) == static_cast<void*>(invalid)) InfantryMutate = nullptr;
    if (static_cast<void*>(Behind) == static_cast<void*>(invalid)) Behind = nullptr;
    if (static_cast<void*>(DropPodWeapon) == static_cast<void*>(invalid)) DropPodWeapon = nullptr;
    if (static_cast<void*>(FlameDamage) == static_cast<void*>(invalid)) FlameDamage = nullptr;
    if (static_cast<void*>(FlameDamage2) == static_cast<void*>(invalid)) FlameDamage2 = nullptr;
    if (static_cast<void*>(NukeWarhead) == static_cast<void*>(invalid)) NukeWarhead = nullptr;
    if (static_cast<void*>(MutateWarhead) == static_cast<void*>(invalid)) MutateWarhead = nullptr;
    if (static_cast<void*>(MutateExplosionWarhead) == static_cast<void*>(invalid)) MutateExplosionWarhead = nullptr;
    if (static_cast<void*>(EMPulseWarhead) == static_cast<void*>(invalid)) EMPulseWarhead = nullptr;
    if (static_cast<void*>(C4Warhead) == static_cast<void*>(invalid)) C4Warhead = nullptr;
    // 67897C checks CrushWarhead but clears C4Warhead.
    if (static_cast<void*>(CrushWarhead) == static_cast<void*>(invalid)) C4Warhead = nullptr;
    if (static_cast<void*>(V3Warhead) == static_cast<void*>(invalid)) V3Warhead = nullptr;
    if (static_cast<void*>(DMislWarhead) == static_cast<void*>(invalid)) DMislWarhead = nullptr;
    if (static_cast<void*>(V3EliteWarhead) == static_cast<void*>(invalid)) V3EliteWarhead = nullptr;
    if (static_cast<void*>(DMislEliteWarhead) == static_cast<void*>(invalid)) DMislEliteWarhead = nullptr;
    if (static_cast<void*>(CMislWarhead) == static_cast<void*>(invalid)) CMislWarhead = nullptr;
    if (static_cast<void*>(CMislEliteWarhead) == static_cast<void*>(invalid)) CMislEliteWarhead = nullptr;
    if (static_cast<void*>(V3Rocket.Type) == static_cast<void*>(invalid)) V3Rocket.Type = nullptr;
    if (static_cast<void*>(DMisl.Type) == static_cast<void*>(invalid)) DMisl.Type = nullptr;
    if (static_cast<void*>(CMisl.Type) == static_cast<void*>(invalid)) CMisl.Type = nullptr;
    if (static_cast<void*>(IvanWarhead) == static_cast<void*>(invalid)) IvanWarhead = nullptr;
    if (static_cast<void*>(DeathWeapon) == static_cast<void*>(invalid)) DeathWeapon = nullptr;
    if (static_cast<void*>(IonCannonWarhead) == static_cast<void*>(invalid)) IonCannonWarhead = nullptr;
    if (static_cast<void*>(EMPulseSparkles) == static_cast<void*>(invalid)) EMPulseSparkles = nullptr;
    if (static_cast<void*>(DefaultLargeGreySmokeSystem) == static_cast<void*>(invalid)) DefaultLargeGreySmokeSystem = nullptr;
    if (static_cast<void*>(DefaultSmallGreySmokeSystem) == static_cast<void*>(invalid)) DefaultSmallGreySmokeSystem = nullptr;
    if (static_cast<void*>(DefaultSparkSystem) == static_cast<void*>(invalid)) DefaultSparkSystem = nullptr;
    if (static_cast<void*>(DefaultLargeRedSmokeSystem) == static_cast<void*>(invalid)) DefaultLargeRedSmokeSystem = nullptr;
    if (static_cast<void*>(DefaultSmallRedSmokeSystem) == static_cast<void*>(invalid)) DefaultSmallRedSmokeSystem = nullptr;
    if (static_cast<void*>(DefaultDebrisSmokeSystem) == static_cast<void*>(invalid)) DefaultDebrisSmokeSystem = nullptr;
    if (static_cast<void*>(DefaultFireStreamSystem) == static_cast<void*>(invalid)) DefaultFireStreamSystem = nullptr;
    if (static_cast<void*>(DefaultTestParticleSystem) == static_cast<void*>(invalid)) DefaultTestParticleSystem = nullptr;
    if (static_cast<void*>(DefaultRepairParticleSystem) == static_cast<void*>(invalid)) DefaultRepairParticleSystem = nullptr;
    if (static_cast<void*>(VeinholeTypeClass) == static_cast<void*>(invalid)) VeinholeTypeClass = nullptr;
    if (static_cast<void*>(WeatherConBoltExplosion) == static_cast<void*>(invalid)) WeatherConBoltExplosion = nullptr;
    if (static_cast<void*>(DominatorFirstAnim) == static_cast<void*>(invalid)) DominatorFirstAnim = nullptr;
    if (static_cast<void*>(DominatorSecondAnim) == static_cast<void*>(invalid)) DominatorSecondAnim = nullptr;
    if (static_cast<void*>(DrainAnimationType) == static_cast<void*>(invalid)) DrainAnimationType = nullptr;
    if (static_cast<void*>(ControlledAnimationType) == static_cast<void*>(invalid)) ControlledAnimationType = nullptr;
    if (static_cast<void*>(PermaControlledAnimationType) == static_cast<void*>(invalid)) PermaControlledAnimationType = nullptr;
    if (static_cast<void*>(Smoke) == static_cast<void*>(invalid)) Smoke = nullptr;
    if (static_cast<void*>(Smoke_) == static_cast<void*>(invalid)) Smoke_ = nullptr;
    if (static_cast<void*>(MoveFlash) == static_cast<void*>(invalid)) MoveFlash = nullptr;
    if (static_cast<void*>(BombParachute) == static_cast<void*>(invalid)) BombParachute = nullptr;
    if (static_cast<void*>(Parachute) == static_cast<void*>(invalid)) Parachute = nullptr;
    if (static_cast<void*>(SmallFire) == static_cast<void*>(invalid)) SmallFire = nullptr;
    if (static_cast<void*>(LargeFire) == static_cast<void*>(invalid)) LargeFire = nullptr;
    if (static_cast<void*>(DropZoneAnim) == static_cast<void*>(invalid)) DropZoneAnim = nullptr;
    if (static_cast<void*>(UnitCrateType) == static_cast<void*>(invalid)) UnitCrateType = nullptr;
    if (static_cast<void*>(Paratrooper) == static_cast<void*>(invalid)) Paratrooper = nullptr;
    if (static_cast<void*>(AlliedDisguise) == static_cast<void*>(invalid)) AlliedDisguise = nullptr;
    if (static_cast<void*>(SovietDisguise) == static_cast<void*>(invalid)) SovietDisguise = nullptr;
    if (static_cast<void*>(ThirdDisguise) == static_cast<void*>(invalid)) ThirdDisguise = nullptr;
    if (static_cast<void*>(Technician) == static_cast<void*>(invalid)) Technician = nullptr;
    if (static_cast<void*>(Engineer) == static_cast<void*>(invalid)) Engineer = nullptr;
    if (static_cast<void*>(Pilot) == static_cast<void*>(invalid)) Pilot = nullptr;
    if (static_cast<void*>(AlliedCrew) == static_cast<void*>(invalid)) AlliedCrew = nullptr;
    if (static_cast<void*>(SovietCrew) == static_cast<void*>(invalid)) SovietCrew = nullptr;
    if (static_cast<void*>(ThirdCrew) == static_cast<void*>(invalid)) ThirdCrew = nullptr;
    if (static_cast<void*>(GDIGateOne) == static_cast<void*>(invalid)) GDIGateOne = nullptr;
    if (static_cast<void*>(GDIGateTwo) == static_cast<void*>(invalid)) GDIGateTwo = nullptr;
    if (static_cast<void*>(NodGateOne) == static_cast<void*>(invalid)) NodGateOne = nullptr;
    if (static_cast<void*>(NodGateTwo) == static_cast<void*>(invalid)) NodGateTwo = nullptr;
    if (static_cast<void*>(WallTower) == static_cast<void*>(invalid)) WallTower = nullptr;
    if (static_cast<void*>(GDIPowerPlant) == static_cast<void*>(invalid)) GDIPowerPlant = nullptr;
    if (static_cast<void*>(NodRegularPower) == static_cast<void*>(invalid)) NodRegularPower = nullptr;
    if (static_cast<void*>(NodAdvancedPower) == static_cast<void*>(invalid)) NodAdvancedPower = nullptr;
    if (static_cast<void*>(ThirdPowerPlant) == static_cast<void*>(invalid)) ThirdPowerPlant = nullptr;
    // 678CA8 checks PrerequisiteProcAlternate but clears ThirdPowerPlant.
    if (static_cast<void*>(PrerequisiteProcAlternate) == static_cast<void*>(invalid)) ThirdPowerPlant = nullptr;
    if (static_cast<void*>(NukeProjectile) == static_cast<void*>(invalid)) NukeProjectile = nullptr;
    if (static_cast<void*>(NukeDown) == static_cast<void*>(invalid)) NukeDown = nullptr;
    if (static_cast<void*>(EMPulseProjectile) == static_cast<void*>(invalid)) EMPulseProjectile = nullptr;
    if (static_cast<void*>(TireVoxelDebris) == static_cast<void*>(invalid)) TireVoxelDebris = nullptr;
    if (static_cast<void*>(ScrapVoxelDebris) == static_cast<void*>(invalid)) ScrapVoxelDebris = nullptr;
    if (static_cast<void*>(AtmosphereEntry) == static_cast<void*>(invalid)) AtmosphereEntry = nullptr;
    if (static_cast<void*>(InfantryExplode) == static_cast<void*>(invalid)) InfantryExplode = nullptr;
    if (static_cast<void*>(IonBlast) == static_cast<void*>(invalid)) IonBlast = nullptr;
    if (static_cast<void*>(IonBeam) == static_cast<void*>(invalid)) IonBeam = nullptr;
    if (static_cast<void*>(ChronoBlast) == static_cast<void*>(invalid)) ChronoBlast = nullptr;
    if (static_cast<void*>(ChronoBlastDest) == static_cast<void*>(invalid)) ChronoBlastDest = nullptr;
    if (static_cast<void*>(ChronoPlacement) == static_cast<void*>(invalid)) ChronoPlacement = nullptr;
    if (static_cast<void*>(ChronoBeam) == static_cast<void*>(invalid)) ChronoBeam = nullptr;
    if (static_cast<void*>(WarpIn) == static_cast<void*>(invalid)) WarpIn = nullptr;
    if (static_cast<void*>(WarpOut) == static_cast<void*>(invalid)) WarpOut = nullptr;
    if (static_cast<void*>(WarpAway) == static_cast<void*>(invalid)) WarpAway = nullptr;
    if (static_cast<void*>(ChronoSparkle1) == static_cast<void*>(invalid)) ChronoSparkle1 = nullptr;
    if (static_cast<void*>(IronCurtainInvokeAnim) == static_cast<void*>(invalid)) IronCurtainInvokeAnim = nullptr;
    if (static_cast<void*>(ForceShieldInvokeAnim) == static_cast<void*>(invalid)) ForceShieldInvokeAnim = nullptr;
    if (static_cast<void*>(WeaponNullifyAnim) == static_cast<void*>(invalid)) WeaponNullifyAnim = nullptr;
    if (static_cast<void*>(Dig) == static_cast<void*>(invalid)) Dig = nullptr;
    if (static_cast<void*>(BarrelExplode) == static_cast<void*>(invalid)) BarrelExplode = nullptr;
    if (static_cast<void*>(BarrelParticle) == static_cast<void*>(invalid)) BarrelParticle = nullptr;
    if (static_cast<void*>(DropPodPuff) == static_cast<void*>(invalid)) DropPodPuff = nullptr;
    if (static_cast<void*>(LightningWarhead) == static_cast<void*>(invalid)) LightningWarhead = nullptr;
    if (static_cast<void*>(DominatorWarhead) == static_cast<void*>(invalid)) DominatorWarhead = nullptr;
    if (static_cast<void*>(RadSiteWarhead) == static_cast<void*>(invalid)) RadSiteWarhead = nullptr;
    remove_reference(BarrelDebris, invalid);
    remove_reference(OnFire, invalid);
    remove_reference(TreeFire, invalid);
    remove_reference(SplashList, invalid);
    remove_reference(Scorches, invalid);
    remove_reference(Scorches1, invalid);
    remove_reference(Scorches2, invalid);
    remove_reference(Scorches3, invalid);
    remove_reference(Scorches4, invalid);
    remove_reference(BaseUnit, invalid);
    remove_reference(DefaultMirageDisguises, invalid);
    remove_reference(HarvesterUnit, invalid);
    remove_reference(SecretInfantry, invalid);
    remove_reference(SecretUnits, invalid);
    remove_reference(SecretBuildings, invalid);
    remove_reference(RepairBay, invalid);
    remove_reference(Shipyard, invalid);
    remove_reference(BuildConst, invalid);
    remove_reference(BuildPower, invalid);
    remove_reference(BuildRefinery, invalid);
    remove_reference(BuildBarracks, invalid);
    remove_reference(BuildTech, invalid);
    remove_reference(BuildWeapons, invalid);
    remove_reference(AlliedBaseDefenses, invalid);
    remove_reference(SovietBaseDefenses, invalid);
    remove_reference(ThirdBaseDefenses, invalid);
    remove_reference(BuildDefense, invalid);
    remove_reference(BuildPDefense, invalid);
    remove_reference(BuildAA, invalid);
    remove_reference(BuildHelipad, invalid);
    remove_reference(BuildRadar, invalid);
    remove_reference(ConcreteWalls, invalid);
    remove_reference(NSGates, invalid);
    remove_reference(EWGates, invalid);
    remove_reference(BuildNavalYard, invalid);
    remove_reference(BuildDummy, invalid);
    remove_reference(NeutralTechBuildings, invalid);
    remove_reference(PadAircraft, invalid);
    remove_reference(DeadBodies, invalid);
    remove_reference(DropPod, invalid);
    remove_reference(MetallicDebris, invalid);
    remove_reference(BridgeExplosions, invalid);
    remove_reference(WeatherConClouds, invalid);
    remove_reference(WeatherConBolts, invalid);
    remove_reference(AmerParaDropInf, invalid);
    remove_reference(AllyParaDropInf, invalid);
    remove_reference(SovParaDropInf, invalid);
    remove_reference(YuriParaDropInf, invalid);
    remove_reference(AnimToInfantry, invalid);
    remove_reference(DamageFireTypes, invalid);
}
