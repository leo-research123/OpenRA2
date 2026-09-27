#include "support/test_support.hpp"
#include "support/type_stream_fixture.hpp"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/ParticleSystemTypeClass.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <vector>
#include <limits>
#include <fstream>

namespace {
using Stream = ra2::test::MemoryStream<static_cast<HRESULT>(0x80004005u), true>;
struct Session : ra2::test::StreamSession<Stream> {
    Session() : StreamSession([] {
        auto t = StreamSession::transport(); t.max_list_elements = 1024; return t;
    }()) {}
    HRESULT io(WeaponTypeClass& object,Stream& stream,bool save,BOOL clear=0) {
        struct Call {WeaponTypeClass* object;Stream* stream;bool save;BOOL clear;HRESULT result;} call{&object,&stream,save,clear,-1};
        game::with_type_stream(*value,[](void* raw){auto& c=*static_cast<Call*>(raw);
            c.result=c.save?c.object->Save(c.stream->handle(),c.clear):c.object->Load(c.stream->handle());},&call);
        return call.result;
    }
    std::uint32_t token(AbstractClass& object){std::uint32_t result=0;
        EXPECT_EQ(game::type_stream_reference_id(*value,&object,result),game::TypeStreamStatus::complete);return result;}
    void bind(std::uint32_t token,AbstractClass& object){
        EXPECT_EQ(game::bind_type_stream_reference(*value,token,&object),game::TypeStreamStatus::complete);}
};
std::uint32_t word(const Stream& s,unsigned offset){auto*p=s.bytes.data()+offset;
    return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);}
void put(Stream& s,unsigned offset,std::uint32_t value){for(unsigned i=0;i<4;++i)s.bytes[offset+i]=static_cast<unsigned char>(value>>(i*8));}
}

