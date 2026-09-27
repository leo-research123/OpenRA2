#include "support/test_support.hpp"
#include "api/rules_runtime.hpp"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/ParticleSystemTypeClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/CRC.h"
#include <bit>
#include <cfenv>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace {
struct RuleScope {
    RulesClass rules;
    RulesClass* previous=RulesClass::Instance;
    int rounding=std::fegetround();
    RuleScope() { RulesClass::Instance=&rules; }
    ~RuleScope() { RulesClass::Instance=previous; std::fesetround(rounding); }
};
constexpr bool WeaponTypeClass::* boolean_fields[]{
    &WeaponTypeClass::UseFireParticles, &WeaponTypeClass::UseSparkParticles,
    &WeaponTypeClass::OmniFire, &WeaponTypeClass::DistributedWeaponFire,
    &WeaponTypeClass::IsRailgun, &WeaponTypeClass::Lobber, &WeaponTypeClass::Bright,
    &WeaponTypeClass::IsSonic, &WeaponTypeClass::Spawner, &WeaponTypeClass::LimboLaunch,
    &WeaponTypeClass::DecloakToFire, &WeaponTypeClass::CellRangefinding,
    &WeaponTypeClass::FireOnce, &WeaponTypeClass::NeverUse, &WeaponTypeClass::RevealOnFire,
    &WeaponTypeClass::TerrainFire, &WeaponTypeClass::SabotageCursor, &WeaponTypeClass::MigAttackCursor,
    &WeaponTypeClass::DisguiseFireOnly, &WeaponTypeClass::InfiniteMindControl,
    &WeaponTypeClass::FireWhileMoving, &WeaponTypeClass::DrainWeapon, &WeaponTypeClass::FireInTransport,
    &WeaponTypeClass::Suicide, &WeaponTypeClass::TurboBoost, &WeaponTypeClass::Supress,
    &WeaponTypeClass::Camera, &WeaponTypeClass::Charges, &WeaponTypeClass::IsLaser,
    &WeaponTypeClass::DiskLaser, &WeaponTypeClass::IsLine, &WeaponTypeClass::IsBigLaser,
    &WeaponTypeClass::IsHouseColor, &WeaponTypeClass::IonSensitive, &WeaponTypeClass::AreaFire,
    &WeaponTypeClass::IsElectricBolt, &WeaponTypeClass::DrawBoltAsLaser, &WeaponTypeClass::IsAlternateColor,
    &WeaponTypeClass::IsRadBeam, &WeaponTypeClass::IsRadEruption, &WeaponTypeClass::IsMagBeam
};
constexpr const char* boolean_keys[]{
    "UseFireParticles","UseSparkParticles","OmniFire","DistributedWeaponFire","IsRailgun","Lobber","Bright",
    "IsSonic","Spawner","LimboLaunch","DecloakToFire","CellRangefinding","FireOnce","NeverUse","RevealOnFire",
    "TerrainFire","SabotageCursor","MigAttackCursor","DisguiseFireOnly","InfiniteMindControl","FireWhileMoving",
    "DrainWeapon","FireInTransport","Suicide","TurboBoost","Supress","Camera","Charges","IsLaser","DiskLaser",
    "IsLine","IsBigLaser","IsHouseColor","IonSensitive","AreaFire","IsElectricBolt","DrawBoltAsLaser",
    "IsAlternateColor","IsRadBeam","IsRadEruption","IsMagBeam"
};
static_assert(std::size(boolean_fields)==std::size(boolean_keys));
}

