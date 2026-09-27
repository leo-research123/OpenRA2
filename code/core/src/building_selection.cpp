// Building branch recovered from the supplied YR 1.001 functions and EXE.
// Geometry: 0x006F5190 / 0x006F5EF0; dimensions: 0x00464AF0.
#include "building_selection.hpp"
#include "yrpp/BuildingClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/TacticalClass.h"
#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <cmath>
namespace game {
namespace {
thread_local BuildingHealthDrawing* health_drawing=nullptr;
int add(int a, int b) noexcept {
    return std::bit_cast<std::int32_t>(std::uint32_t(a) + std::uint32_t(b));
}
int quarter(int a, int b) noexcept {
    // The EXE adds in 32 bits, then uses signed division towards zero BEFORE
    // projection. Interpolating projected pixels changes negative rounding.
    return std::bit_cast<std::int32_t>(3u * std::uint32_t(a) + std::uint32_t(b)) / 4;
}
CoordStruct near_end(CoordStruct a, CoordStruct b) noexcept {
    return {quarter(a.X,b.X), quarter(a.Y,b.Y), quarter(a.Z,b.Z)};
}
void edge(BuildingSelectionGeometry& out, CoordStruct a, CoordStruct b) noexcept {
    if (a.Z <= b.Z) std::swap(a,b); // 0x006F5EF0: higher Z endpoint first.
    out.edges[out.count++]={a,b};
}
void corners(BuildingSelectionGeometry& out, CoordStruct a, CoordStruct b) noexcept {
    edge(out,a,near_end(a,b)); edge(out,b,near_end(b,a));
}
bool clip_line(Point2D& first,Point2D& last,int width,int height) noexcept {
    double x=first.X,y=first.Y,ex=last.X,ey=last.Y;
    // Original FPCW 0xE7F rounds each double-precision x87 operation toward
    // zero. This affects an integer endpoint (e.g. 30,10 -> 36,40 at y=35).
    const auto truncate=[](double nearest,double residual) {
        return (nearest>0 && residual<0) || (nearest<0 && residual>0) ? std::nextafter(nearest,0.0) : nearest;
    };
    const auto divide=[&](double a,double b) {
        const double nearest=a/b;
        if (!std::isfinite(nearest)) return nearest;
        const double remainder=std::fma(-nearest,b,a);
        return truncate(nearest,b<0 ? -remainder : remainder);
    };
    const double dxdy=divide(ex-x,ey-y),dydx=divide(ey-y,ex-x);
    const auto intersection=[&](double delta,double slope,double origin) {
        const double product=truncate(delta*slope,std::fma(delta,slope,-(delta*slope)));
        const double nearest=product+origin,b=nearest-product;
        return truncate(nearest,(product-(nearest-b))+(origin-b));
    };
    const auto code=[&](double a,double b) {return (a>=width ? 2 : a<0 ? 1 : 0)|(b>=height ? 4 : b<0 ? 8 : 0);};
    int a=code(x,y),b=code(ex,ey);
    for (int guard=0;guard<16;++guard) {
        if (!(a|b)) {first={int(x),int(y)};last={int(ex),int(ey)};return true;}
        if (a&b) return false;
        const int c=a ? a : b;
        double nx=x,ny=y;
        if (c&8) {nx=intersection(-y,dxdy,x);ny=0;}
        else if (c&4) {ny=height-1;nx=intersection(ny-y,dxdy,x);}
        else if (c&2) {nx=width-1;ny=intersection(nx-x,dydx,y);}
        else {nx=0;ny=intersection(-x,dydx,y);}
        if (c==a) {x=nx;y=ny;a=code(x,y);}
        else {ex=nx;ey=ny;b=code(ex,ey);}
    }
    return false;
}
DrawingStatus line(const TypeDrawingContext& context,const RectangleStruct& clip,
        Point2D a,Point2D b,int za,int zb,std::uint16_t color,bool glow=false,int strength=0,const ColorStruct* alphaColor=nullptr,int opacity=255) {
    // 0x004BFD30 sorts before clipping and interpolates integer Z using
    // truncated Euclidean distances when clipping moves either endpoint.
    if(a.X>b.X){std::swap(a,b);std::swap(za,zb);}
    const Point2D first=a,last=b;const int zfirst=za,zlast=zb;
    if(!clip_line(a,b,clip.Width,clip.Height))return DrawingStatus::skipped;
    auto distance=[](Point2D p,Point2D q){return int(std::hypot(double(p.X)-q.X,double(p.Y)-q.Y));};
    const int length=distance(first,last);
    if(length){
      if(b!=last)zb=zfirst+int(double(zlast-zfirst)*distance(first,b)/length);
      if(a!=first)za=zlast+int(double(zfirst-zlast)*distance(a,last)/length);
    }
    const int dx=b.X-a.X,dy=std::abs(b.Y-a.Y),dz=std::abs(zb-za);
    const int sy=b.Y>=a.Y?1:-1,sz=zb>=za?1:-1;
    const int major=dz>dx&&dz>dy?2:dx>dy?0:1;
    const int n=major==0?dx:major==1?dy:dz;
    if(n>65536)return DrawingStatus::invalid_argument;
    // Original >0 error rule; the terminal endpoint is NOT drawn.
    int ex=2*dx-n,ey=2*dy-n,ez=2*dz-n;
    DrawingStatus result=DrawingStatus::skipped;
    for(int i=0;i<n;++i){
      RasterDrawingRequest request;request.target=context.target;request.clip=clip;
      request.position={add(a.X,clip.X),add(a.Y,clip.Y)};
      request.width=request.height=1;request.color=color;request.light_strength=strength;
      request.original_line=!glow;request.blend_mode=glow?RasterBlendMode::depth_glow:RasterBlendMode::copy;request.line_z=za;
      if(alphaColor){request.original_line=false;request.blend_mode=RasterBlendMode::depth_alpha;
        request.line_rgb=unsigned(alphaColor->R)|(unsigned(alphaColor->G)<<8)|(unsigned(alphaColor->B)<<16);request.line_opacity=opacity;}
      const auto status=submit_type_raster(context,request);
      if(status!=DrawingStatus::drawn&&status!=DrawingStatus::skipped)return status;
      if(status==DrawingStatus::drawn)result=status;
      if(major==0||ex>0){++a.X;ex-=2*n;}ex+=2*dx;
      if(major==1||ey>0){a.Y+=sy;ey-=2*n;}ey+=2*dy;
      if(major==2||ez>0){za+=sz;ez-=2*n;}ez+=2*dz;
    }
    return result;
}
}
DrawingStatus draw_depth_alpha_line(const TypeDrawingContext& c,const RectangleStruct& clip,Point2D a,Point2D b,int za,int zb,ColorStruct color,int opacity) noexcept {
    if(opacity<8)return DrawingStatus::skipped;
    if(opacity>255||clip.Width<0||clip.Height<0||clip.Width>8192||clip.Height>8192)return DrawingStatus::invalid_argument;
    if(!clip.Width||!clip.Height)return DrawingStatus::skipped;
    try{return line(c,clip,a,b,za,zb,0,false,0,&color,opacity);}
    catch(...){return DrawingStatus::backend_failure;}
}
DrawingStatus draw_depth_glow_line(const TypeDrawingContext& c,const RectangleStruct& clip,Point2D a,Point2D b,int za,int zb,int strength) noexcept {
    try{
        // BuildingLight clips to TacticalRect before invoking DSurface with
        // its full surface rectangle: clipping here does not interpolate Z.
        if(!clip_line(a,b,clip.Width,clip.Height))return DrawingStatus::skipped;
        return line(c,clip,a,b,za+clip.Y,zb+clip.Y,0,true,strength);
    }catch(...){return DrawingStatus::backend_failure;}
}
BuildingHealthDrawing* building_health_drawing() noexcept { return health_drawing; }
DrawingStatus with_building_health_drawing(BuildingHealthDrawing& frame,void (*call)(void*),void* argument) noexcept {
    auto* previous=health_drawing;health_drawing=&frame;
    try {if(call)call(argument);else frame.status=DrawingStatus::invalid_argument;}
    catch(...){frame.status=DrawingStatus::backend_failure;}
    health_drawing=previous;return frame.status;
}
DrawingStatus draw_building_health(const BuildingClass& building,const TypeDrawingContext& drawing,
        SHPStruct* pips,const DrawingPaletteHandle* palette,Point2D point,RectangleStruct bounds) noexcept {
    BuildingHealthDrawing frame{drawing,pips,palette};
    auto* previous=health_drawing;health_drawing=&frame;
    try { building.DrawHealthBar(&point,&bounds,false); }
    catch (...) { frame.status=DrawingStatus::backend_failure; }
    health_drawing=previous;
    return frame.status;
}
DrawingStatus draw_building_extras(const BuildingClass& building,BuildingHealthDrawing& frame,Point2D point,RectangleStruct bounds) noexcept {
    return draw_techno_extras(building,frame,point,bounds);
}
DrawingStatus draw_techno_extras(const TechnoClass& techno,BuildingHealthDrawing& frame,Point2D point,RectangleStruct bounds) noexcept {
    auto* previous=health_drawing;health_drawing=&frame;
    try {techno.DrawExtras(&point,&bounds);}
    catch(...){frame.status=DrawingStatus::backend_failure;}
    health_drawing=previous;return frame.status;
}
bool building_selection_geometry(const BuildingClass& building, BuildingSelectionGeometry& out) noexcept {
    out={};
    const auto* type=building.Type;
    if(!type)return false;
    const int width=type->GetFoundationWidth(),length=type->GetFoundationHeight(false);
    if(int(type->Foundation)<0||int(type->Foundation)>=22||width<0||length<0||width>64||length>64||type->Height<0||type->Height>256)return false;
    const auto center=building.GetCenterCoords();
    const int hx=width*128,hy=length*128;
    // 0x0045B070 initializes 0x0089DDB8 to 104 using the target sqrt/tan
    // tables. Height is not SHP pixel height, nor foundation length/Bib.
    const int height=104*type->Height;
    const auto at=[&](int x,int y,int z){return CoordStruct{add(center.X,x),add(center.Y,y),add(center.Z,z)};};
    const auto left=at(-hx,hy,0),right=at(hx,-hy,0),front=at(hx,hy,0);
    const auto top_left=at(-hx,hy,height),top_right=at(hx,-hy,height),top_front=at(hx,hy,height);
    corners(out,left,front);corners(out,right,front);
    corners(out,top_left,left);corners(out,top_right,right);
    edge(out,front,near_end(front,top_front));
    edge(out,top_right,near_end(top_right,top_front));
    edge(out,top_left,near_end(top_left,top_front));
    // ObjectClass::GetHeight, 0x005F5F40. Use the original cell floor plane,
    // including slopes and bridge subtraction; do not call the host R0 stub.
    if (auto* cell=MapClass::Instance.TryGetCellAt(building.Location)) {
        const Point2D local{building.Location.X-cell->MapCoords.X*256,
                           building.Location.Y-cell->MapCoords.Y*256};
        const int above=building.Location.Z-cell->GetFloorHeight(local)-
            (building.OnBridge?CellClass::BridgeHeight:0);
        out.palette_index=above < -4?12u:15u;
    }
    return true;
}
DrawingStatus draw_building_selection(const TypeDrawingContext& context,const RectangleStruct& clip,
        const Point2D& camera,const BuildingSelectionGeometry& geometry,const BytePalette& palette) noexcept {
    if(geometry.count>geometry.edges.size()||geometry.palette_index>=256)return DrawingStatus::invalid_argument;
    if(clip.Width<=0||clip.Height<=0)return DrawingStatus::skipped;
    const auto c=palette.Entries[geometry.palette_index];
    const std::uint16_t color=std::uint16_t((c.R>>3)<<11|(c.G>>2)<<5|(c.B>>3));
    try {
        DrawingStatus result=DrawingStatus::skipped;
        for(unsigned i=0;i<geometry.count;++i){
            auto a=TacticalClass::CoordsToScreen(geometry.edges[i].first);
            auto b=TacticalClass::CoordsToScreen(geometry.edges[i].last);
            a.X=std::bit_cast<std::int32_t>(std::uint32_t(a.X)-std::uint32_t(camera.X));
            a.Y=std::bit_cast<std::int32_t>(std::uint32_t(a.Y)-std::uint32_t(camera.Y));
            b.X=std::bit_cast<std::int32_t>(std::uint32_t(b.X)-std::uint32_t(camera.X));
            b.Y=std::bit_cast<std::int32_t>(std::uint32_t(b.Y)-std::uint32_t(camera.Y));
            const int za=14-TacticalClass::AdjustForZ(geometry.edges[i].first.Z);
            const int zb=14-TacticalClass::AdjustForZ(geometry.edges[i].last.Z);
            auto s=line(context,clip,a,b,za,zb,color);
            if(s!=DrawingStatus::drawn&&s!=DrawingStatus::skipped)return s;
            if(s==DrawingStatus::drawn)result=s;
        }return result;
    }catch(...){return DrawingStatus::backend_failure;}
}
}
