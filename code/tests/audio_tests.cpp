#include "support/test_support.hpp"
#include "yrpp/Audio.h"
#include "yrpp/RawFileClass.h"
#include "yrpp/CCFileClass.h"
#include "yrpp/MixFileClass.h"
#include "yrpp/CRC.h"
#include "api/filesystem.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using Bytes = std::vector<unsigned char>;

void word(Bytes& bytes, std::uint32_t value, int length = 4) {
    for (int i = 0; i < length; ++i) bytes.push_back(static_cast<unsigned char>(value >> (8 * i)));
}
void chunk(Bytes& bytes, const char* id, const Bytes& payload) {
    bytes.insert(bytes.end(), id, id + 4); word(bytes, std::uint32_t(payload.size()));
    bytes.insert(bytes.end(), payload.begin(), payload.end());
}
Bytes format(unsigned tag, unsigned channels, unsigned rate, unsigned bits, unsigned align) {
    Bytes bytes;
    word(bytes, tag, 2); word(bytes, channels, 2); word(bytes, rate);
    word(bytes, 123); // Deliberately differs from the target's computed ByteRate.
    word(bytes, align, 2); word(bytes, bits, 2);
    if (tag == 17) { word(bytes, 2, 2); word(bytes, 1017, 2); }
    return bytes;
}
Bytes wave() { return {'R','I','F','F',0,0,0,0,'W','A','V','E'}; }
AudioSampleData sentinel() {
    AudioSampleData sample;
    sample.Data = 91; sample.Format = 92; sample.SampleRate = 93; sample.NumChannels = 94;
    sample.BytesPerSample = 95; sample.ByteRate = 96; sample.BlockAlign = 97; sample.Flags = 98;
    return sample;
}
struct Temp {
    std::filesystem::path path = std::filesystem::temp_directory_path() /
        ("ra2-audio-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Temp() { std::filesystem::create_directories(path); }
    ~Temp() { std::error_code error; std::filesystem::remove_all(path, error); }
    std::string write(const char* leaf, const Bytes& bytes) {
        const auto name = (path / leaf).string();
        std::ofstream out(name, std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
        EXPECT_TRUE((bool(out))) << "write resource fixture";
        return name;
    }
    template<class F> void with_file(const Bytes& bytes, F operation) {
        const auto name = (path / "sample.wav").string();
        { std::ofstream out(name, std::ios::binary);
          out.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
          EXPECT_TRUE((bool(out))) << "write audio fixture"; }
        RawFileClass file(name.c_str());
        EXPECT_TRUE((file.Open())) << "open audio fixture";
        operation(file);
    }
};
void wav_contracts(Temp& temp) {
    for (unsigned tag : {1u, 17u}) for (unsigned channels : {1u, 2u}) {
        auto bytes = wave();
        chunk(bytes, "JUNK", {9,8,7}); // Target uses the unrounded odd length.
        chunk(bytes, "fmt ", format(tag, channels, 22050, tag == 1 ? 8 : 4, 512 * channels));
        chunk(bytes, "fact", {1,0,0,0});
        const int payload_offset = int(bytes.size()) + 8;
        chunk(bytes, "data", {0x12,0x34,0x56,0x78});
        temp.with_file(bytes, [&](RawFileClass& file) {
            file.Seek(7, FileSeekMode::Set);
            auto sample = sentinel(); int size = -1;
            EXPECT_TRUE((Audio::ReadWAVFile(&file, &sample, &size))) << "PCM/IMA WAV accepted";
            EXPECT_TRUE((size == 4 && sample.Data == 4 && sample.Format == (tag == 17 ? 1u : 0u) &&
                sample.NumChannels == channels && sample.SampleRate == 22050 &&
                sample.BytesPerSample == (tag == 17 ? 2u : 1u) &&
                sample.ByteRate == (tag == 17 ? 11025u : 22050u) * channels &&
                sample.BlockAlign == (tag == 17 ? 512u * channels : 97u) && sample.Flags == 98)) << "sample information and untouched fields";
            EXPECT_TRUE((file.HasHandle() && file.Seek(0) == payload_offset)) << "leaves open file at data";
            unsigned char data[4]{};
            EXPECT_TRUE((file.ReadBytes(data, 4) == 4 && data[0] == 0x12 && data[3] == 0x78)) << "parser has not consumed sample bytes";
        });
    }
    auto bytes = wave();
    chunk(bytes, "fmt ", format(17, 1, 22050, 4, 512));
    chunk(bytes, "fmt ", format(1, 2, 44100, 16, 4)); // Last format wins.
    chunk(bytes, "data", {});
    temp.with_file(bytes, [](RawFileClass& file) {
        auto sample = sentinel(); int size = -1;
        EXPECT_TRUE((Audio::ReadWAVFile(&file, &sample, &size) && size == 0 && sample.Format == 0 &&
            sample.BytesPerSample == 2 && sample.ByteRate == 176400 && sample.BlockAlign == 97)) << "PCM16, repeated fmt and empty data";
    });
}
void failure_contracts(Temp& temp) {
    auto original = sentinel(); auto sample = original; int size = 7;
    EXPECT_TRUE((!Audio::ReadWAVFile(nullptr, &sample, &size) && !size &&
        !std::memcmp(&sample, &original, sizeof(sample)))) << "null file preserves information";
    auto reject = [&](const Bytes& bytes, int expected_size) {
        temp.with_file(bytes, [&](RawFileClass& file) {
            sample = original; size = 7;
            EXPECT_TRUE((!Audio::ReadWAVFile(&file, &sample, &size) && size == expected_size &&
                !std::memcmp(&sample, &original, sizeof(sample)))) << "failed WAV output state";
        });
    };
    reject({}, 0); reject({'R','I','F','F'}, 0);
    auto bytes = wave(); bytes[8] = 'X'; reject(bytes, 0);
    bytes = wave(); chunk(bytes, "data", {1,2,3}); reject(bytes, 3);
    bytes = wave(); chunk(bytes, "fmt ", format(3, 1, 22050, 32, 4));
    chunk(bytes, "data", {1,2,3,4}); reject(bytes, 4);
    bytes = wave(); chunk(bytes, "fmt ", format(1, 1, 22050, 8, 1)); reject(bytes, 0);
    bytes = wave(); bytes.insert(bytes.end(), {'f','m','t',' ',0xff,0xff,0xff,0xff}); reject(bytes, 0);
    bytes = wave(); chunk(bytes, "fmt ", format(1, 1, 22050, 8, 1)); chunk(bytes, "data", {});
    temp.with_file(bytes, [&](RawFileClass& file) {
        size = 7;
        EXPECT_TRUE((!Audio::ReadWAVFile(&file, nullptr, &size) && !size)) << "null sample output";
        EXPECT_TRUE((!Audio::ReadWAVFile(&file, &sample, nullptr))) << "null size output";
    });
}

Bytes index_fixture(unsigned version) {
    Bytes bytes;
    word(bytes, 0x41424147); word(bytes, version); word(bytes, 3);
    const char* names[]{"Zulu", "alpha", "Middle"};
    for (unsigned i = 0; i < 3; ++i) {
        const auto offset = bytes.size(); bytes.resize(offset + 16);
        std::memcpy(bytes.data() + offset, names[i], std::strlen(names[i]));
        word(bytes, 2 * i); word(bytes, 2); word(bytes, 22050);
        word(bytes, 12 + (i == 1 ? 1 : 0));
        if (version != 1) word(bytes, 512);
    }
    return bytes;
}
void verify_library(AudioIDXData& library, unsigned version) {
    EXPECT_TRUE((library.SampleCount == 3 && library.BagFile && library.BagFile->HasHandle())) << "library owns open BAG";
    EXPECT_TRUE((library.FindSampleIndex("ALPHA") == 0 && library.FindSampleIndex("middle") == 1 &&
        library.FindSampleIndex("zulu") == 2 && library.FindSampleIndex("absent") == -1)) << "case-insensitive sorting and binary lookup";
    EXPECT_TRUE((!std::strcmp(library.GetSampleName(-1), "Invalid") && library.GetSampleSize(3) == 0)) << "invalid-index name and size";
    EXPECT_TRUE((library.GetSampleSize(0) == 2 && library.Samples[0].Offset == 2)) << "index and physical sample offsets differ";
    auto info = sentinel();
    EXPECT_TRUE((library.GetSampleInformation(0, &info) == &info && info.Data == 4 && info.Format == 1 &&
        info.SampleRate == 22050 && info.NumChannels == 2 && info.BytesPerSample == 2 &&
        info.BlockAlign == (version == 1 ? 0u : 512u) && info.ByteRate == 96 && info.Flags == 98)) << "IDX format and preserved fields";
    const auto untouched = info;
    EXPECT_TRUE((!library.GetSampleInformation(-1, &info) && !std::memcmp(&info, &untouched, sizeof(info)))) << "invalid query preserves output";
    const auto flags = library.Samples[0].Flags;
    for (unsigned value : {0u,1u,4u,5u,8u,9u,12u,13u}) {
        library.Samples[0].Flags = value;
        EXPECT_TRUE((library.GetSampleInformation(0, &info) && info.NumChannels == (value & 1) + 1 &&
            info.Format == ((value & 8) ? 1u : 0u) &&
            info.BytesPerSample == ((value & 12) ? 2u : 1u))) << "IDX PCM/ADPCM flag combinations";
    }
    library.Samples[0].Flags = flags;
    EXPECT_TRUE((library.OpenSample(0) && library.CurrentSampleFile == library.BagFile && library.CurrentSampleSize == 2)) << "opens indexed BAG sample";
    unsigned char payload[2]{};
    EXPECT_TRUE((library.CurrentSampleFile->ReadBytes(payload, 2) == 2 && payload[0] == 30 && payload[1] == 40)) << "BAG sample byte selection";
    library.ClearCurrentSample();
    EXPECT_TRUE((!library.CurrentSampleFile && !library.ExternalFile && library.CurrentSampleSize == 2 &&
        library.BagFile->HasHandle())) << "clear sample keeps BAG open and size unchanged";
    EXPECT_TRUE((library.OpenSample(2) && library.CurrentSampleFile->ReadBytes(payload, 2) == 2 && payload[0] == 10)) << "switching samples uses sorted entry offset";
}
void index_contracts(Temp& temp) {
    temp.write("audio.bag", {10,20,30,40,50,60});
    for (unsigned version : {1u,2u,7u}) {
        auto bytes = index_fixture(version);
        if (version == 7) bytes[0] = 0; // Target has no magic/version-2 validation.
        const auto name = temp.write("audio.idx", bytes);
        std::unique_ptr<AudioIDXData> library(AudioIDXData::Create(name.c_str(), nullptr));
        EXPECT_TRUE((bool(library))) << "v1/v2/non-v1 IDX load";
        EXPECT_TRUE((!library->PathFound && !library->Path[0] && !library->CurrentSampleFile)) << "initial library state";
        verify_library(*library, version);
    }
    const auto name = (temp.path / "audio.idx").string();
    auto reject = [&](const Bytes& bytes) {
        temp.write("audio.idx", bytes);
        std::unique_ptr<AudioIDXData> library(AudioIDXData::Create(name.c_str(), nullptr));
        EXPECT_TRUE((!library)) << "malformed IDX must fail";
    };
    reject({});
    auto bytes = index_fixture(2); bytes.pop_back(); reject(bytes);
    bytes = index_fixture(2); std::fill(bytes.begin() + 12, bytes.begin() + 28, 'x'); reject(bytes);
    bytes = index_fixture(2); std::fill(bytes.begin() + 8, bytes.begin() + 12, 0xff); reject(bytes);
    bytes = index_fixture(2); bytes.resize(12); std::fill(bytes.begin() + 8, bytes.end(), 0);
    temp.write("audio.idx", bytes);
    std::unique_ptr<AudioIDXData> empty(AudioIDXData::Create(name.c_str(), nullptr));
    EXPECT_TRUE((empty && empty->SampleCount == 0 && !empty->Samples && empty->FindSampleIndex("x") == -1)) << "empty library is valid";
    empty.reset();
    std::filesystem::remove(temp.path / "audio.bag");
    EXPECT_TRUE((!AudioIDXData::Create(name.c_str(), nullptr) && !AudioIDXData::Create(nullptr, nullptr))) << "missing BAG and null library name";
}
void external_contracts(Temp& temp) {
    const auto name = temp.write("audio.idx", index_fixture(2));
    temp.write("audio.bag", {10,20,30,40,50,60});
    auto bytes = wave(); chunk(bytes, "fmt ", format(1, 1, 44100, 8, 1)); chunk(bytes, "data", {7,8,9});
    temp.write("alpha.wav", bytes);
    const auto path = temp.path.string();
    std::unique_ptr<AudioIDXData> library(AudioIDXData::Create(name.c_str(), path.c_str()));
    EXPECT_TRUE((library && library->PathFound && std::string(library->Path) == path + "\\")) << "external directory detection";
    EXPECT_TRUE((library->OpenSample(0) && library->ExternalFile && library->CurrentSampleFile == library->ExternalFile &&
        library->CurrentSampleSize == 3 && library->Samples[0].Flags == 2 && library->Samples[0].SampleRate == 44100)) << "external WAV overrides BAG and updates sample metadata";
    unsigned char data[3]{};
    EXPECT_TRUE((library->CurrentSampleFile->ReadBytes(data, 3) == 3 && data[0] == 7 && data[2] == 9)) << "external filename ownership and WAV data offset";
    EXPECT_TRUE((library->OpenSample(2) && !library->ExternalFile && library->CurrentSampleFile == library->BagFile)) << "missing external WAV falls back to BAG and deletes previous file";
    temp.write("alpha.wav", {'b','a','d'});
    EXPECT_TRUE((library->OpenSample(0) && !library->ExternalFile && library->CurrentSampleSize == 2)) << "invalid external WAV falls back to BAG";
}
void mix_contracts(Temp& temp) {
    const auto idx = index_fixture(2);
    const Bytes bag{10,20,30,40,50,60};
    struct Entry { std::uint32_t id; const Bytes* bytes; };
    auto id = [](const char* name) { CRCEngine crc; for (; *name; ++name) crc(*name); return std::uint32_t(crc()); };
    std::array<Entry,2> entries{{{id("PACKED.IDX"), &idx}, {id("PACKED.BAG"), &bag}}};
    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
        return std::bit_cast<std::int32_t>(a.id) < std::bit_cast<std::int32_t>(b.id);
    });
    Bytes archive; word(archive, 2, 2); word(archive, std::uint32_t(idx.size() + bag.size()));
    std::uint32_t offset = 0;
    for (const auto& entry : entries) {
        word(archive, entry.id); word(archive, offset); word(archive, std::uint32_t(entry.bytes->size()));
        offset += std::uint32_t(entry.bytes->size());
    }
    for (const auto& entry : entries) archive.insert(archive.end(), entry.bytes->begin(), entry.bytes->end());
    temp.write("packed.mix", archive);
    game::ResourceHandle* output = nullptr; std::string error;
    EXPECT_TRUE((game::create_resources(temp.path.string(), output, error))) << error.c_str();
    std::unique_ptr<game::ResourceHandle, decltype(&game::destroy_resources)> resources(output, game::destroy_resources);
    const auto use = [](void*) {
        MixFileClass archive("packed.mix");
        std::unique_ptr<AudioIDXData> library(AudioIDXData::Create("packed", nullptr));
        EXPECT_TRUE((bool(library))) << "load IDX/BAG inside MIX through existing CCFile";
        verify_library(*library, 2);
    };
    EXPECT_TRUE((game::with_resources(*resources, use, nullptr, error))) << error.c_str();
}
}

TEST(Audio, Contracts) {
    static_assert(sizeof(AudioIDXHeader) == 12 && sizeof(AudioIDXEntry) == 36);
    static_assert(sizeof(AudioSampleData) == 32 && offsetof(AudioSampleData, BlockAlign) == 24);

        Temp temp; wav_contracts(temp); failure_contracts(temp);
        index_contracts(temp); external_contracts(temp); mix_contracts(temp);
}