TEST(WeaponType, AllScalarINIFieldsAndOverlay) {
    WeaponTypeClass weapon("P2_WEAPON"); CCINIClass ini;
    EXPECT_FALSE(weapon.LoadFromINI(nullptr)); EXPECT_FALSE(weapon.LoadFromINI(&ini));
    for (const bool value : {true,false}) {
        for (const auto* key : boolean_keys) ini.WriteString(weapon.ID,key,value?"yes":"no");
        ini.WriteString(weapon.ID,"Name","Ignored by weapon reader");
        ini.WriteString(weapon.ID,"AmbientDamage","73"); ini.WriteString(weapon.ID,"Burst","0");
        ini.WriteString(weapon.ID,"Damage","-10"); ini.WriteString(weapon.ID,"ROF","11");
        ini.WriteString(weapon.ID,"Speed","50"); ini.WriteString(weapon.ID,"Range","2.5");
        ini.WriteString(weapon.ID,"MinimumRange","0.25"); ini.WriteString(weapon.ID,"DisguiseFakeBlinkTime","23");
        ini.WriteString(weapon.ID,"RadLevel","33"); ini.WriteString(weapon.ID,"LaserDuration","255");
        ini.WriteString(weapon.ID,"LaserInnerColor","-1,256,511");
        ini.WriteString(weapon.ID,"LaserOuterColor","1,2,3");
        ini.WriteString(weapon.ID,"LaserOuterSpread","4,5,6");
        ASSERT_TRUE(weapon.LoadFromINI(&ini));
        for (std::size_t i=0;i<std::size(boolean_fields);++i)
            EXPECT_EQ(weapon.*boolean_fields[i],value) << boolean_keys[i];
        EXPECT_STREQ(weapon.Name,"P2_WEAPON");
        EXPECT_EQ(weapon.AmbientDamage,73); EXPECT_EQ(weapon.Burst,0); EXPECT_EQ(weapon.Damage,-10);
        EXPECT_EQ(weapon.Speed,128); EXPECT_EQ(weapon.ROF,11); EXPECT_EQ(weapon.Range,640);
        EXPECT_EQ(weapon.MinimumRange,64); EXPECT_EQ(weapon.DisguiseFakeBlinkTime,23); EXPECT_EQ(weapon.RadLevel,33);
        EXPECT_EQ(static_cast<unsigned char>(weapon.LaserDuration),255);
        EXPECT_EQ(weapon.LaserInnerColor.R,255); EXPECT_EQ(weapon.LaserInnerColor.G,0); EXPECT_EQ(weapon.LaserInnerColor.B,255);
        EXPECT_EQ(weapon.LaserOuterColor.G,2); EXPECT_EQ(weapon.LaserOuterSpread.B,6);
    }
    CCINIClass overlay;
    overlay.WriteString(weapon.ID,"Burst","-3"); overlay.WriteString(weapon.ID,"Speed","-1");
    overlay.WriteString(weapon.ID,"Range","-1");
    ASSERT_TRUE(weapon.LoadFromINI(&overlay));
    EXPECT_EQ(weapon.Burst,-3); EXPECT_EQ(weapon.Speed,128); EXPECT_EQ(weapon.Range,640);
    EXPECT_EQ(weapon.ROF,11); EXPECT_EQ(static_cast<signed char>(weapon.LaserDuration),-1);
}

