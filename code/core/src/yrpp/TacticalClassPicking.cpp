// Existing YRpp TacticalClass, gamemd 6D6590 ground scan and fallback.
#include "yrpp/TacticalClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/Unsorted.h"
#include <bit>
#include <cstdint>
#include <cstdlib>

bool TacticalClass::PickTerrainCell(const Point2D& point,const RectangleStruct& viewport,
        CellStruct& output) noexcept {
    const std::int64_t local_x=std::int64_t(point.X)-viewport.X,local_y=std::int64_t(point.Y)-viewport.Y;
    const auto absolute_x=local_x+TacticalPos.X,absolute_y=local_y+TacticalPos.Y;
    // Host domain guard keeps matrix float-to-int conversion and the 180 pixel
    // scan defined. Accepted maps and viewports are far inside these limits.
    if (absolute_x<-100000 || absolute_x>100000 || absolute_y<-100000 || absolute_y>100000) return false;
    const auto cell_at=[&](int y) {
        const auto world=ApplyMatrix_Pixel({static_cast<int>(absolute_x),y});
        return CellStruct{std::bit_cast<short>(static_cast<unsigned short>(world.X/256)),
            std::bit_cast<short>(static_cast<unsigned short>(world.Y/256))};
    };
    const auto fallback=cell_at(static_cast<int>(absolute_y));
    for (int offset=180;offset>0;--offset) {
        const auto candidate=cell_at(static_cast<int>(absolute_y)+offset);
        const auto* cell=MapClass::Instance.GetCellAt(candidate);
        const int level=static_cast<signed char>(cell->Level);
        int projected_y=static_cast<int>(local_y)+offset-15*level;
        if ((static_cast<DWORD>(cell->Flags)&0x100u)!=0) {
            // 0x6D6590 bridge ends and ramps. A bridge encountered by the
            // downward scan is not a reason to reject the entire click.
            const auto neighbor=[&](int direction) {
                const auto delta=Unsorted::AdjacentCell[direction];
                return MapClass::Instance.GetCellAt(CellStruct{short(candidate.X+delta.X),short(candidate.Y+delta.Y)});
            };
            const auto bridge=[](const CellClass* item){return (static_cast<DWORD>(item->Flags)&0x100u)!=0;};
            const auto* east=neighbor(2);const auto* south=neighbor(4);
            const bool diagonal=(static_cast<DWORD>(cell->Flags)&0x800u)!=0;
            const bool northEnd=diagonal&&!bridge(neighbor(0));
            const bool westEnd=!diagonal&&!bridge(neighbor(6));
            const bool southEnd=diagonal&&!bridge(south);
            const bool eastEnd=!diagonal&&!bridge(east);
            const bool eastRamp=diagonal&&std::abs(level-static_cast<signed char>(east->Level))<=1&&!bridge(east);
            const bool southRamp=!diagonal&&std::abs(level-static_cast<signed char>(south->Level))<=1&&!bridge(south);
            const auto anchor=CoordsToScreen(CoordStruct{int(cell->MapCoords.X)*256,int(cell->MapCoords.Y)*256,0});
            const int anchor_y=anchor.Y-TacticalPos.Y-15*level;
            if(anchor_y<=local_y) {
                if(southEnd||southRamp){output={candidate.X,short(candidate.Y+1)};return true;}
                if(eastEnd||eastRamp){output={short(candidate.X+1),candidate.Y};return true;}
            }
            if(northEnd||westEnd) {
                const int dx=static_cast<int>(local_x)-(anchor.X-TacticalPos.X);
                const int dy=static_cast<int>(local_y)-anchor_y;
                if((northEnd&&dy-dx/2>15)||(westEnd&&dy+dx/2>15))projected_y-=60;
            }else projected_y-=60;
        }
        if (projected_y<=local_y) { output=candidate; return true; }
    }
    output=fallback;
    return true;
}