TEST(WeaponStream, FullRecordAndTypedForwardGraph) {
    WeaponTypeClass source("STREAM_WEAPON"),loaded("BEFORE");
    BulletTypeClass bullet("STREAM_B");WarheadTypeClass warhead("STREAM_W");
    AnimTypeClass a("STREAM_A"),b("STREAM_B2");ParticleSystemTypeClass particle("STREAM_P");
    source.Projectile=&bullet;source.Warhead=&warhead;source.AttachedParticleSystem=&particle;
    source.AssaultAnim=source.OccupantAnim=&a;source.OpenToppedAnim=&b;
    source.Anim.AddItem(&a);source.Anim.AddItem(nullptr);source.Anim.AddItem(&a);
    source.Report.AddItem(9);source.Report.AddItem(-1);source.DownReport.AddItem(31);
    source.Anim.unknown_18=21;source.Report.unknown_18=22;source.DownReport.unknown_18=23;
    source.Report.CapacityIncrement=73;source.UniqueID=0x81234567;source.RefCount=8;source.Dirty=true;
    source.Damage=-9;source.Range=2560;source.MinimumRange=91;source.RadLevel=101;
    source.IsMagBeam=true;source.IsRadBeam=true;source.InfiniteMindControl=true;
    source.LaserInnerColor.R=251;source.LaserOuterSpread.B=241;source.LaserDuration=static_cast<char>(250);
    loaded.RefCount=51;loaded.Report.AddItem(99);loaded.Anim.AddItem(&b);
    Session saving,loading;Stream bytes;
    ASSERT_EQ(saving.io(source,bytes,true,1),0);EXPECT_FALSE(source.Dirty);
    EXPECT_EQ(bytes.bytes.size(),0x160u+4u+12u+24u);
    EXPECT_EQ(word(bytes,4),0x7F73B8u);EXPECT_EQ(word(bytes,4+0x10),source.UniqueID);
    EXPECT_EQ(word(bytes,4+0xC0),0u); // no truncated host list pointer
    EXPECT_EQ(word(bytes,0x164),3u);EXPECT_EQ(word(bytes,0x174),2u);EXPECT_EQ(word(bytes,0x180),1u);
    const int count=WeaponTypeClass::Array.Count;
    ASSERT_EQ(loading.io(loaded,bytes,false),0);
    EXPECT_EQ(loaded.WhatAmI(),AbstractType::WeaponType);EXPECT_EQ(WeaponTypeClass::Array.Count,count);
    EXPECT_EQ(loaded.RefCount,51);EXPECT_EQ(loaded.UniqueID,source.UniqueID);EXPECT_TRUE(loaded.Dirty);
    EXPECT_STREQ(loaded.ID,source.ID);EXPECT_EQ(loaded.Damage,-9);EXPECT_EQ(loaded.Range,2560);
    EXPECT_EQ(loaded.MinimumRange,91);EXPECT_EQ(loaded.RadLevel,101);EXPECT_TRUE(loaded.IsMagBeam);
    EXPECT_TRUE(loaded.IsRadBeam);EXPECT_TRUE(loaded.InfiniteMindControl);
    EXPECT_EQ(loaded.LaserInnerColor.R,251);EXPECT_EQ(loaded.LaserOuterSpread.B,241);
    EXPECT_EQ(static_cast<unsigned char>(loaded.LaserDuration),250);
    EXPECT_EQ(loaded.Report.CapacityIncrement,10);EXPECT_EQ(loaded.Report.unknown_18,22);
    EXPECT_EQ(loaded.Anim.unknown_18,21);EXPECT_EQ(loaded.DownReport.unknown_18,23);
    ASSERT_EQ(loaded.Report.Count,2);EXPECT_EQ(loaded.Report[1],-1);ASSERT_EQ(loaded.Anim.Count,3);
    EXPECT_EQ(loaded.Projectile,nullptr);EXPECT_EQ(loaded.Anim[0],nullptr);
    unsigned unresolved=0;
    EXPECT_EQ(game::resolve_type_stream_references(*loading.value,unresolved),game::TypeStreamStatus::unresolved_references);
    EXPECT_EQ(unresolved,8u);
    for(AbstractClass* object:std::array<AbstractClass*,5>{&bullet,&warhead,&particle,&a,&b})
        loading.bind(saving.token(*object),*object);
    EXPECT_EQ(game::resolve_type_stream_references(*loading.value,unresolved),game::TypeStreamStatus::complete);
    EXPECT_EQ(loaded.Projectile,&bullet);EXPECT_EQ(loaded.Warhead,&warhead);EXPECT_EQ(loaded.AttachedParticleSystem,&particle);
    EXPECT_EQ(loaded.AssaultAnim,&a);EXPECT_EQ(loaded.OccupantAnim,&a);EXPECT_EQ(loaded.OpenToppedAnim,&b);
    EXPECT_EQ(loaded.Anim[0],&a);EXPECT_EQ(loaded.Anim[1],nullptr);EXPECT_EQ(loaded.Anim[2],&a);
}

TEST(WeaponStream, ReloadReplacesPendingDestinationsBeforeFreeingLists) {
    WeaponTypeClass source("RELOAD"),loaded("OLD");AnimTypeClass a("RELOAD_A");
    source.Anim.AddItem(&a);Session saving,loading;Stream first,second;
    ASSERT_EQ(saving.io(source,first,true),0);ASSERT_EQ(loading.io(loaded,first,false),0);
    for(int i=0;i<40;++i)source.Anim.AddItem(&a);
    ASSERT_EQ(saving.io(source,second,true),0);ASSERT_EQ(loading.io(loaded,second,false),0);
    loading.bind(saving.token(a),a);unsigned unresolved=0;
    ASSERT_EQ(game::resolve_type_stream_references(*loading.value,unresolved),game::TypeStreamStatus::complete);
    ASSERT_EQ(loaded.Anim.Count,41);for(auto* item:loaded.Anim)EXPECT_EQ(item,&a);
}

TEST(WeaponStream, WrongReferenceTypeDoesNotPublishPartialGraph) {
    WeaponTypeClass source("WRONG_REF"),loaded("OLD");BulletTypeClass bullet("B");WarheadTypeClass warhead("W");
    source.Projectile=&bullet;source.Warhead=&warhead;
    Session saving,loading;Stream bytes;ASSERT_EQ(saving.io(source,bytes,true),0);
    ASSERT_EQ(loading.io(loaded,bytes,false),0);
    loading.bind(saving.token(bullet),bullet);loading.bind(saving.token(warhead),bullet);
    unsigned unresolved=0;
    EXPECT_EQ(game::resolve_type_stream_references(*loading.value,unresolved),game::TypeStreamStatus::failure);
    EXPECT_EQ(loaded.Projectile,nullptr);EXPECT_EQ(loaded.Warhead,nullptr);
}

