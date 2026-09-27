#include "support/test_support.hpp"
#include "yrpp/AbstractTypeClass.h"
#include "yrpp/TargetClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/ScriptTypeClass.h"
#include "yrpp/CRC.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/AircraftClass.h"
#include "yrpp/AirstrikeClass.h"
#include "yrpp/TeamClass.h"
#include "support/type_stream_fixture.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <new>
#include <vector>
#include <fstream>

namespace {
class Probe : public AbstractClass {
public:
    HRESULT YRPP_STDCALL GetClassID(CLSID*) override { return 0; }
    HRESULT YRPP_STDCALL Load(IStream* s) override { return AbstractClass::Load(s); }
    HRESULT YRPP_STDCALL Save(IStream* s, BOOL clear) override { return AbstractClass::Save(s, clear); }
    AbstractType WhatAmI() const override { return AbstractType::Abstract; }
    int Size() const override { return record_size; }
    CoordStruct* GetCoords(CoordStruct* out) const override { *out = position; return out; }
    bool IsDead() const override { return dead; }
    ULONG YRPP_STDCALL Release() override { ++releases; return release_result; }
    void PointerExpired(AbstractClass* p, bool removed) override { expired = p; was_removed = removed; ++notices; }
    ~Probe() override {
        if (destroyed) ++*destroyed;
        if (enqueue_on_destroy) AbstractClass::PendingDeletes.AddItem(enqueue_on_destroy);
    }
    using AbstractClass::GetCoords;
    CoordStruct position{};
    int record_size = 0x24, releases = 0, notices = 0;
    bool dead = true, was_removed = false;
    ULONG release_result = 1;
    int* destroyed = nullptr;
    Probe* enqueue_on_destroy = nullptr;
    AbstractClass* expired = nullptr;
};
struct TypeProbe : AbstractTypeClass {
    explicit TypeProbe(const char* id) : AbstractTypeClass(id) {}
    HRESULT YRPP_STDCALL GetClassID(CLSID*) override { return 0; }
    HRESULT YRPP_STDCALL Load(IStream* s) override { return AbstractClass::Load(s); }
    HRESULT YRPP_STDCALL Save(IStream* s, BOOL clear) override { return AbstractClass::Save(s, clear); }
    AbstractType WhatAmI() const override { return AbsID; }
    int Size() const override { return 0x98; }
    using AbstractTypeClass::EncodeTypeRecordBase;
    using AbstractTypeClass::DecodeTypeRecordBase;
};
GUID guid(std::array<DWORD, 4> words) {
    GUID result; std::memcpy(&result, words.data(), sizeof(result)); return result;
}
using MemoryStream = ra2::test::MemoryStream<>;
struct Session : ra2::test::StreamSession<MemoryStream> {
    explicit Session(bool types = false) : StreamSession(transport(types)) {}
    static game::TypeStreamTransport transport(bool types) {
        auto transport = StreamSession::transport();
        if (types) {
            transport.encode_abstract = [](void*, const AbstractClass* p, unsigned char* b, std::uint32_t n) noexcept {
                return static_cast<const TypeProbe*>(p)->EncodeTypeRecordBase(b,n) ? HRESULT(0) : HRESULT(-1);
            };
            transport.decode_abstract = [](void*, AbstractClass* p, const unsigned char* b, std::uint32_t n) noexcept {
                try { return static_cast<TypeProbe*>(p)->DecodeTypeRecordBase(b,n) ? HRESULT(0) : HRESULT(-1); }
                catch (...) { return HRESULT(-1); }
            };
        }
        return transport;
    }
    HRESULT io(AbstractClass& object, MemoryStream& stream, bool save, BOOL clear = 0) {
        struct Call { AbstractClass* object; IStream* stream; bool save; BOOL clear; HRESULT result; }
            call{&object, stream.handle(), save, clear, -1};
        game::with_type_stream(*value, [](void* raw) {
            auto& c=*static_cast<Call*>(raw);
            c.result=c.save ? c.object->Save(c.stream,c.clear) : c.object->Load(c.stream);
        }, &call);
        return call.result;
    }
};
}

