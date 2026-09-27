// Qualified pure-virtual base bodies, gamemd 0x410320 / 0x410380.
#include "yrpp/AbstractClass.h"
#if defined(RA2_YRPP_GAME)
#include "abstract_runtime.hpp"
#else
#include "type_stream.hpp"
#include <vector>
#endif

HRESULT YRPP_STDCALL AbstractClass::Save(IStream* stream, BOOL clear) {
    if (!stream) return static_cast<HRESULT>(0x80004003u);
    try {
#if defined(RA2_YRPP_GAME)
        auto* identity = this;
        HRESULT hr = stream->Write(&identity, 4, nullptr);
        if (hr >= 0) {
            hr = stream->Write(this, static_cast<ULONG>(Size()), nullptr);
            if (hr >= 0 && clear) Dirty = false;
        }
#else
        const int size = Size();
        if (size < 0x24 || static_cast<std::uint32_t>(size) > game::type_stream_max_elements())
            return game::type_stream_error(game::stream_invalid_record);
        std::vector<unsigned char> record(static_cast<std::size_t>(size), 0);
        HRESULT hr = game::encode_abstract_record(this, record.data(), size);
        if (hr < 0) return hr;
        std::uint32_t identity = 0;
        hr = game::type_stream_save_token(this, identity);
        if (hr < 0) return hr;
        hr = game::write_type_u32(stream, identity);
        if (hr < 0) return hr;
        hr = game::write_type_bytes(stream, record.data(), size);
        if (hr >= 0 && clear) Dirty = false;
#endif
        return hr;
    } catch (...) {
        return static_cast<HRESULT>(0x80004005u);
    }
}

HRESULT YRPP_STDCALL AbstractClass::Load(IStream* stream) {
    if (!stream) return static_cast<HRESULT>(0x80004003u);
    const LONG live_refcount = RefCount;
    try {
#if defined(RA2_YRPP_GAME)
        std::uint32_t identity = 0;
        HRESULT hr = stream->Read(&identity, 4, nullptr);
        if (hr >= 0) {
            game::register_original_abstract(identity, this);
            // Original derived Load reconstructs its noinit vtables afterwards.
            hr = stream->Read(this, static_cast<ULONG>(Size()), nullptr);
            RefCount = live_refcount;
        }
#else
        const int size = Size();
        if (size < 0x24 || static_cast<std::uint32_t>(size) > game::type_stream_max_elements())
            return game::type_stream_error(game::stream_invalid_record);
        std::uint32_t identity = 0;
        HRESULT hr = game::read_type_u32(stream, identity);
        if (hr < 0) return hr;
        std::vector<unsigned char> record(static_cast<std::size_t>(size));
        hr = game::read_type_bytes(stream, record.data(), size);
        if (hr < 0) return hr;
        hr = game::load_abstract_record(identity, this, record.data(), size);
        RefCount = live_refcount;
#endif
        return hr;
    } catch (...) {
        RefCount = live_refcount;
        return static_cast<HRESULT>(0x80004005u);
    }
}
