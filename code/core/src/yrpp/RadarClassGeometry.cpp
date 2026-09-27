// Existing YRpp 9402d7da RadarClass; fixed gamemd 6557F0, 6550C0,
// 656830 and 656F5E..65712E. Surface/UI and entity tracking are separate.
#include "yrpp/RadarClass.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>

namespace {
int signed_word(DWORD value) noexcept { return std::bit_cast<std::int32_t>(value); }
float store_float(double value) noexcept {
    const float nearest=static_cast<float>(value);
    return std::abs(double(nearest))>std::abs(value)
        ? std::bit_cast<float>(std::bit_cast<std::uint32_t>(nearest)-1u) : nearest;
}
}
Point2D* RadarClass::GetCrdOnRadar(Point2D* output, CoordStruct* world, bool restrict) {
    if (!output || !world || !std::isfinite(RadarSizeFactor) || RadarSizeFactor<=0 ||
        unknown_rect_149C.Width<=0 || unknown_rect_149C.Height<=0) return nullptr;
    const auto x=static_cast<DWORD>(world->X), y=static_cast<DWORD>(world->Y);
    output->X=static_cast<int>(double(signed_word(x+(unknown_1490<<8)-y))*RadarSizeFactor/256.0);
    output->Y=static_cast<int>(double(signed_word(x+y-(unknown_1498<<8)))*RadarSizeFactor/256.0);
    if (restrict) {
        output->X=std::clamp(output->X,0,unknown_rect_149C.Width-1);
        output->Y=std::clamp(output->Y,0,unknown_rect_149C.Height-1);
    }
    return output;
}
Point2D RadarClass::CellToRadar(const CellStruct& cell) const noexcept {
    Point2D point{
        static_cast<int>(double(cell.X+signed_word(unknown_1490)-cell.Y)*RadarSizeFactor+unknown_rect_149C.X),
        static_cast<int>(double(cell.X+cell.Y-signed_word(unknown_1498))*RadarSizeFactor+unknown_rect_149C.Y)};
    if (point.X==unknown_rect_149C.X-1) ++point.X;
    return point;
}
RectangleStruct* RadarClass::CellRadarRect(RectangleStruct* output,const CellStruct& cell) const noexcept {
    if (!output) return nullptr;
    int x=cell.X+signed_word(unknown_1490)-cell.Y,width=2;
    if (x==-1) {x=0;width=1;}
    else if (x==2*VisibleRect.Width-1) width=1;
    *output={x,cell.X+cell.Y-signed_word(unknown_1498),width,1};
    return output;
}
RectangleStruct* RadarClass::CellToRadarPixel(RectangleStruct* output,const CellStruct& cell) const noexcept {
    if (!output) return nullptr;
    const auto point=CellToRadar(cell);
    *output={point.X,point.Y,1,1};
    return output;
}
bool RadarClass::CellOnRadar(const CellStruct& cell) const {
    return IsWithinUsableArea(cell,true);
}
bool RadarClass::RadarToTerrainCell(const Point2D& point, CellStruct& output) const noexcept {
    if (!std::isfinite(RadarSizeFactor) || RadarSizeFactor<=0) return false;
    const double a=(double(point.X)-unknown_rect_149C.X)/RadarSizeFactor-signed_word(unknown_1490);
    const double b=(double(point.Y)-unknown_rect_149C.Y)/RadarSizeFactor+signed_word(unknown_1498);
    const double x=(b+a)*0.5+0.5,y=(b-a)*0.5+0.5;
    if (x<INT32_MIN || x>INT32_MAX || y<INT32_MIN || y>INT32_MAX) return false;
    output={std::bit_cast<short>(static_cast<unsigned short>(static_cast<int>(x))),
        std::bit_cast<short>(static_cast<unsigned short>(static_cast<int>(y)))};
    return true;
}
bool RadarClass::UpdateViewportFrame(const CellStruct& center,const Point2D& viewport) noexcept {
    if (!std::isfinite(RadarSizeFactor) || RadarSizeFactor<=0 || viewport.X<1 || viewport.Y<1 ||
        viewport.X>8192 || viewport.Y>8192) return false;
    const auto point=CellToRadar(center);
    const double dx=60.0/RadarSizeFactor;
    const float dy=store_float(30.0/RadarSizeFactor);
    RectangleStruct rect{point.X-static_cast<int>(viewport.X/dx),
        point.Y-static_cast<int>((2.0*viewport.Y/dy)*0.5),
        static_cast<int>(2.0*viewport.X/dx+1.0),static_cast<int>(2.0*viewport.Y/dy)};
    const auto& bounds=unknown_rect_149C;
    if (rect.X<bounds.X) rect.X=bounds.X;
    else if (rect.X+rect.Width>=bounds.X+bounds.Width) rect.X=bounds.X+bounds.Width-rect.Width-1;
    if (rect.Y<bounds.Y) rect.Y=bounds.Y;
    else if (rect.Y+rect.Height>=bounds.Y+bounds.Height) rect.Y=bounds.Y+bounds.Height-rect.Height-1;
    unknown_rect_14DC=rect;
    return true;
}