TEST(AbstractClass, InterfacesAndIdentity) {
    Probe p;
    EXPECT_EQ(p.UniqueID, 0xFFFFFFFFu);
    EXPECT_EQ(p.RefCount, 0);
    EXPECT_EQ(p.IsDirty(), 1);
    p.RefCount=42; p.Dirty=true;
    EXPECT_EQ(p.AbstractClass::AddRef(), 1u);
    EXPECT_EQ(p.AbstractClass::Release(), 1u);
    EXPECT_EQ(p.RefCount, 42);
    EXPECT_EQ(p.IsDirty(), 0);
    ULARGE_INTEGER size{};
    EXPECT_EQ(p.GetSizeMax(&size), 0); EXPECT_EQ(size.QuadPart, 0x28u);
    p.record_size=-1;
    EXPECT_EQ(p.GetSizeMax(&size), 0); EXPECT_EQ(size.QuadPart, 3u);
    EXPECT_LT(p.GetSizeMax(nullptr), 0);
    for (DWORD first : {0u, 0x109u, 0x10Cu}) {
        void* result=nullptr;
        EXPECT_EQ(p.QueryInterface(guid({first,0,0xC0,0x46000000}),&result),0);
        EXPECT_EQ(result,static_cast<IPersistStream*>(&p));
    }
    void* result=nullptr;
    const auto iid=guid({0x170DAC82,0x11D212E4,0x60007581,0xB55B0508});
    EXPECT_EQ(p.QueryInterface(iid,&result),0);
    EXPECT_EQ(result,static_cast<IRTTITypeInfo*>(&p));
    auto* rtti=static_cast<IRTTITypeInfo*>(result);
    p.UniqueID=0x87654321;
    EXPECT_EQ(std::uint32_t(rtti->Fetch_ID()), p.UniqueID);
    EXPECT_EQ(rtti->What_Am_I(),AbstractType::Abstract);
    IUnknown* identity=nullptr;
    EXPECT_EQ(rtti->QueryInterface(guid({0,0,0xC0,0x46000000}),reinterpret_cast<void**>(&identity)),0);
    EXPECT_EQ(identity,static_cast<IPersistStream*>(&p));
    EXPECT_EQ(p.QueryInterface(guid({1,2,3,4}),&result),HRESULT(0x80004002u));
    EXPECT_EQ(result,nullptr);
    EXPECT_EQ(p.QueryInterface(iid,nullptr),HRESULT(0x80004003u));
    struct ThrowingRef : Probe {
        ULONG YRPP_STDCALL AddRef() override { throw 1; }
    } throwing;
    result=&p;
    EXPECT_EQ(throwing.QueryInterface(iid,&result),HRESULT(0x80004005u));
    EXPECT_EQ(result,nullptr);
    EXPECT_FALSE(static_cast<INoticeSink*>(&p)->INoticeSink_Unknown(1));
    static_cast<INoticeSource*>(&p)->INoticeSource_Unknown();
    EXPECT_STREQ(AbstractClass::GetRTTIName(AbstractType::Unit),"Unit");
    EXPECT_STREQ(AbstractClass::GetRTTIName(AbstractType::BuildingLight),"Light");
    EXPECT_STREQ(AbstractClass::GetRTTIName(AbstractType(0xFFFFFFFFu)),"Unknown");
}

