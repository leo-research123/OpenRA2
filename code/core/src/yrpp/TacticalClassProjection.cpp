// YRpp 9402d7da TacticalClass; YR 6D62E0 map-preview projection. The target
// uses fixed 60x30 pixels per cell and an x origin of 15360, ignoring height.
#include "yrpp/TacticalClass.h"
#include "yrpp/MapClass.h"
#include "map_runtime.hpp"
#include <bit>
#include <cstdint>
#include <cmath>

#if !defined(RA2_YRPP_GAME)
CellStruct* YRPP_STDCALL TacticalClass::AdjustCellForHeight(CellStruct* out,const CoordStruct* at) {
    const CellStruct origin{short(at->X/256),short(at->Y/256)};
    auto& map=MapClass::Instance;
    const bool use_bridge=(static_cast<unsigned>(map.GetCellAt(origin)->Flags)&0x1000u)!=0;
    const int level=static_cast<signed char>(map.GetCellAt(origin)->Level);
    int x=int(origin.X)*256+128+1536,y=int(origin.Y)*256+128+1536;
    for(;;) {
        x-=8;y-=8;
        const CellStruct candidate{short(x/256),short(y/256)};
        const auto* cell=map.GetCellAt(candidate);
        int delta=static_cast<signed char>(cell->Level)-level;
        if(use_bridge)delta+=4*((static_cast<unsigned>(cell->Flags)>>8)&1u);
        const int height=delta*128;
        if((short((x-height)/256)<=origin.X && short((y-height)/256)<=origin.Y) || candidate==origin) {
            *out=candidate;return out;
        }
    }
}

#endif

Point2D TacticalClass::CoordsToMapPixel(int x, int y) {
    const auto wrap = [](std::uint32_t value) { return std::bit_cast<std::int32_t>(value); };
    const auto ux = static_cast<std::uint32_t>(x), uy = static_cast<std::uint32_t>(y);
    const int xx = wrap(static_cast<std::uint32_t>(wrap(60u * ux) / 2) +
        static_cast<std::uint32_t>(wrap(0u - 60u * uy) / 2)) / 256;
    const int yy = wrap(static_cast<std::uint32_t>(wrap(30u * ux) / 2) +
        static_cast<std::uint32_t>(wrap(30u * uy) / 2)) / 256;
    return {wrap(static_cast<std::uint32_t>(xx) + 15360u), yy};
}

// Non-template interface helpers; bodies retained from the corresponding header.

namespace {
int wrap(std::uint32_t value) noexcept { return std::bit_cast<std::int32_t>(value); }
}

Point2D TacticalClass::CoordsToScreen(const CoordStruct& coord) noexcept
{
    auto [x, y] = AdjustForZShapeMove(coord.X, coord.Y);
    return {x, wrap(static_cast<std::uint32_t>(y) - static_cast<std::uint32_t>(AdjustForZ(coord.Z)))};
}

Point2D* TacticalClass::CoordsToScreen(Point2D* output, const CoordStruct* coords) {
    if (!output || !coords) return nullptr;
    *output = CoordsToScreen(*coords);
    return output;
}

bool TacticalClass::CoordsToClient(const CoordStruct& coords, const Point2D& camera,
        const RectangleStruct& bounds, Point2D& output) noexcept {
    const auto projected = CoordsToScreen(coords);
    const Point2D point{
        wrap(static_cast<std::uint32_t>(projected.X) - static_cast<std::uint32_t>(camera.X)),
        wrap(static_cast<std::uint32_t>(projected.Y) - static_cast<std::uint32_t>(camera.Y))};
    const bool visible = point.X >= -360 && point.X <= wrap(static_cast<std::uint32_t>(bounds.Width) + 360u) &&
        point.Y >= -180 && point.Y <= wrap(static_cast<std::uint32_t>(bounds.Height) + 180u);
    output = point;
    return visible;
}

bool TacticalClass::CoordsToClient(const CoordStruct* coords, Point2D* output) const {
    RectangleStruct bounds;
    if (!coords || !output || !game::map_view_bounds(bounds)) return false;
    return CoordsToClient(*coords, TacticalPos, bounds, *output);
}

