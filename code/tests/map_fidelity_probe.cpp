// Diagnostic executable only. Explicitly permitted internal access is listed in
// cmake/Tests.cmake. This is not a new public host API or a second object model.
#include "api/filesystem.hpp"
#include "api/map_objects.hpp"
#include "map_view.hpp"
#include "map_world_internal.hpp"
#include "building_selection.hpp"
#include "building_voxel.hpp"
#include "yrpp/BuildingClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/TerrainClass.h"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/CCFileClass.h"
#include "type_resources.hpp"
#include "yrpp/FileSystem.h"
#include <charconv>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <stdexcept>
#include <exception>

namespace {
std::string quote(const char* text) {
    std::ostringstream out; out << '"';
    for (auto* p = reinterpret_cast<const unsigned char*>(text ? text : ""); *p; ++p) {
        if (*p == '"' || *p == '\\') out << '\\' << char(*p);
        else if (*p < 0x20) {
            const char hex[] = "0123456789abcdef";
            out << "\\u00" << hex[*p >> 4] << hex[*p & 15];
        } else out << char(*p);
    }
    return out.str() + '"';
}
int frame_count(SHPStruct* shape) {
    auto* data = shape ? shape->GetData() : nullptr;
    return data ? data->Frames : 0;
}
void shape(std::ostream& out, const char* key, const char* configured, SHPStruct* image) {
    out << "{\"part\":" << quote(key) << ",\"configured\":" << quote(configured)
        << ",\"loaded\":" << (frame_count(image)>0 ? "true" : "false")
        << ",\"frames\":" << frame_count(image) << '}';
}
void dump(game::MapViewHandle& view, std::ostream& out) {
    auto& world = *view.world;
    auto& w = *world.impl;
    out << "{\n\"overlays\":[";
    bool overlay_first=true;
    for(int i=0;i<OverlayTypeClass::Array.Count;++i){auto*t=OverlayTypeClass::Array[i];unsigned count=0;int min_frame=255,max_frame=0;
        for(auto*c:w.decorated_cells)if(c->OverlayTypeIndex==i){++count;min_frame=std::min(min_frame,int(c->OverlayData));max_frame=std::max(max_frame,int(c->OverlayData));}
        if(!count)continue;if(!overlay_first)out<<',';overlay_first=false;
        char requested[260]{};game::type_image_filename(*t,requested,sizeof(requested),true);
        CCFileClass image_file(requested);
        out<<"{\"requested_image\":"<<quote(requested)<<",\"image_exists\":"<<(image_file.Exists(false)?"true":"false")
            <<",\"new_theater\":"<<(t->NewTheater?"true":"false")<<",\"demand_load\":"<<(t->ImageLoaded?"true":"false")<<",\"id\":"<<quote(t->ID)<<",\"index\":"<<i<<",\"cells\":"<<count
            <<",\"wall\":"<<(t->Wall?"true":"false")<<",\"theater\":"<<(t->Theater?"true":"false")
            <<",\"image\":"<<quote(t->ImageFile)<<",\"frames\":"<<frame_count(t->Image)
            <<",\"min_frame\":"<<min_frame<<",\"max_frame\":"<<max_frame<<'}';
    }
    out << "],\n\"format\":2,\n\"simulation_tick\":" << world.simulation_tick
        << ",\n\"voxel_palette_loaded\":" << (w.voxel_palette_loaded?"true":"false")
        << ",\n\"selection_palette_loaded\":" << (w.selection_palette_loaded?"true":"false")
        << ",\n\"limits\":[\"Factory composite state rendering remains incomplete\","
           "\"Health bar and owner-color ramp still differ from the original\","
           "\"VXL cast-shadow pass and real-asset GPU pixel equivalence are not verified\","
           "\"Resource due-time scheduling is not the EXE priority heap\"],\n\"buildings\":[\n";
    bool first = true;
    for (int i=w.buildings; i<BuildingClass::Array.Count; ++i) {
        const auto& b = *BuildingClass::Array[i];
        if (!b.Type) continue;
        const auto& t = *b.Type;
        if (!first) out << ",\n"; first=false;
        char alias[64]{};
        world.art_ini.ReadString(t.ImageFile,"Image","",alias,sizeof(alias));
        out << "{\"id\":" << b.UniqueID << ",\"type_id\":" << quote(t.ID)
            << ",\"art_section\":" << quote(t.ImageFile)
            << ",\"art_image\":" << quote(alias)
            << ",\"resolved_body\":" << quote(t.TheaterSpecificID)
            << ",\"cell\":[" << b.Location.X/256 << ',' << b.Location.Y/256 << ']'
            << ",\"health\":" << b.Health << ",\"max_health\":" << t.Strength
            << ",\"state_frame\":" << b.GetShapeNumber()
            << ",\"body_frames\":" << frame_count(t.Image)
            << ",\"remapable\":" << (t.Remapable?"true":"false")
            << ",\"turret_enabled\":" << (t.Turret?"true":"false")
            << ",\"turret_voxel\":" << (t.TurretAnimIsVoxel?"true":"false")
            << ",\"unsupported_voxel_turret\":false"
            << ",\"voxel_barrel\":" << quote(t.VoxelBarrelFile);
        game::BuildingSelectionGeometry selection;
        const bool geometry_ok=game::building_selection_geometry(b,selection);
        out << ",\"selection_edges\":" << (geometry_ok?selection.count:0)
            << ",\"selection_palette_index\":" << selection.palette_index;
        const auto resource=[&](const char* name,const VoxelStruct& pair){
            out << "," << quote(name) << ":{\"vxl_loaded\":"
                << (pair.VXL&&!pair.VXL->Initialized?"true":"false")
                << ",\"hva_loaded\":" << (pair.HVA&&!pair.HVA->LoadedFailed?"true":"false")
                << ",\"hva_frames\":" << (pair.HVA?pair.HVA->FrameCount:0)
                << ",\"hva_layers\":" << (pair.HVA?pair.HVA->LayerCount:0) << '}';
        };
        resource("native_turret_resource",t.TurretVoxel);
        resource("native_barrel_resource",t.BarrelVoxel);
        game::BuildingVoxelPlan plan;const bool planned=game::building_voxel_plan(b,plan);
        out << ",\"voxel_plan_ready\":" << (planned?"true":"false")
            << ",\"voxel_parts\":[";
        for(unsigned part=0;part<plan.count;++part){
            game::VoxelSurface surface;
            const auto status=game::render_building_voxel(plan.parts[part],w.voxel_palette,surface);
            unsigned covered=0;for(auto pixel:surface.pixels)if((pixel&0x01000000u)&&(pixel&0xFFu))++covered;
            const char* names[]={"drawn","skipped","unavailable","unsupported","invalid_argument","backend_failure"};
            if(part)out<<',';
            out << "{\"barrel\":" << (plan.parts[part].barrel?"true":"false")
                << ",\"hva_frame\":" << plan.parts[part].frame
                << ",\"raster_status\":" << quote(names[static_cast<unsigned>(status)])
                << ",\"covered_pixels\":" << covered
                << ",\"size\":[" << surface.width << ',' << surface.height << "]}";
        }
        out << "],\"parts\":[";
        const char* keys[] = {"Buildup","BibShape","DeployingAnim","RoofDeployingAnim","DoorAnim",
            "UnderDoorAnim","UnderRoofDoorAnim","SpecialZOverlay","Rubble"};
        SHPStruct* images[] = {t.Buildup,t.BibShape,t.DeployingAnim,t.RoofDeployingAnim,t.DoorAnim,
            t.UnderDoorAnim,t.UnderRoofDoorAnim,t.SpecialZOverlay,t.Rubble};
        for (int k=0;k<9;++k) {
            char name[64]{};world.art_ini.ReadString(t.ImageFile,keys[k],"",name,sizeof(name));
            if(k)out<<',';shape(out,keys[k],name,images[k]);
        }
        out << "],\"animation_slots\":[";
        for (int slot=0;slot<21;++slot) {
            const auto& cfg=t.BuildingAnim[slot];const auto* anim=b.Anims[slot];
            if(slot)out<<',';
            out << "{\"slot\":" << slot << ",\"normal\":" << quote(cfg.Anim)
                << ",\"damaged\":" << quote(cfg.Damaged) << ",\"garrisoned\":" << quote(cfg.Garrisoned)
                << ",\"position\":[" << cfg.Position.X << ',' << cfg.Position.Y << ']'
                << ",\"z_adjust\":" << cfg.ZAdjust << ",\"y_sort\":" << cfg.YSort
                << ",\"active\":" << (anim?"true":"false")
                << ",\"active_type\":" << quote(anim&&anim->Type?anim->Type->ID:"")
                << ",\"image_frames\":" << frame_count(anim&&anim->Type?anim->Type->Image:nullptr)
                << ",\"frame\":" << (anim?anim->Animation.Value:-1)
                << ",\"rate\":" << (anim?anim->Animation.Rate:0) << '}';
        }
        out << "]}";
    }
    out << "\n],\n\"terrain_objects\":" << TerrainClass::Array.Count-w.terrains << "\n}\n";
}
}
int main(int argc,char** argv) {
    std::string directory, map, output;
    int tick_count=0;bool loose=false;
    try {
        for(int i=1;i<argc;++i) {
            std::string flag=argv[i];
            if(flag=="--loose-files"){loose=true;continue;}
            if(i+1>=argc)throw std::runtime_error("missing option value");
            const std::string value=argv[++i];
            if(flag=="--game-dir")directory=value;
            else if(flag=="--map")map=value;
            else if(flag=="--out")output=value;
            else if(flag=="--ticks") {
                const auto result=std::from_chars(value.data(),value.data()+value.size(),tick_count);
                if(result.ec!=std::errc{}||result.ptr!=value.data()+value.size()||tick_count<0||tick_count>1000000)
                    throw std::runtime_error("ticks must be an integer in [0,1000000]");
            } else throw std::runtime_error("unknown option: "+flag);
        }
        if(directory.empty()||map.empty())throw std::runtime_error(
            "usage: ra2_map_fidelity_probe --game-dir PATH --map NAME [--ticks N] [--out FILE] [--loose-files]");
        game::ResourceHandle* raw=nullptr;std::string error;
        if(!game::create_resources(directory,raw,error))throw std::runtime_error(error);
        std::unique_ptr<game::ResourceHandle,decltype(&game::destroy_resources)> resources(raw,game::destroy_resources);
        // Keep SHP teardown in the host, before the file-only environment dies.
        struct Images { ~Images(){FileSystem::ClearNameCache();Destroy_All_Shapes();Unload_All_Shapes();} } images;
        if(!loose&&game::load_resources(*raw,{},error)!=game::ResourceLoadResult::complete)throw std::runtime_error(error);
        game::MapViewHandle* view=nullptr;
        if(!game::create_map_view(*raw,view))throw std::runtime_error("cannot create map view");
        struct Close {game::MapViewHandle*& view;~Close(){game::destroy_map_view(view);}} close{view};
        if(!game::load_map_view(*view,map.c_str(),map.size()))throw std::runtime_error(game::map_view_error(*view));
        for(int i=0;i<tick_count;++i)if(!game::update_game_view(*view,1.0/60.0))throw std::runtime_error("native update failed");
        std::ostringstream text;
        struct Call {game::MapViewHandle& view;std::ostream& out;std::exception_ptr error;} call{*view,text,{}};
        const bool ok=game::with_map_view(*view,[](void* p){auto& c=*static_cast<Call*>(p);
            try{dump(c.view,c.out);}catch(...){c.error=std::current_exception();}},&call);
        if(call.error)std::rethrow_exception(call.error);
        if(!ok)throw std::runtime_error("native diagnostic scope failed");
        if(output.empty())std::cout<<text.str();
        else{std::ofstream file(output,std::ios::binary);file<<text.str();if(!file)throw std::runtime_error("cannot write output");}
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
