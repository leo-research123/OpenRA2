#include "support/test_support.hpp"
#include "api/projectile_diagnostics.hpp"
#include "projectile_diagnostics.hpp"
#include "yrpp/BulletClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/HouseTypeClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/WeaponTypeClass.h"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <string>

TEST(ProjectileDiagnostics, FlushedProvenanceSurvivesOwnerDeletionAndRotation) {
 const auto root=std::filesystem::temp_directory_path()/("ra2-projectile-log-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(root);const auto path=root/"projectiles.log";
 auto cleanup=ra2::test::scope_exit([&]{game::configure_projectile_log(nullptr);std::error_code error;std::filesystem::remove_all(root,error);});
 RulesClass rules;ScenarioClass scenario;auto* oldRules=RulesClass::Instance;auto* oldScenario=ScenarioClass::Instance;
 RulesClass::Instance=&rules;ScenarioClass::Instance=&scenario;
 auto restore=ra2::test::scope_exit([&]{RulesClass::Instance=oldRules;ScenarioClass::Instance=oldScenario;});
 ASSERT_TRUE(game::configure_projectile_log(path.string().c_str()));
 HouseTypeClass country("LOG_COUNTRY");HouseClass house(&country);UnitTypeClass tankType("MGTK");
 BulletTypeClass projectile("InvisibleLow");WeaponTypeClass weapon("MirageGun");
 auto* tank=new UnitClass(&tankType,&house);const int sourceId=tank->UniqueID;
 UnitTypeClass targetType("MTNK");UnitClass target(&targetType,&house);
 auto* bullet=BulletClass::Create();ASSERT_NE(bullet,nullptr);
 auto release=ra2::test::scope_exit([&]{bullet->Release();});
 bullet->Construct(&projectile,&target,tank,100,nullptr,20,false);bullet->SetWeaponType(&weapon);
 bullet->Location={10368,10368,1};bullet->Velocity={10,0,-20};
 game::projectile_log_event(*bullet,"launched");
 bullet->PointerExpired(tank,true);delete tank;ASSERT_EQ(bullet->Owner,nullptr);
 game::projectile_log_contact(*bullet,10378,10368,-19,0,8,false,false,false);
 const auto read=[](const std::filesystem::path& p){std::ifstream in(p);return std::string(std::istreambuf_iterator<char>(in),{});};
 // Read while the writer is open: the record must be durable to process exit
 // without relying on a shutdown callback or a post-crash signal handler.
 auto contents=read(path);const auto start=contents.rfind("event=physics_contact");ASSERT_NE(start,std::string::npos);
 const auto contact=contents.substr(start);
 EXPECT_NE(contact.find("source_type=MGTK"),std::string::npos);
 EXPECT_NE(contact.find("source_id="+std::to_string(sourceId)),std::string::npos);
 EXPECT_NE(contact.find("weapon=MirageGun"),std::string::npos);
 EXPECT_NE(contact.find("initial_target_type=MTNK"),std::string::npos);
 EXPECT_NE(contact.find("owner_id=-1"),std::string::npos);
 EXPECT_NE(contact.find("ramp_original_va=0xB45308"),std::string::npos);
 EXPECT_NE(contents.find("event=owner_removed"),std::string::npos);
 for(int i=0;i<30000;++i)game::projectile_log_event(*bullet,"update_enter");
 const auto previous=std::filesystem::path(path.string()+".previous");ASSERT_TRUE(std::filesystem::exists(previous));
 EXPECT_LE(std::filesystem::file_size(path),8u*1024u*1024u);EXPECT_LE(std::filesystem::file_size(previous),8u*1024u*1024u);
 contents=read(path);EXPECT_NE(contents.find("source_type=MGTK"),std::string::npos);
 ASSERT_TRUE(game::configure_projectile_log(nullptr));const auto size=std::filesystem::file_size(path);
 game::projectile_log_event(*bullet,"disabled");EXPECT_EQ(std::filesystem::file_size(path),size);
 EXPECT_FALSE(game::configure_projectile_log((path/"cannot-open.log").string().c_str()));
 game::projectile_log_event(*bullet,"failed_sink");
}