TEST(WeaponType, ReferenceLookupsAndLists) {
    WeaponTypeClass weapon("P2_REFS"); CCINIClass ini;
    AnimTypeClass a("P2_A"), b("P2_B"); BulletTypeClass projectile("P2_BULLET");
    WarheadTypeClass warhead("P2_WARHEAD"); ParticleSystemTypeClass particle("P2_PARTICLE");
    ini.WriteString(weapon.ID,"Anim","P2_A,P2_B,P2_A,NONE");
    ini.WriteString(weapon.ID,"AssaultAnim","P2_B"); ini.WriteString(weapon.ID,"OccupantAnim","P2_A");
    ini.WriteString(weapon.ID,"OpenToppedAnim","P2_B"); ini.WriteString(weapon.ID,"Projectile","P2_BULLET");
    ini.WriteString(weapon.ID,"Warhead","P2_WARHEAD"); ini.WriteString(weapon.ID,"AttachedParticleSystem","P2_PARTICLE");
    ini.WriteString(weapon.ID,"Report","A,missing,A,B"); ini.WriteString(weapon.ID,"DownReport","B");
    auto services=game::native_rules_runtime();
    services.sound_list_entry=[](void*,const char* name,bool& exists,int& index) {
        exists=std::strcmp(name,"missing")!=0; index=std::strcmp(name,"A")==0?5:-1; return true;
    };
    struct Call { WeaponTypeClass* weapon; CCINIClass* ini; } call{&weapon,&ini};
    game::with_rules_runtime(services,[](void* context) {
        auto& call=*static_cast<Call*>(context); EXPECT_TRUE(call.weapon->LoadFromINI(call.ini));
    },&call);
    ASSERT_EQ(weapon.Anim.Count,3); EXPECT_EQ(weapon.Anim[0],&a); EXPECT_EQ(weapon.Anim[1],&b); EXPECT_EQ(weapon.Anim[2],&a);
    ASSERT_EQ(weapon.Report.Count,3); EXPECT_EQ(weapon.Report[0],5); EXPECT_EQ(weapon.Report[1],5); EXPECT_EQ(weapon.Report[2],-1);
    ASSERT_EQ(weapon.DownReport.Count,1); EXPECT_EQ(weapon.DownReport[0],-1);
    EXPECT_EQ(weapon.AssaultAnim,&b); EXPECT_EQ(weapon.OccupantAnim,&a); EXPECT_EQ(weapon.OpenToppedAnim,&b);
    EXPECT_EQ(weapon.Projectile,&projectile); EXPECT_EQ(weapon.Warhead,&warhead); EXPECT_EQ(weapon.AttachedParticleSystem,&particle);
    const int count=ParticleSystemTypeClass::Array.Count;
    CCINIClass overlay;
    overlay.WriteString(weapon.ID,"Projectile","NONE"); overlay.WriteString(weapon.ID,"Warhead","<none>");
    overlay.WriteString(weapon.ID,"Anim","NONE"); overlay.WriteString(weapon.ID,"AssaultAnim","");
    overlay.WriteString(weapon.ID,"AttachedParticleSystem","<none>");
    ASSERT_TRUE(weapon.LoadFromINI(&overlay));
    EXPECT_EQ(weapon.Projectile,nullptr); EXPECT_EQ(weapon.Warhead,nullptr); EXPECT_EQ(weapon.Anim.Count,0);
    EXPECT_EQ(weapon.AssaultAnim,&b); EXPECT_EQ(weapon.AttachedParticleSystem,&particle);
    EXPECT_EQ(ParticleSystemTypeClass::Array.Count,count); EXPECT_EQ(weapon.Report.Count,3);
    overlay.WriteString(weapon.ID,"AttachedParticleSystem","P2_NEW_PARTICLE_LONG_NAME");
    ASSERT_TRUE(weapon.LoadFromINI(&overlay));
    auto* added=weapon.AttachedParticleSystem;
    ASSERT_NE(added,nullptr); EXPECT_NE(added,&particle);
    EXPECT_STREQ(added->ID,"P2_NEW_PARTICLE_LON");
    EXPECT_EQ(ParticleSystemTypeClass::Array.Count,count+1);
    weapon.AttachedParticleSystem=nullptr; GameDelete(added);
    overlay.WriteString(weapon.ID,"AttachedParticleSystem","none");
    ASSERT_TRUE(weapon.LoadFromINI(&overlay));
    added=weapon.AttachedParticleSystem; ASSERT_NE(added,nullptr);
    EXPECT_STREQ(added->ID,"none");
    weapon.AttachedParticleSystem=nullptr; GameDelete(added);
    EXPECT_EQ(ParticleSystemTypeClass::Array.Count,count);
}

TEST(WeaponType, FreshReferencesUseTheRealTypeFactories) {
    WeaponTypeClass weapon("P2_FRESH"); CCINIClass ini;
    ASSERT_EQ(BulletTypeClass::Find("P2_NEW_PROJECTILE"),nullptr);
    ASSERT_EQ(WarheadTypeClass::Find("P2_NEW_WARHEAD"),nullptr);
    const int bullets=BulletTypeClass::Array.Count,warheads=WarheadTypeClass::Array.Count;
    ini.WriteString(weapon.ID,"Projectile","P2_NEW_PROJECTILE");
    ini.WriteString(weapon.ID,"Warhead","P2_NEW_WARHEAD");
    ASSERT_TRUE(weapon.LoadFromINI(&ini));
    auto* bullet=weapon.Projectile; auto* warhead=weapon.Warhead;
    ASSERT_NE(bullet,nullptr); ASSERT_NE(warhead,nullptr);
    EXPECT_EQ(BulletTypeClass::Find("p2_new_projectile"),bullet);
    EXPECT_EQ(WarheadTypeClass::Find("p2_new_warhead"),warhead);
    ASSERT_TRUE(weapon.LoadFromINI(&ini));
    EXPECT_EQ(weapon.Projectile,bullet); EXPECT_EQ(weapon.Warhead,warhead);
    EXPECT_EQ(BulletTypeClass::Array.Count,bullets+1); EXPECT_EQ(WarheadTypeClass::Array.Count,warheads+1);
    weapon.Projectile=nullptr; weapon.Warhead=nullptr;
    GameDelete(bullet); GameDelete(warhead);
    EXPECT_EQ(BulletTypeClass::Array.Count,bullets); EXPECT_EQ(WarheadTypeClass::Array.Count,warheads);
}

