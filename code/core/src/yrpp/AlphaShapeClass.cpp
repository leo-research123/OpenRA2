// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// Adaptations Copyright 2026 RedAlert2Open; EA terms: third_party/opents/LICENSE.md.
// OpenTS 44fac744 alphashp.cpp lifecycle, calibrated to YR 0x00420960..0x00420F3A.
#include "yrpp/AlphaShapeClass.h"
#include "yrpp/ObjectClass.h"
#include "yrpp/ObjectTypeClass.h"
#include "yrpp/CRC.h"
#include "yrpp/SwizzleManagerClass.h"
#include <cstdlib>
#include <cstring>
#include <new>

#if !defined(RA2_YRPP_GAME)
namespace { DynamicVectorClass<AlphaShapeClass*> shapes; }
DynamicVectorClass<AlphaShapeClass*>& AlphaShapeClass::Array=shapes;
#endif

namespace {
void prepare_brightness() noexcept {
#if defined(RA2_YRPP_GAME)
    // Original DrawAll can still consume this table in an injected build.
    auto& ready=*reinterpret_cast<bool*>(0x0089A134);
    if(ready)return;
    ready=true;
    auto* table=reinterpret_cast<BYTE*>(0x0088A118);
    for(int i=0;i<0x10000;++i){const int value=(i%0x100)*(i/0x100)/127;table[i]=BYTE(value>0xFF?0xFF:value);}
#endif
    // Native raster backends evaluate this same formula directly.
}
void register_shape(AlphaShapeClass* shape) noexcept {
#if defined(RA2_YRPP_GAME)
    // Original attempts both insertions even when growing either list fails.
    AlphaShapeClass::Array.AddItem(shape);AbstractClass::Array.AddItem(shape);
#else
    // Same two original registries, with native ObjectClass's fatal policy for
    // allocation failure: never leave an untracked live attachment behind.
    if(!AlphaShapeClass::Array.AddItem(shape)||!AbstractClass::Array.AddItem(shape))std::abort();
#endif
    prepare_brightness();
}
}

AlphaShapeClass::AlphaShapeClass(ObjectClass* owner,int x,int y) noexcept
    : AbstractClass(),AttachedTo(owner),Rect{},AlphaImage(owner->GetType()->AlphaImage),IsObjectGone(false) {
    Rect={x,y,AlphaImage->Width,AlphaImage->Height};
    register_shape(this);
}
AlphaShapeClass::AlphaShapeClass() noexcept
    : AbstractClass(),AttachedTo(nullptr),Rect{},AlphaImage(nullptr),IsObjectGone(false) {
    register_shape(this);
}
AlphaShapeClass::~AlphaShapeClass() {
    Array.Remove(this);
    AbstractClass::Array.Remove(this);
}
void AlphaShapeClass::PointerExpired(AbstractClass* object,bool) {
    // 0x00420E70 marks even a non-permanent expiration and keeps the pointer.
    // DrawAll still consumes the cached image until the next logic purge.
    if(object==AttachedTo)IsObjectGone=true;
}
void YRPP_CDECL AlphaShapeClass::UpdateAll() {
    prepare_brightness();
    for(int i=Array.Count-1;i>=0;--i)
        if(Array[i]->IsObjectGone)GameDelete(Array[i]);
}
AbstractType AlphaShapeClass::WhatAmI() const {return AbsID;}
int AlphaShapeClass::Size() const {return sizeof(*this);}
void AlphaShapeClass::ComputeCRC(CRCEngine& crc) const {
    AbstractClass::ComputeCRC(crc);
    crc(Rect.X);crc(Rect.Y);crc(Rect.Width);crc(Rect.Height);
}
HRESULT YRPP_STDCALL AlphaShapeClass::GetClassID(CLSID* result) {
    if(!result)return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD id[]{0x623C7584u,0x11D274E7u,0x6000F5B8u,0xED09C808u};
    static_assert(sizeof(id)==sizeof(CLSID));
    std::memcpy(result,id,sizeof(id));
    return 0;
}
HRESULT YRPP_STDCALL AlphaShapeClass::Save(IStream* stream,BOOL clearDirty) {
#if defined(RA2_YRPP_GAME)
    const auto result=AbstractClass::Save(stream,clearDirty);
    return result<0?result:0;
#else
    // Full original savegame transport is not part of the native map renderer.
    // A successful placeholder must not silently discard the attachment.
    (void)clearDirty;
    return static_cast<HRESULT>(stream?0x80004001u:0x80004003u);
#endif
}
HRESULT YRPP_STDCALL AlphaShapeClass::Load(IStream* stream) {
#if defined(RA2_YRPP_GAME)
    const auto result=AbstractClass::Load(stream);
    if(result>=0){
        ::new(this) AlphaShapeClass(noinit_t());
        SwizzleManagerClass::Instance.Swizzle(reinterpret_cast<void**>(&AttachedTo));
        AlphaImage=nullptr;
    }
    return result;
#else
    return static_cast<HRESULT>(stream?0x80004001u:0x80004003u);
#endif
}

#if defined(_MSC_VER)&&defined(_M_IX86)
static_assert(sizeof(AlphaShapeClass)==0x40);
static_assert(offsetof(AlphaShapeClass,AttachedTo)==0x24);
static_assert(offsetof(AlphaShapeClass,Rect)==0x28);
static_assert(offsetof(AlphaShapeClass,AlphaImage)==0x38);
static_assert(offsetof(AlphaShapeClass,IsObjectGone)==0x3C);
#endif
