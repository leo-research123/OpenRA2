#include "bridge/type_drawing_check.hpp"
#include "type_drawing_packets.hpp"
#include "yrpp/OverlayTypeClass.h"
#include "yrpp/SmudgeTypeClass.h"
#include "yrpp/IsometricTileTypeClass.h"
#include "yrpp/FileFormats/SHP.h"
#include "yrpp/Memory.h"
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <cstring>
#include <sstream>
#include <stdexcept>
namespace {
using game::DrawingStatus;
void put32(std::vector<byte>& b, std::size_t at, std::uint32_t v) {
    for(int n=0;n<4;++n) b.at(at+n)=byte(v>>(n*8));
}
std::vector<byte> shape_fixture() {
    constexpr int w=48,h=32,frames=3;
    const std::size_t header=sizeof(SHPStruct)+frames*sizeof(SHPFrame);
    std::vector<byte> bytes(header+frames*w*h,0);
    auto* s=new (bytes.data()) SHPStruct;
    s->Width=w;s->Height=h;s->Frames=frames;
    auto* f=reinterpret_cast<SHPFrame*>(bytes.data()+sizeof(SHPStruct));
    for (int k=0;k<frames;++k) {
        f[k].Width=w;f[k].Height=h;f[k].Offset=int(header+k*w*h);
        for(int y=0;y<h;++y) for(int x=0;x<w;++x)
            if(x>k+2 && x<w-k-3 && y>2 && y<h-3 && ((x/6+y/4+k)%3)!=0)
                bytes[header+k*w*h+y*w+x]=byte(32+(x/6+k*4+y/4)%16);
    }
    return bytes;
}
std::vector<byte> tile_fixture() {
    constexpr std::size_t base=20, pixels=base+sizeof(TMPImage), z=pixels+900, extra=z+900, ez=extra+120;
    std::vector<byte> b(ez+120,0);
    put32(b,0,1);put32(b,4,1);put32(b,8,60);put32(b,12,30);put32(b,16,base);
    auto* im=reinterpret_cast<TMPImage*>(b.data()+base);
    im->Flags=3;im->ZOffset=int(z-base);im->ExtraOffset=int(extra-base);im->ExtraZOffset=int(ez-base);
    im->ExtraX=20;im->ExtraY=-10;im->ExtraWidth=20;im->ExtraHeight=6;
    for(int i=0;i<900;++i) { b[pixels+i]=byte(48+i%16);b[z+i]=byte((i/30)%20); }
    for(int i=0;i<120;++i) { b[extra+i]=(i%5) ? byte(80+i%8) : 0;b[ez+i]=byte(20+i/20); }
    return b;
}
struct Host {
    const godot::Callable* sink;
    int palette_token=0,target_token=0;
    int intensity=1000;
    game::TypeGpuTarget target{384,224,33,0,65535,true};
    DrawingStatus emit(const game::TypeGpuPacket& packet) {
        if(packet.texels.empty()) return DrawingStatus::skipped;
        godot::PackedByteArray params,texels;
        params.resize(80);texels.resize(std::int64_t(packet.texels.size())*4);
        for(std::size_t i=0;i<20;++i) for(int n=0;n<4;++n)
            params.ptrw()[i*4+n]=byte(std::uint32_t(packet.parameters[i])>>(n*8));
        for(std::size_t i=0;i<packet.texels.size();++i) for(int n=0;n<4;++n)
            texels.ptrw()[i*4+n]=byte(packet.texels[i]>>(n*8));
        const godot::Variant result=sink->call(params,texels);
        if(result.get_type()!=godot::Variant::INT) return DrawingStatus::backend_failure;
        const auto code=std::int64_t(result);
        return code>=0 && code<=5 ? static_cast<DrawingStatus>(code) : DrawingStatus::backend_failure;
    }
    bool valid(game::DrawingTargetHandle* t,const game::DrawingPaletteHandle* p) {
        return t==reinterpret_cast<game::DrawingTargetHandle*>(&target_token) &&
            p==reinterpret_cast<game::DrawingPaletteHandle*>(&palette_token);
    }
};
DrawingStatus shape(void* user,const game::ShapeDrawingRequest& r) {
    auto& h=*static_cast<Host*>(user);if(!h.valid(r.target,r.palette))return DrawingStatus::invalid_argument;
    game::TypeGpuPacket p;const auto status=game::prepare_type_shape(r,h.target,p);
    return status==DrawingStatus::drawn ? h.emit(p) : status;
}
DrawingStatus tile(void* user,const game::TileDrawingRequest& r) {
    auto& h=*static_cast<Host*>(user);if(!h.valid(r.target,r.palette))return DrawingStatus::invalid_argument;
    game::TypeGpuPacket p,e;auto status=game::prepare_type_tile(r,h.target,p,e);
    if(status!=DrawingStatus::drawn)return status;
    status=h.emit(p);
    if(status!=DrawingStatus::drawn && status!=DrawingStatus::skipped)return status;
    const auto tail=h.emit(e);
    return tail==DrawingStatus::skipped ? status : tail;
}
}
void RA2TypeDrawingCheck::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("render_packets","sink","frame","intensity","depth","unsupported_mode","clip"),
        &RA2TypeDrawingCheck::render_packets);
}
godot::String RA2TypeDrawingCheck::render_packets(const godot::Callable& sink,int frame,int intensity,
        bool depth,bool unsupported_mode,godot::Rect2i clip) {
    try {
        if(!sink.is_valid()) return "unavailable: GPU sink not bound";
        Host host{&sink};host.intensity=intensity;
        game::TypeDrawingContext c;c.backend.shape=shape;c.backend.tile=tile;c.backend_context=&host;
        c.target=reinterpret_cast<game::DrawingTargetHandle*>(&host.target_token);
        c.palette=reinterpret_cast<game::DrawingPaletteHandle*>(&host.palette_token);
        c.world_context=&host;
        c.smudge_cell=[](void* p,const CellStruct& cell,const game::DrawingPaletteHandle*& palette,int& light) {
            // Explicit synthetic test cell, not a replacement MapClass.
            if(cell!=CellStruct{2,3})return DrawingStatus::unavailable;
            auto& h=*static_cast<Host*>(p);palette=reinterpret_cast<game::DrawingPaletteHandle*>(&h.palette_token);
            light=h.intensity;return DrawingStatus::drawn;
        };
        auto shp=shape_fixture();auto tmp=tile_fixture();
        OverlayTypeClass overlay("GPU-CHECK-OVERLAY");overlay.Image=reinterpret_cast<SHPStruct*>(shp.data());
        SmudgeTypeClass smudge("GPU-CHECK-SMUDGE");
        auto* owned=YRMemory::Allocate(shp.size());if(!owned)throw std::bad_alloc();
        new (owned) SHPStruct; std::memcpy(owned,shp.data(),shp.size());smudge.Image=static_cast<SHPStruct*>(owned);smudge.Width=1;smudge.Height=1;
        IsometricTileTypeClass iso(0,0,0,"gpucheck",0);
        if(!iso.ReadTMP(tmp.data(),tmp.size()))throw std::runtime_error("TMP fixture validation failed");
        const RectangleStruct bounds{clip.position.x,clip.position.y,clip.size.x,clip.size.y};
        const auto a=overlay.Draw(c,{30,30},bounds,(frame%3+3)%3);
        const auto b=smudge.DrawIt(c,{160,50},bounds,(frame%3+3)%3,0,{2,3});
        const auto d=iso.DrawTMP(c,0,100,140,bounds,0,intensity,depth,0,false,false,unsupported_mode,0);
        const auto e=iso.DrawTMP(c,0,116,148,bounds,2,intensity,depth,0,false,false,unsupported_mode,0);
        std::ostringstream out;out<<"Overlay: "<<game::drawing_status_name(a)<<" | Smudge: "<<game::drawing_status_name(b)
            <<" | TMP: "<<game::drawing_status_name(d)<<", "<<game::drawing_status_name(e);
        return godot::String::utf8(out.str().c_str());
    } catch(const std::exception& e) { return godot::String("backend_failure: ")+godot::String::utf8(e.what()); }
    catch(...) { return "backend_failure: unknown native exception"; }
}
