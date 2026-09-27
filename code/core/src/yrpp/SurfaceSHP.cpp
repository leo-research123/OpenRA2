// Copyright 2025 Electronic Arts Inc.
// SPDX-License-Identifier: GPL-3.0-or-later
// Adapted from fixed Renegade 3e00c3a1 Code/wwlib/blit.cpp RLE_Blit/Blit.
// YRpp Surface/SHP/Blitter interfaces retained. YR 437A10/4373B0/4AED70
// calibration supplies Z gradients, alpha rings, tint and auxiliary SHA data.
#include "yrpp/Memory.h"
#include "yrpp/Surface.h"
#include "yrpp/Drawing.h"
#include "Blitter.hpp"
#include "yrpp/FileFormats/SHP.h"
#include "yrpp/ConvertClass.h"
#include "images/original_abi.hpp"
#include <new>
#include <vector>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <cstdint>

namespace {
RectangleStruct intersect_image_rect(const RectangleStruct& bounds, const RectangleStruct& region) {
    if (bounds.Width <= 0 || bounds.Height <= 0 || region.Width <= 0 || region.Height <= 0) return {};
    const int x = std::max(bounds.X, region.X), y = std::max(bounds.Y, region.Y);
    const int right = std::min(bounds.X + bounds.Width, region.X + region.Width);
    const int bottom = std::min(bounds.Y + bounds.Height, region.Y + region.Height);
    return right > x && bottom > y ? RectangleStruct{x, y, right-x, bottom-y} : RectangleStruct{};
}
bool clip_image_rects(RectangleStruct& d, const RectangleStruct& dc, RectangleStruct& s, const RectangleStruct& sc) {
    // The original relative-coordinate clipping ignores clip origins in the
    // equal-size path; they are added when locking, as in WWLib Blit_Clip.
    if (d.Width == s.Width && d.Height == s.Height) {
        if (d.X < 0) { s.X -= d.X; s.Width += d.X; d.Width += d.X; d.X=0; }
        if (d.Y < 0) { s.Y -= d.Y; s.Height += d.Y; d.Height += d.Y; d.Y=0; }
        int excess = d.X+d.Width-dc.Width;
        if (excess>0) { s.Width-=excess; d.Width-=excess; }
        excess=d.Y+d.Height-dc.Height;
        if (excess>0) { s.Height-=excess; d.Height-=excess; }
        if (s.X<0) { d.X-=s.X; s.Width+=s.X; d.Width+=s.X; s.X=0; }
        if (s.Y<0) { d.Y-=s.Y; s.Height+=s.Y; d.Height+=s.Y; s.Y=0; }
        excess=s.X+s.Width-sc.Width;
        if (excess>0) { s.Width=sc.Width-s.X; d.Width-=excess; }
        excess=s.Y+s.Height-sc.Height;
        if (excess>0) { s.Height=sc.Height-s.Y; d.Height-=excess; }
    } else { d=intersect_image_rect(dc,d); s=intersect_image_rect(sc,s); }
    return d.Width>0 && d.Height>0 && s.Width>0 && s.Height>0;
}
void invoke_rle_blitter(RLEBlitter* self, void* dest, BYTE* source, int length, int lead, int zbase,
    WORD* z, WORD* a, int light, int warp, BYTE* offsets, WORD tint) {
    if (tint) self->Blit_Copy_Tinted(dest, source, length, lead, zbase, z, a, light, warp, offsets, tint);
    else self->Blit_Copy(dest, source, length, lead, zbase, z, a, light, warp, offsets);
}
void invoke_plain_blitter(Blitter* self, bool backwards, void* dest, BYTE* source, int length,
    int zbase, WORD* z, WORD* a, int light, int warp, WORD tint) {
    if (backwards) {
        if (tint) self->Blit_Move_Tinted(dest, source, length, zbase, z, a, light, tint);
        else self->Blit_Move(dest, source, length, zbase, z, a, light);
    } else {
        if (tint) self->Blit_Copy_Tinted(dest, source, length, zbase, z, a, light, warp, tint);
        else self->Blit_Copy(dest, source, length, zbase, z, a, light, warp);
    }
}
std::uint16_t line_length(const BYTE* p) { return std::uint16_t(p[0] | unsigned(p[1])<<8); }
template<typename Buffer> WORD* advance_ring(WORD* pointer, int words, Buffer* buffer, bool backwards=false) {
    auto address = reinterpret_cast<std::uintptr_t>(pointer) + static_cast<std::uintptr_t>(std::ptrdiff_t(words) * 2);
    if (backwards) {
        if (address < reinterpret_cast<std::uintptr_t>(buffer->BufferHead)) address += buffer->BufferSize;
    } else if (address >= reinterpret_cast<std::uintptr_t>(buffer->BufferTail)) address -= buffer->BufferSize;
    return reinterpret_cast<WORD*>(address);
}
}
bool Drawing::RLEBlit(Surface* dest, const RectangleStruct* dc, const RectangleStruct* desired,
    Surface* source, const RectangleStruct* sc, const RectangleStruct* selected, RLEBlitter* blitter,
    int zadjust, int gradient_index, int brightness, int, Surface* zshape, int zx, int zy, int tint) {
    auto s=*selected, d=*desired;
    if (!clip_image_rects(d,*dc,s,*sc)) return false;
    const int left=s.X-sc->X, top=s.Y-sc->Y;
#ifndef RA2_IMAGE_GAME
    if (ZBuffer::Instance) {
        Drawing::GetZGradient(gradient_index);
        if (!ABuffer::Instance)
            throw std::logic_error("Original RLE Z rendering also requires an ABuffer ring");
    }
#endif
#ifndef RA2_IMAGE_GAME
    // Z-adjust readers can consume two bytes per source pixel. Allocate before
    // acquiring surfaces so allocation failure cannot strand a surface lock.
    std::vector<BYTE> zero_adjust;
    if (ZBuffer::Instance) zero_adjust.resize((std::size_t(sc->Width) + 1u) * 2u, BYTE(0));
#endif
    auto* output=static_cast<BYTE*>(dest->Lock(dc->X+d.X,dc->Y+d.Y));
    if (!output) return false;
    auto* z=ZBuffer::Instance; auto* a=ABuffer::Instance;
    WORD *zp=nullptr, *ap=nullptr;
    int zbase=0, zstep=0, zlimit=0, phase=0, zpitch=0, apitch=0, warp=0, aux_pitch=0;
    BYTE* aux=nullptr;
    const auto* gradient=z ? Drawing::GetZGradient(gradient_index) : nullptr;
    if (z) {
        const int y=dc->Y+d.Y-z->Bounds.Y;
        zp=static_cast<WORD*>(z->GetBuffer(dc->X+d.X,y));
        zstep=gradient[2]; zlimit=gradient[3]; zpitch=z->Width;
        const int ratio=zlimit/zstep;
        if (BYTE(gradient[5])) zbase=gradient[1]*((zadjust+WORD(z->MaxValue-y))/gradient[1]);
        else {
            zbase=zadjust+WORD(z->Bounds.Y+z->MaxValue-dc->Y-desired->Height-desired->Y+1);
            if (!zshape) {
                const int rows=desired->Y+desired->Height-d.Y;
                zbase=ratio*(zbase/ratio)-rows/ratio;
                phase=zlimit-rows%ratio;
                if (phase==zlimit) { phase=0; zbase+=gradient[4]; }
            }
        }
        if (zshape) {
            RectangleStruct bounds; zshape->GetRect(&bounds); aux_pitch=bounds.Width;
            aux=static_cast<BYTE*>(zshape->Lock(left+zx+bounds.X,top+zy+bounds.Y));
        } else {
#ifdef RA2_IMAGE_GAME
            aux=game::OriginalZeroZAdjust();
#else
            aux=zero_adjust.data();
#endif
        }
    }
    if (a) {
        const int x=dc->X+d.X, y=dc->Y+d.Y-a->Bounds.Y;
        ap=static_cast<WORD*>(a->GetBuffer(x,y)); apitch=a->Width;
        warp=(2*(y&1)) | ((x^y)&1);
    }
    auto* input=static_cast<BYTE*>(source->Lock(0, 0));
    if (!input) { dest->Unlock(); return false; }
    for (int i=0;i<top;++i) { input+=line_length(input); warp^=3; }
    const int pitch=dest->GetPitch(), height=std::min(s.Height,d.Height);
    for (int y=0;y<height;++y) {
        invoke_rle_blitter(blitter,output,input+2,s.Width,left,zbase,zp,ap,brightness,warp,aux,WORD(tint));
        output+=pitch; input+=line_length(input); warp^=3;
        if (z) {
            zp=advance_ring(zp,zpitch,z);
            if (zshape) aux+=aux_pitch;
            else { phase+=zstep; if (phase>=zlimit) { zbase+=gradient[4]; phase-=zlimit; } }
            // YR updates the alpha row inside the Z-buffer branch.
            ap=advance_ring(ap,apitch,a);
        }
    }
    dest->Unlock(); source->Unlock();
    return true;
}

