#include "support/test_support.hpp"
#include "filesystem/resource_environment.hpp"
#include "filesystem/mix_crypto.hpp"
#include "filesystem/resource_globals.hpp"
#include "filesystem/resource_startup.hpp"
#include "filesystem/resource_context.hpp"
#include "filesystem/file_system.hpp"
#include "yrpp/CCFileClass.h"
#include "yrpp/Memory.h"
#include <memory>
#include "filesystem/file_names.hpp"
#include "yrpp/Memory.h"
#include "third_party/xcc/blowfish.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace game;
namespace fs = std::filesystem;
using Bytes = std::vector<uint8_t>;
namespace {

template<class F> void rejects(F action, const std::string& message) {
    bool rejected = false;
    try { action(); } catch (const std::exception&) { rejected = true; }
    EXPECT_TRUE((rejected)) << message;
}
Bytes bytes(std::string_view value) { return {value.begin(), value.end()}; }
void append(Bytes& out, uint32_t value, unsigned size = 4) {
    for (unsigned i = 0; i < size; ++i) out.push_back(uint8_t(value >> (8 * i)));
}
Bytes unhex(std::string_view value) {
    Bytes result;
    for (size_t i = 0; i + 1 < value.size(); i += 2)
        result.push_back(uint8_t(std::stoul(std::string(value.substr(i, 2)), nullptr, 16)));
    return result;
}
Bytes mix(std::vector<std::pair<std::string, Bytes>> files, bool old = false) {
    std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) {
        return std::bit_cast<int32_t>(filename_id(a.first)) < std::bit_cast<int32_t>(filename_id(b.first));
    });
    Bytes body, index;
    for (const auto& [name, data] : files) {
        append(index, filename_id(name)); append(index, uint32_t(body.size()));
        append(index, uint32_t(data.size()));
        body.insert(body.end(), data.begin(), data.end());
    }
    Bytes result;
    if (!old) append(result, 0);
    append(result, uint32_t(files.size()), 2); append(result, uint32_t(body.size()));
    result.insert(result.end(), index.begin(), index.end());
    result.insert(result.end(), body.begin(), body.end());
    return result;
}
void write(const fs::path& path, const Bytes& data) {
    fs::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(data.data()), std::streamsize(data.size()));
    EXPECT_TRUE((bool(file))) << "fixture write failed";
}
struct Temp {
    fs::path path = fs::temp_directory_path() / ("ra2-resources-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Temp() { fs::create_directories(path); }
    ~Temp() { std::error_code ignored; fs::remove_all(path, ignored); }
};
bool exists(ResourceEnvironment& files, const char* name) {
    ResourceScope scope({&files, nullptr, {}});
    CCFileClass file(name);
    return file.Exists();
}
std::string read(ResourceEnvironment& files, std::string_view name) {
    ResourceScope scope({&files, nullptr, {}});
    const std::string filename(name);
    CCFileClass file(filename.c_str());
    EXPECT_TRUE((file.Open(FileAccessMode::Read))) << "cannot find " + filename;
    const int size = file.GetFileSize();
    EXPECT_TRUE((size >= 0)) << "invalid resource size";
    std::string data(size_t(size), '\0');
    EXPECT_TRUE((!size || file.ReadBytes(data.data(), size) == size)) << "short resource read";
    return data;
}
void hashes_and_crypto() {
    for (const auto& [name, id] : std::vector<std::pair<std::string, uint32_t>>{
        {"",0}, {"a",0xeb978531}, {"ab",0x421faa6e}, {"abc",0x33aff496},
        {"abcd",0xdb1720a5}, {"abcde",0x518e3366}, {"gi.shp",0xadd10028},
        {"DIR/A.TXT",0x17a9c7ab}, {"DIR\\A.TXT",0xde8738bb}})
        EXPECT_TRUE((filename_id(name) == id)) << "CRC golden vector: " + name;
    std::array<uint8_t, 80> source{};
    for (unsigned i = 0; i < source.size(); ++i) source[i] = uint8_t(i);
    const auto expected = unhex("fd0b077d5780a7d08d96681dcbe1bea9450e77fcba6632451c94208a0711274b905a205f5aff6bc04d920b8f3d2f29868be2030a36d40c8e");
    const auto recovered = mix_key(source);
    EXPECT_TRUE((std::equal(expected.begin(), expected.end(), recovered.begin()))) << "RSA key recovery golden vector";
    ra2::xcc::Cblowfish cipher;
    const std::array<uint8_t, 56> zero_key{};
    cipher.set_key(zero_key);
    std::array<uint8_t, 8> block{};
    cipher.encipher(block.data(), block.data(), 8);
    const auto golden = unhex("4ef997456198dd78");
    EXPECT_TRUE((std::equal(golden.begin(), golden.end(), block.begin()))) << "Blowfish golden vector";
    cipher.decipher(block.data(), block.data(), 8);
    EXPECT_TRUE((block == std::array<uint8_t, 8>{})) << "Blowfish inverse";
    rejects([&] { cipher.decipher(block.data(), block.data(), 7); }, "partial crypto block accepted");
}
void mix_without_resource_context() {
    Temp fixture;
    FileSystem platform(fixture.path);
    FileSystemScope files(&platform);
    EXPECT_TRUE((!try_current_context())) << "test must begin without a resource context";
    const auto child_bytes = mix({{"leaf.txt", bytes("nested without context")}});
    write(fixture.path / "parent.mix", mix({{"padding", bytes("123456789")}, {"child.mix", child_bytes}}));
    {
        MixFileClass parent("parent.mix");
        const auto* entry = parent.find(filename_id("child.mix"));
        EXPECT_TRUE((entry != nullptr)) << "missing child directory entry";
        MixFileClass child("child.mix");
        EXPECT_TRUE((child.IsValid() && child.FileStartOffset == parent.FileStartOffset + int(entry->Offset) + 22)) << "nested MIX must use CCFile bias without a resource context";
        CCFileClass leaf("leaf.txt");
        char output[64]{};
        EXPECT_TRUE((leaf.Open(FileAccessMode::Read) && leaf.ReadBytes(output, 64) == 22
            && std::string_view(output, 22) == "nested without context")) << "context-free nested file read";
    }
    {
        MixFileClass parent("parent.mix");
        const auto content = mix({{"padding", bytes("123456789")}, {"child.mix", child_bytes}});
        Bytes cache(content.begin() + parent.FileStartOffset, content.end());
        parent.Data = cache.data();
        MixFileClass child("child.mix");
        EXPECT_TRUE((child.FileStartOffset == 22)) << "cached MIX bias must come from the opened file view";
        CCFileClass leaf("leaf.txt");
        char output[64]{};
        EXPECT_TRUE((leaf.Open(FileAccessMode::Read) && leaf.ReadBytes(output, 64) == 22)) << "context-free cached nested file read";
        parent.Data = nullptr;
    }
    auto unkeyed = mix({{"plain.txt", bytes("pass through")}});
    unkeyed[2] = 2;
    write(fixture.path / "unkeyed.mix", unkeyed);
    {
        MixFileClass archive("unkeyed.mix", nullptr);
        EXPECT_TRUE((archive.IsEncrypted && archive.IsValid() && archive.FileStartOffset == 22)) << "null PKey must leave the stream unchanged, without encrypted padding";
    }
    const uint8_t exponent[]{2, 1, 1};
    std::array<uint8_t, 44> modulus{};
    modulus[0] = 2; modulus[1] = 42; modulus[2] = 1; modulus.back() = 0x15;
    PKey alternate(exponent, modulus.data());
    EXPECT_TRUE((alternate.BitPrecision == 328 && alternate.Plain_Block_Size() == 40
        && alternate.Crypt_Block_Size() == 41)) << "external PKey precision/layout";
    std::array<uint8_t, 80> plaintext{};
    for (unsigned i = 0; i < plaintext.size(); ++i) plaintext[i] = uint8_t(i * 3 + 7);
    std::array<uint8_t, 82> encoded_key{};
    EXPECT_TRUE((alternate.Encrypt(plaintext.data(), 80, encoded_key.data()) == 82)) << "PKey encrypt block count";
    std::array<uint8_t, 80> decoded{};
    EXPECT_TRUE((alternate.Decrypt(encoded_key.data(), 82, decoded.data()) == 80 && decoded == plaintext)) << "alternate PKey must use supplied modulus/exponent";
    decoded.fill(0xcc);
    EXPECT_TRUE((alternate.Decrypt(encoded_key.data(), 81, decoded.data()) == 40
        && decoded[40] == 0xcc)) << "partial key block must remain unprocessed";
    auto ordinary = mix({{"alt.txt", bytes("external key")}});
    Bytes index(ordinary.begin() + 4, ordinary.begin() + 22);
    index.resize(24);
    ra2::xcc::Cblowfish cipher;
    cipher.set_key(std::span<const uint8_t, 56>(plaintext.data(), 56));
    cipher.encipher(index.data(), index.data(), int(index.size()));
    Bytes encrypted;
    append(encrypted, 0x20000);
    encrypted.insert(encrypted.end(), encoded_key.begin(), encoded_key.end());
    encrypted.insert(encrypted.end(), index.begin(), index.end());
    encrypted.insert(encrypted.end(), ordinary.begin() + 22, ordinary.end());
    write(fixture.path / "alternate.mix", encrypted);
    {
        MixFileClass archive("alternate.mix", &alternate);
        EXPECT_TRUE((archive.IsValid() && archive.FileStartOffset == 110 && archive.CountFiles == 1)) << "MIX key length must follow external PKey, not fixed 80 bytes";
        CCFileClass leaf("alt.txt");
        char output[12]{};
        EXPECT_TRUE((leaf.Open(FileAccessMode::Read) && leaf.ReadBytes(output, 12) == 12
            && std::string_view(output, 12) == "external key")) << "external-key MIX payload";
    }
    // An optional diagnostic scope must not replace an explicitly selected root.
    FileLocation source;
    {
        ResourceScope diagnostics(source);
        MixFileClass archive("parent.mix");
        EXPECT_TRUE((archive.IsValid() && !source.physical_file.empty())) << "optional diagnostics changed file selection";
    }
    EXPECT_TRUE((!try_current_context() && MixFileClass::MIXes.empty())) << "construction leaked a context or registered node";
}
void files_and_mounts() {
    Temp fixture;
    const auto unicode_dir = fixture.path / path_from_utf8("中文资源");
    write(unicode_dir / "loose.txt", bytes("utf8 directory"));
    {
    ResourceEnvironment unicode_files(path_from_utf8(path_to_utf8(unicode_dir)));
    EXPECT_TRUE((read(unicode_files,"LOOSE.TXT") == "utf8 directory")) << "UTF-8 directory path";
    }
    std::ifstream encrypted_file(fs::path(RA2_TEST_FIXTURE_DIR) / "encrypted.mix.hex");
    std::string encoded; encrypted_file >> encoded;
    EXPECT_TRUE((!encoded.empty())) << "missing encrypted fixture";
    write(fixture.path / "encrypted.mix", unhex(encoded));
    write(fixture.path / "first.mix", mix({{"both.txt", bytes("first")},
        {"child.mix", mix({{"leaf.txt", bytes("nested")}})}}, true));
    write(fixture.path / "second.mix", mix({{"both.txt", bytes("second")}}));
    ResourceEnvironment files(fixture.path);
    auto& encrypted = files.mount("ENCRYPTED.MIX");
    EXPECT_TRUE((encrypted.IsEncrypted && encrypted.IsDigest)) << "encrypted flags";
    EXPECT_TRUE((read(files,"a.txt") == "alpha" && read(files,"B.TXT") == "beta")) << "signed ID binary search / encrypted index";
    auto& first = files.mount("FIRST.MIX");
    files.mount("second.mix");
    EXPECT_TRUE((read(files,"Both.txt") == "first")) << "first registered archive must win";
    EXPECT_TRUE((!exists(files, "leaf.txt"))) << "nested archives must not auto-mount";
    const auto& child = files.mount("CHILD.MIX");
    EXPECT_TRUE((files.source_of(child).source_chain.size() == 2)) << "nested source trace";
    EXPECT_TRUE((read(files,"leaf.txt") == "nested")) << "nested bias must add physical offsets";
    CCFileClass leaf("leaf.txt");
    FileLocation leaf_source;
    {
        ResourceScope scope({&files, &leaf_source, {}});
        EXPECT_TRUE((leaf.Open(FileAccessMode::Read))) << "open leaf";
    }
    EXPECT_TRUE((leaf_source.source_chain.size() == 3)) << "leaf source chain";
    std::array<uint8_t, 20> buffer{};
    EXPECT_TRUE((leaf.ReadBytes(buffer.data(), int(buffer.size())) == 6 &&
        leaf.ReadBytes(buffer.data(), int(buffer.size())) == 0)) << "read must stop at member end";
    EXPECT_TRUE((leaf.Seek(7, FileSeekMode::Set) == 6)) << "original seek must clamp to member end";
    EXPECT_TRUE((leaf.Seek(5, FileSeekMode::Set) == 5 && leaf.ReadBytes(buffer.data(), 2) == 1 &&
        buffer[0] == 'd')) << "original read must clip to member end";
    write(fixture.path / "bOtH.TxT", bytes("loose"));
    EXPECT_TRUE((read(files,"BOTH.TXT") == "loose")) << "case-insensitive loose file must override MIX";
    MixFileClass* match = nullptr;
    EXPECT_TRUE((MixFileClass::Offset("BOTH.TXT", nullptr, &match, nullptr, nullptr) && match == &first)) << "MIX Offset is separate from CCFile Open";
    const auto previous_match = match;
    EXPECT_TRUE((!MixFileClass::Offset("absent.txt", nullptr, &match, nullptr, nullptr) && match == previous_match)) << "failed lookup modified caller outputs";
    fs::remove(fixture.path / "bOtH.TxT");
    files.unmount(first);
    EXPECT_TRUE((read(files,"both.txt") == "second")) << "unmount must unlink from priority chain";
    // An already-open stream owns its file and stays readable after unmount.
    EXPECT_TRUE((leaf.Seek(0, FileSeekMode::Set) == 0)) << "rewind after unmount";
    EXPECT_TRUE((leaf.ReadBytes(buffer.data(), 6) == 6 && !std::memcmp(buffer.data(), "nested", 6))) << "open file lost carrier ownership";
    auto& missing = files.mount("missing.mix");
    const MixHeaderData previous_entry{};
    const MixHeaderData* missing_entries = &previous_entry;
    int missing_count = 123;
    EXPECT_TRUE((!missing.IsValid() && !missing.headers(missing_entries, missing_count) &&
        !missing_entries && missing_count == 0)) << "missing object clears directory outputs and remains unregistered";
    auto count = files.mixes().size();
    write(fixture.path / "bad.mix", bytes("CLASS"));
    rejects([&] { files.mount("bad.mix"); }, "truncated archive accepted");
    EXPECT_TRUE((files.mixes().size() == count)) << "failed mount modified active chain";
    auto invalid = mix({{"bad.txt", bytes("x")}});
    invalid[18] = 255; // first entry size = 255, declared data size = 1
    write(fixture.path / "bounds.mix", invalid);
    rejects([&] { files.mount("bounds.mix"); }, "out-of-range index accepted");
    invalid = mix({}); invalid[4] = 0xff; invalid[5] = 0xff;
    write(fixture.path / "negative.mix", invalid);
    rejects([&] { files.mount("negative.mix"); }, "signed negative count accepted");
    auto truncated = unhex(encoded); truncated.resize(90);
    write(fixture.path / "short-encrypted.mix", truncated);
    rejects([&] { files.mount("short-encrypted.mix"); }, "truncated encrypted index accepted");
    files.clear();
    EXPECT_TRUE((files.mixes().empty())) << "clear must unlink all archives";
}
void file_output_lifetime() {
    Temp fixture;
    write(fixture.path / "loose.txt", bytes("loose"));
    write(fixture.path / "carrier.mix", mix({{"member.txt", bytes("member")}}));
    ResourceEnvironment files(fixture.path);
    files.mount("carrier.mix");
    ResourceScope scope({&files, nullptr, {}});
    CCFileClass file;
    std::array<char, 5> buffer{};
    EXPECT_TRUE((!file.HasHandle())) << "default file is open";
    EXPECT_TRUE((file.OpenEx("member.txt", FileAccessMode::Read))) << "open member";
    EXPECT_TRUE((file.ReadBytes(buffer.data(), 3) == 3 && !std::memcmp(buffer.data(), "mem", 3))) << "read member prefix";
    EXPECT_TRUE((file.OpenEx("loose.txt", FileAccessMode::Read) && file.Seek(0, FileSeekMode::Current) == 0 &&
        file.ReadBytes(buffer.data(), 5) == 5 && !std::memcmp(buffer.data(), "loose", 5))) << "reopen did not replace file and reset cursor";
    EXPECT_TRUE((!file.OpenEx("absent.txt", FileAccessMode::Read) && !file.HasHandle())) << "missing file retained handle";
    EXPECT_TRUE((file.OpenEx("loose.txt", FileAccessMode::Read))) << "reopen after missing file";
    fs::remove(fixture.path / "carrier.mix");
    rejects([&] { file.OpenEx("member.txt", FileAccessMode::Read); }, "missing carrier accepted");
    file.Close();
    EXPECT_TRUE((file.OpenEx("loose.txt", FileAccessMode::Read))) << "reopen after carrier error";
    file.Close();
    file.Close();
    EXPECT_TRUE((!file.HasHandle())) << "close retained handle";
}
void mix_object_contracts() {
    Temp fixture;
    const auto pack = mix({{"owned.txt", bytes("payload")},
        {"child.mix", mix({{"cached-leaf.txt", bytes("nested cache")}})}});
    write(fixture.path / "owned.mix", pack);
    ResourceEnvironment files(fixture.path);
    char name[] = "owned.mix";
    FileLocation source;
    game::ResourceScope scope({&files, &source, {}});
    auto* object = new MixFileClass(name);
    GenericNode* base = object;
    name[0] = 'X';
    EXPECT_TRUE((std::strcmp(object->FileName, "owned.mix") == 0)) << "MIX borrowed caller filename";
    EXPECT_TRUE((object->IsValid() && files.mixes().front() == object && files.mixes().size() == 1)) << "constructor did not register itself at tail";
    EXPECT_TRUE((object->GenericNode::Next()->GenericNode::Prev() == object && object->GenericNode::Prev()->GenericNode::Next() == object)) << "broken sentinel links";
    EXPECT_TRUE((!object->Data && !object->IsAllocated && object->CountFiles == 2)) << "constructor eagerly cached body or lost directory count";

    const MixHeaderData* entries = nullptr;
    int entry_count = 0;
    EXPECT_TRUE((object->headers(entries, entry_count) && entries == object->Headers && entry_count == 2)) << "MIX directory query must borrow the original entries and count";
    EXPECT_TRUE((object->headers(entries, object->CountFiles) && entries == object->Headers && object->CountFiles == 2)) << "directory count output may alias the original count";
    const int original_count = object->CountFiles;
    for (const int empty_count : {0, -1}) {
        object->CountFiles = empty_count;
        entries = object->Headers;
        entry_count = 123;
        const bool present = object->headers(entries, entry_count);
        object->CountFiles = original_count;
        EXPECT_TRUE((!present && !entries && entry_count == 0)) << "nonpositive directory count clears both outputs";
    }
    auto* original_headers = object->Headers;
    object->Headers = nullptr;
    entries = original_headers;
    entry_count = 123;
    const bool present = object->headers(entries, entry_count);
    object->Headers = original_headers;
    EXPECT_TRUE((!present && !entries && entry_count == 0)) << "missing directory storage clears both outputs";

    void* data = object;
    MixFileClass* owner = nullptr;
    int32_t position = -1, length = -1;
    EXPECT_TRUE((MixFileClass::Offset("OWNED.TXT", &data, &owner, &position, &length) && !data &&
        owner == object && length == 7 && position >= object->FileStartOffset)) << "uncached Offset outputs";
    EXPECT_TRUE((Bytes(pack.begin() + position, pack.begin() + position + length) == bytes("payload"))) << "Offset did not include physical FileStartOffset";
    data = object;
    const auto previous_position = position;
    for (const char* absent : {static_cast<const char*>(nullptr), "absent.txt"}) {
        EXPECT_TRUE((!MixFileClass::Offset(absent, &data, &owner, &position, &length))) << "missing Offset succeeded";
        EXPECT_TRUE((data == object && owner == object && position == previous_position && length == 7)) << "failed Offset changed an output parameter";
    }
    EXPECT_TRUE((MixFileClass::Offset("owned.txt", nullptr, nullptr, nullptr, nullptr))) << "Offset requires unwanted output arguments";

    // Supply an externally owned body. Offset and CCFile must use it without
    // transferring ownership. The production first-stage Cache remains a no-op.
    std::vector<char> borrowed(pack.begin() + object->FileStartOffset, pack.end());
    object->Data = borrowed.data();
    EXPECT_TRUE((MixFileClass::Offset("owned.txt", &data, &owner, &position, &length) &&
        data == borrowed.data() + position && length == 7)) << "cached Offset outputs";
    fs::remove(fixture.path / "owned.mix");
    EXPECT_TRUE((read(files, "owned.txt") == "payload")) << "CCFile did not read borrowed cache";
    write(fixture.path / "owned.txt", bytes("loose"));
    EXPECT_TRUE((read(files, "owned.txt") == "loose")) << "cached MIX overrode loose file";
    fs::remove(fixture.path / "owned.txt");
    auto& child = files.mount("child.mix");
    EXPECT_TRUE((read(files, "cached-leaf.txt") == "nested cache")) << "nested cached carrier bias";
    EXPECT_TRUE((files.mixes().size() == 2 && object->GenericNode::Next() == &child && child.GenericNode::Prev() == object)) << "nested constructor changed registration priority";
    files.unmount(child);
    EXPECT_TRUE((files.mixes().size() == 1 && object->GenericNode::Next()->GenericNode::Prev() == object)) << "destructor did not repair adjacent links";
    delete base; // Exercise the virtual destructor and caller-owned Data branch.
    EXPECT_TRUE((files.mixes().empty() && !borrowed.empty())) << "base deletion failed to unlink MIX";

    write(fixture.path / "allocated.mix", mix({{"a.txt", bytes("a")}}));
    auto& allocated = files.mount("allocated.mix");
    allocated.Data = new char[1]{'a'};
    allocated.IsAllocated = true;
    EXPECT_TRUE((read(files, "a.txt") == "a")) << "owned cache read";
    auto& missing = files.mount("absent.mix");
    EXPECT_TRUE((missing.FileName && !missing.IsValid() && !missing.Headers && missing.CountFiles == 0)) << "missing MIX must own its name without registration";
    // Sanitizers check invalid/mismatched frees; leak checking depends on host support.
    files.clear();
    EXPECT_TRUE((files.mixes().empty())) << "owned object teardown left registry nodes";
}
void yrpp_context_and_file_interfaces() {
    Temp first_root, second_root;
    write(first_root.path / "base.mix", mix({{"same.txt", bytes("first")}}));
    write(second_root.path / "base.mix", mix({{"same.txt", bytes("second")}}));
    {
    ResourceEnvironment first(first_root.path);
    rejects([&] { ResourceEnvironment competing(second_root.path); }, "parallel host resource environment accepted");
    auto& first_mix = first.mount("base.mix");
    {
        const auto absolute = path_to_utf8(first_root.path / "base.mix");
        MixFileClass unbound(absolute.c_str());
        EXPECT_TRUE((unbound.IsValid() && !try_current_context())) << "absolute MIX must construct without a host context";
    }
    {
        game::ResourceScope outer({&first, nullptr, {}});
        void* data = nullptr;
        MixFileClass* owner = nullptr;
        int offset = 0, length = 0;
        const auto lookup = [&] { return MixFileClass::Offset("same.txt", &data, &owner, &offset, &length); };
        EXPECT_TRUE((lookup() && owner == &first_mix)) << "YRpp Offset did not use outer context";
        FileLocation nested_source;
        {
            game::ResourceScope inner({&first, &nested_source, {}});
            EXPECT_TRUE((lookup() && owner == &first_mix && game::current_context().source == &nested_source)) << "nested service context changed the authoritative MIX chain";
        }
        EXPECT_TRUE((lookup() && owner == &first_mix)) << "nested context did not restore caller";
        rejects([&] {
            game::ResourceScope inner({&first, &nested_source, {}});
            throw std::runtime_error("scope unwind");
        }, "test exception missing");
        EXPECT_TRUE((lookup() && owner == &first_mix)) << "exception leaked nested context";
        data = &first_mix;
        const auto previous_offset = offset;
        EXPECT_TRUE((!MixFileClass::Offset("absent.txt", &data, &owner, &offset, &length) &&
            data == &first_mix && owner == &first_mix && offset == previous_offset && length == 5)) << "YRpp pointer outputs changed on miss";
        for (unsigned mask = 0; mask < 16; ++mask) {
            EXPECT_TRUE((MixFileClass::Offset("same.txt", mask & 1 ? &data : nullptr,
                mask & 2 ? &owner : nullptr, mask & 4 ? &offset : nullptr, mask & 8 ? &length : nullptr))) << "nullable original Offset outputs";
        }
        CCFileClass file("same.txt");
        FileClass& api = file;
        EXPECT_TRUE((api.Exists() && api.Open(FileAccessMode::Read))) << "YRpp virtual open/exists on MIX member";
        char buffer[20]{};
        EXPECT_TRUE((api.GetFileSize() == 5 && api.ReadBytes(buffer, sizeof(buffer)) == 5 &&
            std::string_view(buffer, 5) == "first")) << "YRpp virtual member bounds/read";
        EXPECT_TRUE((api.ReadBytes(buffer, 1) == 0)) << "YRpp member read passed end";
        EXPECT_TRUE((api.Seek(0, FileSeekMode::Set) == 0 && api.ReadBytes(buffer, 1) == 1)) << "YRpp virtual seek";
        EXPECT_TRUE((!api.OpenEx("absent.txt", FileAccessMode::Read) && !api.HasHandle())) << "YRpp failed reopen retained previous handle";
        // The cache is borrowed: closing a CCFile must never release its carrier.
        std::vector<BYTE> cached{'c','a','c','h','e'};
        first_mix.Data = cached.data();
        EXPECT_TRUE((api.OpenEx("same.txt", FileAccessMode::Read) && api.ReadBytes(buffer, 20) == 5 &&
            std::string_view(buffer, 5) == "cache")) << "YRpp cached member path";
        api.Close();
        EXPECT_TRUE((!api.HasHandle() && !file.Buffer.Buffer && file.Position == 0)) << "YRpp close retained borrowed buffer/cursor";
        first_mix.Data = nullptr;
        EXPECT_TRUE((api.OpenEx("same.txt", FileAccessMode::Write))) << "CCFile write must create loose override";
        char replacement[] = "written";
        EXPECT_TRUE((api.WriteBytes(replacement, 7) == 7)) << "CCFile loose write";
        api.Close();
        EXPECT_TRUE((read(first, "same.txt") == "written")) << "loose write must override MIX without changing archive";
    }
    rejects([] { game::current_context(); }, "context survived scope lifetime");
    }
    EXPECT_TRUE((MixFileClass::MIXes.empty())) << "destroyed host left global registrations";
    ResourceEnvironment second(second_root.path);
    auto& second_mix = second.mount("base.mix");
    MixFileClass* owner = nullptr;
    EXPECT_TRUE((MixFileClass::Offset("same.txt", nullptr, &owner, nullptr, nullptr) && owner == &second_mix &&
        read(second, "same.txt") == "second")) << "sequential host did not use fresh global state";
}

void caller_owned_mix_teardown() {
    Temp old_root, new_root;
    write(old_root.path / "base.mix", mix({{"old.txt", bytes("old")}, {"same.txt", bytes("old")}}));
    write(old_root.path / "owned.mix", mix({{"owned.txt", bytes("owned")}}));
    write(new_root.path / "base.mix", mix({{"same.txt", bytes("new")}}));
    std::unique_ptr<MixFileClass> retired;
    {
        ResourceEnvironment files(old_root.path);
        ResourceScope scope({&files, nullptr, {}});
        retired = std::make_unique<MixFileClass>("base.mix");
        MixFileClass stack_object("base.mix");
        MixFileClass::Generics.RA2 = &files.mount("owned.mix");
        EXPECT_TRUE((files.mixes().size() == 3 && exists(files, "old.txt"))) << "mixed ownership setup";
        files.clear();
        EXPECT_TRUE((files.mixes().empty() && !MixFileClass::Generics.RA2)) << "clear retained caller-owned MIX registrations";
        for (const auto* object : {retired.get(), &stack_object}) {
            EXPECT_TRUE((!object->GenericNode::Next() && !object->GenericNode::Prev() && !object->IsValid() &&
                object->find(filename_id("old.txt")) && std::strcmp(object->FileName, "base.mix") == 0)) << "clear must detach caller-owned objects without deleting their metadata";
        }
        files.clear(); // Empty-list cleanup is idempotent and preserves sentinels.
        retired = std::make_unique<MixFileClass>("base.mix");
        EXPECT_TRUE((files.mixes().size() == 1)) << "list could not register after explicit clear";
        // No explicit clear this time: ResourceEnvironment destruction must also detach.
    }
    EXPECT_TRUE((MixFileClass::MIXes.empty() && !retired->IsValid())) << "host destruction left a caller-owned MIX registered";
    {
        ResourceEnvironment next(new_root.path);
        EXPECT_TRUE((!exists(next, "old.txt"))) << "empty next environment searched the retired registry";
        auto& current = next.mount("base.mix");
        EXPECT_TRUE((read(next, "same.txt") == "new" && !exists(next, "old.txt"))) << "retired MIX contaminated the next directory's lookup";
        retired.reset(); // A late caller destructor must not disturb the new chain.
        EXPECT_TRUE((next.mixes().size() == 1 && next.mixes().front() == &current && current.IsValid())) << "retired destructor damaged the next environment";
    }
}

void common_map_archives() {
    Temp fixture;
    write(fixture.path/"conqmd.mix",mix({{"pips.shp",bytes("md pips")}}));
    write(fixture.path/"conquer.mix",mix({{"pips.shp",bytes("base pips")},{"onlybase.shp",bytes("base fallback")}}));
    write(fixture.path/"genermd.mix",mix({{"pips.shp",bytes("terrain shadow")}}));
    for(const auto* name:{"generic.mix","isogenmd.mix","isogen.mix"})write(fixture.path/name,mix({}));
    ResourceEnvironment files(fixture.path);
    game::ResourceScope scope({&files,nullptr,{}});
    EXPECT_TRUE((MixFileClass::LoadTerrainMixes())) << "mount common map resources";
    EXPECT_TRUE((read(files,"PIPS.SHP")=="md pips"&&read(files,"ONLYBASE.SHP")=="base fallback")) << "original CONQMD precedence and CONQUER fallback";
    std::vector<std::string> names;
    for(const auto* archive:MixFileClass::MIXes)names.emplace_back(archive->FileName);
    EXPECT_TRUE((names==std::vector<std::string>{"CONQMD.MIX","GENERMD.MIX","GENERIC.MIX","ISOGENMD.MIX","ISOGEN.MIX","CONQUER.MIX"})) << "0x00530460 common archive ordering";
    const auto count=files.mixes().size();
    EXPECT_TRUE((MixFileClass::LoadTerrainMixes()&&files.mixes().size()==count)) << "common archives are not remounted per scene";
    files.clear();
    EXPECT_TRUE((!MixFileClass::Generics.CONQMD&&!MixFileClass::Generics.CONQUER)) << "common archive slots clear on resource teardown";
}
void bootstrap_and_lifetime() {
    Temp fixture;
    const auto cache = mix({{"cache.txt", bytes("base")}});
    write(fixture.path / "langmd.mix", mix({{"winner.txt", bytes("language")},
        {"expandmd98.mix", mix({{"notmounted.txt",bytes("hidden")}})}}));
    write(fixture.path / "language.mix", mix({}));
    write(fixture.path / "expandmd99.mix", mix({{"winner.txt",bytes("99")},
        {"expand.txt",bytes("99")}, {"cachemd.mix",mix({{"cache.txt",bytes("patch")}})}}));
    write(fixture.path / "expandmd01.mix", mix({{"expand.txt",bytes("01")}}));
    write(fixture.path / "ra2md.mix", mix({{"cachemd.mix",cache}, {"localmd.mix",mix({})}}));
    write(fixture.path / "ra2.mix", mix({{"cache.mix",cache}, {"local.mix",mix({})}}));
    write(fixture.path / "thememd.mix", bytes("CLASS")); // later phase must not touch this
    {
    ResourceEnvironment files(fixture.path);
    game::mount_language_md(files); game::mount_language(files);
    game::ResourceScope scope({&files, nullptr, {}});
    game::disk_selection = 7;
    EXPECT_TRUE((MixFileClass::Bootstrap() && game::disk_selection == 7)) << "synchronous bootstrap/disk restoration";
    std::vector<std::string> mounted;
    for (const auto* archive : MixFileClass::MIXes) mounted.emplace_back(archive->FileName);
    EXPECT_TRUE((mounted == std::vector<std::string>{"LANGMD.MIX","LANGUAGE.MIX","EXPANDMD99.MIX",
        "EXPANDMD01.MIX","RA2MD.MIX","RA2.MIX","CACHEMD.MIX","CACHE.MIX","LOCALMD.MIX","LOCAL.MIX"})) << "original mount order";
    EXPECT_TRUE((&files.mixes() == &MixFileClass::MIXes)) << "second lookup authority";
    EXPECT_TRUE((MixFileClass::Array.Count == 2 && MixFileClass::Array.Capacity == 10)) << "expansion probes/growth";
    EXPECT_TRUE((!exists(files, "notmounted.txt"))) << "nested expansion leaked into registry";
    EXPECT_TRUE((read(files,"winner.txt") == "language")) << "language pre-mount priority";
    EXPECT_TRUE((read(files,"expand.txt") == "99")) << "descending expansion priority";
    EXPECT_TRUE((read(files,"cache.txt") == "patch")) << "nested MIX must use global source lookup";
    auto& g = MixFileClass::Generics;
    EXPECT_TRUE((MixFileClass::Cache("CACHEMD.MIX") && g.LOCAL->IsValid())) << "generic slot/cache contract";
    const auto* existing_language = g.LANGMD;
    const auto* existing_ra2md = g.RA2MD;
    EXPECT_TRUE((MixFileClass::Bootstrap())) << "second synchronous entry";
    EXPECT_TRUE((g.LANGMD == existing_language && files.mixes().front() == existing_language)) << "stage must preserve caller-established language state";
    EXPECT_TRUE((g.RA2MD != existing_ra2md && files.mixes().size() == 18 && MixFileClass::Array.Count == 4)) << "stage must append without clearing existing registry";
    std::vector<std::string> released;
    game::shutdown_resource_files(files, [](void* data, const char* name) noexcept {
        static_cast<std::vector<std::string>*>(data)->emplace_back(name);
    }, &released);
    EXPECT_TRUE((released == std::vector<std::string>{"EXPANDMD99.MIX","EXPANDMD01.MIX",
        "EXPANDMD99.MIX","EXPANDMD01.MIX","CACHE.MIX","CACHEMD.MIX","LOCAL.MIX","LOCALMD.MIX",
        "RA2.MIX","RA2MD.MIX","LANGUAGE.MIX","LANGMD.MIX"})) << "original shutdown subsequence";
    EXPECT_TRUE((MixFileClass::Array.Count == 0 && MixFileClass::Array.Capacity == 10 &&
        !g.RA2MD && !g.LANGMD && files.mixes().size() == 6)) << "slot shutdown must retain storage and overwritten orphan objects until host teardown";
    game::shutdown_resource_files(files); // idempotent, including empty slots
    files.clear();
    EXPECT_TRUE((MixFileClass::MIXes.empty() && !MixFileClass::Array.Items &&
        MixFileClass::Array.Capacity == 0 && game::disk_selection == 0)) << "full host teardown";
    // Disable growth to exercise actual YRpp append failure, not a fake vector.
    MixFileClass::Array.CapacityIncrement = 0;
    EXPECT_TRUE((MixFileClass::Bootstrap() && MixFileClass::Array.Count == 0 && files.mixes().size() == 8)) << "append failure removed a registered expansion or failed bootstrap";
    game::shutdown_resource_files(files);
    EXPECT_TRUE((files.mixes().size() == 2)) << "append-failed expansions must survive slot shutdown";
    }
    EXPECT_TRUE((MixFileClass::MIXes.empty() && MixFileClass::Array.CapacityIncrement == 10)) << "destructor did not reclaim/reset host state";
    Temp empty;
    ResourceEnvironment missing(empty.path);
    game::mount_language_md(missing); game::mount_language(missing);
    game::ResourceScope scope({&missing, nullptr, {}});
    EXPECT_TRUE((MixFileClass::Bootstrap() && missing.mixes().empty())) << "missing archive constructor must succeed unregistered";
    EXPECT_TRUE((MixFileClass::Generics.RA2 && !MixFileClass::Generics.RA2->IsValid())) << "missing generics retain slots";
    missing.clear();
    write(empty.path / "ra2.mix", bytes("CLASS"));
    game::disk_selection = 7;
    rejects([] { MixFileClass::Bootstrap(); }, "malformed present pack must surface host error");
    EXPECT_TRUE((game::disk_selection == -2 && MixFileClass::Generics.RA2MD && !MixFileClass::Generics.RA2 &&
        !MixFileClass::Generics.CACHEMD)) << "host exception rolled back partial original state";
}
void non_owning_containers() {
    struct Item : GenericNode { int& destroyed; explicit Item(int& count) : destroyed(count) {} ~Item() { ++destroyed; } };
    int destroyed = 0;
    {
        Item item(destroyed);
        {
            List<Item> list;
            list.AddTail(&item);
            DynamicVectorClass<Item*> array;
            EXPECT_TRUE((array.IsInitialized && !array.IsAllocated && array.Count == 0 && array.CapacityIncrement == 10)) << "original array initializer";
            EXPECT_TRUE((array.AddItem(&item) && array.Capacity == 10 && array.IsAllocated)) << "default vector growth";
            for (int i = 0; i < 10; ++i) EXPECT_TRUE((array.AddItem(&item))) << "second growth";
            EXPECT_TRUE((array.Count == 11 && array.Capacity == 20)) << "growth quantum";
            EXPECT_TRUE((!array.SetCapacity(-1) && array.Count == 11 && array.Capacity == 20)) << "failed resize changed state";
            EXPECT_TRUE((array.SetCapacity(3) && array.Count == 3)) << "shrink truncates count";
            EXPECT_TRUE((array.RemoveItem(0) && array.Count == 2 && array[0] == &item)) << "remove shifts elements";
            array.Clear();
            EXPECT_TRUE((!array.Items && !array.IsAllocated && !array.Count && destroyed == 0)) << "array storage release deleted its pointee";
            Item* storage[1]{};
            DynamicVectorClass<Item*> external(1, storage);
            EXPECT_TRUE((external.AddItem(&item) && !external.AddItem(&item) && external.Items == storage &&
                external.Count == 1 && !external.IsAllocated)) << "external vector illegally grew";
        }
        EXPECT_TRUE((!item.IsValid() && !item.GenericNode::Next() && !item.GenericNode::Prev() && !destroyed)) << "global-list destructor must unlink without deleting nodes";
    }
    EXPECT_TRUE((destroyed == 1)) << "non-owning containers deleted object twice";
}

void image_file_io() {
    Temp fixture;
    const Bytes payload{0, 7, 255, 42, 11};
    write(fixture.path / "loose.bin", payload);
    write(fixture.path / "empty.bin", {});
    write(fixture.path / "image.mix", mix({{"member.bin", payload},
        {"inner.mix", mix({{"nested.bin", payload}})}}));
    write(fixture.path / "cache.mix", mix({{"cached.bin", payload}}));
    ResourceEnvironment files(fixture.path);
    files.mount("image.mix");
    files.mount("inner.mix");
    auto& cache = files.mount("cache.mix");
    Bytes cached = payload;
    cache.Data = cached.data(); // borrowed MIX body, never freed by a reader
    game::ResourceScope scope({&files, nullptr, {}});
    for (const char* name : {"LOOSE.BIN", "member.bin", "nested.bin", "cached.bin"}) {
        alignas(CCFileClass) std::byte file_storage[sizeof(CCFileClass)];
        std::unique_ptr<CCFileClass, decltype(&game::DestroyFile)> file(
            game::ConstructFile(file_storage, name), game::DestroyFile);
        EXPECT_TRUE((file->Exists(false) && !file->HasHandle())) << "image existence opened a stream";
        EXPECT_TRUE((file->GetFileSize() == 5 && !file->HasHandle() &&
            std::string_view(file->GetFileName()) == name)) << "image size mutated unopened CCFile";
        void* data = file->ReadWholeFile();
        EXPECT_TRUE((data && !std::memcmp(data, payload.data(), 5) && !file->HasHandle())) << "image whole-file bytes or lazy close";
        YRMemory::Deallocate(data);
        EXPECT_TRUE((file->GetFileSize() == 5 && !file->HasHandle())) << "size lost the closed MIX carrier bias";
    }
    EXPECT_TRUE((cached == payload && cache.Data == cached.data())) << "image reader freed/changed borrowed cache";
    {
        struct ObservedFile : CCFileClass {
            using CCFileClass::CCFileClass;
            int opens = 0;
            bool Open(FileAccessMode mode) override { ++opens; return CCFileClass::Open(mode); }
        } zero("cached.bin");
        EXPECT_TRUE((zero.ReadBytes(nullptr, 0) == 0 && zero.opens == 1 && !zero.HasHandle())) << "zero-byte CCFile read skipped original open/close side effects";
    }
    {
        char name[] = "cached.bin";
        alignas(CCFileClass) std::byte file_storage[sizeof(CCFileClass)];
        std::unique_ptr<CCFileClass, decltype(&game::DestroyFile)> file(
            game::ConstructFile(file_storage, name), game::DestroyFile);
        name[0] = '!';
        EXPECT_TRUE((file->Exists(false))) << "CCFile constructor failed to copy name";
        char first[2]{};
        file->ReadBytes(first, 2);
        EXPECT_TRUE((!file->HasHandle() && !std::memcmp(first, payload.data(), 2))) << "lazy cached read";
        file->Open(FileAccessMode::Read);
        file->Seek(2, FileSeekMode::Set);
        EXPECT_TRUE((file->GetFileSize() == 5 && file->HasHandle())) << "size closed caller stream";
        char rest[8]{};
        file->ReadBytes(rest, 8);
        EXPECT_TRUE((file->HasHandle() && !std::memcmp(rest, payload.data() + 2, 3) && rest[3] == 0)) << "open image stream cursor/EOF/ownership";
        file->Close();
    }
    {
        alignas(CCFileClass) std::byte missing_storage[sizeof(CCFileClass)];
        std::unique_ptr<CCFileClass, decltype(&game::DestroyFile)> missing(
            game::ConstructFile(missing_storage, "absent.bin"), game::DestroyFile);
        EXPECT_TRUE((!missing->Exists(false) && missing->GetFileSize() == 0 && !missing->ReadWholeFile() &&
            !missing->HasHandle())) << "missing image file status";
        alignas(CCFileClass) std::byte empty_storage[sizeof(CCFileClass)];
        std::unique_ptr<CCFileClass, decltype(&game::DestroyFile)> empty(
            game::ConstructFile(empty_storage, "empty.bin"), game::DestroyFile);
        EXPECT_TRUE((empty->Exists(false) && empty->GetFileSize() == 0)) << "empty image file status";
        void* zero = empty->ReadWholeFile();
        YRMemory::Deallocate(zero); // zero-size allocation is allowed; never skipped by ReadWholeFile
    }
    // ReadWholeFile retains the original short-read result, not an invented
    // all-or-nothing policy. An already-open file remains at the advanced cursor.
    {
        const auto path = path_to_utf8(fixture.path / "loose.bin");
        RawFileClass raw(path.c_str());
        EXPECT_TRUE((raw.Open(FileAccessMode::Read) && raw.Seek(3, FileSeekMode::Set) == 3)) << "raw image setup";
        void* tail = raw.ReadWholeFile();
        EXPECT_TRUE((tail && !std::memcmp(tail, payload.data() + 3, 2) && raw.HasHandle() && raw.Seek(0) == 5)) << "whole-file short read discarded buffer or closed caller stream";
        YRMemory::Deallocate(tail);
    }
    char* copy = game::DuplicateName("Abc-123");
    EXPECT_TRUE((copy != nullptr && game::DuplicateName(nullptr) == nullptr)) << "image name duplication";
    game::UppercaseName(copy);
    EXPECT_TRUE((std::string_view(copy) == "ABC-123")) << "image filename uppercase";
    game::FreeName(copy);
    game::FreeName(nullptr);
    YRMemory::Deallocate(nullptr);
    cache.Data = nullptr;
}

int reference(const fs::path& directory) {
    ResourceEnvironment files(directory);
    const auto start = std::chrono::steady_clock::now();
    game::mount_language_md(files); game::mount_language(files);
    game::ResourceScope scope({&files, nullptr, {}});
    EXPECT_TRUE((MixFileClass::Bootstrap())) << "reference bootstrap failed";
    size_t entries = 0;
    for (const auto* archive : MixFileClass::MIXes) {
        const MixHeaderData* directory = nullptr;
        int count = 0;
        if (archive->headers(directory, count)) entries += static_cast<size_t>(count);
    }
    std::cout << "Mounted " << files.mixes().size() << " archives, " << entries << " entries, "
              << std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count() << " ms\n";
    for (const auto* archive : files.mixes()) {
        const MixHeaderData* directory = nullptr;
        int count = 0;
        const bool has_entries = archive->headers(directory, count);
        std::cout << archive->FileName << '\t' << count << '\t'
                  << files.source_of(*archive).physical_file.filename().string() << '\t'
                  << files.source_of(*archive).offset << '\t' << archive->FileStartOffset << '\n';
        // Exercise original signed binary lookup for every stored entry.
        if (has_entries) for (int i = 0; i < count; ++i)
            EXPECT_TRUE((archive->find(directory[i].ID) != nullptr)) << std::string("unfindable reference index entry: ") + archive->FileName;
    }
    return 0;
}
}

TEST(Resource, CryptoStreamsBootstrapAndOwnership) {
    hashes_and_crypto(); mix_without_resource_context(); files_and_mounts(); file_output_lifetime(); mix_object_contracts(); yrpp_context_and_file_interfaces(); caller_owned_mix_teardown(); bootstrap_and_lifetime(); common_map_archives(); non_owning_containers(); image_file_io();
}

namespace {
std::optional<int> legacy_command(int argc, char** argv) {
    if (argc == 3 && std::string_view(argv[1]) == "--reference") return reference(path_from_utf8(argv[2]));
    return std::nullopt;
}
const ra2::test::CommandRegistration command(legacy_command);
}
