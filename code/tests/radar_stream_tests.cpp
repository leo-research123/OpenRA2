#include "support/test_support.hpp"
#include "support/type_stream_fixture.hpp"
#include "yrpp/MouseClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/TerrainTypeClass.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <vector>

namespace {
constexpr HRESULT failure=static_cast<HRESULT>(0x80004005u);
using Stream = ra2::test::MemoryStream<failure, true, true>;
struct Session : ra2::test::StreamSession<Stream> {
    HRESULT io(RadarClass& radar,Stream& stream,bool save) {
        struct Call { RadarClass& radar; Stream& stream; bool save; HRESULT result=-1; } call{radar,stream,save};
        game::with_type_stream(*value,[](void* raw) {
            auto& c=*static_cast<Call*>(raw);
            c.result=c.save?c.radar.RadarClass::Save(c.stream.handle()):c.radar.RadarClass::Load(c.stream.handle());
        },&call);
        return call.result;
    }
    std::uint32_t token(AbstractClass& object) {
        std::uint32_t id=0;
        EXPECT_EQ(game::type_stream_reference_id(*value,&object,id),game::TypeStreamStatus::complete);
        return id;
    }
};
struct Layers {
    Layers() { clear(); }
    ~Layers() { clear(); }
    static void clear() { for(auto& layer:MapClass::ObjectsInLayers) layer.Clear(); }
};
void append(std::vector<unsigned char>& bytes,std::uint32_t word) {
    for(unsigned i=0;i<4;++i) bytes.push_back(static_cast<unsigned char>(word>>(8*i)));
}
Stream radar_record() {
    Stream stream;
    for(unsigned value:{0u,0u,0u,0u,0u,2u,10u,20u,30u,40u,1u,0xFFFD0002u,3u,1u,4u,2u})
        append(stream.bytes,value);
    return stream;
}
}

TEST(RadarStream, OriginalRecordAndDeferredLayerReferences) {
    Layers layers;
    MouseClass source,loaded;
    TerrainTypeClass terrain_type("RADAR_STREAM_TREE");
    TerrainClass first(&terrain_type,{1,2}),second(&terrain_type,{3,4});
    MapClass::ObjectsInLayers[0].AddItem(&first);
    MapClass::ObjectsInLayers[0].AddItem(nullptr);
    MapClass::ObjectsInLayers[0].AddItem(&first);
    MapClass::ObjectsInLayers[3].AddItem(&second);
    for(int i=0;i<23;++i) source.unknown_points_125C.AddItem({i,-i});
    for(short i=0;i<12;++i) source.unknown_cells_1124.AddItem({i,static_cast<short>(-i)});
    source.unknown_14B0=3; source.unknown_14AC=1; source.unknown_14B4=4; source.unknown_14B8=2;
    Session saving,loading; Stream stream;
    ASSERT_EQ(saving.io(source,stream,true),0);
    std::vector<unsigned char> expected;
    for(unsigned n:{3u,saving.token(first),0u,saving.token(first),0u,0u,1u,saving.token(second),0u,23u}) append(expected,n);
    for(int i=0;i<23;++i) { append(expected,i); append(expected,static_cast<unsigned>(-i)); }
    append(expected,12);
    for(short i=0;i<12;++i) append(expected,static_cast<unsigned short>(i)|
        (std::uint32_t(static_cast<unsigned short>(-i))<<16));
    for(unsigned n:{3u,1u,4u,2u}) append(expected,n);
    EXPECT_EQ(stream.bytes,expected);
    Layers::clear();
    loaded.unknown_timer_1500.StartTime=0x12345678;
    loaded.unknown_timer_1500.TimeLeft=77;
    loaded.unknown_14FC=25; loaded.IsAvailableNow=true;
    loaded.RadarAudio.Unused=0xAABBCCDD;
    ASSERT_EQ(loading.io(loaded,stream,false),0);
    EXPECT_EQ(loaded.unknown_points_125C.Count,23);
    EXPECT_EQ(loaded.unknown_points_125C.Capacity,30);
    EXPECT_EQ(loaded.unknown_points_125C.CapacityIncrement,10);
    EXPECT_EQ(loaded.unknown_points_125C[22],(Point2D{22,-22}));
    EXPECT_EQ(loaded.unknown_cells_1124.Count,12);
    EXPECT_EQ(loaded.unknown_cells_1124[11],(CellStruct{11,-11}));
    EXPECT_EQ(loaded.unknown_14B0,3u); EXPECT_EQ(loaded.unknown_14AC,1u);
    EXPECT_EQ(loaded.unknown_14B4,4u); EXPECT_EQ(loaded.unknown_14B8,2u);
    EXPECT_EQ(loaded.unknown_timer_1500.StartTime,0x12345678);
    EXPECT_EQ(loaded.unknown_timer_1500.TimeLeft,77);
    EXPECT_EQ(loaded.unknown_14FC,25u); EXPECT_TRUE(loaded.IsAvailableNow);
    EXPECT_EQ(loaded.RadarAudio.Unused,0xAABBCCDDu);
    EXPECT_EQ(loaded.RadarAudio.Event,nullptr);
    EXPECT_EQ(loaded.RadarAudio.AudioIndex,&AudioIDXData::Instance);
    unsigned unresolved=0;
    EXPECT_EQ(game::resolve_type_stream_references(*loading.value,unresolved),game::TypeStreamStatus::unresolved_references);
    EXPECT_EQ(unresolved,3u);
    EXPECT_EQ(MapClass::ObjectsInLayers[0][0],nullptr);
    ASSERT_EQ(game::bind_type_stream_reference(*loading.value,saving.token(first),&first),game::TypeStreamStatus::complete);
    ASSERT_EQ(game::bind_type_stream_reference(*loading.value,saving.token(second),&second),game::TypeStreamStatus::complete);
    ASSERT_EQ(game::resolve_type_stream_references(*loading.value,unresolved),game::TypeStreamStatus::complete);
    EXPECT_EQ(MapClass::ObjectsInLayers[0][0],&first);
    EXPECT_EQ(MapClass::ObjectsInLayers[0][1],nullptr);
    EXPECT_EQ(MapClass::ObjectsInLayers[0][2],&first);
    EXPECT_EQ(MapClass::ObjectsInLayers[3][0],&second);
}