bool Drawing::PlainBlit(Surface* dest, const RectangleStruct* dc, const RectangleStruct* desired,
    Surface* source, const RectangleStruct* sc, const RectangleStruct* selected, Blitter* blitter,
    int zadjust, int gradient_index, int brightness, int tint, int) {
    auto d=*desired, s=*selected;
    if (d.Width<=0 || d.Height<=0 || dc->Width<=0 || dc->Height<=0 ||
        s.Width<=0 || s.Height<=0 || sc->Width<=0 || sc->Height<=0 || !clip_image_rects(d,*dc,s,*sc)) return false;
    const bool backwards=dest==source && s.X<d.X+d.Width && s.Y<d.Y+d.Height &&
        s.X+s.Width>d.X && s.Y+s.Height>d.Y && (s.Y<d.Y || (s.Y==d.Y && s.X<d.X));
#ifndef RA2_IMAGE_GAME
    if (ZBuffer::Instance) Drawing::GetZGradient(gradient_index);
#endif
    auto* output=static_cast<BYTE*>(dest->Lock(dc->X+d.X,dc->Y+d.Y));
    if (!output) return false;
    auto* input=static_cast<BYTE*>(source->Lock(sc->X+s.X,sc->Y+s.Y));
    if (!input) { dest->Unlock(); return false; }
    auto* z=ZBuffer::Instance; auto* a=ABuffer::Instance;
    WORD *zp=nullptr, *ap=nullptr;
    int zbase=0, zstep=0, zlimit=0, phase=0, warp=0;
    const auto* gradient=z ? Drawing::GetZGradient(gradient_index) : nullptr;
    if (a) {
        const int x=dc->X+d.X, y=dc->Y+d.Y-a->Bounds.Y;
        ap=static_cast<WORD*>(a->GetBuffer(x,y)); warp=(2*(y&1)) | ((x^y)&1);
    }
    if (z) {
        zp=static_cast<WORD*>(z->GetBuffer(dc->X+d.X,dc->Y+d.Y-z->Bounds.Y));
        zstep=gradient[2]; zlimit=gradient[3]; const int ratio=zlimit/zstep;
        if (BYTE(gradient[5])) zbase=gradient[1]*((zadjust+WORD(z->Bounds.Y+z->MaxValue-d.Y-dc->Y))/gradient[1]);
        else {
            zbase=ratio*((zadjust+WORD(z->Bounds.Y+z->MaxValue-d.Height-d.Y-dc->Y+1))/ratio)-d.Height/ratio;
            phase=zlimit-d.Height%ratio;
            if (phase==zlimit) { phase=0; zbase+=gradient[4]; }
        }
    }
    const int dest_pitch=dest->GetPitch(), source_pitch=source->GetPitch();
    if (d.Width*dest->GetBytesPerPixel()==dest_pitch && dest_pitch==source_pitch) {
        invoke_plain_blitter(blitter,backwards,output,input,std::min(s.Width*s.Height,d.Width*d.Height),0,nullptr,nullptr,1000,0,0);
    } else {
        int dp=dest_pitch, sp=source_pitch, zpitch=z?z->Width:0, apitch=a?a->Width:0;
        if (backwards) {
            dp=-dp; sp=-sp; zpitch=-zpitch; apitch=-apitch;
            input+=(s.Height-1)*source_pitch; output+=(d.Height-1)*dest_pitch;
            if (z) { zp=advance_ring(zp,s.Height-z->Width,z); zbase+=1-s.Height; }
            if (a) ap=advance_ring(ap,s.Height-a->Width,a);
        }
        const int height=std::min(s.Height,d.Height);
        for (int y=0;y<height;++y) {
            invoke_plain_blitter(blitter,backwards,output,input,s.Width,zbase,zp,ap,brightness,warp,WORD(tint));
            output+=dp; input+=sp;
            if (!backwards) warp^=3;
            if (z) {
                zp=advance_ring(zp,zpitch,z,backwards); phase+=zstep;
                if (phase>=zlimit) { zbase+=gradient[4]; phase-=zlimit; }
            }
            if (a) ap=advance_ring(ap,apitch,a,backwards);
        }
    }
    dest->Unlock(); source->Unlock();
    return true;
}