TEST(AbstractClass, TargetIdentityAndNoInsertion) {
    const auto count=AbstractClass::TargetIndex.Count();
    Probe p; p.UniqueID=0x71234567u;
    TargetClass target(&p);
    EXPECT_EQ(target.m_ID, 0x71234567);
    EXPECT_EQ(target.m_RTTI,52);
    EXPECT_EQ(target.As_Abstract(),nullptr);
    EXPECT_EQ(AbstractClass::TargetIndex.Count(),count);
    ASSERT_TRUE(AbstractClass::TargetIndex.AddIndex(target.m_ID,&p));
    EXPECT_EQ(target.As<AbstractClass>(),&p);
    EXPECT_EQ(target.As_AbstractType(),nullptr);
    EXPECT_EQ(target.As_Object(),nullptr);
    EXPECT_EQ(target.As_Foot(),nullptr);
    EXPECT_EQ(target.As_Techno(),nullptr);
    EXPECT_EQ(target.As_Tag(),nullptr); EXPECT_EQ(target.As_TagType(),nullptr);
    EXPECT_EQ(target.As_Trigger(),nullptr); EXPECT_EQ(target.As_TriggerType(),nullptr);
    EXPECT_EQ(target.As_House(),nullptr); EXPECT_EQ(target.As_TechnoType(),nullptr);
    EXPECT_EQ(target.As_Team(),nullptr); EXPECT_EQ(target.As_TeamType(),nullptr);
    EXPECT_EQ(target.As_Terrain(),nullptr); EXPECT_EQ(target.As_Bullet(),nullptr);
    EXPECT_EQ(target.As_Anim(),nullptr); EXPECT_EQ(target.As_Infantry(),nullptr);
    EXPECT_EQ(target.As_Unit(),nullptr); EXPECT_EQ(target.As_Building(),nullptr);
    EXPECT_EQ(target.As_Aircraft(),nullptr); EXPECT_EQ(target.As_Cell(),nullptr);
    ASSERT_TRUE(AbstractClass::TargetIndex.RemoveIndex(target.m_ID));
    EXPECT_EQ(target.As_Abstract(),nullptr);
    EXPECT_EQ(AbstractClass::TargetIndex.Count(),count);
    TypeProbe type("P1"); type.UniqueID=0x71234568;
    ASSERT_TRUE(AbstractClass::TargetIndex.AddIndex(type.Fetch_ID(),&type));
    TargetClass tt(&type);
    EXPECT_EQ(tt.As_AbstractType(),&type);
    EXPECT_EQ(tt.As_TechnoType(),nullptr);
    EXPECT_TRUE(AbstractClass::TargetIndex.RemoveIndex(type.Fetch_ID()));
    TargetClass empty(static_cast<AbstractClass*>(nullptr));
    EXPECT_EQ(empty.m_ID,0); EXPECT_EQ(empty.m_RTTI,0);
}

TEST(AbstractClass, TargetCellsAndPackedLayout) {
    for (const auto cell : {CellStruct{-1,-1},CellStruct{12,25},CellStruct{-2,-3},CellStruct{32767,32767}}) {
        TargetClass t(cell);
        EXPECT_EQ(t.m_ID,cell.X+1000*cell.Y); EXPECT_EQ(t.m_RTTI,11);
    }
    for (const auto coord : {CoordStruct{-255,-257,9},CoordStruct{256,511,0},CoordStruct{-1,-1,-1}}) {
        TargetClass t(coord);
        EXPECT_EQ(t.m_ID,coord.X/256+1000*(coord.Y/256));
    }
    alignas(TargetClass) unsigned char storage[sizeof(TargetClass)];
    std::memset(storage,0xA5,sizeof(storage));
    auto* none=new(storage) TargetClass(CellStruct::Empty);
    EXPECT_EQ(none->m_RTTI,0);
    for (int i=0;i<4;++i) EXPECT_EQ(storage[i],0xA5);
    EXPECT_EQ(none->As_Abstract(),nullptr); none->~TargetClass();
    struct TestCell : CellClass { TestCell() : CellClass() {} } cell;
    cell.MapCoords={7,8};
    TargetClass t(&cell);
    EXPECT_EQ(t.m_ID,8007); EXPECT_EQ(t.m_RTTI,11);
    auto* expected=MapClass::Instance.GetCellAt(CellStruct{7,8});
    EXPECT_EQ(t.As_Cell(),expected); EXPECT_EQ(t.As_Abstract(),expected);
    t.m_RTTI=0xFF; EXPECT_EQ(t.As_Abstract(),nullptr);
}

