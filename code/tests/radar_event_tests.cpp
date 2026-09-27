#include "support/test_support.hpp"
#include "yrpp/RadarEventClass.h"
#include "yrpp/RadarClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/Surface.h"
#include "api/type_drawing.hpp"
#include "game_ui_runtime.hpp"
#include <array>
#include <bit>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
namespace {

unsigned bits(float value) {return std::bit_cast<unsigned>(value);}
TEST(RadarEvent, Contracts) {
    std::ifstream file(RA2_RADAR_EVENT_FIXTURE);EXPECT_TRUE((bool(file))) << "original event fixture exists";
    auto rules=std::make_unique<RulesClass>(); const auto previous=RulesClass::Instance;RulesClass::Instance=rules.get();
    struct Cleanup {RulesClass* previous; ~Cleanup(){RadarEventClass::Clear();RulesClass::Instance=previous;}} cleanup{previous};
    auto& radar=RadarClass::Instance;radar.RadarSizeFactor=1;radar.unknown_rect_149C={0,0,140,108};
    std::string line;unsigned updates=0,vertices=0,gradients=0,foundations=0,visibility=0,drawings=0;
    while(std::getline(file,line)) {
        if(line.empty() || line[0]=='#') continue;
        std::istringstream in(line);char type;in>>type;
        if(type=='U' || type=='V') {
            RadarEventClass::Clear();Unsorted::CurrentFrame=0;
            int kind=0,steps=0;unsigned radius,angle;
            if(type=='U') in>>kind>>radius>>angle>>steps;else in>>radius>>angle;
            EXPECT_TRUE((RadarEventClass::Create(static_cast<RadarEventType>(kind),{30,40}))) << "normal original event lifecycle";
            auto& event=*RadarEventClass::Array[0];event.Speed=std::bit_cast<float>(radius);event.RotationValue=std::bit_cast<float>(angle);
            if(type=='U') {
                for(int frame=1;frame<=steps;++frame){Unsorted::CurrentFrame=frame;event.Update();}
                const unsigned values[]={bits(event.Speed),bits(event.RotationValue),bits(event.RotationSpeed),bits(event.ColorValue),bits(event.ColorSpeed),
                    unsigned(event.DurationTimer.StartTime),unsigned(event.DurationTimer.TimeLeft),unsigned(event.VisibilityTimer.StartTime),unsigned(event.VisibilityTimer.TimeLeft),unsigned(event.Rotating),unsigned(event.Visible)};
                for(unsigned i=0;i<11;++i) {unsigned expected;in>>expected;if(values[i]!=expected) {
                    std::cerr<<"event sample "<<updates<<" field "<<i<<" expected "<<expected<<" actual "<<values[i]<<'\n';
                    throw std::runtime_error("original event state difference");
                }} ++updates;
            } else {
                Point2D output[4];EXPECT_TRUE((event.GetVertices(output,4))) << "valid event vertices";
                for(auto point:output){int x,y;in>>x>>y;if(point!=Point2D{x,y}){std::cerr<<"vertex sample "<<vertices<<" expected "<<x<<','<<y<<" actual "<<point.X<<','<<point.Y<<'\n';throw std::runtime_error("original event vertices differ");}}++vertices;
            }
        } else if(type=='G') {
            Point2D start,end;unsigned step_bits,phase_bits,expected_step,expected_phase,expected_hash;
            in>>start.X>>start.Y>>end.X>>end.Y>>step_bits>>phase_bits>>expected_step>>expected_phase>>expected_hash;
            float step=std::bit_cast<float>(step_bits),phase=std::bit_cast<float>(phase_bits);
            std::array<WORD,48*36> pixels;pixels.fill(0x1234);
            game::TypeDrawingContext drawing;drawing.target=reinterpret_cast<game::DrawingTargetHandle*>(pixels.data());drawing.backend_context=&pixels;
            drawing.backend.raster=[](void* user,const game::RasterDrawingRequest& request) {
                EXPECT_TRUE((request.position.X>=0 && request.position.X<48 && request.position.Y>=0 && request.position.Y<36)) << "clipped gradient pixels";
                auto& pixels=*static_cast<std::array<WORD,48*36>*>(user);
                pixels[request.position.Y*48+request.position.X]=request.color;return game::DrawingStatus::drawn;
            };
            const auto result=DSurface::SubmitGradientLine(drawing,{0,0,48,36},start,end,{255,0,255},{128,0,128},step,phase);
            EXPECT_TRUE((result==game::DrawingStatus::drawn || result==game::DrawingStatus::skipped)) << "gradient status";
            unsigned hash=2166136261u;
            for(auto value:pixels){hash=(hash^(value&255))*16777619u;hash=(hash^(value>>8))*16777619u;}
            if(hash!=expected_hash || bits(step)!=expected_step || bits(phase)!=expected_phase) {
                std::cerr<<"gradient "<<gradients<<" hash "<<hash<<'/'<<expected_hash<<" step "<<bits(step)<<'/'<<expected_step<<" phase "<<bits(phase)<<'/'<<expected_phase<<'\n';
                throw std::runtime_error("original gradient pixels or phase differ");
            } ++gradients;
        } else if(type=='D') {
            RadarEventClass::Clear();
            int kind,x,y;unsigned radius,angle,speed,value,expected;
            in>>kind>>x>>y>>radius>>angle>>speed>>value>>expected;
            EXPECT_TRUE((RadarEventClass::Create(static_cast<RadarEventType>(kind),{30,40}))) << "construct event for full drawing reference";
            auto& event=*RadarEventClass::Array[0];event.RadarX=x;event.RadarY=y;
            event.Speed=std::bit_cast<float>(radius);event.RotationValue=std::bit_cast<float>(angle);
            event.ColorSpeed=std::bit_cast<float>(speed);event.ColorValue=std::bit_cast<float>(value);
            DSurface::SidebarBounds={1112,158,168,562};radar.unknown_11F0=16;radar.unknown_11F4=49;
            radar.unknown_rect_149C={16,49,140,108};
            std::array<WORD,140*108> pixels;pixels.fill(0x1234);
            game::MapDrawingContext drawing;
            drawing.plain_palette=[](void*,const BytePalette&,const game::DrawingPaletteHandle*&) noexcept {
                return game::DrawingStatus::drawn; // Event RGB lines do not use a palette.
            };
            drawing.types.target=reinterpret_cast<game::DrawingTargetHandle*>(pixels.data());
            drawing.types.backend_context=&pixels;
            drawing.types.backend.raster=[](void* user,const game::RasterDrawingRequest& request) {
                const int x=request.position.X-1128,y=request.position.Y-49;
                EXPECT_TRUE((x>=0 && x<140 && y>=0 && y<108)) << "event image stays inside translated radar content";
                auto& pixels=*static_cast<std::array<WORD,140*108>*>(user);
                pixels[y*140+x]=request.color;return game::DrawingStatus::drawn;
            };
            game::UiResources resources;game::MapDrawStatistics statistics;
            game::GameUiFrame frame{drawing,resources,statistics};
            const auto status=game::with_game_ui_frame(frame,[]{RadarEventClass::Array[0]->Draw();});
            EXPECT_TRUE((status==game::DrawingStatus::drawn || status==game::DrawingStatus::skipped)) << "event drawing scope and backend succeed";
            unsigned hash=2166136261u;
            for(auto pixel:pixels){hash=(hash^(pixel&255))*16777619u;hash=(hash^(pixel>>8))*16777619u;}
            if(hash!=expected) {
                if(const char* path=std::getenv("RA2_RADAR_EVENT_DUMP")) {
                    std::ofstream image(path,std::ios::binary);image.write(reinterpret_cast<const char*>(pixels.data()),sizeof(pixels));
                }
                std::cerr<<"event image "<<drawings<<" type "<<kind<<" hash "<<hash<<'/'<<expected<<'\n';throw std::runtime_error("whole original radar event image differs");
            }
            ++drawings;
        } else if(type=='F') {
            unsigned scale,index,count,expected;in>>scale>>index>>count>>expected;
            radar.RadarSizeFactor=std::bit_cast<float>(scale);
            EXPECT_TRUE((index<22 && radar.BuildFoundationPixels())) << "original foundation mask generation";
            const auto& pixels=radar.FoundationTypePixels[index];unsigned hash=2166136261u;
            for(auto point:pixels) for(auto value:{unsigned(point.X),unsigned(point.Y)})
                for(unsigned shift=0;shift<32;shift+=8) hash=(hash^((value>>shift)&255u))*16777619u;
            EXPECT_TRUE((unsigned(pixels.Count)==count && hash==expected)) << "building foundation pixels match original x86";++foundations;
        } else if(type=='S') {
            auto& map=MapClass::Instance;
            if (!visibility) {
                EXPECT_TRUE((map.CreateEmptyCells({0,0,8,8},0))) << "normal Cells for original visibility corpus";
                for(int i=0;i<map.Cells.Capacity;++i) if(auto* cell=map.Cells[i]) {
                    const int x=cell->MapCoords.X,y=cell->MapCoords.Y;
                    cell->AltFlags=(x*17+y*13)%3 ? AltCellFlags::Mapped : AltCellFlags{};
                    cell->unknown_13C=(x+y)%5<2 ? 1 : 0;
                }
            }
            CoordStruct world;bool shroud,gap;in>>world.X>>world.Y>>world.Z>>shroud>>gap;
            EXPECT_TRUE((map.IsLocationShrouded(world)==shroud && map.IsLocationGapped(world)==gap)) << "height-aware shroud/gap match original x86";
            EXPECT_TRUE((!map.IsLocationFogged(world))) << "fixed target has disabled separate fog query";++visibility;
        } else throw std::runtime_error("unknown reference record");
        EXPECT_TRUE((bool(in))) << "complete reference row";
    }
    EXPECT_TRUE((updates==408 && vertices==300 && gradients==160 && drawings==204 && foundations==176 && visibility==400)) << "all original samples consumed";
    RadarEventClass::Clear();Unsorted::CurrentFrame=0;
    EXPECT_TRUE((RadarEventClass::Create(RadarEventType::Combat,{30,40}))) << "first combat notification";
    EXPECT_TRUE((!RadarEventClass::Create(RadarEventType::Combat,{31,41}) && RadarEventClass::Array.Count==1)) << "nearby duplicate suppressed";
    EXPECT_TRUE((RadarEventClass::Create(RadarEventType::Combat,{50,50}))) << "distant notification independent";
    EXPECT_TRUE((RadarEventClass::Create(RadarEventType::Noncombat,{30,40}) && RadarEventClass::Create(RadarEventType::Noncombat,{30,40}))) << "noncombat table allows duplicates";
    EXPECT_TRUE((!RadarEventClass::Create(static_cast<RadarEventType>(17),{0,0}))) << "invalid event rejected";
    const int before_cleanup=RadarEventClass::Array.Count;
    auto* expired=RadarEventClass::Array[before_cleanup-1];
    expired->Rotating=false; expired->DurationTimer.Start(0); expired->VisibilityTimer.Start(0);
    RadarEventClass::UpdateAll();
    EXPECT_EQ(RadarEventClass::Array.Count,before_cleanup) << "update does not erase an event before the drawing phase";
    RadarEventClass::RemoveFinished();
    EXPECT_EQ(RadarEventClass::Array.Count,before_cleanup-1);
    for(int frame=1;frame<1200;++frame){Unsorted::CurrentFrame=frame;RadarEventClass::UpdateAll();RadarEventClass::RemoveFinished();}
    EXPECT_TRUE((!RadarEventClass::Array.Count)) << "expired events removed from original Array";
    MapClass::Instance.ReleaseCellStorage();
    std::cout<<updates<<" event states, "<<vertices<<" vertex sets, "<<gradients<<" gradient images, "<<drawings<<" whole event images, "<<foundations<<" foundation masks, "<<visibility<<" shroud/gap queries match original x86\n";
}
}
