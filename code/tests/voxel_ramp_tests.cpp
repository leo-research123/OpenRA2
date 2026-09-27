#include "support/test_support.hpp"
#include "yrpp/Matrix3D.h"
#include "yrpp/BulletClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/HouseTypeClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/BounceClass.h"
#include "yrpp/DriveLocomotionClass.h"
#include "voxel_ramp.hpp"
#include <cmath>
#include <fstream>
#include <string>

TEST(VoxelRamp, AllMatricesMatchOriginalInitializer) {
    std::ifstream input(RA2_VOXEL_RAMP_FIXTURE);ASSERT_TRUE(input.good());
    std::string version;int count=0;input>>version>>count;
    ASSERT_EQ(version,"VOXEL_RAMP_V1");ASSERT_EQ(count,21);
    static_assert(sizeof(Matrix3D)==0x30);
    for(int ramp=0;ramp<count;++ramp)for(int row=0;row<3;++row)for(int column=0;column<4;++column){
        SCOPED_TRACE(::testing::Message()<<ramp<<' '<<row<<' '<<column);
        float expected=0;ASSERT_TRUE(bool(input>>expected));
        EXPECT_EQ(Matrix3D::VoxelRampMatrix[ramp].row[row][column],expected);
    }
}

TEST(VoxelRamp, OriginalTransitionsShadowsBodiesAndBounceCorpus) {
    RulesClass rules;ScenarioClass scenario;
    auto* oldRules=RulesClass::Instance;auto* oldScenario=ScenarioClass::Instance;const auto oldFrame=Unsorted::CurrentFrame;
    RulesClass::Instance=&rules;ScenarioClass::Instance=&scenario;
    auto& map=MapClass::Instance;
    auto restore=ra2::test::scope_exit([&]{map.ReleaseCellStorage();RulesClass::Instance=oldRules;ScenarioClass::Instance=oldScenario;Unsorted::CurrentFrame=oldFrame;});
    ASSERT_TRUE(map.CreateEmptyCells({0,0,64,64},0));
    HouseTypeClass country("RAMP_CORPUS");HouseClass house(&country);UnitTypeClass type("RAMP_CORPUS_UNIT");
    type.VoxelScaleX=12;type.VoxelScaleY=8;UnitClass actor(&type,&house);DriveLocomotionClass driver;
    ASSERT_GE(driver.Link_To_Object(&actor),0);actor.Location={10368,10368,0};
    auto* cell=map.GetCellAt(actor.Location);
    std::ifstream input(RA2_VOXEL_RAMP_USAGE_FIXTURE);ASSERT_TRUE(input.good());std::string version;input>>version;
    ASSERT_EQ(version,"VOXEL_RAMP_USAGE_V1");char kind;int qCount=0,tCount=0,rCount=0,lCount=0,hCount=0,dCount=0,aCount=0,cCount=0,sCount=0;
    while(input>>kind){
        if(kind=='A'){
            Vector3D<float> axis;float angle;ASSERT_TRUE(bool(input>>axis.X>>axis.Y>>axis.Z>>angle));
            SCOPED_TRACE(::testing::Message()<<kind<<' '<<aCount);
            Quaternion actual;Quaternion::FromAxis(&actual,axis,angle);
            for(int i=0;i<4;++i){float expected;ASSERT_TRUE(bool(input>>expected));EXPECT_EQ(actual[i],expected);}++aCount;
        }else if(kind=='C'){
            BounceClass bounce;auto& q=bounce.CurrentAngle;ASSERT_TRUE(bool(input>>q.X>>q.Y>>q.Z>>q.W));
            SCOPED_TRACE(::testing::Message()<<kind<<' '<<cCount);
            const auto actual=bounce.GetDrawingMatrix();
            for(int i=0;i<12;++i){float expected;ASSERT_TRUE(bool(input>>expected));EXPECT_EQ(actual.Data[i],expected);}++cCount;
        }else if(kind=='S'){
            Quaternion a,b,actual;float time;ASSERT_TRUE(bool(input>>a.X>>a.Y>>a.Z>>a.W>>b.X>>b.Y>>b.Z>>b.W>>time));
            SCOPED_TRACE(::testing::Message()<<kind<<' '<<sCount);
            Quaternion::Slerp(&actual,a,b,time);
            for(int i=0;i<4;++i){float expected;ASSERT_TRUE(bool(input>>expected));EXPECT_EQ(actual[i],expected);}++sCount;
        }else if(kind=='Q'){
            int ramp;input>>ramp;SCOPED_TRACE(::testing::Message()<<kind<<' '<<ramp);
            for(int i=0;i<4;++i){float expected;ASSERT_TRUE(bool(input>>expected));EXPECT_EQ(Quaternion::VoxelRampQuaternion[ramp][i],expected);}++qCount;
        }else if(kind=='T'){
            int previous,current;double ratio;input>>previous>>current>>ratio;
            SCOPED_TRACE(::testing::Message()<<kind<<' '<<previous<<' '<<current<<' '<<ratio);
            const auto actual=game::voxel_ramp_matrix(previous,current,ratio);
            for(int i=0;i<12;++i){float expected;ASSERT_TRUE(bool(input>>expected));EXPECT_EQ(actual.Data[i],expected);}++tCount;
        }else if(kind=='R'){
            int ramp;float x,y,z;double elasticity;input>>ramp>>x>>y>>z>>elasticity;
            SCOPED_TRACE(::testing::Message()<<kind<<' '<<ramp<<' '<<x<<' '<<y<<' '<<z<<' '<<elasticity);
            cell->SlopeIndex=static_cast<BYTE>(ramp);BounceClass bounce;
            // Guarantee ground contact within the same cell, independently
            // of the slope's height at the vector's destination.
            const int floor=map.GetCellFloorHeight({int(10368+x),int(10368+y),0});
            bounce.Coords={10368,10368,float(floor-1)};bounce.Velocity={x,y,z+6};
            bounce.Gravity=6;bounce.Elasticity=elasticity;
            EXPECT_NE(bounce.Update(),BounceClass::Status::None);
            float vx,vy,vz;ASSERT_TRUE(bool(input>>vx>>vy>>vz));
            EXPECT_EQ(bounce.Velocity.X,vx);EXPECT_EQ(bounce.Velocity.Y,vy);EXPECT_EQ(bounce.Velocity.Z,vz);++rCount;
        }else if(kind=='L'||kind=='H'||kind=='D'){
            int previous,current,phase,facing,expectedKey;input>>previous>>current>>phase>>facing>>expectedKey;
            SCOPED_TRACE(::testing::Message()<<kind<<' '<<previous<<' '<<current<<' '<<phase<<' '<<facing);
            cell->SlopeIndex=static_cast<BYTE>(current);Unsorted::CurrentFrame=phase;
            driver.PreviousRamp=previous;driver.CurrentRamp=current;driver.SlopeTimer.StartTime=0;
            driver.SlopeTimer.TimeLeft=driver.SlopeTimer.Rate=phase<4?3:0;
            actor.PrimaryFacing.SetCurrent(DirStruct(static_cast<unsigned short>(facing*2048)));
            VoxelIndexKey key(2);Matrix3D actual;
            if(kind=='L'){actual=driver.LocomotionClass::Shadow_Matrix(&key);++lCount;}
            else if(kind=='H'){actual=driver.Shadow_Matrix(&key);++hCount;}
            else {actual=driver.Draw_Matrix(&key);++dCount;}
            EXPECT_EQ(key.Value,expectedKey);
            for(int i=0;i<12;++i){float expected;ASSERT_TRUE(bool(input>>expected));EXPECT_NEAR(actual.Data[i],expected,0.000002);}
        }else FAIL()<<"Unknown fixture row "<<kind;
    }
    EXPECT_EQ(qCount,21);EXPECT_EQ(tCount,3087);EXPECT_EQ(rCount,252);
    EXPECT_EQ(lCount,525);EXPECT_EQ(hCount,525);EXPECT_EQ(dCount,525);
    EXPECT_EQ(aCount,30);EXPECT_EQ(cCount,30);EXPECT_EQ(sCount,20);
}

