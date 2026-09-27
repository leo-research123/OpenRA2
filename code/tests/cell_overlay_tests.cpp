#include "support/test_support.hpp"
// Explicit test-only access to the synchronous overlay host contract. Inputs
// and expected requests/state come from fixed-EXE instruction execution.
#include "overlay_drawing.hpp"
#include "yrpp/CellClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/BuildingTypeClass.h"
#include "yrpp/TiberiumClass.h"
#include <array>
#include <bit>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace {
struct Input {
    int index,type_index,flags,x,y,level,frame,slope,land,normal,terrain,bridge;
    int light,rubble,count,cache,point_x,point_y,clip_x,clip_y,clip_w,clip_h,top,resource;
};
std::istream& operator>>(std::istream& in,Input& c) {
    return in>>c.index>>c.type_index>>c.flags>>c.x>>c.y>>c.level>>c.frame>>c.slope>>c.land
        >>c.normal>>c.terrain>>c.bridge>>c.light>>c.rubble>>c.count>>c.cache
        >>c.point_x>>c.point_y>>c.clip_x>>c.clip_y>>c.clip_w>>c.clip_h>>c.top>>c.resource;
}
struct Fixture {
    std::array<SHPStruct,265> shapes;
    std::vector<std::unique_ptr<OverlayTypeClass>> overlays;
    TiberiumClass resource{"OVERLAY-RESOURCE"};
    BuildingTypeClass building{"OVERLAY-RUBBLE",BuildingTypeClass::ConstructionDefaults{}};
    std::unique_ptr<CellClass,GameDeleter> cell{CellClass::Create()};
    game::OverlayDrawing drawing;
    std::vector<game::ShapeDrawingRequest> requests;
    int light_token=0;
    Fixture() {
        for(int i=0;i<256;++i) {
            const auto name="OVERLAY-"+std::to_string(i);
            overlays.push_back(std::make_unique<OverlayTypeClass>(name.c_str()));
            overlays.back()->Image=&shapes[i];
        }
        resource.Image=overlays[102].get();resource.NumImages=12;resource.NumSlopes=8;
        resource.ArrayIndex=0;
        drawing.context=this;drawing.types.backend_context=this;
        drawing.types.target=reinterpret_cast<game::DrawingTargetHandle*>(this);
        drawing.frame=0x12345;drawing.redraws=5;
        for(int i=1;i<=4;++i)drawing.slope_depth[i]=&shapes[260+i];
        drawing.initialize_light=[](void* p,CellClass& cell) noexcept {
            auto& f=*static_cast<Fixture*>(p);
            cell.LightConvert=reinterpret_cast<LightConvertClass*>(&f.light_token);
            cell.Intensity_Normal=1101;cell.Intensity_Terrain=1202;cell.Color1_Blue=1303;
            return game::DrawingStatus::drawn;
        };
        drawing.palette=[](void*,CellClass&,game::OverlayPalette kind,const game::DrawingPaletteHandle*& output) noexcept {
            output=reinterpret_cast<const game::DrawingPaletteHandle*>(std::uintptr_t(kind)+1);
            return game::DrawingStatus::drawn;
        };
        drawing.types.backend.shape=[](void* p,const game::ShapeDrawingRequest& r) {
            static_cast<Fixture*>(p)->requests.push_back(r);return game::DrawingStatus::drawn;
        };
    }
    ~Fixture() {
        // All image/converter pointers in this observer are borrowed.
        cell->LightConvert=nullptr;building.Image=nullptr;building.Rubble=nullptr;
        for(auto& type:overlays)type->Image=nullptr;
    }
    void setup(const Input& c) {
        requests.clear();
        for(int i=0;i<256;++i) {
            auto& t=*overlays[i];t.ArrayIndex=i;t.Tiberium=t.Wall=t.Crate=t.IsRubble=t.IsARock=t.DrawFlat=false;
            t.LandType=LandType::Clear;shapes[i].Frames=static_cast<short>(c.count);
        }
        auto& t=*overlays[c.index];t.ArrayIndex=c.type_index;
        t.DrawFlat=c.flags&1;t.IsARock=c.flags&2;t.Crate=c.flags&4;
        t.IsRubble=c.flags&8;t.Tiberium=c.flags&16;t.Wall=c.flags&32;t.LandType=static_cast<LandType>(c.land);
        cell->OverlayTypeIndex=c.index;cell->MapCoords={short(c.x),short(c.y)};
        cell->Flags=static_cast<CellFlags>(c.flags&64?0x80:0);
        cell->Level=static_cast<char>(c.level);cell->SlopeIndex=static_cast<BYTE>(c.slope);
        cell->OverlayData=static_cast<BYTE>(c.frame);
        cell->Intensity_Normal=WORD(c.normal);cell->Intensity_Terrain=WORD(c.terrain);cell->Color1_Blue=WORD(c.bridge);
        cell->LightConvert=c.light?reinterpret_cast<LightConvertClass*>(&light_token):nullptr;
        cell->Rubble=c.rubble?&building:nullptr;building.LeaveRubble=c.rubble>=2;
        building.Rubble=c.rubble==5?&shapes[257]:nullptr;
        building.Image=c.rubble==2?nullptr:&shapes[256];
        shapes[256].Frames=shapes[257].Frames=static_cast<short>(c.count);
        resource.ArrayIndex=c.resource==2?-1:0;
        cell->RedrawFrame=c.cache?0x12345:0;cell->unknown_118=c.cache==2?4:5;
        cell->InViewportRect={c.clip_x,c.clip_y,c.clip_w,c.clip_h};
        auto& rect=cell->InViewportRect;
        if(c.cache==3)++rect.X;if(c.cache==4)++rect.Y;if(c.cache==5)++rect.Width;if(c.cache==6)++rect.Height;
        drawing.tactical_rect={5,c.top,1280,720};
        if(c.cache==7)rect=drawing.tactical_rect;
    }
};
}

