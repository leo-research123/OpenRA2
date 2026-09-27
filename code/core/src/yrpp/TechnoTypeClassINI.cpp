// OpenTS 44fac744 techtype.cpp Read_INI movement fields, YR 0x00712170.
// GPL-3.0-or-later; EA Section 7 terms: third_party/opents/LICENSE.md.
// Display/movement and weapon/deployment fields; the full INI package remains open.
#include "yrpp/TechnoTypeClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/UnitTypeClass.h"
#include "type_resources.hpp"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/ParticleSystemTypeClass.h"
#include "RulesClassReaders.hpp"
#include <cstdio>
#include "yrpp/HouseTypeClass.h"
#include "yrpp/FileSystem.h"
bool TechnoTypeClass::LoadFromINI(CCINIClass*ini){
 try{
 if(!ini||!ObjectTypeClass::LoadFromINI(ini))return false;
 SpeedType=static_cast<::SpeedType>(ini->ReadSpeedType(ID,"SpeedType",static_cast<int>(SpeedType)));
 Speed=read_rule_percentage(*ini,ID,"Speed",Speed);
 MovementZone=static_cast<::MovementZone>(ini->ReadMovementZone(ID,"MovementZone",static_cast<int>(MovementZone)));
 IsSubterranean=MovementZone==::MovementZone::Subterrannean;
 char locomotor[128]{};if(ini->ReadString(ID,"Locomotor","",locomotor,sizeof(locomotor)))read_rule_guid(locomotor,Locomotor);
 ThreatAvoidanceCoefficient=ini->ReadDouble(ID,"ThreatAvoidanceCoefficient",ThreatAvoidanceCoefficient);
 WalkRate=ini->ReadInteger(ID,"WalkRate",WalkRate);IdleRate=ini->ReadInteger(ID,"IdleRate",IdleRate);
 // OpenTS Primary/Secondary slots, calibrated to YR 0x712170: elite slots
 // are independent (GetWeapon supplies the fallback), not TS's third slot.
 if(!game::type_resources().combat_unavailable){
 TurretCount=ini->ReadInteger(ID,"TurretCount",TurretCount);
 WeaponCount=ini->ReadInteger(ID,"WeaponCount",WeaponCount);
 ClearAllWeapons=ini->ReadBool(ID,"ClearAllWeapons",ClearAllWeapons);
 if(TurretCount<=0){
  if(!ClearAllWeapons){
   read_rule_type(*ini,ID,"Primary",AbstractType::WeaponType,Weapon[0].WeaponType);
   read_rule_type(*ini,ID,"Secondary",AbstractType::WeaponType,Weapon[1].WeaponType);
   read_rule_type(*ini,ID,"ElitePrimary",AbstractType::WeaponType,EliteWeapon[0].WeaponType);
   read_rule_type(*ini,ID,"EliteSecondary",AbstractType::WeaponType,EliteWeapon[1].WeaponType);
  }
 }else{
  if(WeaponCount>MaxWeapons)return false; // never reproduce a fixed-array overrun
  for(int i=0;i<WeaponCount;++i){
   char key[32];std::snprintf(key,sizeof(key),"Weapon%d",i+1);
   read_rule_type(*ini,ID,key,AbstractType::WeaponType,Weapon[i].WeaponType);
   std::snprintf(key,sizeof(key),"EliteWeapon%d",i+1);
   read_rule_type(*ini,ID,key,AbstractType::WeaponType,EliteWeapon[i].WeaponType);
  }
 }
 if(ClearAllWeapons){Weapon[0].WeaponType=Weapon[1].WeaponType=nullptr;EliteWeapon[0].WeaponType=EliteWeapon[1].WeaponType=nullptr;}
 DeployFireWeapon=ini->ReadInteger(ID,"DeployFireWeapon",DeployFireWeapon);
 DeployFire=ini->ReadBool(ID,"DeployFire",DeployFire);
 // YR 0x00714BA8..0x00714BBA uses the input rules INI and type ID.
 // Reading ART leaves Yuri at -1: AI then auto-deploys him indefinitely
 // instead of honoring the psychic-wave deployment's 150-frame delay.
 UndeployDelay=ini->ReadInteger(ID,"UndeployDelay",UndeployDelay);
 }
#define B(f) f=ini->ReadBool(ID,#f,f)
 B(Repairable);B(Crewed);
 B(Turret);B(TurretSpins);B(DamageSparks);B(ImmuneToPsionics);B(NoShadow);
 B(PipsDrawForAll);B(Explodes);B(Crusher);B(Accelerates);B(TiltsWhenCrushes);
 B(Trainable);B(DontScore);B(OpenTopped);B(Gunner);B(HasTurretTooltips);B(Naval);B(Organic);B(IsChargeTurret);
 B(CanApproachTarget);B(CanRecalcApproachTarget);
 B(CanDisguise);B(PermaDisguise);B(DetectDisguise);B(DisguiseWhenStill);
 B(Teleporter);B(ResourceGatherer);B(ResourceDestination);B(SelfHealing);B(IsGattling);
 B(RequiresStolenAlliedTech);B(RequiresStolenSovietTech);B(RequiresStolenThirdTech);
 // YR 0x7142B4 / 0x7142CE / 0x714F86: a LimboLaunch attacker must
 // retain the rules-defined return/selection contract (not just its weapon).
 B(ReselectIfLimboed);B(RejoinTeamIfLimboed);B(Parasiteable);
#undef B
 ini->ReadAbilities(reinterpret_cast<byte*>(&VeteranAbilities),ID,"VeteranAbilities",reinterpret_cast<byte*>(&VeteranAbilities));
 ini->ReadAbilities(reinterpret_cast<byte*>(&EliteAbilities),ID,"EliteAbilities",reinterpret_cast<byte*>(&EliteAbilities));
#define I(f) f=ini->ReadInteger(ID,#f,f)
 I(PixelSelectionBracketDelta);I(Soylent);I(Cost);I(Sight);I(Points);I(TechLevel);I(BuildLimit);I(AIBasePlanningSide);
 I(ROT);I(SlowdownDistance);I(IFVMode);I(FlightLevel);
 RollAngle=ini->ReadDouble(ID,"RollAngle",RollAngle);
 PitchAngle=ini->ReadDouble(ID,"PitchAngle",PitchAngle);
 PitchSpeed=ini->ReadDouble(ID,"PitchSpeed",PitchSpeed);
 I(MinDebris);I(MaxDebris);I(Passengers);I(InitialAmmo);I(Ammo);
 I(WeaponStages);I(RateUp);I(RateDown);
#undef I
 // YR 0x712170: Gattling stage thresholds are read independently from
 // TurretCount/WeaponCount; stage transitions use the previous stage's limit.
 if(IsGattling&&WeaponStages>1){
  if(WeaponStages>6)return false;
  for(int i=0;i<WeaponStages;++i){
   char key[32];std::snprintf(key,sizeof(key),"Stage%d",i+1);
   WeaponStage[i]=ini->ReadInteger(ID,key,WeaponStage[i]);
   std::snprintf(key,sizeof(key),"EliteStage%d",i+1);
   EliteStage[i]=ini->ReadInteger(ID,key,EliteStage[i]);
  }
 }
 read_rule_prerequisites(*ini,ID,"Prerequisite",Prerequisite);
 read_rule_type_list(*ini,ID,"Dock",AbstractType::BuildingType,Dock);
 read_rule_type(*ini,ID,"UnloadingClass",AbstractType::UnitType,UnloadingClass);
 Storage=ini->ReadInteger(ID,"Storage",Storage);
 read_rule_type(*ini,ID,"RefinerySmokeParticleSystem",AbstractType::ParticleSystemType,RefinerySmokeParticleSystem);
 ini->ReadPoint3D(RefinerySmokeOffsetOne,ID,"RefinerySmokeOffsetOne",RefinerySmokeOffsetOne);
 ini->ReadPoint3D(RefinerySmokeOffsetTwo,ID,"RefinerySmokeOffsetTwo",RefinerySmokeOffsetTwo);
 ini->ReadPoint3D(RefinerySmokeOffsetThree,ID,"RefinerySmokeOffsetThree",RefinerySmokeOffsetThree);
 ini->ReadPoint3D(RefinerySmokeOffsetFour,ID,"RefinerySmokeOffsetFour",RefinerySmokeOffsetFour);
 read_rule_prerequisites(*ini,ID,"PrerequisiteOverride",PrerequisiteOverride);
 const auto houses=[&](const char* key,DWORD& value){
  char buffer[1024]{};if(!ini->ReadString(ID,key,"",buffer,sizeof(buffer)))return;
  value=0;char* cursor=buffer;while(char* token=next_rule_token(cursor))if(auto* house=HouseTypeClass::Find(token))value|=DWORD(1)<<house->GetArrayIndex();
 };
 houses("Owner",OwnerFlags);houses("RequiredHouses",RequiredHouses);houses("ForbiddenHouses",ForbiddenHouses);
 DeployTime=ini->ReadDouble(ID,"DeployTime",DeployTime);
 AccelerationFactor=ini->ReadDouble(ID,"AccelerationFactor",AccelerationFactor);
 DecelerationFactor=ini->ReadDouble(ID,"DeaccelerationFactor",DecelerationFactor);
 Size=ini->ReadDouble(ID,"Size",Size);SizeLimit=ini->ReadDouble(ID,"SizeLimit",SizeLimit);
 read_rule_type_list(*ini,ID,"Explosion",AbstractType::AnimType,Explosion);
 read_rule_type_list(*ini,ID,"DestroyAnim",AbstractType::AnimType,DestroyAnim);
 read_rule_type_list(*ini,ID,"DebrisAnims",AbstractType::AnimType,DebrisAnims);
 PipScale=static_cast<::PipScale>(ini->ReadPipScale(ID,"PipScale",static_cast<int>(PipScale)));
 auto&art=game::type_art_ini();Normalized=art.ReadBool(ImageFile,"Normalized",Normalized);TurretOffset=art.ReadInteger(ImageFile,"TurretOffset",TurretOffset);Remapable=art.ReadBool(ImageFile,"Remapable",Remapable);WalkRate=art.ReadInteger(ImageFile,"WalkRate",WalkRate);IdleRate=art.ReadInteger(ImageFile,"IdleRate",IdleRate);art.ReadString(ImageFile,"Palette",PaletteFile,PaletteFile,sizeof(PaletteFile));
 art.ReadString(ImageFile,"Cameo",CameoFile,CameoFile,sizeof(CameoFile));
 art.ReadString(ImageFile,"AltCameo",AltCameoFile,AltCameoFile,sizeof(AltCameoFile));
 const auto cameo=[](const char* name)->SHPStruct*{if(!*name||INIClass::IsBlankValue(name))return nullptr;char file[64];std::snprintf(file,sizeof(file),"%s.shp",name);return static_cast<SHPStruct*>(FileSystem::LoadFile(file,false));};
 Cameo=cameo(*CameoFile?CameoFile:"XXICON");AltCameo=cameo(AltCameoFile);
 const auto weapon_art=[&](WeaponStruct& slot,const char* flh,const char* length,const char* thickness,const WeaponStruct& fallback){
  auto at=fallback.FLH;art.ReadPoint3D(slot.FLH,ImageFile,flh,at);
  slot.BarrelLength=art.ReadInteger(ImageFile,length,fallback.BarrelLength);
  slot.BarrelThickness=art.ReadInteger(ImageFile,thickness,fallback.BarrelThickness);
 };
 if(!game::type_resources().combat_unavailable&&TurretCount<=0){
  weapon_art(Weapon[0],"PrimaryFireFLH","PBarrelLength","PBarrelThickness",Weapon[0]);
  weapon_art(Weapon[1],"SecondaryFireFLH","SBarrelLength","SBarrelThickness",Weapon[1]);
  weapon_art(EliteWeapon[0],"ElitePrimaryFireFLH","ElitePBarrelLength","ElitePBarrelThickness",Weapon[0]);
  weapon_art(EliteWeapon[1],"EliteSecondaryFireFLH","EliteSBarrelLength","EliteSBarrelThickness",Weapon[1]);
 }else if(!game::type_resources().combat_unavailable){
  for(int i=0;i<WeaponCount;++i){
   char flh[64],length[64],thickness[64],locked[64];
   for(int elite=0;elite<2;++elite){
    const char* prefix=elite?"EliteWeapon":"Weapon";
    std::snprintf(flh,sizeof(flh),"%s%dFLH",prefix,i+1);
    std::snprintf(length,sizeof(length),"%s%dBarrelLength",prefix,i+1);
    std::snprintf(thickness,sizeof(thickness),"%s%dBarrelThickness",prefix,i+1);
    std::snprintf(locked,sizeof(locked),"%s%dTurretLocked",prefix,i+1);
    auto& slot=elite?EliteWeapon[i]:Weapon[i];
    weapon_art(slot,flh,length,thickness,Weapon[i]);
    slot.TurretLocked=art.ReadBool(ImageFile,locked,Weapon[i].TurretLocked);
   }
  }
 }
 if(!game::type_resources().combat_unavailable)for(int i=0;i<5;++i){char key[32];std::snprintf(key,sizeof(key),"AlternateFLH%d",i);auto fallback=Weapon[0].FLH;art.ReadPoint3D(AlternativeFLH[i],ImageFile,key,fallback);}
 return true;
 }catch(...){return false;}
}