TEST(AbstractClass, DeletionDuplicatesReleaseAndReentrancy) {
    ASSERT_EQ(AbstractClass::PendingDeletes.Count,0);
    int destroyed=0;
    auto* dead=new Probe; dead->destroyed=&destroyed;
    auto* extra=new Probe; extra->destroyed=&destroyed;
    dead->enqueue_on_destroy=extra;
    Probe alive; alive.dead=false;
    Probe borrowed; borrowed.release_result=0;
    AbstractClass::PendingDeletes.AddItem(&alive);
    AbstractClass::PendingDeletes.AddItem(dead);
    AbstractClass::PendingDeletes.AddItem(&borrowed);
    AbstractClass::PendingDeletes.AddItem(dead);
    AbstractClass::RemoveAllInactive();
    EXPECT_EQ(destroyed,2); EXPECT_EQ(borrowed.releases,1); EXPECT_EQ(alive.releases,0);
    ASSERT_EQ(AbstractClass::PendingDeletes.Count,1);
    EXPECT_EQ(AbstractClass::PendingDeletes[0],&alive);
    AbstractClass::PendingDeletes.Remove(&alive);
}

TEST(AbstractClass, StreamRoundTripAndFailure) {
    Probe before,after; before.UniqueID=0xAB123456; before.RefCount=17; before.Dirty=true;
    before.unknown_18=0x1234; after.RefCount=29;
    MemoryStream bytes;
    Session session;
    ASSERT_EQ(session.io(before,bytes,true,1),0);
    EXPECT_FALSE(before.Dirty); ASSERT_EQ(bytes.bytes.size(),0x28u);
    EXPECT_EQ(bytes.bytes[4+0x20],1);
    ASSERT_EQ(session.io(after,bytes,false),0);
    EXPECT_EQ(after.UniqueID,before.UniqueID); EXPECT_EQ(after.RefCount,29);
    EXPECT_TRUE(after.Dirty); EXPECT_EQ(after.unknown_18,0x1234u);
    EXPECT_EQ(after.WhatAmI(),AbstractType::Abstract);
    EXPECT_EQ(after.Load(nullptr),HRESULT(0x80004003u));
    EXPECT_EQ(after.Save(nullptr,1),HRESULT(0x80004003u));
    for (unsigned fail : {1u,2u}) {
        MemoryStream failed; failed.fail_call=fail; before.Dirty=true;
        EXPECT_LT(session.io(before,failed,true,1),0); EXPECT_TRUE(before.Dirty);
    }
    MemoryStream short_write; short_write.limit=2; before.Dirty=true;
    EXPECT_LT(session.io(before,short_write,true,1),0); EXPECT_TRUE(before.Dirty);
    for (std::size_t length : {0u,3u,4u,20u,39u}) {
        MemoryStream truncated; truncated.bytes.assign(bytes.bytes.begin(),bytes.bytes.begin()+length);
        Probe sentinel; sentinel.UniqueID=73; sentinel.RefCount=91;
        Session load;
        EXPECT_LT(load.io(sentinel,truncated,false),0);
        EXPECT_EQ(sentinel.UniqueID,73u); EXPECT_EQ(sentinel.RefCount,91);
    }
    before.record_size=0x98;
    MemoryStream unsupported;
    EXPECT_LT(session.io(before,unsupported,true),0);
    EXPECT_TRUE(unsupported.bytes.empty());
    // A rejected record must not leave a token bound to the failed destination.
    Session failed_load;
    MemoryStream malformed; malformed.bytes=bytes.bytes;
    malformed.bytes[4+0x20]=2;
    Probe failed_destination, valid_destination;
    EXPECT_LT(failed_load.io(failed_destination,malformed,false),0);
    MemoryStream valid; valid.bytes=bytes.bytes;
    EXPECT_EQ(failed_load.io(valid_destination,valid,false),0);
}

