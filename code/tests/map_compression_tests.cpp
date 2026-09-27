#include "support/test_support.hpp"
#include "yrpp/Pipes.h"
#include "yrpp/Straws.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(LCWPipe) == 40 && sizeof(LCWStraw) == 40 && sizeof(LZOPipe) == 40 && sizeof(LZOStraw) == 40);
static_assert(offsetof(LCWPipe, Control) == 12 && offsetof(LCWPipe, BlockHeader_CompCount) == 36);
static_assert(offsetof(LZOStraw, Buffer) == 20 && offsetof(LZOStraw, BlockHeader_UncompCount) == 38);
#endif

namespace {
using Bytes = std::vector<unsigned char>;

struct Sink : Pipe {
    Bytes bytes;
    int flushes = 0;
    int Put(const void* input, int length) override {
        if (input && length > 0) {
            const auto* p = static_cast<const unsigned char*>(input); bytes.insert(bytes.end(), p, p + length);
        }
        return length;
    }
    int Flush() override { ++flushes; return 0; }
};
Bytes hex(const std::string& text) {
    Bytes result;
    for (std::size_t i = 0; i < text.size(); i += 2) result.push_back(static_cast<unsigned char>(std::stoul(text.substr(i, 2), nullptr, 16)));
    return result;
}
Bytes framed(const Bytes& bytes, int count) {
    Bytes result{static_cast<unsigned char>(bytes.size()), static_cast<unsigned char>(bytes.size() >> 8),
        static_cast<unsigned char>(count), static_cast<unsigned char>(count >> 8)};
    result.insert(result.end(), bytes.begin(), bytes.end()); return result;
}
template<class P, class S> void original_sample(Bytes plain, const Bytes& encoded) {
    const Bytes expected = framed(encoded, int(plain.size()));
    Sink sink;
    {
        P compressor(0, 8192); compressor.Put_To(sink);
        int total = 0;
        for (std::size_t i = 0; i < plain.size(); i += 7)
            total += compressor.Put(plain.data() + i, int(std::min<std::size_t>(7, plain.size() - i)));
        total += compressor.Flush();
        EXPECT_TRUE((total == int(expected.size()) && sink.bytes == expected)) << "compressed bytes differ from original instructions";
        EXPECT_TRUE((compressor.Counter == 0)) << "compressor staging state";
    }
    EXPECT_TRUE((!sink.ChainFrom)) << "pipe destructor unlinks chain";
    BufferStraw input(plain.data(), int(plain.size()));
    {
        S compressor(0, 8192); compressor.Get_From(input);
        Bytes actual(expected.size() + 16);
        EXPECT_TRUE((compressor.Get(actual.data(), int(actual.size())) == int(expected.size()))) << "straw compressed size";
        actual.resize(expected.size()); EXPECT_TRUE((actual == expected)) << "straw bytes differ from original instructions";
    }
    EXPECT_TRUE((!input.ChainFrom)) << "straw destructor unlinks chain";
    Sink decoded;
    P decompressor(1, 8192); decompressor.Put_To(decoded);
    int total = 0;
    for (std::size_t i = 0; i < expected.size(); ++i) total += decompressor.Put(expected.data() + i, 1);
    EXPECT_TRUE((total == int(plain.size()) && decoded.bytes == plain)) << "fragmented original compressed input";
    EXPECT_TRUE((decompressor.Counter == 0 && decompressor.BlockHeader_CompCount == -1)) << "decompressor block state";
    Bytes stream_bytes = expected;
    BufferStraw source(stream_bytes.data(), int(stream_bytes.size()));
    S straw(1, 8192); straw.Get_From(source);
    Bytes result(plain.size());
    for (std::size_t i = 0; i < result.size(); i += 11) {
        const int count = int(std::min<std::size_t>(11, result.size() - i));
        EXPECT_TRUE((straw.Get(result.data() + i, count) == count)) << "partial decompressed output";
    }
    unsigned char byte;
    EXPECT_TRUE((result == plain && straw.Get(&byte, 1) == 0)) << "decoded original data and clean EOF";
}
template<class P, class S> void state_and_errors() {
    Sink sink;
    { P pipe(0, 8192); pipe.Put_To(sink); EXPECT_TRUE((pipe.Put("a", 1) == 0)) << "one byte is staged"; }
    EXPECT_TRUE((sink.bytes.empty())) << "destructor does not implicitly flush";
    P decoder(1, 64); decoder.Put_To(sink);
    const unsigned char partial_header[]{5, 0};
    EXPECT_TRUE((decoder.Put(partial_header, 2) == 0 && decoder.Flush() == 2 && sink.bytes == Bytes({5, 0}))) << "partial header Flush passes through";
    sink.bytes.clear();
    const unsigned char partial_payload[]{5, 0, 9, 0, 0x81, 'a'};
    EXPECT_TRUE((decoder.Put(partial_payload, 6) == 0 && decoder.Flush() == 6 &&
        sink.bytes == Bytes(std::begin(partial_payload), std::end(partial_payload)))) << "partial payload Flush passes header and data";
    const unsigned char invalid[]{0, 0, 1, 0};
    EXPECT_TRUE((decoder.Put(invalid, 4) == -1 && decoder.Flush() == -1)) << "invalid length latches error";
    unsigned char truncated[]{5, 0, 9};
    BufferStraw input(truncated, 3); S reader(1, 64); reader.Get_From(input);
    unsigned char out[16]{};
    EXPECT_TRUE((reader.Get(out, 16) == -1 && reader.Get(out, 1) == -1)) << "truncated Straw header latches error";
    // Back reference before any output, as well as corrupted block lengths,
    // must fail without writing past the destination on either codec.
    for (Bytes malformed : {Bytes{2, 0, 10, 0, 0, 0}, Bytes{1, 0, 65, 0, 0x80}, Bytes{1, 0, 1, 0, 0xff}}) {
        Sink unused; P bad(1, 64); bad.Put_To(unused);
        EXPECT_TRUE((bad.Put(malformed.data(), int(malformed.size())) == -1 && unused.bytes.empty())) << "malformed compressed block";
    }
    // Multi-block framing and the corrected one-byte LCW end case.
    for (int block : {1, 2, 63, 8192}) {
        Bytes plain(std::size_t(block) * 2 + 1);
        for (std::size_t i = 0; i < plain.size(); ++i) plain[i] = static_cast<unsigned char>((i * 17 + i / 13) & 255);
        Sink packed; P writer(0, block); writer.Put_To(packed);
        EXPECT_TRUE((writer.Put(plain.data(), int(plain.size())) >= 0 && writer.Flush() >= 0)) << "multiple blocks and final one-byte block";
        BufferStraw source(packed.bytes.data(), int(packed.bytes.size())); S reader2(1, block); reader2.Get_From(source);
        Bytes decoded(plain.size()); EXPECT_TRUE((reader2.Get(decoded.data(), int(decoded.size())) == int(decoded.size()) && decoded == plain)) << "multi-block round trip";
    }
}
}

TEST(MapCompression, Contracts) {
    const auto [argc, argv] = ra2::test::arguments();


            const std::string path = argc > 1 ? argv[1] : RA2_COMPRESSION_FIXTURE;
            std::ifstream input(path); EXPECT_TRUE((bool(input))) << "open original compression reference";
            std::string codec, raw, encoded; int count = 0;
            while (input >> codec >> raw >> encoded) {
                if (codec == "lcw") original_sample<LCWPipe, LCWStraw>(hex(raw), hex(encoded));
                else original_sample<LZOPipe, LZOStraw>(hex(raw), hex(encoded));
                ++count;
            }
            EXPECT_TRUE((input.eof() && count == 52)) << "complete original compression reference";
            state_and_errors<LCWPipe, LCWStraw>(); state_and_errors<LZOPipe, LZOStraw>();
}