TEST(WeaponType, SpeedInputBoundariesAndBorrowedListFallback) {
    WeaponTypeClass weapon("P2_LIMITS"); CCINIClass ini;
    const int saved_rounding=std::fegetround();
    struct Restore { int rounding; ~Restore(){std::fesetround(rounding);} } restore{saved_rounding};
    struct Row { const char* text; int expected; } rows[]{
        {"-100",0},{"-1",77},{"0",0},{"1",2},{"99",253},{"100",255},{"900",255},{"bad",0}};
    for(const auto& row:rows) {
        weapon.Speed=77; ini.WriteString(weapon.ID,"Speed",row.text);
        ASSERT_TRUE(weapon.LoadFromINI(&ini)); EXPECT_EQ(weapon.Speed,row.expected) << row.text;
    }
    int reports[]{3,7};
    weapon.Report.SetCapacity(2,reports); weapon.Report.Count=2;
    ASSERT_TRUE(weapon.LoadFromINI(&ini));
    EXPECT_TRUE(weapon.Report.IsAllocated); EXPECT_NE(weapon.Report.Items,reports);
    ASSERT_EQ(weapon.Report.Count,2); EXPECT_EQ(weapon.Report[0],3); EXPECT_EQ(weapon.Report[1],7);
}

TEST(WeaponType, DependenciesFailWithoutThrowingOrPretendingSuccess) {
    WeaponTypeClass weapon("P2_FAIL"); CCINIClass ini;
    ini.WriteString(weapon.ID,"Damage","57"); ini.WriteString(weapon.ID,"Report","Sound");
    auto services=game::native_rules_runtime();
    services.sound_list_entry=[](void*,const char*,bool&,int&) -> bool { throw 42; };
    struct Call { WeaponTypeClass* weapon; CCINIClass* ini; } call{&weapon,&ini};
    game::with_rules_runtime(services,[](void* context) {
        auto& call=*static_cast<Call*>(context); EXPECT_FALSE(call.weapon->LoadFromINI(call.ini));
    },&call);
    EXPECT_EQ(weapon.Damage,57); // sequential original reader, not an invented transaction
    EXPECT_EQ(weapon.Report.Count,0);
}

TEST(WeaponType, RegistryAndNativeFinalization) {
    RuleScope scope; scope.rules.Gravity=6;
    const int count=WeaponTypeClass::Array.Count;
    {
        WeaponTypeClass weapon("P2_REGISTER"); BulletTypeClass bullet("P2_PARAMETER");
        EXPECT_EQ(WeaponTypeClass::FindOrAllocate("p2_register"),&weapon);
        EXPECT_EQ(WeaponTypeClass::Array.Count,count+1);
        EXPECT_EQ(WeaponTypeClass::FindOrAllocate("NONE"),nullptr);
        auto& runtime=game::native_rules_runtime();
        ASSERT_NE(runtime.finalize_weapon,nullptr);
        EXPECT_FALSE(runtime.finalize_weapon(nullptr,&bullet));
        weapon.Speed=77; EXPECT_TRUE(runtime.finalize_weapon(nullptr,&weapon)); EXPECT_EQ(weapon.Speed,77);
        weapon.Projectile=&bullet; bullet.ROT=1;
        EXPECT_TRUE(runtime.finalize_weapon(nullptr,&weapon)); EXPECT_EQ(weapon.Speed,77);
        bullet.ROT=0; weapon.Range=256;
        EXPECT_TRUE(runtime.finalize_weapon(nullptr,&weapon)); EXPECT_EQ(weapon.Speed,42);
    }
    EXPECT_EQ(WeaponTypeClass::Array.Count,count); EXPECT_EQ(WeaponTypeClass::Find("P2_REGISTER"),nullptr);
}