TEST(AbstractClass, TypeRegistryCrcIniAndRecord) {
    const int count=AbstractTypeClass::Array.Count;
    {
        TypeProbe before("123456789012345678901234EXTRA"),after("OTHER");
        EXPECT_EQ(AbstractTypeClass::Array.Count,count+2);
        EXPECT_EQ(std::strlen(before.get_ID()),24u);
        EXPECT_STREQ(before.Name,"123456789012345678901234");
        CCINIClass ini;
        ASSERT_TRUE(ini.WriteString(before.ID,"Name","P1 name"));
        ASSERT_TRUE(ini.WriteString(before.ID,"Unrelated","remove"));
        EXPECT_TRUE(before.LoadFromINI(&ini));
        EXPECT_STREQ(before.Name,"P1 name");
        EXPECT_TRUE(before.SaveToINI(&ini));
        EXPECT_FALSE(ini.GetKeyName(before.ID,2));
        EXPECT_FALSE(before.LoadFromINI(nullptr));
        CRCEngine crc,reference;
        before.ComputeCRC(crc);
        reference(int(before.UniqueID)); reference(before.Dirty);
        reference(before.ID,int(std::strlen(before.ID)));
        reference(before.Name,int(std::strlen(before.Name)));
        reference(before.UINameLabel,int(std::strlen(before.UINameLabel)));
        EXPECT_EQ(crc(),reference());
        MemoryStream bytes; Session session(true);
        before.UniqueID=123; before.Dirty=true; after.RefCount=33;
        ASSERT_EQ(session.io(before,bytes,true),0);
        ASSERT_EQ(session.io(after,bytes,false),0);
        EXPECT_STREQ(after.ID,before.ID); EXPECT_STREQ(after.Name,before.Name);
        EXPECT_EQ(after.UniqueID,123u); EXPECT_EQ(after.RefCount,33);
        EXPECT_STREQ(after.UIName,L"");
        Probe listener;
        AbstractClass::TypeExpirationListeners.AddItem(&listener);
        before.NotifyTypeExpired(false);
        EXPECT_EQ(listener.expired,&before); EXPECT_FALSE(listener.was_removed);
        AbstractClass::TypeExpirationListeners.Remove(&listener);
        AbstractClass::TriggerExpirationListeners.AddItem(&listener);
        before.NotifyTriggerNodeExpired(true);
        EXPECT_EQ(listener.notices,2); EXPECT_TRUE(listener.was_removed);
        AbstractClass::TriggerExpirationListeners.Remove(&listener);
    }
    EXPECT_EQ(AbstractTypeClass::Array.Count,count);
}

TEST(AbstractClass, TypeRegistryRefusedGrowth) {
    struct Restore {
        DynamicVectorClass<AbstractTypeClass*> saved;
        Restore() { saved.Swap(AbstractTypeClass::Array); }
        ~Restore() { saved.Swap(AbstractTypeClass::Array); }
    } restore;
    AbstractTypeClass::Array.CapacityIncrement=0;
    {
        TypeProbe unregistered("NO_GROWTH");
        EXPECT_EQ(AbstractTypeClass::Array.Count,0);
        EXPECT_STREQ(unregistered.ID,"NO_GROWTH");
    }
    EXPECT_EQ(AbstractTypeClass::Array.Count,0);
}