TEST(RadarStream, LoadFailureRetainsOriginalPartialWrites) {
    Layers layers;
    for(unsigned failed=5;failed<=14;++failed) {
        MouseClass radar;
        // Borrowed pre-load storage models the original reconstruction contract.
        Point2D point{91,92},foundation{93,94}; CellStruct cell{95,96};
        radar.unknown_points_125C.SetCapacity(1,&point); radar.unknown_points_125C.Count=1;
        radar.unknown_cells_1124.SetCapacity(1,&cell); radar.unknown_cells_1124.Count=1;
        radar.FoundationTypePixels[0].SetCapacity(1,&foundation); radar.FoundationTypePixels[0].Count=1;
        radar.unknown_14B0=91; radar.unknown_14AC=92; radar.unknown_14B4=93; radar.unknown_14B8=94;
        radar.RadarAudio.Stamp=123; radar.RadarAudio.Unused=0x11223344;
        Session session; auto stream=radar_record(); stream.fail_call=failed;
        EXPECT_EQ(session.io(radar,stream,false),failure) << failed;
        EXPECT_EQ(stream.calls,failed);
        EXPECT_EQ(radar.unknown_points_125C.Count,failed<=6?1:failed-7<2?int(failed-7):2);
        EXPECT_EQ(radar.unknown_cells_1124.Count,failed<=9?1:failed==10?0:1);
        EXPECT_EQ(radar.FoundationTypePixels[0].Count,failed<=10?1:0);
        EXPECT_EQ(radar.unknown_14B0,failed<=11?91u:3u);
        EXPECT_EQ(radar.unknown_14AC,failed<=12?92u:1u);
        EXPECT_EQ(radar.unknown_14B4,failed<=13?93u:4u);
        EXPECT_EQ(radar.unknown_14B8,94u);
        EXPECT_EQ(radar.RadarAudio.Stamp,failed<=6?123u:0u);
        EXPECT_EQ(radar.RadarAudio.Unused,0x11223344u);
    }
}

TEST(RadarStream, SaveStopsAtEachFailedWrite) {
    Layers layers; MouseClass radar;
    radar.unknown_points_125C.AddItem({10,20}); radar.unknown_points_125C.AddItem({30,40});
    radar.unknown_cells_1124.AddItem({2,-3});
    Session session; Stream complete;
    ASSERT_EQ(session.io(radar,complete,true),0);
    for(unsigned failed=1;failed<=complete.calls;++failed) {
        Stream stream; stream.fail_call=failed;
        EXPECT_EQ(session.io(radar,stream,true),failure);
        EXPECT_EQ(stream.calls,failed);
        EXPECT_TRUE(std::equal(stream.bytes.begin(),stream.bytes.end(),complete.bytes.begin()));
    }
}

TEST(RadarStream, DisplayLoadContinuesAndReturnsLastLayerResult) {
    Layers layers; MouseClass radar; Session session;
    // A failed first layer does not advance this stream. The remaining four
    // layers and the radar tail therefore each read a zero, exactly as YR does.
    Stream stream; stream.bytes.resize(64); stream.fail_call=1;
    EXPECT_EQ(session.io(radar,stream,false),0);
    EXPECT_EQ(stream.calls,11u);
    EXPECT_EQ(radar.RadarClass::Load(nullptr),static_cast<HRESULT>(0x80004003u));
    EXPECT_EQ(radar.RadarClass::Save(nullptr),static_cast<HRESULT>(0x80004003u));
}

TEST(RadarStream, NonnegativeShortTransferKeepsOriginalHRESULTContract) {
    Layers layers; MouseClass radar; Session session; Stream written;
    written.short_from=1;
    EXPECT_EQ(session.io(radar,written,true),0);
    EXPECT_EQ(written.calls,11u); EXPECT_EQ(written.bytes.size(),22u);
    Stream read; read.bytes.resize(7*4);
    for(unsigned char byte:{0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88}) read.bytes.push_back(byte);
    read.short_from=8;
    radar.unknown_14B0=radar.unknown_14AC=radar.unknown_14B4=radar.unknown_14B8=0xAABBCCDD;
    EXPECT_EQ(session.io(radar,read,false),0);
    EXPECT_EQ(radar.unknown_14B0,0xAABB2211u); EXPECT_EQ(radar.unknown_14AC,0xAABB4433u);
    EXPECT_EQ(radar.unknown_14B4,0xAABB6655u); EXPECT_EQ(radar.unknown_14B8,0xAABB8877u);
}