TEST(WeaponStream, CorruptionAndTruncationDoNotCommitObjectOrIdentity) {
    WeaponTypeClass source("GOOD");source.Report.AddItem(7);source.Dirty=true;
    Session saving;Stream bytes;ASSERT_EQ(saving.io(source,bytes,true),0);
    for(auto size:{0u,3u,4u,100u,355u,356u,360u,static_cast<unsigned>(bytes.bytes.size()-1)}) {
        WeaponTypeClass loaded("UNCHANGED");loaded.Damage=17;loaded.Report.AddItem(91);
        Session loading;Stream broken=bytes;broken.bytes.resize(size);
        EXPECT_LT(loading.io(loaded,broken,false),0);EXPECT_STREQ(loaded.ID,"UNCHANGED");
        EXPECT_EQ(loaded.Damage,17);ASSERT_EQ(loaded.Report.Count,1);EXPECT_EQ(loaded.Report[0],91);
        Stream retry=bytes;EXPECT_EQ(loading.io(loaded,retry,false),0);
    }
    for(unsigned offset:{4u+0x129,4u+0x15C,4u+0x20,4u+0x3C,0x164u}) {
        WeaponTypeClass loaded("UNCHANGED"),other("OTHER");Session loading;Stream broken=bytes;
        if(offset==0x164)put(broken,offset,0xFFFFFFFF);else broken.bytes[offset]=2;
        EXPECT_LT(loading.io(loaded,broken,false),0);EXPECT_STREQ(loaded.ID,"UNCHANGED");
        Stream retry=bytes;EXPECT_EQ(loading.io(other,retry,false),0); // failed load did not reserve token
    }
}

TEST(WeaponStream, SaveFailureRetainsTheOriginalDirtyClearPoint) {
    WeaponTypeClass object("FAIL_SAVE");Session session;
    for(unsigned call=1;call<=5;++call) {
        object.Dirty=true;Stream failed;failed.fail_call=call;
        EXPECT_LT(session.io(object,failed,true,1),0);
        EXPECT_EQ(object.Dirty,call<=2); // tail failures happen after the original base clear
    }
    EXPECT_EQ(object.Load(nullptr),HRESULT(0x80004003u));EXPECT_EQ(object.Save(nullptr,1),HRESULT(0x80004003u));
}

TEST(WeaponStream, OriginalSaveTailAndWriteOrder) {
    std::ifstream input(RA2_WEAPON_STREAM_FIXTURE);ASSERT_TRUE(input.good());
    unsigned animations,reports,down,total,dirty,cases=0;
    while(input>>animations>>reports>>down>>total>>dirty) {
        WeaponTypeClass weapon("SAVE_CORPUS");AnimTypeClass a("SAVE_A"),b("SAVE_B");
        Session session;EXPECT_EQ(session.token(a),1u);EXPECT_EQ(session.token(b),2u);
        AnimTypeClass* refs[]{&a,&b,nullptr};
        for(unsigned i=0;i<animations;++i)weapon.Anim.AddItem(refs[i%3]);
        for(unsigned i=0;i<reports;++i)weapon.Report.AddItem(17-9*int(i));
        for(unsigned i=0;i<down;++i)weapon.DownReport.AddItem(100+int(i));
        weapon.Dirty=true;Stream bytes;ASSERT_EQ(session.io(weapon,bytes,true,1),0);
        EXPECT_EQ(bytes.bytes.size(),total);EXPECT_EQ(weapon.Dirty,dirty!=0);
        ASSERT_EQ(bytes.writes.size(),5u+animations+reports+down);
        EXPECT_EQ(bytes.writes[0],4u);EXPECT_EQ(bytes.writes[1],0x160u);
        for(unsigned i=2;i<bytes.writes.size();++i)EXPECT_EQ(bytes.writes[i],4u);
        for(unsigned offset=0x164;offset<total;offset+=4) {
            unsigned expected;ASSERT_TRUE(bool(input>>expected));EXPECT_EQ(word(bytes,offset),expected)<<cases;
        }
        ++cases;
    }
    EXPECT_EQ(cases,64u);
}
