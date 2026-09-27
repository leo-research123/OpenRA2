#include "support/test_support.hpp"
#include "api/type_resources.hpp"
#include "api/ini_runtime.hpp"
#include "api/filesystem.hpp"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/CRC.h"
#include "yrpp/FileFormats/VXL.h"
#include "yrpp/FileFormats/HVA.h"
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>

TEST(BulletType, CompleteINIAndArtFields) {
 struct Context {BulletTypeClass bullet{"P2_BINI"};CCINIClass ini,art;AnimTypeClass trailer{"P2_TRAILER"};
  WeaponTypeClass burst{"P2_BURST"},shrapnel{"P2_SHRAPNEL"};} c;
 game::TypeResourceServices resources;resources.art=&c.art;
 const auto operation=[](void* raw) {
  auto& c=*static_cast<Context*>(raw);auto& b=c.bullet;auto& ini=c.ini;
  EXPECT_FALSE(b.LoadFromINI(nullptr));EXPECT_FALSE(b.LoadFromINI(&ini));
  struct Flag {const char* key;bool BulletTypeClass::* member;};
  constexpr Flag flags[]{
#define F(n) {#n,&BulletTypeClass::n}
   F(Arcing),F(Floater),F(SubjectToCliffs),F(SubjectToElevation),F(SubjectToWalls),F(VeryHigh),F(Shadow),
   F(Dropping),F(Level),F(Inviso),F(Proximity),F(Ranged),F(Inaccurate),F(FlakScatter),F(AA),F(AG),
   F(Degenerates),F(Bouncy),F(Airburst),F(Scalable),F(Vertical),F(FirersPalette)
#undef F
  };
  for(bool value:{true,false}) {
   for(auto f:flags)ini.WriteString(b.ID,f.key,value?"yes":"no");
   for(auto key:{"Arm","ROT","CourseLockDuration","Acceleration","Cluster","ShrapnelCount","DetonationAltitude"})
    ini.WriteString(b.ID,key,"17");
   ini.WriteString(b.ID,"Name","Base name");ini.WriteString(b.ID,"Strength","31");
   ini.WriteString(b.ID,"Elasticity","0.375");ini.WriteString(b.ID,"Image","P2_BIMAGE");
   ini.WriteString(b.ID,"AirburstWeapon","P2_BURST");ini.WriteString(b.ID,"ShrapnelWeapon","P2_SHRAPNEL");
   c.art.WriteString("P2_BIMAGE","Trailer","P2_TRAILER");c.art.WriteString("P2_BIMAGE","SpawnDelay","13");
   c.art.WriteString("P2_BIMAGE","Rotates",value?"yes":"no");c.art.WriteString("P2_BIMAGE","Flat",value?"yes":"no");
   c.art.WriteString("P2_BIMAGE","AnimPalette",value?"yes":"no");
   c.art.WriteString("P2_BIMAGE","AnimLow","-1");c.art.WriteString("P2_BIMAGE","AnimHigh","256");
   c.art.WriteString("P2_BIMAGE","AnimRate","257");
   ASSERT_TRUE(b.LoadFromINI(&ini));
   for(auto f:flags)EXPECT_EQ(b.*f.member,value)<<f.key;
   EXPECT_STREQ(b.Name,"Base name");EXPECT_EQ(b.Strength,31);EXPECT_EQ(b.Elasticity,0.375);
   EXPECT_EQ(b.Arm,17);EXPECT_EQ(b.ROT,17);EXPECT_EQ(b.CourseLockDuration,17);EXPECT_EQ(b.Acceleration,17);
   EXPECT_EQ(b.Cluster,17);EXPECT_EQ(b.ShrapnelCount,17);EXPECT_EQ(b.DetonationAltitude,17);
   EXPECT_EQ(b.AirburstWeapon,&c.burst);EXPECT_EQ(b.ShrapnelWeapon,&c.shrapnel);EXPECT_EQ(b.Trailer,&c.trailer);
   EXPECT_EQ(b.SpawnDelay,13);EXPECT_EQ(b.NoRotate,!value);EXPECT_EQ(b.Flat,value);EXPECT_EQ(b.AnimPalette,value);
   EXPECT_EQ(b.AnimLow,255);EXPECT_EQ(b.AnimHigh,0);EXPECT_EQ(b.AnimRate,1);
  }
  CCINIClass overlay;overlay.WriteString(b.ID,"AirburstWeapon","NONE");overlay.WriteString(b.ID,"Inviso","yes");
  ASSERT_TRUE(b.LoadFromINI(&overlay));EXPECT_STREQ(b.ImageFile,"");EXPECT_EQ(b.AirburstWeapon,nullptr);
  EXPECT_EQ(b.ShrapnelWeapon,&c.shrapnel);EXPECT_EQ(b.Trailer,&c.trailer);EXPECT_EQ(b.SpawnDelay,13);
 };
 EXPECT_EQ(game::with_type_resources(resources,operation,&c),game::TypeResourceStatus::complete);
}
namespace {
void write_voxel_pair(const std::filesystem::path& root) {
 // One section, one occupied voxel, and one HVA frame. No original assets.
 VoxFileHeader header{};std::memcpy(header.filename,"Voxel Animation",15);
 header.countHeaders_OrSections1=header.countTailers_OrSections2=1;header.totalSize=13;
 VoxelSectionFileHeader section{};
 const std::uint32_t spans[]{0,4};const byte voxel[]{0,1,42,0,1};
 VoxelSectionFileTailer tail{0,4,8,2.0f,Matrix3D{},{0,0,0},{1,1,1},1,1,1,2};
 tail.TransformationMatrix.MakeIdentity();
 std::ofstream vxl(root/"P2_VOXIMG.VXL",std::ios::binary);
 vxl.write(reinterpret_cast<const char*>(&header),sizeof(header));
 vxl.write(reinterpret_cast<const char*>(&section),sizeof(section));
 vxl.write(reinterpret_cast<const char*>(spans),sizeof(spans));
 vxl.write(reinterpret_cast<const char*>(voxel),sizeof(voxel));
 vxl.write(reinterpret_cast<const char*>(&tail),sizeof(tail));
 const char name[16]{};const std::uint32_t counts[]{1,1};
 Matrix3D matrix;matrix.MakeIdentity();matrix.row[0][3]=3.0f;
 std::ofstream hva(root/"P2_VOXIMG.HVA",std::ios::binary);
 hva.write(name,sizeof(name));hva.write(reinterpret_cast<const char*>(counts),sizeof(counts));
 hva.write(name,sizeof(name));hva.write(reinterpret_cast<const char*>(&matrix),sizeof(matrix));
 ASSERT_TRUE(vxl.good());ASSERT_TRUE(hva.good());
}
}
TEST(BulletType, VoxelResourcesPreserveOriginalINISuccessAndFailureCleanup) {
 const auto root=std::filesystem::temp_directory_path()/
  ("ra2-bullet-voxel-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directory(root);
 struct Cleanup {std::filesystem::path path;~Cleanup(){std::error_code ec;std::filesystem::remove_all(path,ec);}} cleanup{root};
 game::ResourceHandle* handle=nullptr;std::string error;
 ASSERT_TRUE(game::create_resources(root.string(),handle,error))<<error;
 std::unique_ptr<game::ResourceHandle,decltype(&game::destroy_resources)> files(handle,game::destroy_resources);
 struct Context {BulletTypeClass bullet{"P2_VOX"};CCINIClass ini,art;bool result=false;
  game::TypeResourceStatus status=game::TypeResourceStatus::failure;} c;
 c.ini.WriteString(c.bullet.ID,"Image","P2_VOXIMG");c.art.WriteString("P2_VOXIMG","Voxel","yes");
 const auto load=[&] {
  ASSERT_TRUE(game::with_resources(*handle,[](void* raw){auto& c=*static_cast<Context*>(raw);
   game::TypeResourceServices resources;resources.art=&c.art;
   c.status=game::with_type_resources(resources,[](void* raw){auto& c=*static_cast<Context*>(raw);
    c.result=c.bullet.LoadFromINI(&c.ini);},&c);
  },&c,error))<<error;
  // 0x46C417 calls ObjectType::LoadVoxel, then 0x46C41C returns true
  // even when 0x5F8110 could not acquire the pair. INI success != drawable.
  EXPECT_EQ(c.status,game::TypeResourceStatus::complete);EXPECT_TRUE(c.result);EXPECT_TRUE(c.bullet.Voxel);
 };
 for(const char* fault:{"missing_vxl","bad_vxl","missing_hva","bad_hva"}) {
  SCOPED_TRACE(fault);
  write_voxel_pair(root);load();
  ASSERT_NE(c.bullet.MainVoxel.VXL,nullptr);ASSERT_NE(c.bullet.MainVoxel.HVA,nullptr);
  EXPECT_FALSE(c.bullet.MainVoxel.VXL->Initialized);EXPECT_FALSE(c.bullet.MainVoxel.HVA->LoadedFailed);
  EXPECT_EQ(c.bullet.MainVoxel.VXL->CountHeaders,1u);
  EXPECT_EQ(c.bullet.MainVoxel.HVA->GetLayerMatrix(0,0).row[0][3],6.0f);
  if(!std::strcmp(fault,"missing_vxl"))std::filesystem::remove(root/"P2_VOXIMG.VXL");
  if(!std::strcmp(fault,"missing_hva"))std::filesystem::remove(root/"P2_VOXIMG.HVA");
  if(!std::strcmp(fault,"bad_vxl"))std::ofstream(root/"P2_VOXIMG.VXL",std::ios::binary)<<"bad";
  if(!std::strcmp(fault,"bad_hva"))std::ofstream(root/"P2_VOXIMG.HVA",std::ios::binary)<<"bad";
  load();
  EXPECT_EQ(c.bullet.MainVoxel.VXL,nullptr);EXPECT_EQ(c.bullet.MainVoxel.HVA,nullptr);
 }
}
TEST(BulletType, OriginalCRCCorpus) {
 BulletTypeClass b("P2_BCRC");std::ifstream input(RA2_BULLET_FIXTURE);ASSERT_TRUE(input.good());unsigned cases=0,id,mask;
 constexpr bool BulletTypeClass::* flags[]{&BulletTypeClass::Airburst,&BulletTypeClass::Shadow,&BulletTypeClass::Arcing,
  &BulletTypeClass::Dropping,&BulletTypeClass::Level,&BulletTypeClass::Inviso,&BulletTypeClass::Proximity,&BulletTypeClass::Ranged,
  &BulletTypeClass::NoRotate,&BulletTypeClass::Inaccurate,&BulletTypeClass::FlakScatter,&BulletTypeClass::AA,&BulletTypeClass::AG,
  &BulletTypeClass::Degenerates,&BulletTypeClass::Bouncy,&BulletTypeClass::Flat};
 while(input>>id>>mask) {
  unsigned expected,staging,value;int index;
  ASSERT_TRUE(bool(input>>b.Elasticity>>b.Acceleration>>b.ROT>>b.Arm>>expected>>index>>staging>>value));
  b.UniqueID=id;b.Dirty=(mask&(1u<<16))!=0;for(unsigned i=0;i<std::size(flags);++i)b.*flags[i]=(mask&(1u<<i))!=0;
  CRCEngine crc;b.ComputeCRC(crc);EXPECT_EQ(std::uint32_t(crc.CRC),expected);EXPECT_EQ(crc.Index,index);
  EXPECT_EQ(std::uint32_t(crc.StagingBuffer.Composite),staging);EXPECT_EQ(std::uint32_t(crc()),value);++cases;
 }
 EXPECT_EQ(cases,64u);
}