CoordStruct TacticalClass::ClientToCoords(Point2D const& client) const
{
    CoordStruct buffer;
    this->ClientToCoords(&buffer, client);
    return buffer;
}

Point2D TacticalClass::AdjustForZShapeMove(int x, int y) noexcept
{
    // 6D1FE0 performs each multiply and sum in 32 bits before signed division.
    // Keep the original wrapping while avoiding signed-overflow UB on hosts.
    const auto ux = static_cast<std::uint32_t>(x), uy = static_cast<std::uint32_t>(y);
    return {
        wrap(static_cast<std::uint32_t>(wrap(0u - 60u * uy) / 2) +
             static_cast<std::uint32_t>(wrap(60u * ux) / 2)) / 256,
        wrap(static_cast<std::uint32_t>(wrap(30u * uy) / 2) +
             static_cast<std::uint32_t>(wrap(30u * ux) / 2)) / 256};
}

int YRPP_FASTCALL TacticalClass::AdjustForZ(int Height) noexcept
        // VA: 0x006D20E0; also inlined in the game.
{
    return static_cast<int>(Height * *game::map_runtime().height_scale + int(Height >= 728) + 0.5);
}

int YRPP_FASTCALL TacticalClass::PixelToZ(int pixels) noexcept
{
    const double value = (pixels - 0.5) * (1.0 / *game::map_runtime().height_scale);
    // Match the original toward-zero FISTP indefinite result without host UB.
    if (!std::isfinite(value) || value < -2147483648.0 || value >= 2147483648.0) return INT32_MIN;
    return static_cast<int>(value);
}

namespace {
Point2D matrix_pixel(const Matrix3D& matrix,const Point2D& offset) {
    // 5AFB80 evaluates with x87 precision then stores each output float under
    // the game's toward-zero control word. Host float multiply/add rounds at
    // different points and can select a neighboring cell near a boundary.
    const auto to_float=[](double value) {
        const float nearest=static_cast<float>(value);
        return std::abs(static_cast<double>(nearest))>std::abs(value)
            ? std::bit_cast<float>(std::bit_cast<std::uint32_t>(nearest)-1u) : nearest;
    };
    const double x=to_float(offset.X),y=to_float(offset.Y);
    Point2D result;
    const auto& r=matrix.row;
    result.X=static_cast<int>(to_float(double(r[0][1])*y+double(r[0][0])*x+r[0][3]));
    result.Y=static_cast<int>(to_float(double(r[1][0])*x+double(r[1][1])*y+r[1][3]));
    return result;
}
}
Point2D TacticalClass::ApplyMatrix_Pixel(const Point2D& offset) {
    return matrix_pixel(IsoTransformMatrix,offset);
}
#if !defined(RA2_YRPP_GAME)
// OpenTS 44fac744 tactical.cpp Pixel_To_Coord; YR 0x006D2280.
CoordStruct* TacticalClass::ClientToCoords(CoordStruct* output,const Point2D& client) const {
    if (!output) return nullptr;
    RectangleStruct bounds=ViewBounds;
    (void)game::map_view_bounds(bounds);
    if (client.X>=bounds.Width || client.Y>=wrap(unsigned(bounds.Y)+unsigned(bounds.Height))) {
        *output={-1,-1,-1}; // Original 0x00B0CE08 sentinel, not Vector3D::Empty.
    } else {
        // Original does not reject negative client coordinates or subtract
        // viewport X/Y. The matrix receives camera + client in virtual pixels.
        const auto point=matrix_pixel(IsoTransformMatrix,
            {wrap(unsigned(TacticalPos.X)+unsigned(client.X)),wrap(unsigned(TacticalPos.Y)+unsigned(client.Y))});
        *output={point.X,point.Y,0};
    }
    return output;
}
#endif
CoordStruct* TacticalClass::PixelToCoordsAbsolute(CoordStruct* output,const Point2D& pixel) {
    if(pixel.X>=ViewBounds.Width || pixel.Y>=ViewBounds.Height)*output=CoordStruct::Empty;
    else {const auto leptons=ApplyMatrix_Pixel(pixel);*output={leptons.X,leptons.Y,0};}
    return output;
}
