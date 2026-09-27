#include "support/test_support.hpp"
// Original-x86 reference contains synthetic states, not commercial assets.
#include "building_voxel.hpp"
#include "yrpp/BuildingClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/FlyLocomotionClass.h"
#include "yrpp/DriveLocomotionClass.h"
#include "yrpp/Memory.h"
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void resource(VoxelStruct& resource,int frames) {
    resource.VXL=GameCreate<VoxLib>(nullptr,false);
    resource.VXL->Initialized=false;resource.VXL->CountHeaders=1;
    resource.VXL->BodyData=static_cast<std::uint8_t*>(YRMemory::Allocate(1));
    resource.HVA=GameCreate<MotLib>(nullptr);
    resource.HVA->LoadedFailed=false;resource.HVA->LayerCount=1;resource.HVA->FrameCount=frames;
    resource.HVA->Matrixes=static_cast<Matrix3D*>(YRMemory::Allocate(sizeof(Matrix3D)*frames));
    if(!resource.VXL->BodyData||!resource.HVA->Matrixes)throw std::bad_alloc();
}
float reference_float(const std::string& text,unsigned index) {
    std::uint32_t word=0;
    for(unsigned byte=0;byte<4;++byte)
        word|=std::uint32_t(std::stoul(text.substr(index*8+byte*2,2),nullptr,16))<<(byte*8);
    return std::bit_cast<float>(word);
}
}

TEST(BuildingVoxelTransform, OriginalMatrixBits) {
    const auto [argc, argv] = ra2::test::arguments();
    ra2::test::ContinueAfterFailure collect_rows;

        const char* path=argc==2?argv[1]:RA2_VOXEL_TRANSFORM_REFERENCE;
        std::ifstream input(path);std::string format;unsigned count=0;
        if(!(input>>format>>count)||format!="BUILDING_VOXEL_TRANSFORMS_V1")throw std::runtime_error("Invalid original fixture");
        for(unsigned i=0;i<count;++i) {
            std::string name;int mode,facing,pitch,motion,mission,animation,spotlight,expected_count;
            input>>name>>mode>>facing>>pitch>>motion>>mission>>animation>>spotlight>>expected_count;
            BuildingTypeClass type("VOXEL_TEST",BuildingTypeClass::ConstructionDefaults{});BuildingClass building(&type,nullptr);
            type.Turret=true;type.TurretAnimIsVoxel=mode!=2;type.BarrelAnimIsVoxel=true;
            type.TurretOffset=15;type.BuildingAnimFrame[0].dwUnknown=0;type.BuildingAnimFrame[0].FrameCount=8;
            type.VoxelBarrelScale=mode==2&&motion?1.5:1;
            type.VoxelBarrelOffsetToBuildingPivotPoint={1,2,3};type.VoxelBarrelOffsetToRotatePivotPoint={4,5,6};
            type.VoxelBarrelOffsetToPitchPivotPoint={7,8,9};type.HasSpotlight=spotlight;
            if(mode==0)resource(type.TurretVoxel,3);resource(type.BarrelVoxel,4);
            DirStruct direction;direction.Raw=facing;building.PrimaryFacing.SetCurrent(direction);
            direction.Raw=pitch;building.BarrelFacing.SetCurrent(direction);
            building.TurretRecoil.State=mode==0&&(motion==1||motion==3)?RecoilData::RecoilState::Compressing:RecoilData::RecoilState::Inactive;
            building.BarrelRecoil.State=mode==0&&(motion==2||motion==3)?RecoilData::RecoilState::Compressing:RecoilData::RecoilState::Inactive;
            building.TurretRecoil.TravelSoFar=2.25f;building.BarrelRecoil.TravelSoFar=4.5f;
            building.CurrentMission=static_cast<Mission>(mission);building.QueuedMission=static_cast<Mission>(0);
            building.Animation.Value=animation;building.TurretAnimFrame=5;
            game::BuildingVoxelPlan plan;game::building_voxel_plan(building,plan);
            bool different=plan.count!=unsigned(expected_count);unsigned words=0;
            for(int p=0;p<expected_count;++p) {
                int barrel,frame;std::string matrix;input>>barrel>>frame>>matrix;
                if(matrix.size()!=96)throw std::runtime_error("Invalid matrix in "+name);
                if(unsigned(p)>=plan.count)continue;
                const auto& part=plan.parts[p];different|=part.barrel!=bool(barrel)||part.frame!=unsigned(frame);
                for(unsigned n=0;n<12;++n)if(part.local.Data[n]!=reference_float(matrix,n)) {
                    ++words;
                    if(words==1)std::cout<<"word "<<p<<','<<n<<": core=0x"<<std::hex<<std::bit_cast<std::uint32_t>(part.local.Data[n])
                        <<" original=0x"<<std::bit_cast<std::uint32_t>(reference_float(matrix,n))<<std::dec<<'\n';
                }
            }
            different|=words!=0;
            EXPECT_FALSE(different) << name << ": parts=" << plan.count << "/" << expected_count << ", matrix words=" << words;
            if(different)std::cout<<"FAIL "<<name<<": parts="<<plan.count<<'/'<<expected_count<<", matrix words="<<words<<'\n';
            if(!input)throw std::runtime_error("Truncated fixture");
        }
}

