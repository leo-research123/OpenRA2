#include "support/test_support.hpp"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/ParticleSystemTypeClass.h"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/VoxelAnimTypeClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/CRC.h"
#include <cstring>
#include <fstream>

namespace {
struct Flag {const char* key;bool WarheadTypeClass::*member;};
constexpr Flag flags[]{
#define F(n) {#n,&WarheadTypeClass::n}
 F(CausesDelayKill),F(Conventional),F(Wall),F(WallAbsoluteDestroyer),F(PenetratesBunker),F(Wood),F(Tiberium),
 F(Sparky),F(Sonic),F(Rocker),F(DirectRocker),F(Fire),F(Bright),F(CLDisableRed),F(CLDisableGreen),F(CLDisableBlue),
 F(EMEffect),F(MindControl),F(Poison),F(IvanBomb),F(ElectricAssault),F(Parasite),F(Temporal),F(IsLocomotor),
 F(Airstrike),F(Psychedelic),F(BombDisarm),F(Culling),F(MakesDisguise),F(NukeMaker),F(Radiation),
 F(PsychicDamage),F(AffectsAllies),F(Bullets),F(Veinhole)
#undef F
};
}
TEST(WarheadType, AllINIFieldsAndOverlay) {
 WarheadTypeClass w("P2_WH");CCINIClass ini;
 EXPECT_FALSE(w.LoadFromINI(nullptr));EXPECT_FALSE(w.LoadFromINI(&ini));
 for(bool value:{true,false}) {
  for(auto f:flags)ini.WriteString(w.ID,f.key,value?"yes":"no");
  for(auto key:{"CellSpread","CellInset","PercentAtMax","DelayKillAtMax","CombatLightSize","Deform","ProneDamage"})
   ini.WriteString(w.ID,key,"0.75");
  for(auto key:{"DelayKillFrames","DeformThreshhold","InfDeath","Paralyzes","ShakeXlo","ShakeXhi","ShakeYlo","ShakeYhi"})
   ini.WriteString(w.ID,key,"7");
  ini.WriteString(w.ID,"Name","Not read here");
  ini.WriteString(w.ID,"Locomotor","{11223344-5566-7788-99aa-bbccddeeff00}");
  ini.WriteString(w.ID,"MaxDebris","-9");ini.WriteString(w.ID,"MinDebris","-3");
  ASSERT_TRUE(w.LoadFromINI(&ini));
  for(auto f:flags)EXPECT_EQ(w.*f.member,value)<<f.key;
  EXPECT_STREQ(w.Name,"P2_WH");
  EXPECT_EQ(w.CellSpread,0.75f);EXPECT_EQ(w.CellInset,0.75f);EXPECT_EQ(w.PercentAtMax,0.75f);
  EXPECT_EQ(w.DelayKillAtMax,0.75f);EXPECT_EQ(w.CombatLightSize,0.75f);EXPECT_EQ(w.Deform,0.75);EXPECT_EQ(w.ProneDamage,0.75);
  EXPECT_EQ(w.DelayKillFrames,7);EXPECT_EQ(w.DeformTreshold,7);EXPECT_EQ(int(w.InfDeath),7);EXPECT_EQ(w.Paralyzes,7);
  EXPECT_EQ(w.ShakeXlo,7);EXPECT_EQ(w.ShakeXhi,7);EXPECT_EQ(w.ShakeYlo,7);EXPECT_EQ(w.ShakeYhi,7);
  EXPECT_EQ(w.MaxDebris,0);EXPECT_EQ(w.MinDebris,0);
  EXPECT_EQ(w.Locomotor.Data1,0x11223344u);EXPECT_EQ(w.Locomotor.Data2,0x5566);EXPECT_EQ(w.Locomotor.Data3,0x7788);
  const unsigned char tail[]{0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,0};
  EXPECT_EQ(std::memcmp(w.Locomotor.Data4,tail,8),0);
 }
 CCINIClass overlay;overlay.WriteString(w.ID,"MinDebris","8");overlay.WriteString(w.ID,"Locomotor","invalid");
 ASSERT_TRUE(w.LoadFromINI(&overlay));EXPECT_EQ(w.MaxDebris,8);EXPECT_EQ(w.MinDebris,8);
 EXPECT_EQ(w.Locomotor.Data1,0x11223344u);EXPECT_EQ(w.Deform,0.75);
}
TEST(WarheadType, ReferencesListsAndExpiration) {
 WarheadTypeClass w("P2_WREF");AnimTypeClass a("P2_WANIM");VoxelAnimTypeClass debris("P2_WDEBRIS");
 CCINIClass ini;const int systems=ParticleSystemTypeClass::Array.Count;
 ini.WriteString(w.ID,"Particle","P2_WPARTICLE");ini.WriteString(w.ID,"AnimList","P2_WANIM,P2_WANIM,NONE");
 ini.WriteString(w.ID,"DebrisTypes","P2_WDEBRIS,P2_WDEBRIS");ini.WriteString(w.ID,"DebrisMaximums","2,-3,7");
 ASSERT_TRUE(w.LoadFromINI(&ini));
 auto* particle=w.Particle;ASSERT_NE(particle,nullptr);EXPECT_EQ(particle->WhatAmI(),AbstractType::ParticleSystemType);
 EXPECT_EQ(ParticleSystemTypeClass::Array.Count,systems+1);
 ASSERT_EQ(w.AnimList.Count,2);EXPECT_EQ(w.AnimList[0],&a);EXPECT_EQ(w.AnimList[1],&a);
 ASSERT_EQ(w.DebrisTypes.Count,2);EXPECT_EQ(w.DebrisTypes[0],&debris);EXPECT_EQ(w.DebrisTypes[1],&debris);
 ASSERT_EQ(w.DebrisMaximums.Count,3);EXPECT_EQ(w.DebrisMaximums[1],-3);
 GameDelete(particle);EXPECT_EQ(w.Particle,nullptr);EXPECT_EQ(ParticleSystemTypeClass::Array.Count,systems);
 CCINIClass overlay;overlay.WriteString(w.ID,"AnimList","NONE");overlay.WriteString(w.ID,"Particle","NONE");
 ASSERT_TRUE(w.LoadFromINI(&overlay));EXPECT_EQ(w.AnimList.Count,0);EXPECT_EQ(w.Particle,nullptr);
 EXPECT_EQ(w.DebrisMaximums.Count,3);
}
TEST(WarheadType, VersesUseIntegerPercentAndMissingKeyResets) {
 WarheadTypeClass w("P2_VERSES");CCINIClass ini;
 ini.WriteString(w.ID,"Verses","12.5%,0.125,-20%,150%,0,0.7,0,100%,1,25%,bad");
 ASSERT_TRUE(w.LoadFromINI(&ini));
 const double expected[]{0.12,0.125,-0.2,1.5,0,0.7,0,1,1,0.25,0};
 for(unsigned i=0;i<11;++i)EXPECT_DOUBLE_EQ(w.Verses[i],expected[i]);EXPECT_TRUE(w.unknown_bool_149);
 CCINIClass overlay;overlay.WriteString(w.ID,"Wall","yes");ASSERT_TRUE(w.LoadFromINI(&overlay));
 for(auto value:w.Verses)EXPECT_EQ(value,1);EXPECT_FALSE(w.unknown_bool_149);
 ini.WriteString(w.ID,"Verses","0,0");EXPECT_FALSE(w.LoadFromINI(&ini));
 for(auto value:w.Verses)EXPECT_EQ(value,1);
}
TEST(WarheadType, OriginalCRCCorpus) {
 WarheadTypeClass w("P2_WH_CRC");std::ifstream input(RA2_WARHEAD_FIXTURE);ASSERT_TRUE(input.good());
 constexpr bool WarheadTypeClass::* crc_flags[]{&WarheadTypeClass::Wall,&WarheadTypeClass::Wood,&WarheadTypeClass::Tiberium,
  &WarheadTypeClass::unknown_bool_149,&WarheadTypeClass::Sparky,&WarheadTypeClass::Sonic,&WarheadTypeClass::Fire,
  &WarheadTypeClass::Conventional,&WarheadTypeClass::Rocker,&WarheadTypeClass::DirectRocker,&WarheadTypeClass::Bright,
  &WarheadTypeClass::CLDisableRed,&WarheadTypeClass::CLDisableGreen,&WarheadTypeClass::CLDisableBlue,&WarheadTypeClass::Veinhole};
 unsigned cases=0,id,mask;
 while(input>>id>>mask) {
  int death,count,index;unsigned crc_expected,staging,value;
  ASSERT_TRUE(bool(input>>w.Deform>>w.DeformTreshold>>w.ProneDamage>>w.ShakeXlo>>w.ShakeXhi>>w.ShakeYlo>>w.ShakeYhi));
  w.UniqueID=id;w.Dirty=(mask&(1u<<15))!=0;
  for(unsigned i=0;i<std::size(crc_flags);++i)w.*crc_flags[i]=(mask&(1u<<i))!=0;
  unsigned char guid[16];for(auto& b:guid){unsigned n;ASSERT_TRUE(bool(input>>n));b=static_cast<unsigned char>(n);}
  std::memcpy(&w.Locomotor,guid,16);
  for(auto& verse:w.Verses)ASSERT_TRUE(bool(input>>verse));
  ASSERT_TRUE(bool(input>>count>>death>>crc_expected>>index>>staging>>value));
  w.AnimList.Count=count;w.InfDeath=static_cast<::InfDeath>(death);
  CRCEngine crc;w.ComputeCRC(crc);w.AnimList.Count=0;
  EXPECT_EQ(std::uint32_t(crc.CRC),crc_expected);EXPECT_EQ(crc.Index,index);
  EXPECT_EQ(std::uint32_t(crc.StagingBuffer.Composite),staging);EXPECT_EQ(std::uint32_t(crc()),value);++cases;
 }
 EXPECT_EQ(cases,64u);
}