TEST(VoxelRamp, OrphanedProjectileTouchesEverySlopeAndIsRetired) {
    RulesClass rules;ScenarioClass scenario;
    auto* oldRules=RulesClass::Instance;auto* oldScenario=ScenarioClass::Instance;
    RulesClass::Instance=&rules;ScenarioClass::Instance=&scenario;
    auto& map=MapClass::Instance;
    auto restore=ra2::test::scope_exit([&]{map.ReleaseCellStorage();RulesClass::Instance=oldRules;ScenarioClass::Instance=oldScenario;});
    ASSERT_TRUE(map.CreateEmptyCells({0,0,64,64},0));
    const auto oldVisible=map.VisibleRect;map.VisibleRect=map.MapRect;
    auto restoreVisible=ra2::test::scope_exit([&]{map.VisibleRect=oldVisible;});
    HouseTypeClass country("RAMP_COUNTRY");HouseClass house(&country);UnitTypeClass unitType("RAMP_FIRER");
    BulletTypeClass type("RAMP_PROJECTILE");WarheadTypeClass warhead("RAMP_WARHEAD");
    type.ROT=0;type.Vertical=false;type.Dropping=false;type.Trailer=nullptr;
    type.Elasticity=0.75;type.Cluster=0;rules.Gravity=6;
    // No secondary explosion damage here: this test isolates real Update,
    // collision/reflection and UnInit after the normal expiration notification.
    auto* cell=map.GetCellAt(CellStruct{40,40});
    for(int slope=0;slope<21;++slope){
        SCOPED_TRACE(slope);cell->SlopeIndex=static_cast<BYTE>(slope);
        auto* owner=new UnitClass(&unitType,&house);
        auto* bullet=BulletClass::Create();ASSERT_NE(bullet,nullptr);
        auto release=ra2::test::scope_exit([&]{ObjectClass::PendingDeletes.Remove(bullet);bullet->Release();});
        bullet->Construct(&type,nullptr,owner,10,&warhead,20,false);
        bullet->PointerExpired(owner,true);delete owner;ASSERT_EQ(bullet->Owner,nullptr);
        bullet->IsAlive=true;bullet->InLimbo=true;bullet->IsFallingDown=false;
        bullet->Location={10368,10368,0};bullet->Location.Z=map.GetCellFloorHeight(bullet->Location)+1;
        bullet->Velocity={10,0,-20};
        bullet->Update();
        EXPECT_FALSE(bullet->IsAlive);EXPECT_GE(ObjectClass::PendingDeletes.FindItemIndex(bullet),0);
        EXPECT_TRUE(std::isfinite(bullet->Velocity.X));EXPECT_TRUE(std::isfinite(bullet->Velocity.Y));
        EXPECT_TRUE(std::isfinite(bullet->Velocity.Z));
        if(slope==0){EXPECT_EQ(bullet->Velocity.X,7.5);EXPECT_EQ(bullet->Velocity.Y,0);EXPECT_EQ(bullet->Velocity.Z,19.5);}
    }
}