// The locomotor is the explicit boundary supplied to the original DrawAsVXL
// fixture. All body/turret/barrel composition below is production code.
TEST(UnitVoxelTransform, OriginalMatrixBitsFramesAndOrder) {
    struct FixtureDrive final : DriveLocomotionClass {
        Matrix3D input;
        int cache_key=-1;
        Matrix3D YRPP_STDCALL Draw_Matrix(VoxelIndexKey* key) override { if(key)key->Value=cache_key;return input; }
    } driver;
    UnitTypeClass type("UNIT_VXL_TEST");type.Turret=true;type.NoShadow=true;type.Voxel=true;
    resource(type.MainVoxel,4);resource(type.TurretVoxel,3);resource(type.BarrelVoxel,1);
    for(unsigned i=0;i<4;++i){resource(type.ChargerTurrets[i],3);resource(type.ChargerBarrels[i],1);}
    UnitClass unit(&type,nullptr);unit.Locomotor=&driver;
    std::ifstream input(RA2_UNIT_VOXEL_TRANSFORM_REFERENCE);std::string format;unsigned cases=0;
    input>>format>>cases;EXPECT_EQ(format,"UNIT_VOXEL_TRANSFORMS_V3");
    unsigned failures=0;
    for(unsigned i=0;i<cases;++i) {
        unsigned index,hull,turret,pitch,walk,anim,count;int offset,mode;std::string matrix;
        input>>index>>hull>>turret>>pitch>>offset>>walk>>anim>>driver.cache_key>>mode>>matrix>>count;
        if(!input||matrix.size()!=96)throw std::runtime_error("Truncated unit transform fixture");
        unit.PrimaryFacing.SetCurrent(DirStruct(int(hull)));unit.SecondaryFacing.SetCurrent(DirStruct(int(turret)));
        unit.BarrelFacing.SetCurrent(DirStruct(int(pitch)));type.TurretOffset=offset;
        unit.WalkedFramesSoFar=int(walk);unit.TurretAnimFrame=int(anim);
        type.TurretCount=mode<0?0:4;unit.CurrentTurretNumber=mode<0?0:mode;
        for(unsigned n=0;n<12;++n)driver.input.Data[n]=reference_float(matrix,n);
        game::BuildingVoxelPart parts[4];unsigned actual=0;
        const bool valid=game::unit_voxel_parts(unit,parts,4,actual);
        bool different=!valid||actual!=count;
        for(unsigned part=0;part<count;++part) {
            unsigned resourceOffset,frame;int cacheKey;input>>resourceOffset>>frame>>cacheKey>>matrix;
            if(part>=actual){different=true;continue;}
            const auto& result=parts[part];
            const auto* expected=resourceOffset==176?&type.MainVoxel:resourceOffset==184?&type.TurretVoxel:resourceOffset==192?&type.BarrelVoxel:
                resourceOffset<344?&type.ChargerTurrets[(resourceOffset-200)/8]:&type.ChargerBarrels[(resourceOffset-344)/8];
            different|=result.resource!=expected||result.frame!=frame||result.cache_key!=cacheKey;
            for(unsigned n=0;n<12;++n)if(std::bit_cast<std::uint32_t>(result.local.Data[n])!=std::bit_cast<std::uint32_t>(reference_float(matrix,n))) {
                if(failures<5)std::cout<<"UNIT_MATRIX "<<index<<" part="<<part<<" word="<<n<<" core=0x"<<std::hex<<std::bit_cast<std::uint32_t>(result.local.Data[n])<<" original=0x"<<std::bit_cast<std::uint32_t>(reference_float(matrix,n))<<std::dec<<"\n";
                different=true;
            }
        }
        if(different)++failures;
    }
    unit.Locomotor=nullptr;
    EXPECT_EQ(failures,0u);EXPECT_EQ(cases,513u);
}

TEST(AircraftVoxelTransform, OriginalFlyMatrixSpeedAndLayer) {
    struct FlatAircraft final : AircraftClass {
        int height=0;
        explicit FlatAircraft(AircraftTypeClass* type):AircraftClass(type,nullptr){}
        int GetHeight() const override {return height;}
    };
    AircraftTypeClass type("FLY_REFERENCE");type.Speed=20;
    FlatAircraft air(&type);FlyLocomotionClass driver;driver.Link_To_Object(&air);
    std::ifstream input(RA2_FLY_REFERENCE);std::string magic;unsigned cases=0;
    input>>magic>>cases;ASSERT_EQ(magic,"FLY_REFERENCE_V1");ASSERT_EQ(cases,768u);
    const int oldFrame=Unsorted::CurrentFrame;
    struct Restore {int value;~Restore(){Unsorted::CurrentFrame=value;}}restore{oldFrame};
    unsigned differences=0;
    for(unsigned i=0;i<cases;++i){
        int facing,turn,height,speed,layer;double fraction;unsigned expectedKey;std::string matrix;
        input>>facing>>turn>>height>>fraction>>expectedKey>>speed>>layer>>matrix;
        ASSERT_TRUE(input.good());ASSERT_EQ(matrix.size(),96u);
        air.height=height;driver.CurrentSpeed=fraction;Unsorted::CurrentFrame=100;
        air.SecondaryFacing=FacingClass{};air.SecondaryFacing.SetCurrent(DirStruct(facing));air.SecondaryFacing.SetROT(5);
        air.SecondaryFacing.SetDesired(DirStruct((facing+turn)&0xFFFF));Unsorted::CurrentFrame=102;
        VoxelIndexKey key(0);auto actual=driver.Draw_Matrix(&key);
        bool different=unsigned(key.Value)!=expectedKey||driver.Apparent_Speed()!=speed||int(driver.In_Which_Layer())!=layer;
        for(unsigned word=0;word<12;++word)if(std::bit_cast<std::uint32_t>(actual.Data[word])!=std::bit_cast<std::uint32_t>(reference_float(matrix,word))){
            if(differences<3)std::cout<<"FLY_MATRIX "<<i<<" word="<<word<<"\n";
            different=true;
        }
        if(different)++differences;
    }
    EXPECT_EQ(differences,0u);
}
