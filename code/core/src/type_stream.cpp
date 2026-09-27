#include "type_stream.hpp"
#include "yrpp/TechnoTypeClass.h"
#include "yrpp/ObjectClass.h"
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <limits>
#include <new>
namespace game {
struct TypeStreamSession {
    TypeStreamTransport transport;
    unsigned borrow_count=0;
    std::unordered_map<const AbstractClass*,std::uint32_t> saving;
    std::unordered_map<std::uint32_t,AbstractClass*> loaded;
    std::vector<TypeStreamFixup> pending;
    std::uint32_t next_token=1;
    TypeStreamStatus status=TypeStreamStatus::complete;
};
namespace { thread_local TypeStreamSession* active=nullptr; }
TypeStreamStatus create_type_stream_session(const TypeStreamTransport& transport, TypeStreamSession*& out) noexcept {
    if(out || !transport.max_list_elements) return TypeStreamStatus::invalid_argument;
    try { out=new TypeStreamSession; out->transport=transport; return TypeStreamStatus::complete; }
    catch (...) { return TypeStreamStatus::failure; }
}
void destroy_type_stream_session(TypeStreamSession*& session) noexcept {
    if(session && session->borrow_count) { session->status=TypeStreamStatus::failure; return; }
    delete session; session=nullptr;
}
TypeStreamStatus type_stream_reference_id(TypeStreamSession& session, const AbstractClass* object,
        std::uint32_t& output) noexcept {
    if(!object) { output=0; return TypeStreamStatus::complete; }
    try {
        const auto found=session.saving.find(object);
        if(found!=session.saving.end()) { output=found->second; return TypeStreamStatus::complete; }
        if(!session.next_token) return TypeStreamStatus::failure;
        const auto token=session.next_token;
        session.saving.emplace(object,token);
        ++session.next_token; output=token;
        return TypeStreamStatus::complete;
    } catch (...) { return TypeStreamStatus::failure; }
}
TypeStreamStatus bind_type_stream_reference(TypeStreamSession& session, std::uint32_t token,
        AbstractClass* object) noexcept {
    if(!token || !object) return TypeStreamStatus::invalid_argument;
    try {
        auto [where,inserted]=session.loaded.emplace(token,object);
        return inserted || where->second==object ? TypeStreamStatus::complete : TypeStreamStatus::failure;
    } catch (...) { return TypeStreamStatus::failure; }
}
TypeStreamStatus resolve_type_stream_references(TypeStreamSession& session, std::uint32_t& unresolved) noexcept {
    unresolved=0;
    // Validate all references first; no partially committed pointer graph.
    for(const auto& item:session.pending) {
        const auto found=session.loaded.find(item.token);
        if(found==session.loaded.end()) { ++unresolved; continue; }
        if(!item.accepts(found->second)) return TypeStreamStatus::failure;
    }
    if(unresolved) return TypeStreamStatus::unresolved_references;
    for(const auto& item:session.pending)
        item.assign(item.destination,session.loaded.find(item.token)->second);
    session.pending.clear();
    return TypeStreamStatus::complete;
}
void cancel_type_stream_references(TypeStreamSession& session) noexcept { session.pending.clear(); }
TypeStreamStatus with_type_stream(TypeStreamSession& session, void (*operation)(void*), void* argument) noexcept {
    if(!operation) return TypeStreamStatus::invalid_argument;
    auto* previous=active; const auto previous_status=session.status;
    session.status=TypeStreamStatus::complete; active=&session; ++session.borrow_count;
    try { operation(argument); }
    catch (...) { session.status=TypeStreamStatus::failure; }
    const auto result=session.status;
    active=previous; --session.borrow_count;
    if(previous==&session && previous_status!=TypeStreamStatus::complete) session.status=previous_status;
    return result;
}
HRESULT type_stream_error(HRESULT hr) noexcept {
    if(hr<0 && active) active->status=TypeStreamStatus::failure;
    return hr;
}
HRESULT read_stream_bytes(IStream* stream, void* data, std::uint32_t size) noexcept {
    if(!stream || !data) return type_stream_error(stream_pointer_error);
    try {
        if(active && active->transport.read) {
            std::uint32_t ignored=0;
            return type_stream_error(active->transport.read(active->transport.context,stream,data,size,ignored));
        }
#ifdef _WIN32
        return type_stream_error(stream->Read(data,size,nullptr));
#else
        if(active) active->status=TypeStreamStatus::unavailable;
        return static_cast<HRESULT>(0x80004001u);
#endif
    } catch (...) { return type_stream_error(stream_failure); }
}
HRESULT write_stream_bytes(IStream* stream, const void* data, std::uint32_t size) noexcept {
    if(!stream || !data) return type_stream_error(stream_pointer_error);
    try {
        if(active && active->transport.write) {
            std::uint32_t ignored=0;
            return type_stream_error(active->transport.write(active->transport.context,stream,data,size,ignored));
        }
#ifdef _WIN32
        return type_stream_error(stream->Write(data,size,nullptr));
#else
        if(active) active->status=TypeStreamStatus::unavailable;
        return static_cast<HRESULT>(0x80004001u);
#endif
    } catch (...) { return type_stream_error(stream_failure); }
}
HRESULT swizzle_object_reference(ObjectClass*& slot) noexcept {
    if(!slot) return 0;
    if(!active) return static_cast<HRESULT>(0x80004001u);
    const auto token=reinterpret_cast<std::uintptr_t>(slot);
    if(token>std::numeric_limits<std::uint32_t>::max())
        return type_stream_error(static_cast<HRESULT>(0x80070057u));
    try {
        active->pending.push_back(type_fixup(slot,static_cast<std::uint32_t>(token),nullptr));
    } catch (...) {
        // Like SwizzleManager::Swizzle, clear the destination even if its
        // request cannot be recorded; the session also reports the failure.
        type_stream_error(stream_failure);
    }
    slot=nullptr;
    return 0;
}
HRESULT read_type_bytes(IStream* stream, void* data, std::uint32_t size) noexcept {
    if(!stream || !data) return type_stream_error(stream_pointer_error);
    if(!active) return static_cast<HRESULT>(0x80004001u);
    try {
        std::uint32_t transferred=0; HRESULT hr;
        if(active->transport.read) hr=active->transport.read(active->transport.context,stream,data,size,transferred);
        else {
#ifdef _WIN32
            ULONG bytes=0; hr=stream->Read(data,size,&bytes); transferred=bytes;
#else
            active->status=TypeStreamStatus::unavailable; return static_cast<HRESULT>(0x80004001u);
#endif
        }
        if(hr<0) return type_stream_error(hr);
        if(transferred!=size) return type_stream_error(static_cast<HRESULT>(0x8003001eu));
        return hr;
    } catch (...) { return type_stream_error(stream_failure); }
}
HRESULT write_type_bytes(IStream* stream, const void* data, std::uint32_t size) noexcept {
    if(!stream || !data) return type_stream_error(stream_pointer_error);
    if(!active) return static_cast<HRESULT>(0x80004001u);
    try {
        std::uint32_t transferred=0; HRESULT hr;
        if(active->transport.write) hr=active->transport.write(active->transport.context,stream,data,size,transferred);
        else {
#ifdef _WIN32
            ULONG bytes=0; hr=stream->Write(data,size,&bytes); transferred=bytes;
#else
            active->status=TypeStreamStatus::unavailable; return static_cast<HRESULT>(0x80004001u);
#endif
        }
        if(hr<0) return type_stream_error(hr);
        if(transferred!=size) return type_stream_error(static_cast<HRESULT>(0x8003001du));
        return hr;
    } catch (...) { return type_stream_error(stream_failure); }
}
HRESULT type_stream_save_token(const AbstractClass* object,std::uint32_t& token) noexcept {
    if(!active) return static_cast<HRESULT>(0x80004001u);
    return type_stream_reference_id(*active,object,token)==TypeStreamStatus::complete ? 0 : type_stream_error(stream_failure);
}
HRESULT type_stream_load_token(std::uint32_t token,AbstractClass* object) noexcept {
    if(!active) return static_cast<HRESULT>(0x80004001u);
    return bind_type_stream_reference(*active,token,object)==TypeStreamStatus::complete ? 0 : type_stream_error(stream_failure);
}
HRESULT queue_techno_references(TechnoTypeClass** const* slots,const std::uint32_t* tokens,std::size_t count) noexcept {
    if(!active) return static_cast<HRESULT>(0x80004001u);
    try {
        // Reserve first, so allocation failure cannot publish half a new fixup set.
        active->pending.reserve(active->pending.size()+count);
        for(std::size_t i=0;i<count;++i) {
            auto& pending=active->pending;
            pending.erase(std::remove_if(pending.begin(),pending.end(),[&](auto& f){return f.destination==slots[i];}),pending.end());
            *slots[i]=nullptr;
            if(tokens[i]) pending.push_back(type_fixup(*slots[i],tokens[i],nullptr));
        }
        return 0;
    } catch (...) { return type_stream_error(stream_failure); }
}
std::uint32_t type_stream_max_elements() noexcept { return active ? active->transport.max_list_elements : 0; }
HRESULT commit_type_references(std::uint32_t token, AbstractClass* object,
        const TypeStreamFixup* fixups, std::size_t count) noexcept {
    if (!active) return static_cast<HRESULT>(0x80004001u);
    if (!token || !object || (count && !fixups)) return type_stream_error(stream_invalid_record);
    try {
        auto& pending=active->pending;
        if (count>pending.max_size()-pending.size()) return type_stream_error(stream_invalid_record);
        // Both allocations precede mutation of existing bindings/destinations.
        pending.reserve(pending.size()+count);
        auto [where,inserted]=active->loaded.emplace(token,object);
        if (!inserted && where->second!=object) return type_stream_error(stream_invalid_record);
        pending.erase(std::remove_if(pending.begin(),pending.end(),
            [object](const auto& fixup){return fixup.owner==object;}),pending.end());
        for (std::size_t i=0;i<count;++i) {
            fixups[i].assign(fixups[i].destination,nullptr);
            if (fixups[i].token) pending.push_back(fixups[i]);
        }
        return 0;
    } catch (...) { return type_stream_error(stream_failure); }
}
HRESULT encode_abstract_record(const AbstractClass* object, unsigned char* record, std::uint32_t size) noexcept {
    if (!active) return static_cast<HRESULT>(0x80004001u);
    if (active->transport.encode_abstract)
        return type_stream_error(active->transport.encode_abstract(active->transport.context, object, record, size));
    if (size != 0x24) return type_stream_error(static_cast<HRESULT>(0x80004001u));
    constexpr std::uint32_t tables[]{0x7E1F50, 0x7E1F34, 0x7E1F2C, 0x7E1F24};
    for (unsigned i = 0; i < 4; ++i) put_u32(record + i*4, tables[i]);
    put_u32(record + 0x10, object->UniqueID);
    put_u32(record + 0x14, static_cast<std::uint32_t>(object->AbstractFlags));
    put_u32(record + 0x18, object->unknown_18);
    put_u32(record + 0x1C, static_cast<std::uint32_t>(object->RefCount));
    record[0x20] = object->Dirty;
    return 0;
}
HRESULT decode_abstract_record(AbstractClass* object, const unsigned char* record, std::uint32_t size) noexcept {
    if (!active) return static_cast<HRESULT>(0x80004001u);
    if (active->transport.decode_abstract)
        return type_stream_error(active->transport.decode_abstract(active->transport.context, object, record, size));
    if (size != 0x24) return type_stream_error(static_cast<HRESULT>(0x80004001u));
    if (record[0x20] > 1) return type_stream_error(stream_invalid_record);
    object->UniqueID = get_u32(record + 0x10);
    object->AbstractFlags = static_cast<AbstractFlags>(get_u32(record + 0x14));
    object->unknown_18 = get_u32(record + 0x18);
    object->Dirty = record[0x20] != 0;
    return 0;
}
HRESULT load_abstract_record(std::uint32_t token, AbstractClass* object,
        const unsigned char* record, std::uint32_t size) noexcept {
    if (!active) return static_cast<HRESULT>(0x80004001u);
    if (!token) return type_stream_error(stream_invalid_record);
    try {
        auto [position, inserted] = active->loaded.emplace(token, object);
        if (!inserted && position->second != object) return type_stream_error(stream_invalid_record);
        const auto hr = decode_abstract_record(object, record, size);
        if (hr < 0 && inserted) active->loaded.erase(token);
        return hr;
    } catch (...) { return type_stream_error(stream_failure); }
}
}