TEST(WeaponType, AllowedThreatsRequiresAProjectile) {
    WeaponTypeClass weapon("P2_THREAT");
    EXPECT_DEATH(weapon.AllowedThreats(),"");
}

TEST(WeaponType, OriginalParameterAndCRCCorpus) {
    RuleScope scope;
    WeaponTypeClass weapon("P2_CORPUS"); BulletTypeClass bullet("P2_CORPUS_B"); WarheadTypeClass warhead("P2_CORPUS_W");
    std::ifstream input(RA2_WEAPON_FIXTURE); ASSERT_TRUE(input.good());
    unsigned speed_cases=0,crc_cases=0,threat_cases=0; char kind;
    while(input>>kind) {
        if (kind=='S') {
            int gravity,range,floater,rot,present,expected;
            ASSERT_TRUE(bool(input>>gravity>>range>>floater>>rot>>present>>expected));
            scope.rules.Gravity=gravity; weapon.Range=range; bullet.Floater=floater!=0; bullet.ROT=rot;
            weapon.Projectile=present?&bullet:nullptr; weapon.Speed=71;
            std::fesetround(FE_TONEAREST); weapon.CalculateSpeed(); EXPECT_EQ(weapon.Speed,expected) << "S " << speed_cases;
            ++speed_cases;
        } else if (kind=='T') {
            int aa,ag; unsigned expected; ASSERT_TRUE(bool(input>>aa>>ag>>expected));
            weapon.Projectile=&bullet; bullet.AA=aa!=0; bullet.AG=ag!=0;
            EXPECT_EQ(static_cast<unsigned>(weapon.AllowedThreats()),expected); ++threat_cases;
        } else {
            ASSERT_EQ(kind,'C');
            unsigned id,projectile_id,warhead_id,mask,duration,expected_crc,expected_staging,expected_value;
            int report_count,down_count,anim_count,expected_index;
            ASSERT_TRUE(bool(input>>id>>projectile_id>>warhead_id>>weapon.AmbientDamage>>weapon.Burst>>weapon.Damage
                >>weapon.Speed>>weapon.ROF>>weapon.Range>>report_count>>down_count>>anim_count>>mask>>duration
                >>expected_crc>>expected_index>>expected_staging>>expected_value));
            weapon.UniqueID=id; weapon.Dirty=(mask&(1u<<15))!=0;
            weapon.Projectile=projectile_id?&bullet:nullptr; bullet.UniqueID=projectile_id;
            weapon.Warhead=warhead_id?&warhead:nullptr; warhead.UniqueID=warhead_id;
            // CRC reads counts, not elements. These are scoped receiver fixtures.
            weapon.Report.Count=report_count; weapon.DownReport.Count=down_count; weapon.Anim.Count=anim_count;
            constexpr bool WeaponTypeClass::* flags[]{&WeaponTypeClass::IsSonic,&WeaponTypeClass::TurboBoost,
                &WeaponTypeClass::Supress,&WeaponTypeClass::Camera,&WeaponTypeClass::IsLaser,&WeaponTypeClass::IsHouseColor,
                &WeaponTypeClass::UseFireParticles,&WeaponTypeClass::Lobber,&WeaponTypeClass::IsBigLaser,
                &WeaponTypeClass::IsRailgun,&WeaponTypeClass::Charges,&WeaponTypeClass::Bright,
                &WeaponTypeClass::UseSparkParticles,&WeaponTypeClass::OmniFire,&WeaponTypeClass::DistributedWeaponFire};
            for(unsigned i=0;i<std::size(flags);++i) weapon.*flags[i]=(mask&(1u<<i))!=0;
            weapon.LaserDuration=static_cast<char>(duration);
            CRCEngine crc; weapon.ComputeCRC(crc);
            EXPECT_EQ(std::uint32_t(crc.CRC),expected_crc); EXPECT_EQ(crc.Index,expected_index);
            EXPECT_EQ(std::uint32_t(crc.StagingBuffer.Composite),expected_staging);
            EXPECT_EQ(std::uint32_t(crc()),expected_value);
            weapon.Report.Count=weapon.DownReport.Count=weapon.Anim.Count=0;
            ++crc_cases;
        }
    }
    EXPECT_EQ(speed_cases,180u); EXPECT_EQ(threat_cases,4u); EXPECT_EQ(crc_cases,64u);
}