void YRPP_FASTCALL CC_Draw_Shape(Surface* dest, ConvertClass* convert, SHPStruct* shape, int frame,
    const Vector2D<int>* position, const RectangleStruct* bounds, BlitterFlags draw_flags, BYTE* remap,
    int zadjust, ZGradient z_gradient, int brightness, int tint, SHPStruct* zshape, int zframe, int zx, int zy) {
    unsigned flags = static_cast<unsigned>(draw_flags);
    const int gradient = static_cast<int>(z_gradient);
    if (!shape) return;
    if (auto* reference=shape->AsReference()) { if (!reference->Loaded) reference->Load(); shape=reference->Data; }
    if (!shape) return;
    RectangleStruct surface_rect; dest->GetRect(&surface_rect);
    const auto clip=intersect_image_rect(surface_rect,*bounds);
    if (clip.Width<=0 || clip.Height<=0) return;
    convert->CurrentZRemap=remap;
    auto rect=shape->GetFrameBounds(frame);
    auto* pixels=shape->GetPixels(frame);
    alignas(BSurface) std::byte storage[sizeof(BSurface)];
    auto* source=BSurface::Initialize(storage,rect.Width,rect.Height,1,pixels);
    struct SourceLifetime { BSurface* value; ~SourceLifetime() { if (value) value->~BSurface(); } } source_lifetime{source};
    BSurface* auxiliary=nullptr;
    std::unique_ptr<BSurface, decltype(&BSurface::Destroy)> auxiliary_lifetime(nullptr, BSurface::Destroy);
    RectangleStruct zrect{};
    if (zshape) {
        zrect=zshape->GetFrameBounds(zframe);
        // Target allocates before fetching Z pixels, which can trigger loading.
        void* object=YRMemory::Allocate(sizeof(BSurface));
        if (object) {
            try { auxiliary=BSurface::Initialize(object,zrect.Width,zrect.Height,1,zshape->GetPixels(zframe)); }
            catch (...) { YRMemory::Deallocate(object); throw; }
            auxiliary_lifetime.reset(auxiliary);
        }
    }
    int x=position->X, y=position->Y;
    if (flags&0x200) { x+=shape->Width/-2; y+=shape->Height/-2; }
    if (zshape) { zx-=shape->Width/2-rect.X; zy-=shape->Height/2-rect.Y; }
    x+=rect.X; y+=rect.Y; rect.X=rect.Y=0;
    if (zshape) {
        zx+=zrect.X; zy+=zrect.Y;
        rect=intersect_image_rect(rect,RectangleStruct{-zx,-zy,zrect.Width,zrect.Height});
    }
    if (!zshape || (rect.Width>0 && rect.Height>0)) {
        if (remap) flags|=0x10;
        const RectangleStruct desired{x,y,rect.Width,rect.Height};
        if (shape->HasCompression(frame)) {
            if (auto* blitter=convert->SelectRLEBlitter(static_cast<BlitterFlags>(flags)))
                Drawing::RLEBlit(dest,&clip,&desired,reinterpret_cast<Surface*>(source),&rect,&rect,blitter,
                    zadjust,gradient,brightness,0,reinterpret_cast<Surface*>(auxiliary),zx,zy,tint);
        } else {
            const RectangleStruct source_rect{0,0,source->Width,source->Height};
            if (auto* blitter=convert->SelectPlainBlitter(static_cast<BlitterFlags>(flags)))
                Drawing::PlainBlit(dest,&clip,&desired,reinterpret_cast<Surface*>(source),&source_rect,&rect,
                    blitter,zadjust,gradient,brightness,tint,0);
        }
    }
    auxiliary_lifetime.release();
    if (auxiliary) BSurface::Destroy(auxiliary);
    source_lifetime.value = nullptr;
    source->~BSurface();
}

// Non-template interface helpers; bodies retained from the corresponding header.

void DSurface::DrawSHP(ConvertClass* Palette, SHPStruct* SHP, int FrameIndex,
    const Point2D* const Position, const RectangleStruct* const Bounds, BlitterFlags Flags, BYTE* Remap,
    int ZAdjust, ZGradient ZGradientDescIndex, int Brightness, int TintColor,
    SHPStruct* ZShape, int ZShapeFrame, int XOffset, int YOffset)
{
    CC_Draw_Shape(this, Palette, SHP, FrameIndex, Position, Bounds, Flags, Remap, ZAdjust,
        ZGradientDescIndex, Brightness, TintColor, ZShape, ZShapeFrame, XOffset, YOffset);
}
