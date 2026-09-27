#pragma once
#include "api/type_stream.hpp"
#include "yrpp/AbstractTypeClass.h"
#include <array>
#include <bit>
#include <cstddef>
#include <cstring>
class TechnoTypeClass;
class ObjectClass;
namespace game {
inline constexpr HRESULT stream_pointer_error = static_cast<HRESULT>(0x80004003u);
inline constexpr HRESULT stream_failure = static_cast<HRESULT>(0x80004005u);
inline constexpr HRESULT stream_invalid_record = static_cast<HRESULT>(0x800300fbu);
HRESULT type_stream_error(HRESULT) noexcept;
HRESULT read_type_bytes(IStream*, void*, std::uint32_t) noexcept;
HRESULT write_type_bytes(IStream*, const void*, std::uint32_t) noexcept;
// Original IStream calls with a null transferred-count pointer: preserve the
// HRESULT, including nonnegative short transfers. Checked type-record codecs
// continue to use read_type_bytes/write_type_bytes above.
HRESULT read_stream_bytes(IStream*, void*, std::uint32_t) noexcept;
HRESULT write_stream_bytes(IStream*, const void*, std::uint32_t) noexcept;
// A layer slot contains a saved 32-bit identity until this deferred swizzle.
// Register only after the layer has finished growing; storage must outlive
// resolve/cancel. No exception crosses the transport/registration boundary.
HRESULT swizzle_object_reference(ObjectClass*&) noexcept;
HRESULT type_stream_save_token(const AbstractClass*, std::uint32_t&) noexcept;
HRESULT type_stream_load_token(std::uint32_t, AbstractClass*) noexcept;
HRESULT queue_techno_references(TechnoTypeClass** const*, const std::uint32_t*, std::size_t) noexcept;
// Internal typed fixups; no T** -> AbstractClass** aliasing or parallel objects.
struct TypeStreamFixup {
    void* destination;
    std::uint32_t token;
    const AbstractClass* owner;
    bool (*accepts)(AbstractClass*) noexcept;
    void (*assign)(void*, AbstractClass*) noexcept;
};
template<class T> TypeStreamFixup type_fixup(T*& slot, std::uint32_t token,
        const AbstractClass* owner) noexcept {
    return {&slot,token,owner,
        [](AbstractClass* value) noexcept { return dynamic_cast<T*>(value)!=nullptr; },
        [](void* target,AbstractClass* value) noexcept { *static_cast<T**>(target)=static_cast<T*>(value); }};
}
// Atomically reserves/registers/replaces this owner's pending fixups. Call only
// after full record validation/allocation; destination storage must remain live.
HRESULT commit_type_references(std::uint32_t, AbstractClass*, const TypeStreamFixup*, std::size_t) noexcept;
std::uint32_t type_stream_max_elements() noexcept;
HRESULT encode_abstract_record(const AbstractClass*, unsigned char*, std::uint32_t) noexcept;
HRESULT decode_abstract_record(AbstractClass*, const unsigned char*, std::uint32_t) noexcept;
HRESULT load_abstract_record(std::uint32_t, AbstractClass*, const unsigned char*, std::uint32_t) noexcept;
inline std::uint32_t get_u32(const unsigned char* p) noexcept {
    return std::uint32_t(p[0]) | (std::uint32_t(p[1])<<8) | (std::uint32_t(p[2])<<16) | (std::uint32_t(p[3])<<24);
}
inline std::int32_t get_i32(const unsigned char* p) noexcept { return std::bit_cast<std::int32_t>(get_u32(p)); }
inline void put_u32(unsigned char* p, std::uint32_t v) noexcept {
    for (unsigned i=0;i<4;++i) p[i]=static_cast<unsigned char>(v>>(i*8));
}
inline HRESULT read_type_u32(IStream* stream, std::uint32_t& value) noexcept {
    unsigned char bytes[4]{}; const auto hr=read_type_bytes(stream,bytes,4);
    if(hr>=0) value=get_u32(bytes); return hr;
}
inline HRESULT write_type_u32(IStream* stream, std::uint32_t value) noexcept {
    unsigned char bytes[4]; put_u32(bytes,value); return write_type_bytes(stream,bytes,4);
}
template<std::size_t N> HRESULT read_type_record(IStream* stream, std::array<unsigned char,N>& record,
        std::uint32_t& token) noexcept {
    auto hr=read_type_u32(stream,token); if(hr<0) return hr;
    if(!token) return type_stream_error(stream_invalid_record);
    hr=read_type_bytes(stream,record.data(),N);
    if(hr<0) return hr;
    // Base ID has a dedicated terminator at +60, not inside its 24 bytes.
    if(record[60] || !std::memchr(record.data()+61,0,32) ||
        !std::memchr(record.data()+100,0,49) || record[32]>1)
        return type_stream_error(stream_invalid_record);
    return 0;
}
template<std::size_t N> HRESULT write_type_record(IStream* stream, AbstractTypeClass& self,
        std::array<unsigned char,N>& record, BOOL clear) noexcept {
    std::uint32_t token=0; auto hr=type_stream_save_token(&self,token); if(hr<0)return hr;
    hr=write_type_u32(stream,token); if(hr<0)return hr;
    hr=write_type_bytes(stream,record.data(),N);
    if(hr>=0 && clear) self.Dirty=false;
    return hr;
}
}
