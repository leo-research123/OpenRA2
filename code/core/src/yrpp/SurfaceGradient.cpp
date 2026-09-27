// DSurface 4BF750 + Line_In_Bounds 7BC2B0 + RGB interpolation 661020.
// Original gradient phase, endpoint ordering and >0 Bresenham tie rule.
#include "yrpp/Surface.h"
#include "api/type_drawing.hpp"
#include "type_drawing.hpp"
#include "building_selection.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>

namespace {
float store_float(double value) noexcept {
    const float nearest=static_cast<float>(value);
    return std::abs(double(nearest))>std::abs(value) ?
        std::bit_cast<float>(std::bit_cast<unsigned>(nearest)-1u) : nearest;
}
bool clip_line(Point2D& first,Point2D& last,int width,int height,int left=0,int top=0) noexcept {
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
    const auto code=[&](double a,double b) {return (a>=left+width ? 2 : a<left ? 1 : 0)|(b>=top+height ? 4 : b<top ? 8 : 0);};
    int a=code(x,y),b=code(ex,ey);
    for (int guard=0;guard<16;++guard) {
        if (!(a|b)) {first={int(x),int(y)};last={int(ex),int(ey)};return true;}
        if (a&b) return false;
        const int c=a ? a : b;
        double nx=x,ny=y;
        if (c&8) {nx=intersection(top-y,dxdy,x);ny=top;}
        else if (c&4) {ny=top+height-1;nx=intersection(ny-y,dxdy,x);}
        else if (c&2) {nx=left+width-1;ny=intersection(nx-x,dydx,y);}
        else {nx=left;ny=intersection(left-x,dydx,y);}
        if (c==a) {x=nx;y=ny;a=code(x,y);}
        else {ex=nx;ey=ny;b=code(ex,ey);}
    }
    return false;
}
}
bool YRPP_FASTCALL Line_In_Bounds(Point2D* first,Point2D* last,RectangleStruct* bounds) {
    return first&&last&&bounds&&bounds->Width>0&&bounds->Height>0
        &&clip_line(*first,*last,bounds->Width,bounds->Height,bounds->X,bounds->Y);
}
bool DSurface::SubmitBlendedLine(const RectangleStruct& clip,Point2D a,Point2D b,ColorStruct color,int opacity,int za,int zb) noexcept {
    const auto* drawing=game::active_type_drawing();if(!drawing)return false;
    const auto status=game::draw_depth_alpha_line(*drawing,clip,a,b,za,zb,color,opacity);
    game::record_type_drawing_result(status);return status==game::DrawingStatus::drawn;
}
bool DSurface::DrawBlendedLine(RectangleStruct* clip,Point2D* a,Point2D* b,ColorStruct* color,int opacity,int za,int zb){
    if(!clip||!a||!b||!color)return false;
    RectangleStruct bounds;GetRect(&bounds);
    const int left=std::max(bounds.X,clip->X),top=std::max(bounds.Y,clip->Y);
    const RectangleStruct cropped{left,top,std::max(0,std::min(bounds.X+bounds.Width,clip->X+clip->Width)-left),
        std::max(0,std::min(bounds.Y+bounds.Height,clip->Y+clip->Height)-top)};
    return SubmitBlendedLine(cropped,*a,*b,*color,opacity,za,zb);
}
game::DrawingStatus DSurface::SubmitGradientLine(const game::TypeDrawingContext& drawing,
    const RectangleStruct& clip,Point2D start,Point2D end,ColorStruct first,ColorStruct last,
    float& step,float& phase) noexcept {
    using game::DrawingStatus;
    if (!std::isfinite(step) || !std::isfinite(phase) || clip.Width<0 || clip.Height<0 ||
        clip.Width>8192 || clip.Height>8192 || std::int64_t(clip.X)+clip.Width>INT32_MAX ||
        std::int64_t(clip.Y)+clip.Height>INT32_MAX) return DrawingStatus::invalid_argument;
    if (!clip.Width || !clip.Height || !clip_line(start,end,clip.Width,clip.Height)) return DrawingStatus::skipped;
    if (start.X>end.X) std::swap(start,end);
    const int dx=end.X-start.X,dy=std::abs(end.Y-start.Y),direction=end.Y>=start.Y ? 1 : -1;
    const bool horizontal=dx>dy;
    const int length=std::max(dx,dy);
    int error=horizontal ? 2*dy-dx : 2*dx-dy;
    for (int i=0;i<=length;++i) {
        const auto channel=[&](byte a,byte b) {return int(std::clamp((1.0-phase)*a+double(b)*phase,0.0,255.0));};
        const int r=channel(first.R,last.R),g=channel(first.G,last.G),b=channel(first.B,last.B);
        game::RasterDrawingRequest request;
        request.target=drawing.target; request.position={clip.X+start.X,clip.Y+start.Y}; request.clip=clip;
        request.width=request.height=1; request.color=WORD((r>>3<<11)|(g>>2<<5)|(b>>3));
        const auto status=game::submit_type_raster(drawing,request);
        if (status!=DrawingStatus::drawn && status!=DrawingStatus::skipped) return status;
        const double value=double(phase)+step; phase=store_float(value);
        if (value<0 && step<0) { phase=0; step=-step; }
        else if (value>1 && step>0) { phase=1; step=-step; }
        if (horizontal) {
            if (error>0) {start.Y+=direction;error-=2*dx;}
            error+=2*dy; ++start.X;
        } else {
            if (error>0) {++start.X;error-=2*dy;}
            error+=2*dx;start.Y+=direction;
        }
    }
    return DrawingStatus::drawn;
}