TEST(CellOverlay, OriginalInstructionRequestsAndState) {
    Fixture f;
    ASSERT_NE(f.cell,nullptr);
    std::ifstream input(RA2_CELL_OVERLAY_FIXTURE);
    std::string tag,fields;std::getline(input,tag);ASSERT_EQ(tag,"CELL_OVERLAY_V2");std::getline(input,fields);
    int cases=0,lookups=0;
    std::unique_ptr<TiberiumClass> second;
    while(input>>tag) {
        if(tag=="LOOKUP") {
            int index,tiberium,count,expected;input>>index>>tiberium>>count>>expected;
            if(!second) {
                second=std::make_unique<TiberiumClass>("OVERLAY-SECOND-RESOURCE");
                second->Image=f.overlays[147].get();second->NumImages=12;second->NumSlopes=8;
                second->ArrayIndex=1;
            }
            for(int i=0;i<256;++i){f.overlays[i]->ArrayIndex=i;f.overlays[i]->Tiberium=false;}
            f.resource.ArrayIndex=0;
            if(index>=0)f.overlays[index]->Tiberium=tiberium!=0;
            TiberiumClass::Array.Count=count;
            const int actual=TiberiumClass::FindIndex(index);
            TiberiumClass::Array.Count=2;
            EXPECT_EQ(actual,expected)<<"overlay="<<index<<" tiberium="<<tiberium<<" resources="<<count;
            ++lookups;continue;
        }
        ASSERT_EQ(tag,"CASE");std::string name;int shadow;Input c{};input>>name>>shadow>>c;
        SCOPED_TRACE(name+"/"+std::to_string(shadow));
        int count;std::array<int,6> cache{};std::array<int,3> light{};
        input>>tag>>count;ASSERT_EQ(tag,"RESULT");
        for(auto& value:cache)input>>value;for(auto& value:light)input>>value;
        f.setup(c);
        const auto status=game::draw_cell_overlay(*f.cell,f.drawing,{c.point_x,c.point_y},
            {c.clip_x,c.clip_y,c.clip_w,c.clip_h},shadow!=0);
        EXPECT_TRUE(status==game::DrawingStatus::drawn||status==game::DrawingStatus::skipped);
        ASSERT_EQ(f.requests.size(),std::size_t(count));
        for(const auto& r:f.requests) {
            std::array<int,19> values{};input>>tag;ASSERT_EQ(tag,"DRAW");for(auto& value:values)input>>value;
            const int image=int(r.image-f.shapes.data());
            const int depth=r.depth_image?int(r.depth_image-f.shapes.data()):0;
            const std::array<int,19> actual={int(reinterpret_cast<std::uintptr_t>(r.palette)),image,r.frame,
                r.position.X,r.position.Y,r.clip.X,r.clip.Y,r.clip.Width,r.clip.Height,int(r.flags),0,
                r.depth_adjustment,r.gradient,r.intensity,0,depth,r.depth_frame,r.depth_offset.X,r.depth_offset.Y};
            EXPECT_EQ(actual,values);
            EXPECT_EQ(r.target,f.drawing.types.target);
            EXPECT_EQ(r.depth_mode,game::ShapeDepthMode::legacy);
        }
        const auto& rect=f.cell->InViewportRect;
        EXPECT_EQ((std::array<int,6>{int(f.cell->RedrawFrame),rect.X,rect.Y,rect.Width,rect.Height,
            static_cast<unsigned char>(f.cell->unknown_118)}),cache);
        EXPECT_EQ((std::array<int,3>{std::bit_cast<short>(f.cell->Intensity_Normal),
            std::bit_cast<short>(f.cell->Intensity_Terrain),std::bit_cast<short>(f.cell->Color1_Blue)}),light);
        ++cases;
    }
    EXPECT_TRUE(input.eof());EXPECT_EQ(cases,1652);EXPECT_EQ(lookups,96);
    EXPECT_EQ(game::overlay_drawing(),nullptr);
}

