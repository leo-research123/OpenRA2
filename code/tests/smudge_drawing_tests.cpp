#include "support/test_support.hpp"
// Integration regression for 0x00480350 -> 0x006B55F0. Uses the production
// Cell/Smudge classes and GPU packet preparation, without a Godot dependency.
#include "api/map_view.hpp"
#include "tactical_drawing.hpp"
#include "type_drawing_packets.hpp"
#include "yrpp/CellClass.h"
#include "yrpp/SmudgeTypeClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/TacticalClass.h"
#include "yrpp/ConvertClass.h"
#include "yrpp/FileSystem.h"
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {

struct Recorder {
    game::ShapeDrawingRequest shape{};
    game::TileDrawingRequest tile{};
    game::TypeGpuPacket packet;
    std::vector<char> order;
    int palette_calls=0,red=0,green=0,blue=0,shades=0;
    game::DrawingStatus shape_result=game::DrawingStatus::drawn;
};
TEST(SmudgeDrawing, Contracts) {
    std::vector<byte> tmp(sizeof(TMPStruct)+sizeof(int)+sizeof(TMPImage)+900,0);
    TMPStruct header{1,1,60,30};std::memcpy(tmp.data(),&header,sizeof(header));
    const int offset=sizeof(TMPStruct)+sizeof(int);
    std::memcpy(tmp.data()+sizeof(TMPStruct),&offset,sizeof(offset));
    IsometricTileTypeClass tile(0,0,0,"SMUDGE-FLOOR",0);
    EXPECT_TRUE((tile.ReadTMP(tmp.data(),tmp.size()))) << "load native TMP directory";
    const auto destroy=[](CellClass* cell){GameDelete(cell);};
    std::unique_ptr<CellClass,decltype(destroy)> owned_cell(CellClass::Create(),destroy);
    EXPECT_TRUE((bool(owned_cell))) << "allocate cell";
    auto& cell=*owned_cell;cell.MapCoords={4,7};cell.IsoTileTypeIndex=tile.ArrayIndex;
    cell.Intensity_Normal=321;cell.Intensity_Terrain=1234;
    cell.Color2_Red=850;cell.Color2_Green=950;cell.Color2_Blue=1100;
    SmudgeTypeClass smudge("SMUDGE-TEST");
    const int image_bytes=sizeof(SHPStruct)+sizeof(SHPFrame)+60*60;
    auto* storage=static_cast<byte*>(YRMemory::Allocate(image_bytes));EXPECT_TRUE((storage)) << "allocate SHP";
    std::memset(storage,0,image_bytes);
    smudge.Image=new(storage) SHPStruct;
    smudge.Image->Width=60;smudge.Image->Height=60;smudge.Image->Frames=1;
    auto* frame=reinterpret_cast<SHPFrame*>(storage+sizeof(SHPStruct));
    frame->Width=60;frame->Height=60;frame->Offset=sizeof(SHPStruct)+sizeof(SHPFrame);
    std::memset(storage+frame->Offset,42,60*60);
    cell.SmudgeTypeIndex=smudge.ArrayIndex;
    Recorder recorded;
    game::MapDrawingContext context;context.types.backend_context=&recorded;
    context.types.target=reinterpret_cast<game::DrawingTargetHandle*>(&recorded);
    context.types.overlay_offset={0,9};
    context.terrain_palette=[](void* pointer,const BytePalette& source,int r,int g,int b,int shades,
            const game::DrawingPaletteHandle*& palette) noexcept {
        auto& state=*static_cast<Recorder*>(pointer);++state.palette_calls;
        state.red=r;state.green=g;state.blue=b;state.shades=shades;
        if(&source!=&FileSystem::ISOx_PAL)return game::DrawingStatus::invalid_argument;
        palette=reinterpret_cast<const game::DrawingPaletteHandle*>(pointer);
        return game::DrawingStatus::drawn;
    };
    context.types.backend.tile=[](void* pointer,const game::TileDrawingRequest& request) {
        auto& state=*static_cast<Recorder*>(pointer);state.tile=request;state.order.push_back('T');
        return game::DrawingStatus::drawn;
    };
    context.types.backend.shape=[](void* pointer,const game::ShapeDrawingRequest& request) {
        auto& state=*static_cast<Recorder*>(pointer);state.shape=request;state.order.push_back('S');
        if(state.shape_result!=game::DrawingStatus::drawn)return state.shape_result;
        return game::prepare_type_shape(request,{320,240,state.shades,0,0x8000,true},state.packet);
    };
    const RectangleStruct clip{13,17,280,210};
    int cases=0;
    for(int width:{1,3})for(int data:{0,1,5})for(int level:{0,2,7})for(int slope:{0,1,7}) {
        recorded={};smudge.Width=width;cell.SmudgeData=BYTE(data);cell.Level=BYTE(level);cell.SlopeIndex=BYTE(slope);
        const Point2D point{120,140+15*level};
        EXPECT_TRUE((game::draw_cell_terrain(cell,context,point,clip)==game::DrawingStatus::drawn)) << "cell submits smudge";
        EXPECT_TRUE((recorded.order==std::vector<char>{'T','S'})) << "TMP then smudge, including slopes";
        EXPECT_TRUE((recorded.palette_calls==1&&recorded.tile.palette==recorded.shape.palette)) << "same actual cell palette";
        int red=850,green=950,blue=1100;
        const int shades=LightConvertClass::PrepareCellTint(red,green,blue,context.lighting_quality);
        EXPECT_TRUE((recorded.red==red&&recorded.green==green&&recorded.blue==blue&&recorded.shades==shades)) << "actual cell tint";
        const auto& r=recorded.shape;
        EXPECT_TRUE((r.image==smudge.Image&&r.frame==0&&r.intensity==1234)) << "single-frame SHP and terrain intensity";
        EXPECT_TRUE((r.flags==0xE00&&r.gradient==0&&r.depth_mode==game::ShapeDepthMode::legacy&&
            r.depth_adjustment==-1-TacticalClass::AdjustForZ(level*Unsorted::LevelHeight))) << "original drawing mode and height";
        EXPECT_TRUE((r.position==Point2D{137+30*(data/width-data%width),132-15*(data/width+data%width)})) << "top vertex, clip origin, draw offset and multi-cell displacement";
        EXPECT_TRUE((recorded.packet.parameters[13]==0&&recorded.packet.texels.size()==3600)) << "all 60 rows reach GPU packet without generic fixed-depth rejection";
        ++cases;
    }
    // Owning TMP is below the viewport, but the data offset moves the SHP in.
    recorded={};cell.Level=0;cell.SmudgeData=5;smudge.Width=3;
    EXPECT_TRUE((game::draw_cell_terrain(cell,context,{120,240},clip)==game::DrawingStatus::drawn&&
        recorded.order==std::vector<char>{'S'})) << "offscreen TMP must not cull visible multi-cell smudge";
    recorded={};cell.SmudgeData=0;
    EXPECT_TRUE((game::draw_cell_terrain(cell,context,{120,225},clip)==game::DrawingStatus::drawn&&
        recorded.order==std::vector<char>{'S'})) << "visible SHP can extend above offscreen TMP";
    recorded={};
    EXPECT_TRUE((game::draw_cell_terrain(cell,context,{120,400},clip)==game::DrawingStatus::skipped)) << "backend clips truly invisible SHP";
    recorded={};recorded.shape_result=game::DrawingStatus::backend_failure;
    EXPECT_TRUE((game::draw_cell_terrain(cell,context,{120,140},clip)==game::DrawingStatus::backend_failure)) << "smudge backend failures propagate";
    recorded={};smudge.Width=0;cell.SmudgeData=1;
    EXPECT_TRUE((game::draw_cell_terrain(cell,context,{120,140},clip)==game::DrawingStatus::invalid_argument)) << "invalid multi-cell width is not silently dropped";
    recorded={};cell.SmudgeTypeIndex=-1;
    EXPECT_TRUE((game::draw_cell_terrain(cell,context,{120,140},clip)==game::DrawingStatus::drawn&&
        recorded.order==std::vector<char>{'T'})) << "ordinary terrain remains unchanged";
    std::cout<<"Smudge: "<<cases<<" cell/data/height/slope cases plus clipping and failure cases passed\n";
}
}

