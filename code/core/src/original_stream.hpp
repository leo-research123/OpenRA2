#pragma once
#include "yrpp/Interfaces.h"

namespace game {
// IStream remains opaque on portable hosts. COM Read/Write are slots 3/4.
inline HRESULT original_stream_read(IStream* stream, void* data, ULONG length) {
    const auto table = *reinterpret_cast<void* const* const*>(stream);
    const auto read = reinterpret_cast<HRESULT(YRPP_STDCALL*)(IStream*, void*, ULONG, ULONG*)>(table[3]);
    return read(stream, data, length, nullptr);
}
inline HRESULT original_stream_write(IStream* stream, const void* data, ULONG length) {
    const auto table = *reinterpret_cast<void* const* const*>(stream);
    const auto write = reinterpret_cast<HRESULT(YRPP_STDCALL*)(IStream*, const void*, ULONG, ULONG*)>(table[4]);
    return write(stream, data, length, nullptr);
}
}