bool RadarClass::FitTerrainRadar(int width,int height,Point2D& size,float& factor) noexcept {
    // All accepted maps fit this domain, including the maximum 512 cell sum.
    if (width<1 || height<1 || width>2048 || height>2048) return false;
    float scale=store_float(140.0/width);
    const double h=double(height)*scale;
    Point2D result;
    if (h>=108.0) { scale=store_float(108.0/height); result={static_cast<int>(double(width)*scale),108}; }
    else result={140,static_cast<int>(store_float(h))};
    if (result.X<1 || result.Y<1) return false;
    size=result; factor=scale; return true;
}

bool RadarClass::ResampleTerrainRadar(const ColorStruct* source,unsigned source_count,
        int width,int height,WORD* output,unsigned output_count) noexcept {
    Point2D size; float factor;
    if (!source || !output || !FitTerrainRadar(width,height,size,factor) ||
        source_count<unsigned(width*height) || output_count<unsigned(size.X*size.Y)) return false;
    const float step_x=store_float(double(width)/size.X),step_y=store_float(double(height)/size.Y);
    const float norm=store_float(1.0/((double(height)/size.Y)*step_x));
    float y=0;
    for (int row=0;row<size.Y;++row) {
        const int top=std::min(int(y),height);
        const double end_y=double(y)+step_y;
        const int bottom=std::min(int(end_y)+1,height);
        float x=0;
        for (int col=0;col<size.X;++col) {
            const int left=std::min(int(x),width);
            const double end_x=double(x)+step_x;
            const float stored_end_x=store_float(end_x);
            const int right=std::min(int(end_x)+1,width);
            double red=0,green=0,blue=0;
            for (int yy=top;yy<bottom;++yy) {
                const double wy=bottom-top<=1 ? double(step_y) : yy==top ? double(yy+1)-y :
                    yy==bottom-1 ? end_y-yy : 1.0;
                for (int xx=left;xx<right;++xx) {
                    const double wx=right-left<=1 ? double(step_x) : xx==left ? double(xx+1)-x :
                        xx==right-1 ? double(stored_end_x)-xx : 1.0;
                    const double weight=(wx*wy)*norm;
                    const auto& color=source[yy*width+xx];
                    red+=color.R*weight; green+=color.G*weight; blue+=color.B*weight;
                }
            }
            const int r=std::min(int(red+0.5),255),g=std::min(int(green+0.5),255),b=std::min(int(blue+0.5),255);
            output[row*size.X+col]=WORD((r>>3<<11)|(g>>2<<5)|(b>>3));
            x=stored_end_x;
        }
        y=store_float(end_y);
    }
    return true;
}
