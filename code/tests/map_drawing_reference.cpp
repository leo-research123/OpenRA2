// Real-map software reference for the production GPU frame. This executable
// links the optional rasterizer; the Godot extension continues to link core only.
#include "api/filesystem.hpp"
#include "api/clock.hpp"
#include "api/map_view.hpp"
#include "api/software_type_drawing.hpp"
#include "map_view.hpp"
#include "map_world_internal.hpp"
#include "tactical_drawing.hpp"
#include "yrpp/Unsorted.h"
#include "yrpp/RadarClass.h"
#include "yrpp/Surface.h"
#include "yrpp/DrawingBuffers.h"
#include "yrpp/Drawing.h"
#include "yrpp/ConvertClass.h"
#include "yrpp/ColorScheme.h"
#include "yrpp/FileSystem.h"
#include "yrpp/UnitClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/BulletClass.h"
#include "yrpp/BulletTypeClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/LineTrail.h"
#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <vector>
#include <cstring>

namespace {
game::DrawingStatus (*software_raster)(void*,const game::RasterDrawingRequest&)=nullptr;
int trail_requests=0,trail_drawn=0;
game::DrawingStatus trace_raster(void* context,const game::RasterDrawingRequest& request) {
    const auto result=software_raster(context,request);
    if(request.blend_mode==game::RasterBlendMode::depth_alpha){
        ++trail_requests;if(result==game::DrawingStatus::drawn)++trail_drawn;
    }
    return result;
}
game::DrawingStatus (*software_shape)(void*,const game::ShapeDrawingRequest&)=nullptr;
game::DrawingStatus trace_shape(void* context,const game::ShapeDrawingRequest& request) {
    try { return software_shape(context,request); }
    catch (const std::exception& e) {
        const auto* reference=request.image ? request.image->AsReference() : nullptr;
        std::cerr<<"SHP "<<(reference ? reference->Filename : "<raw>")<<": "<<e.what()<<'\n';
        return game::DrawingStatus::backend_failure;
    }
}
struct Palettes {
    std::map<std::pair<const BytePalette*,std::array<int,4>>,std::unique_ptr<ConvertClass>> values;
    static game::DrawingStatus plain(void* context,const BytePalette& source,
        const game::DrawingPaletteHandle*& output) noexcept {
        return standard(context,source,1,output);
    }
    static game::DrawingStatus standard(void* context,const BytePalette& source,int shades,
        const game::DrawingPaletteHandle*& output) noexcept {
        auto& self=*static_cast<Palettes*>(context);
        try {
            auto& value=self.values[{&source,{-1,-1,-1,shades}}];
            if (!value) value=std::make_unique<ConvertClass>(source,source,2,shades,false);
            output=reinterpret_cast<const game::DrawingPaletteHandle*>(value.get());
            return game::DrawingStatus::drawn;
        } catch (...) { return game::DrawingStatus::backend_failure; }
    }
    static game::DrawingStatus scheme(void* context,const BytePalette& source,int shades,
        const game::DrawingPaletteHandle*& output) noexcept {
        auto& self=*static_cast<Palettes*>(context);
        try {
            auto& value=self.values[{&source,{-2,-2,-2,shades}}];
            if(!value){
                auto converted=std::make_unique<ConvertClass>(source,source,2,shades,false);
                if(!ColorScheme::BuildColorTable(source,static_cast<WORD*>(converted->FullColorData),
                    std::size_t(shades)*256,shades,2,false))return game::DrawingStatus::backend_failure;
                value=std::move(converted);
            }
            output=reinterpret_cast<const game::DrawingPaletteHandle*>(value.get());
            return game::DrawingStatus::drawn;
        }catch(...){return game::DrawingStatus::backend_failure;}
    }
    static game::DrawingStatus resolve(void* context,const BytePalette& source,int r,int g,int b,int shades,
        const game::DrawingPaletteHandle*& output) noexcept {
        auto& self=*static_cast<Palettes*>(context);
        try {
            const auto key=std::make_pair(&source,std::array<int,4>{r,g,b,shades});
            auto found=self.values.find(key);
            if (found==self.values.end()) {
                auto value=std::make_unique<LightConvertClass>(const_cast<BytePalette*>(&source),
                    const_cast<BytePalette*>(&FileSystem::TEMPERAT_PAL),2,r,g,b,true,nullptr,shades);
                found=self.values.emplace(key,std::move(value)).first;
            }
            output=reinterpret_cast<const game::DrawingPaletteHandle*>(found->second.get());
            return game::DrawingStatus::drawn;
        } catch (...) { return game::DrawingStatus::backend_failure; }
    }
};
}
int main(int argc,char** argv) {
    bool ui=false,radar=false,radarActive=false,radarNames=false,center=false,ifvTrail=false,panCheck=false,backgroundOnly=false,tilesOnly=false;int side=0,ticks=0,centerX=0,centerY=0;
    try {
        if(argc<4)throw std::invalid_argument("arguments");
        for(int i=4;i<argc;++i){
            const std::string flag=argv[i];
            if(flag=="--ui")ui=true;
            else if(flag=="--pan-check")panCheck=true;
            else if(flag=="--background-only")backgroundOnly=true;
            else if(flag=="--tiles-only"){backgroundOnly=true;tilesOnly=true;}
            else if(flag=="--ifv-trail")ifvTrail=true;
            else if(flag=="--radar")radar=true;
            else if(flag=="--radar-active"){radarActive=true;ui=true;}
            else if(flag=="--radar-names"){radarNames=true;ui=true;}
            else if(flag.rfind("--side=",0)==0)side=std::stoi(flag.substr(7));
            else if(flag=="--ticks"&&i+1<argc)ticks=std::stoi(argv[++i]);
            else if(flag=="--center"&&i+2<argc){centerX=std::stoi(argv[++i]);centerY=std::stoi(argv[++i]);center=true;}
            else throw std::invalid_argument("option");
        }
        if(side<0||side>2||ticks<0)throw std::invalid_argument("range");
    }catch(...){
        std::cerr << "usage: map_drawing_reference GAME_DATA MAP OUTPUT.rgb565 [--radar|--ui [--side=0|1|2]] [--radar-active|--radar-names] [--ticks N] [--center X Y] [--ifv-trail] [--pan-check] [--background-only|--tiles-only]\n";return 2;
    }
    game::ResourceHandle* resource=nullptr; game::MapViewHandle* map=nullptr; std::string error;
    struct Cleanup {
        game::ResourceHandle*& resource; game::MapViewHandle*& map;
        ~Cleanup() { game::destroy_map_view(map); game::destroy_resources(resource); }
    } cleanup{resource,map};
    try {
        if (!game::create_resources(argv[1],resource,error) ||
            game::load_resources(*resource,{},error)!=game::ResourceLoadResult::complete ||
            !game::create_map_view(*resource,map) || !game::load_map_view(*map,argv[2],std::char_traits<char>::length(argv[2]))) {
            std::cerr << (map ? game::map_view_error(*map) : error.c_str()) << '\n'; return 1;
        }
        constexpr int width=1280,height=720;
        // Test fixture only. Production identity comes from Scenario startup;
        // neither the Godot bridge nor the public API gains a side override.
        if (!game::with_map_view(*map,[](void* value) {
            ScenarioClass::Instance->PlayerSideIndex=*static_cast<int*>(value);
        },&side)) return 1;
        struct Advance {game::MapViewHandle& view;int ticks;} advance{*map,ticks};
        if(ticks&&!game::with_map_view(*map,[](void* opaque){
            auto& state=*static_cast<Advance*>(opaque);
            for(int i=0;i<state.ticks;++i){++Unsorted::CurrentFrame;game::update_map_world(*state.view.world);}
        },&advance)){std::cerr<<game::map_view_error(*map)<<'\n';return 1;}
        if (radar) {
            game::MapRadarInfo info{};
            if (!game::get_map_radar_info(*map,info)) return 1;
            std::vector<std::uint16_t> pixels(info.width*info.height);
            if (!game::copy_map_radar_pixels(*map,pixels.data(),pixels.size())) return 1;
            std::ofstream output(argv[3],std::ios::binary);
            output.write(reinterpret_cast<const char*>(pixels.data()),pixels.size()*sizeof(std::uint16_t));
            if (!output) return 1;
            std::cout<<"RADAR_SIZE "<<info.width<<' '<<info.height<<'\n';
            return 0;
        }
        BSurface target(width,height,2); target.Fill(0);
        int gradients[5][6]{};
        if (ui) {
            std::ifstream table(RA2_UI_Z_GRADIENTS); std::string comment;
            std::getline(table,comment);
            for (auto& row : gradients) for (auto& value : row) table>>value;
            if (!table) { std::cerr<<"missing original SHP gradient fixture\n"; return 1; }
            Drawing::ZGradientTable=gradients;
        }
        struct GradientScope { ~GradientScope() { Drawing::ZGradientTable=nullptr; } } gradient_scope;
        ABuffer alpha({0,0,width,height}); ZBuffer depth({0,0,width,height});
        ABuffer::Instance=&alpha; ZBuffer::Instance=&depth;
        struct Buffers {
            ABuffer& a; ZBuffer& z;
            ~Buffers() { ABuffer::Instance=nullptr; ZBuffer::Instance=nullptr; a.ReleaseSurface(); z.ReleaseSurface(); }
        } buffers{alpha,depth};
        LightConvertClass::LightMode=2; LightConvertClass::UseMMX=0;
        Palettes palettes;
        game::MapDrawingContext context;
        context.types=game::make_software_type_drawing(&target,nullptr,nullptr);
        if(ifvTrail){software_raster=context.types.backend.raster;context.types.backend.raster=trace_raster;}
        software_shape=context.types.backend.shape;
        context.types.backend.shape=trace_shape;
        context.types.backend_context=&palettes;
        context.terrain_palette=Palettes::resolve;
        context.plain_palette=Palettes::plain;
        context.shape_palette=Palettes::standard;
        context.color_scheme_palette=Palettes::scheme;
        if (!(ui ? game::set_game_view_size(*map,width,height) : game::set_map_viewport(*map,width,height))) return 1;
        if(ifvTrail){
            Point2D focus{};
            if(!game::with_map_view(*map,[](void* p){
                UnitClass* owner=nullptr;for(auto* unit:UnitClass::Array)if(!std::strcmp(unit->Type->ID,"FV")){owner=unit;break;}
                if(!owner)throw std::runtime_error("IFV missing");
                auto* weapon=WeaponTypeClass::Find("HoverMissile");
                if(!weapon||!weapon->Projectile)throw std::runtime_error("IFV weapon missing");
                auto* bullet=weapon->Projectile->CreateBullet(owner->GetCell(),owner,weapon->Damage,weapon->Warhead,weapon->Speed,false);
                if(!bullet)throw std::runtime_error("IFV missile allocation failed");
                bullet->SetWeaponType(weapon);
                if(!bullet->MoveTo(owner->Location,{1.0,0.0,0.0}))throw std::runtime_error("IFV missile placement failed");
                if(!bullet->LineTrailer)throw std::runtime_error("IFV trail missing");
                // Deterministic in-flight samples, keeping simulation/AI out
                // of this whole-scene visibility probe.
                for(int i=0;i<12;++i){bullet->Location.X+=40;bullet->Location.Z=owner->Location.Z+512;LineTrail::UpdateAll();}
                *static_cast<Point2D*>(p)=TacticalClass::CoordsToScreen(bullet->Location);
            },&focus))throw std::runtime_error(game::map_view_error(*map));
            if(!game::center_map_view(*map,focus.X,focus.Y))return 1;
        }
        if(center&&!game::center_map_view(*map,centerX,centerY))return 1;
        game::MapDrawStatistics stats;
        const auto render=[&]{
            if(!backgroundOnly)return ui?game::draw_game_view(*map,context,stats):game::draw_map_view(*map,context,stats);
            game::TacticalDrawingFrame frame{map->world.get(),&context,{},&stats};
            struct Background {game::TacticalDrawingFrame& frame;bool tilesOnly;} background{frame,tilesOnly};
            game::with_map_view(*map,[](void* p){auto& pass=*static_cast<Background*>(p);auto& f=pass.frame;f.bounds=TacticalClass::ViewBounds;
                game::with_tactical_drawing(f,[](void* p){auto* f=game::tactical_drawing();auto* t=TacticalClass::Instance;
                    t->DrawShroud(f->bounds);t->DrawTiles(f->bounds,f->bounds);
                    if(static_cast<Background*>(p)->tilesOnly)return;
                    if(f->status!=game::DrawingStatus::drawn&&f->status!=game::DrawingStatus::skipped)return;
                    t->BuildBackgroundDrawRequests();
                    if(f->status!=game::DrawingStatus::drawn&&f->status!=game::DrawingStatus::skipped)return;
                    for(auto& r:f->world->impl->sprites){const auto status=game::submit_world_sprite(r,*f->drawing,f->bounds,*f->statistics);
                        game::record_tactical_drawing(status);if(status!=game::DrawingStatus::drawn&&status!=game::DrawingStatus::skipped)return;}
                },p);
            },&background);return frame.status;
        };
        if(radarActive || radarNames) {
            // Presentation fixture: drive the original opening transition with
            // authoritative system time, without directly assigning frame 32.
            if(!game::with_map_view(*map,[](void* p){
                if(*static_cast<bool*>(p))RadarClass::Instance.SetRadarMode(2,false);
                else RadarClass::Instance.SetRadarAvailability(true);
            },&radarNames))
                throw std::runtime_error("radar activation failed");
            for(int i=0;i<32;++i) {
                if(!game::advance_clock(0.064))throw std::runtime_error("radar clock advance failed");
                // A fresh color frame must also start with fresh occlusion
                // buffers; otherwise earlier fixture draws hide terrain.
                target.Fill(0);alpha.Fill(127);depth.Fill(0xFFFF);
                const auto status=render();
                if(status!=game::DrawingStatus::drawn&&status!=game::DrawingStatus::skipped)
                    throw std::runtime_error(game::map_view_error(*map));
            }
            if(RadarClass::Instance.unknown_14AC!=1||RadarClass::Instance.unknown_14FC!=32)
                throw std::runtime_error("radar opening did not complete");
        }
        const auto result=render();
        if(ifvTrail)std::cout<<"TRAIL_PIXELS submitted="<<trail_requests<<" drawn="<<trail_drawn<<'\n';
        if(ifvTrail&&!trail_drawn)throw std::runtime_error("IFV trail produced no visible pixels");
        if (result!=game::DrawingStatus::drawn) {
            std::cerr << game::drawing_status_name(result)<<": "<<game::map_view_error(*map)<<'\n'; return 1;
        }
        if(panCheck){
            // No simulation tick or input event occurs between these draws.
            // This separates camera/background errors from moving animations.
            const auto capture=[&]{std::vector<WORD> out(width*height);auto* data=static_cast<const byte*>(target.Lock(0,0));
                for(int y=0;y<height;++y)std::memcpy(out.data()+y*width,data+y*target.GetPitch(),width*2);target.Unlock();return out;};
            const auto original=capture();const auto camera=map->tactical.TacticalPos;
            game::GameViewLayout layout;game::get_game_view_layout(*map,layout);
            const int mapWidth=ui?layout.map.Width:width,mapHeight=ui?layout.map.Height:height;
            const auto redraw=[&]{target.Fill(0);alpha.Fill(127);depth.Fill(0xFFFF);
                const auto status=render();
                if(status!=game::DrawingStatus::drawn)throw std::runtime_error(game::map_view_error(*map));};
            if(!game::scroll_map_view(*map,80,-40))throw std::runtime_error("pan failed");
            redraw();const auto moved=capture();const auto shift=map->tactical.TacticalPos-camera;
            int differences=0;std::map<std::pair<int,int>,int> blocks;
            for(int y=0;y<mapHeight;++y)for(int x=0;x<mapWidth;++x){const int sx=x+shift.X,sy=y+shift.Y;
                if(sx>=0&&sy>=0&&sx<mapWidth&&sy<mapHeight&&moved[y*width+x]!=original[sy*width+sx]){++differences;++blocks[{x/64,y/64}];}}
            if(!game::scroll_map_view(*map,-shift.X,-shift.Y))throw std::runtime_error("return pan failed");
            redraw();const auto restored=capture();int roundTrip=0;
            for(int y=0;y<mapHeight;++y)for(int x=0;x<mapWidth;++x)if(restored[y*width+x]!=original[y*width+x])++roundTrip;
            std::cout<<"FROZEN_PAN differences="<<differences<<" round_trip="<<roundTrip<<'\n';
            if(differences)for(const auto& [block,count]:blocks)std::cout<<"BLOCK "<<block.first*64<<","<<block.second*64<<" count="<<count<<'\n';
            if(differences||roundTrip){
                // Preserve exact failure evidence without changing acceptance.
                const auto save=[&](const char* suffix,const std::vector<WORD>& pixels){
                    std::ofstream out(std::string(argv[3])+suffix,std::ios::binary);
                    out.write(reinterpret_cast<const char*>(pixels.data()),pixels.size()*sizeof(WORD));};
                save(".original",original);save(".moved",moved);save(".restored",restored);
                std::cout<<"PAN_SHIFT "<<shift.X<<' '<<shift.Y<<'\n';
                throw std::runtime_error("frozen map pixels changed");
            }
        }
        const auto* pixels=static_cast<const char*>(target.Lock(0,0));
        if (!pixels) return 1;
        std::ofstream output(argv[3],std::ios::binary);
        for (int y=0;y<height;++y) output.write(pixels+y*target.GetPitch(),width*2);
        target.Unlock();
        if (!output) return 1;
        std::cout << argv[2] << ": visited=" << stats.visited << ", drawn=" << stats.drawn << ", skipped=" << stats.skipped << '\n';
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