namespace {
class AttackerProbe : public TechnoClass {
public:
    AttackerProbe() : TechnoClass(nullptr) {}
    HRESULT YRPP_STDCALL GetClassID(CLSID*) override { return 0; }
    AbstractType WhatAmI() const override { return AbstractType::Unit; }
    int Size() const override { return sizeof(*this); }
    void Destroyed(ObjectClass*) override {}
    bool Mission_Revert() override {
        ++reverts;
        if (trace) trace->push_back(order);
        if (replacement) Target=replacement;
        return true;
    }
    void SetTarget(AbstractClass* p) override { ++assignments; Target=p; }
    int reverts=0,assignments=0,order=0;
    AbstractClass* replacement=nullptr;
    std::vector<int>* trace=nullptr;
};
class AirstrikeProbe : public AirstrikeClass {
public: AirstrikeProbe() : AirstrikeClass(noinit_t{}) { Owner=nullptr; Target=nullptr; }
};
class TeamProbe : public TeamClass {
public: TeamProbe() : TeamClass(nullptr,nullptr,0) {}
};
class AircraftProbe : public AircraftClass {
public:
    AircraftProbe() : AircraftClass(nullptr,nullptr) {}
    AbstractType WhatAmI() const override { return AbstractType::Aircraft; }
    bool Mission_Revert() override { ++reverts; return true; }
    void SetTarget(AbstractClass* p) override { ++assignments; Target=p; }
    int reverts=0,assignments=0;
};
}

TEST(AbstractClass, UntargetableAirstrikeGuardAndReverseDispatch) {
    AttackerProbe target, a, b, different;
    Probe unrelated;
    AirstrikeProbe strike;
    TeamProbe team;
    std::vector<int> order;
    a.trace=&order; a.order=1; b.trace=&order; b.order=2;
    a.Target=b.Target=&target;
    different.Target=&unrelated;
    strike.Target=&target; a.Airstrike=&strike;
    target.Health=100; target.IsAlive=true;
    team.QueuedFocus=team.Focus=&target; team.Owner=nullptr;
    target.BecomeUntargetable();
    EXPECT_EQ(a.Target,&target); EXPECT_EQ(a.reverts,0);
    EXPECT_EQ(b.Target,nullptr); EXPECT_EQ(b.reverts,1); EXPECT_EQ(b.assignments,1);
    EXPECT_EQ(different.Target,&unrelated);
    EXPECT_EQ(team.QueuedFocus,nullptr); EXPECT_EQ(team.Focus,nullptr);
    EXPECT_EQ(order,(std::vector<int>{2}));
    target.IsAlive=false; order.clear(); b.Target=&target;
    a.replacement=&unrelated;
    target.BecomeUntargetable();
    EXPECT_EQ(order,(std::vector<int>{2,1}));
    EXPECT_EQ(a.Target,&unrelated); EXPECT_EQ(a.assignments,0);
    a.replacement=nullptr; a.Target=&target; target.IsAlive=true; target.Health=0;
    target.BecomeUntargetable();
    EXPECT_EQ(a.Target,nullptr); EXPECT_EQ(a.assignments,1);
}

TEST(AbstractClass, UntargetableAircraftPatrolState) {
    Probe target;
    AircraftProbe aircraft;
    aircraft.Target=&target; aircraft.CurrentMission=Mission::Patrol;
    aircraft.MissionStatus=7; aircraft.IsLocked=true;
    target.BecomeUntargetable();
    EXPECT_EQ(aircraft.reverts,1); EXPECT_EQ(aircraft.assignments,1);
    EXPECT_EQ(aircraft.Target,nullptr); EXPECT_EQ(aircraft.MissionStatus,0); EXPECT_FALSE(aircraft.IsLocked);
    aircraft.Target=&target; aircraft.CurrentMission=Mission::Attack;
    aircraft.MissionStatus=7; aircraft.IsLocked=true;
    target.BecomeUntargetable();
    EXPECT_EQ(aircraft.MissionStatus,7); EXPECT_TRUE(aircraft.IsLocked);
}

