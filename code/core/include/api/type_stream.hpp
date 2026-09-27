#pragma once
#include <cstdint>
#include "yrpp/Interfaces.h"
class AbstractClass;
namespace game {
struct TypeStreamSession;
struct TypeStreamTransport {
    void* context = nullptr;
    HRESULT (*read)(void*, IStream*, void*, std::uint32_t, std::uint32_t&) noexcept = nullptr;
    HRESULT (*write)(void*, IStream*, const void*, std::uint32_t, std::uint32_t&) noexcept = nullptr;
    std::uint32_t max_list_elements = 1048576;
    // Optional full x86 record codecs for qualified AbstractClass::Save/Load.
    // Size() is the target record size, never native sizeof. Codecs must not
    // serialize host vptrs/pointers; a failing decoder must leave the object
    // unchanged, and decoding must retain the live RefCount.
    // Without codecs only the 0x24-byte AbstractClass record is supported.
    HRESULT (*encode_abstract)(void*, const AbstractClass*, unsigned char*, std::uint32_t) noexcept = nullptr;
    HRESULT (*decode_abstract)(void*, AbstractClass*, const unsigned char*, std::uint32_t) noexcept = nullptr;
};
enum class TypeStreamStatus { complete, invalid_argument, unavailable, failure, unresolved_references };
// A session owns only token maps/fixup bookkeeping, not types or streams.
// Objects and fixup destination fields must outlive resolve/cancel/destroy.
// Windows uses IStream directly when the corresponding callback is absent.
TypeStreamStatus create_type_stream_session(const TypeStreamTransport&, TypeStreamSession*&) noexcept;
// A borrowed session cannot be destroyed (including an outer nested scope):
// the pointer remains live and its scope reports failure. Destroy it after exit.
void destroy_type_stream_session(TypeStreamSession*&) noexcept;
// A stable 32-bit token within this session, never a truncated native pointer.
TypeStreamStatus type_stream_reference_id(TypeStreamSession&, const AbstractClass*, std::uint32_t&) noexcept;
// Bind external references (e.g. a real InfantryType loaded by another codec).
// Imported original 32-bit pointer values are accepted as opaque tokens.
TypeStreamStatus bind_type_stream_reference(TypeStreamSession&, std::uint32_t, AbstractClass*) noexcept;
TypeStreamStatus resolve_type_stream_references(TypeStreamSession&, std::uint32_t& unresolved) noexcept;
// Discard pending addresses before their owning objects are destroyed. No
// destination is dereferenced by cancellation or session destruction.
void cancel_type_stream_references(TypeStreamSession&) noexcept;
TypeStreamStatus with_type_stream(TypeStreamSession&, void (*operation)(void*), void*) noexcept;
}
