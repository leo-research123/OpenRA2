// YRpp 9402d7da RadarClass; terrain subset of YR 654490, 654650, 654EA0.
// No independent map representation: iterate actual MapClass cells, retain RGB
// in the original radar field, and let the host copy the presentation texture.
#include "yrpp/RadarClass.h"
#include "yrpp/Surface.h"
#include <algorithm>
#include <bit>
#include <cstring>

#if !defined(RA2_YRPP_GAME)
void RadarClass::SetVisibleRect(const RectangleStruct& rect) {
    MapClass::SetVisibleRect(rect);
    unknown_rect_149C={10000,10000,0,0};
    const auto a=static_cast<short>(unsigned(VisibleRect.X)+unsigned(VisibleRect.Y)+1u);
    const auto b=static_cast<short>(unsigned(VisibleRect.Y)+unsigned(MapRect.Width)-unsigned(VisibleRect.X));
    unknown_1490=static_cast<DWORD>(b-a);
    unknown_1494=static_cast<DWORD>(static_cast<short>(unsigned(VisibleRect.Width)-1u+unsigned(a))-
        static_cast<short>(1u-unsigned(VisibleRect.Width)+unsigned(b)));
    unknown_1498=static_cast<DWORD>(-1);
    // The original resets the global Map iterator, then iterates this Map.
    MapClass::Instance.CellIteratorReset();
    while(auto* cell=CellIteratorNext()){
        if(!IsWithinUsableArea(cell->MapCoords,true))continue;
        if(unknown_1498==static_cast<DWORD>(-1))unknown_1498=cell->MapCoords.X+cell->MapCoords.Y;
        RectangleStruct r;CellRadarRect(&r,cell->MapCoords);
        auto& bounds=unknown_rect_149C;
        if(r.X<bounds.X)bounds.X=r.X;
        else if(r.X+r.Width>bounds.X+bounds.Width)bounds.Width=r.X+r.Width-bounds.X;
        if(r.Y<bounds.Y)bounds.Y=r.Y;
        else if(r.Y+r.Height>bounds.Y+bounds.Height)bounds.Height=r.Y+r.Height-bounds.Y;
    }
}
#endif

bool RadarClass::BuildTerrainRadar() noexcept {
    // Host fresh/reload wrapper. Original range calculation and image creation
    // remain independently callable; the host additionally retires old queues.
    ReleaseTerrainRadar();InitRadar();
    if(!unknown_1258 || !Cells.Items || MapRect.Width<2 || VisibleRect.Width<1 || VisibleRect.Height<1)return false;
    try {SetVisibleRect(VisibleRect);ComputeRadarImage();}
    catch(...) {ReleaseTerrainRadar();return false;}
    return unknown_123C && unknown_1274 && unknown_1220;
}
void RadarClass::ComputeRadarImage() noexcept {
    // 0x00654650: invalid geometry is a no-op. The valid path retires graphics,
    // not the tracking hash or saved point/cell queues. Whole-image composition
    // replaces the original foreground/background incremental copies.
    if(unknown_rect_149C.Width<=0 || unknown_rect_149C.Height<=0)return;
    const int width=unknown_rect_149C.Width,height=unknown_rect_149C.Height;
    unknown_rect_149C.X=unknown_rect_149C.Y=0;
    const auto release_images=[&] {
        GameDelete(unknown_121C);unknown_121C=nullptr;
        GameDelete(unknown_1220);unknown_1220=nullptr;
        YRMemory::Deallocate(unknown_123C);unknown_123C=nullptr;
        YRMemory::Deallocate(unknown_1274);unknown_1274=nullptr;
    };
    release_images();
    try {
        Point2D size;float factor;
        if(!FitTerrainRadar(width,height,size,factor))return;
        auto* pixels=static_cast<ColorStruct*>(YRMemory::AllocateZeroed(std::size_t(width)*height*sizeof(ColorStruct)));
        if(!pixels)return;
        unknown_123C=pixels;unknown_1240=width;unknown_1244=height;
        CellIteratorReset();
        while(auto* cell=CellIteratorNext()){
            const auto c=cell->MapCoords;
            const int x=c.X-c.Y+std::bit_cast<int>(unknown_1490),y=c.X+c.Y-std::bit_cast<int>(unknown_1498);
            if(y<0 || y>=height || x<-1 || x>=width)continue;
            const auto color=cell->GetTerrainRadarColor();
            for(int xx=std::max(x,0);xx<std::min(x+2,width);++xx)pixels[y*width+xx]=color;
        }
        RadarSizeFactor=factor;
        unknown_rect_149C={int(unknown_11F0)+(140-size.X)/2,int(unknown_11F4)+(108-size.Y)/2,size.X,size.Y};
        unknown_1274=static_cast<byte*>(YRMemory::AllocateZeroed(size.X*size.Y/8+1));
        if(!unknown_1274 || !BuildFoundationPixels() || !RebuildTerrainRadarCache()){
            release_images();return;
        }
        unknown_bool_14D9=unknown_bool_14DA=true;
    } catch(...) {release_images();}
}

bool RadarClass::RebuildTerrainRadarCache() noexcept {
    // A single persistent terrain Surface replaces the original two-surface
    // incremental copies. Dynamic layers never modify this background.
    GameDelete(unknown_1220); unknown_1220=nullptr;
    const auto& rect=unknown_rect_149C;
    if (!unknown_123C || rect.Width<1 || rect.Height<1) return false;
    BSurface* surface=nullptr;
    try {
        surface=BSurface::Create(rect.Width,rect.Height,2);
        if (!surface || !surface->Buffer.Buffer ||
            !ResampleTerrainRadar(unknown_123C,unknown_1240*unknown_1244,
                int(unknown_1240),int(unknown_1244),static_cast<WORD*>(surface->Buffer.Buffer),
                unsigned(rect.Width*rect.Height))) {
            BSurface::Destroy(surface); return false;
        }
        unknown_1220=surface;
        return true;
    } catch (...) { BSurface::Destroy(surface); return false; }
}

bool RadarClass::CopyTerrainRadar(WORD* pixels,unsigned count) const noexcept {
    auto* surface=unknown_1220;
    const auto& rect=unknown_rect_149C;
    if (!pixels || !surface || rect.Width<1 || rect.Height<1 ||
        count<unsigned(rect.Width*rect.Height) || surface->GetWidth()!=rect.Width ||
        surface->GetHeight()!=rect.Height || surface->GetBytesPerPixel()!=2) return false;
    const int pitch=surface->GetPitch();
    if (pitch<rect.Width*2) return false;
    const auto* source=static_cast<const byte*>(surface->Lock(0,0));
    if (!source) { surface->Unlock(); return false; }
    for (int row=0;row<rect.Height;++row)
        std::memcpy(pixels+row*rect.Width,source+row*pitch,std::size_t(rect.Width)*2);
    surface->Unlock();
    return true;
}