TEST(AbstractClass, ProductionBuildingAndTerrainTargetLifetime) {
    ScenarioClass scenario;
    auto* previous=ScenarioClass::Instance;
    ScenarioClass::Instance=&scenario;
    struct Restore { ScenarioClass* value; ~Restore(){ScenarioClass::Instance=value;} } restore{previous};
    const auto count=AbstractClass::TargetIndex.Count();
    BuildingTypeClass building_type("TARGET_BUILDING",BuildingTypeClass::ConstructionDefaults{});
    TerrainTypeClass terrain_type("TARGET_TERRAIN");
    TargetClass building_target,terrain_target;
    {
        BuildingClass building(&building_type,nullptr);
        TerrainClass terrain(&terrain_type,{1,2});
        building_target=TargetClass(&building); terrain_target=TargetClass(&terrain);
        EXPECT_NE(building.Fetch_ID(),terrain.Fetch_ID());
        EXPECT_EQ(AbstractClass::TargetIndex.Count(),count+2);
        EXPECT_EQ(building_target.As_Building(),&building);
        EXPECT_EQ(building_target.As_Techno(),&building);
        EXPECT_EQ(building_target.As_Object(),&building);
        EXPECT_EQ(terrain_target.As_Terrain(),&terrain);
        EXPECT_EQ(terrain_target.As_Techno(),nullptr);
    }
    EXPECT_EQ(AbstractClass::TargetIndex.Count(),count);
    EXPECT_EQ(building_target.As_Abstract(),nullptr);
    EXPECT_EQ(terrain_target.As_Abstract(),nullptr);
}

TEST(AbstractClass, Geometry) {
    Probe p,q;
    q.position={3,4,12};
    EXPECT_EQ(p.DistanceFrom(&q),5); EXPECT_EQ(p.DistanceFrom3D(&q),13);
    EXPECT_EQ(p.DistanceFrom(nullptr),0); EXPECT_EQ(p.DistanceFrom3D(nullptr),0);
    for (const auto& item : std::array<std::pair<CoordStruct,unsigned>,4>{{
        {{0,-256,0},0},{{256,0,0},0x3FFF},{{0,256,0},0x7FFF},{{-256,0,0},0xC001}}}) {
        q.position=item.first; EXPECT_EQ(p.GetTargetDirection(&q).Raw,item.second);
    }
    BuildingTypeClass type("P1BUILD", BuildingTypeClass::ConstructionDefaults{});
    BuildingClass building(&type,nullptr);
    building.Location={1000,0,0}; type.Foundation=Foundation(0);
    EXPECT_EQ(p.DistanceFrom(&building),872);
    building.Location={100,0,0}; EXPECT_EQ(p.DistanceFrom3D(&building),0);
}

TEST(AbstractClass, OriginalInstructionCorpus) {
    std::ifstream input(RA2_ABSTRACT_FIXTURE);
    ASSERT_TRUE(input.good());
    char kind; int count=0;
    while (input >> kind) {
        ++count;
        if (kind=='C') {
            int x,y,id,tag; ASSERT_TRUE(bool(input>>x>>y>>id>>tag));
            alignas(TargetClass) unsigned char raw[5]; std::memset(raw,0xA5,5);
            auto* t=new(raw) TargetClass(CellStruct{short(x),short(y)});
            int observed; std::memcpy(&observed,raw,4);
            EXPECT_EQ(observed,id); EXPECT_EQ(t->m_RTTI,tag);
            t->~TargetClass();
        } else if (kind=='Q') {
            CoordStruct q; int id,tag; ASSERT_TRUE(bool(input>>q.X>>q.Y>>q.Z>>id>>tag));
            TargetClass t(q); EXPECT_EQ(t.m_ID,id); EXPECT_EQ(t.m_RTTI,tag);
        } else {
            ASSERT_EQ(kind,'G');
            Probe p,q; std::uint32_t raw,d2,d3;
            ASSERT_TRUE(bool(input>>q.position.X>>q.position.Y>>q.position.Z>>raw>>d2>>d3));
            auto direction=p.GetTargetDirection(&q);
            std::uint32_t actual; std::memcpy(&actual,&direction,4);
            EXPECT_EQ(actual,raw) << q.position.X << ',' << q.position.Y;
            EXPECT_EQ(std::uint32_t(p.DistanceFrom(&q)),d2);
            EXPECT_EQ(std::uint32_t(p.DistanceFrom3D(&q)),d3);
        }
    }
    EXPECT_EQ(count,280);
}
