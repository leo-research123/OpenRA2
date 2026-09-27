// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// OpenTS 44fac744 layer.cpp Load/Save; YR 0x00551B90/0x00551B20 restores COM
// HRESULT, append-before-swizzle order and 32-bit identities, not host pointers.
// Additional terms: third_party/opents/LICENSE.md.
#include "yrpp/MapClass.h"
#include "yrpp/ObjectClass.h"
#include "type_stream.hpp"

#if !defined(RA2_YRPP_GAME)
HRESULT LayerClass::Save(IStream* stream) {
    if(!stream) return game::stream_pointer_error;
    const int count=Count;
    auto result=game::write_stream_bytes(stream,&count,4);
    if(result<0) return result;
    std::uint32_t identity=0;
    for(int i=0;i<count;++i) {
        result=game::type_stream_save_token(Items[i],identity);
        if(result<0) return result;
        result=game::write_stream_bytes(stream,&identity,4);
        if(result<0) return result;
    }
    return 0;
}
HRESULT LayerClass::Load(IStream* stream) {
    if(!stream) return game::stream_pointer_error;
    int count=0;
    auto result=game::read_stream_bytes(stream,&count,4);
    if(result<0) return result;
    std::uint32_t identity=0;
    for(int i=0;i<count;++i) {
        result=game::read_stream_bytes(stream,&identity,4);
        if(result<0) return result;
        try { AddItem(reinterpret_cast<ObjectClass*>(static_cast<std::uintptr_t>(identity))); }
        catch (...) { IsInitialized=true; } // Original ignores failed vector growth.
    }
    // Deliberately the first count slots, as in YR, even when appending to an
    // existing layer. Load callers supply storage for those slots. Register
    // only after all reads/growth, and do not replace a read failure by swizzling.
    for(int i=0;i<count;++i) game::swizzle_object_reference(Items[i]);
    return 0;
}
#endif
