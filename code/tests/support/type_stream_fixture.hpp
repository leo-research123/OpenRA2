#pragma once

#include "test_support.hpp"
#include "api/type_stream.hpp"
#include <algorithm>
#include <cstring>
#include <limits>
#include <vector>

namespace ra2::test {

// Preserve each transport's allocation failure and transferred-count contract.
template<HRESULT AllocationFailure = static_cast<HRESULT>(0x8007000Eu),
         bool RecordWrites = false, bool RetainFailedTransfer = false>
struct MemoryStream {
    std::vector<unsigned char> bytes;
    std::vector<unsigned> writes;
    std::size_t cursor = 0, limit = std::numeric_limits<std::size_t>::max();
    unsigned calls = 0, fail_call = 0, short_from = 0;
    IStream* handle() noexcept { return reinterpret_cast<IStream*>(this); }
    std::size_t transfer_limit() const {
        return short_from && calls >= short_from ? std::min(limit, std::size_t(2)) : limit;
    }
    static HRESULT read(void*, IStream* input, void* output, std::uint32_t size,
                        std::uint32_t& done) noexcept {
        auto& s = *reinterpret_cast<MemoryStream*>(input); done = 0;
        if (++s.calls == s.fail_call) return static_cast<HRESULT>(0x80004005u);
        done = static_cast<std::uint32_t>(std::min({std::size_t(size),
            s.bytes.size() - s.cursor, s.transfer_limit()}));
        if (done) std::memcpy(output, s.bytes.data() + s.cursor, done);
        s.cursor += done;
        return done == size ? 0 : 1;
    }
    static HRESULT write(void*, IStream* output, const void* input, std::uint32_t size,
                         std::uint32_t& done) noexcept {
        auto& s = *reinterpret_cast<MemoryStream*>(output); done = 0;
        if (++s.calls == s.fail_call) return static_cast<HRESULT>(0x80004005u);
        try {
            if constexpr (RecordWrites) s.writes.push_back(size);
            const auto count = static_cast<std::uint32_t>(std::min(std::size_t(size), s.transfer_limit()));
            if constexpr (RetainFailedTransfer) done = count;
            const auto* p = static_cast<const unsigned char*>(input);
            s.bytes.insert(s.bytes.end(), p, p + count);
            done = count;
            return done == size ? 0 : 1;
        } catch (...) { return AllocationFailure; }
    }
};

template<class Stream>
struct StreamSession {
    game::TypeStreamSession* value = nullptr;
    static game::TypeStreamTransport transport() {
        game::TypeStreamTransport t; t.read = Stream::read; t.write = Stream::write;
        return t;
    }
    explicit StreamSession(const game::TypeStreamTransport& t = transport()) {
        EXPECT_EQ(game::create_type_stream_session(t, value), game::TypeStreamStatus::complete);
    }
    ~StreamSession() { game::destroy_type_stream_session(value); }
    StreamSession(const StreamSession&) = delete;
    StreamSession& operator=(const StreamSession&) = delete;
};

} // namespace ra2::test