TEST(CellOverlay, HostFailuresAreReportedAndDoNotPoisonBridgeCache) {
    Fixture f;
    Input c{74,74,65,8,7,0,1,0,1,901,802,703,1,0,36,0,100,200,11,13,400,300,7,1};
    f.setup(c);
    f.drawing.types.backend.shape=[](void*,const game::ShapeDrawingRequest&) {return game::DrawingStatus::backend_failure;};
    EXPECT_EQ(game::draw_cell_overlay(*f.cell,f.drawing,{100,200},{11,13,400,300},false),game::DrawingStatus::backend_failure);
    EXPECT_EQ(f.cell->RedrawFrame,0u);
    EXPECT_EQ(game::overlay_drawing(),nullptr);
    f.drawing.types.backend.shape=[](void*,const game::ShapeDrawingRequest&) -> game::DrawingStatus {throw 7;};
    EXPECT_EQ(game::draw_cell_overlay(*f.cell,f.drawing,{100,200},{11,13,400,300},false),game::DrawingStatus::backend_failure);
    EXPECT_EQ(f.cell->RedrawFrame,0u);
    EXPECT_EQ(game::overlay_drawing(),nullptr);
    c.flags=16;c.index=c.type_index=102;c.slope=1;f.setup(c);
    f.drawing.slope_depth[1]=nullptr;
    EXPECT_EQ(game::draw_cell_overlay(*f.cell,f.drawing,{100,200},{11,13,400,300},false),game::DrawingStatus::unavailable);
    c.slope=5;f.setup(c);
    EXPECT_EQ(game::draw_cell_overlay(*f.cell,f.drawing,{100,200},{11,13,400,300},false),game::DrawingStatus::invalid_argument);
    c.slope=0;f.setup(c);TiberiumClass::Array.Count=0;
    const auto missing_resource=game::draw_cell_overlay(*f.cell,f.drawing,{100,200},{11,13,400,300},false);
    TiberiumClass::Array.Count=1;
    EXPECT_EQ(missing_resource,game::DrawingStatus::invalid_argument);
    c.flags=1;c.slope=0;c.light=0;f.setup(c);f.drawing.initialize_light=nullptr;
    EXPECT_EQ(game::draw_cell_overlay(*f.cell,f.drawing,{100,200},{11,13,400,300},false),game::DrawingStatus::unavailable);
}
