#include <cfenv>
#include "support/test_support.hpp"
#include "yrpp/CCINIClass.h"
#include "yrpp/Straws.h"
#include "yrpp/Pipes.h"
#include "yrpp/CRC.h"
#include "filesystem/resource_environment.hpp"
#include "filesystem/resource_context.hpp"
#include "filesystem/resource_startup.hpp"
#include <bit>
#include <cmath>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

// GenericNode's original copy operation accepts non-const sources, which the
// usual is_copy_constructible/is_copy_assignable traits alone do not test.
static_assert(!std::is_constructible_v<INIClass::INIEntry, INIClass::INIEntry&>);
static_assert(!std::is_assignable_v<INIClass::INIEntry&, INIClass::INIEntry&>);
static_assert(!std::is_copy_constructible_v<INIClass::INIEntry>);
static_assert(!std::is_copy_assignable_v<INIClass::INIEntry>);
static_assert(!std::is_move_constructible_v<INIClass::INIEntry>);
static_assert(!std::is_move_assignable_v<INIClass::INIEntry>);
static_assert(!std::is_constructible_v<INIClass::INISection, INIClass::INISection&>);
static_assert(!std::is_assignable_v<INIClass::INISection&, INIClass::INISection&>);


int load(INIClass& ini, const std::string& text, bool comments = false) {
    BufferStraw input(const_cast<char*>(text.data()), static_cast<int>(text.size()));
    return ini.ReadStraw(input, comments);
}
class TextPipe : public Pipe {
public:
    std::string text;
    int Put(const void* input, int count) override { text.append(static_cast<const char*>(input), size_t(count)); return count; }
};
std::string save(INIClass& ini) { TextPipe output; ini.WritePipe(output); return output.text; }
std::string value(INIClass& ini, const char* section, const char* key, const char* fallback = "missing") {
    char text[512]; ini.ReadString(section, key, fallback, text, sizeof(text)); return text;
}
std::string hex(const char* data, size_t length) {
    constexpr char digits[] = "0123456789abcdef";
    std::string out;
    for (size_t i = 0; i < length; ++i) { const auto c = static_cast<unsigned char>(data[i]); out += digits[c >> 4]; out += digits[c & 15]; }
    return out;
}
void quoted(const std::string& data) { std::cout << '"' << hex(data.data(), data.size()) << '"'; }
void comments(INIClass::INIComment* item) {
    std::cout << '['; bool first = true;
    for (; item; item = item->Next) {
        if (!first) std::cout << ','; first = false;
        if (item->Value) quoted(item->Value); else std::cout << "null";
    }
    std::cout << ']';
}
void snapshot(INIClass& ini, int status) {
    std::cout << "{\"status\":" << status << ",\"sections\":[";
    bool first = true;
    for (auto* section : ini.Sections) {
        if (!first) std::cout << ','; first = false;
        std::cout << '['; quoted(section->Name); std::cout << ','; comments(section->Comments); std::cout << ",[";
        bool entry_first = true;
        for (auto* entry : section->Entries) {
            if (!entry_first) std::cout << ','; entry_first = false;
            std::cout << '['; quoted(entry->Key); std::cout << ','; quoted(entry->Value); std::cout << ',';
            comments(entry->Comments); std::cout << ',';
            if (entry->CommentString) quoted(entry->CommentString); else std::cout << "null";
            std::cout << ',' << entry->PreIndentCursor << ',' << entry->PostIndentCursor << ',' << entry->CommentCursor << ']';
        }
        std::cout << "]]";
    }
    std::cout << "],\"comments\":"; comments(ini.LineComments);
    std::cout << ",\"serialized\":"; quoted(save(ini));
    std::cout << ",\"queries\":[";
    first = true;
    for (auto* section : ini.Sections) for (auto* entry : section->Entries) {
        if (!first) std::cout << ','; first = false;
        quoted(value(ini, section->Name, entry->Key));
    }
    std::cout << "]}\n";
}
#if UINTPTR_MAX == 0xffffffff
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
// These assert native x86 data layout only, not MSVC/GNU vtable interoperability.
static_assert(sizeof(INIClass) == 0x40 && sizeof(CCINIClass) == 0x58);
static_assert(sizeof(INIClass::INIEntry) == 0x28 && sizeof(INIClass::INISection) == 0x44);
static_assert(offsetof(INIClass, Sections) == 0x0c && offsetof(INIClass, SectionIndex) == 0x28);
static_assert(offsetof(INIClass, LineComments) == 0x3c && offsetof(CCINIClass, Digested) == 0x40);
static_assert(offsetof(CCINIClass, Digest) == 0x41);
static_assert(offsetof(INIClass::INIEntry, Key) == 0x0c && offsetof(INIClass::INIEntry, CommentCursor) == 0x24);
static_assert(offsetof(INIClass::INISection, Entries) == 0x10 && offsetof(INIClass::INISection, Comments) == 0x40);
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#endif
void file_tests() {
    struct Temporary {
        std::filesystem::path directory = std::filesystem::temp_directory_path() /
            ("ra2-ini-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        Temporary() { std::filesystem::create_directories(directory); }
        ~Temporary() { std::error_code ignored; std::filesystem::remove_all(directory, ignored); }
    } temp;
    const auto path = game::path_to_utf8(temp.directory / "config.ini");
    CCINIClass ini; ini.WriteString("S", "K", "value");
    RawFileClass file(path.c_str());
    EXPECT_TRUE((ini.WriteCCFile(&file) == 14)) << "serialized byte count";
    EXPECT_TRUE((!file.HasHandle())) << "writer closes its own handle";
    ini.Clear();
    EXPECT_TRUE((ini.ReadCCFile(&file) == 1 && !file.HasHandle())) << "reader opens and closes handle";
    EXPECT_TRUE((value(ini, "S", "K") == "value")) << "physical INI round trip";
    EXPECT_TRUE((file.Open(FileAccessMode::Read))) << "open borrowed input";
    ini.Clear(); EXPECT_TRUE((ini.ReadCCFile(&file) == 1 && file.HasHandle())) << "reader preserves borrowed handle";
    file.Close();
    EXPECT_TRUE((file.Open(FileAccessMode::Write))) << "open borrowed output";
    ini.WriteCCFile(&file); EXPECT_TRUE((file.HasHandle())) << "writer preserves borrowed handle"; file.Close();
    EXPECT_TRUE((ini.WriteCCFile(&file, true) > 14 && ini.Digested && !ini.Exists("Digest", nullptr))) << "digest write removes temporary section";
    CCINIClass verified;
    EXPECT_TRUE((verified.ReadCCFile(&file, true) == 1 && verified.Digested && !verified.Exists("Digest", nullptr))) << "SHA-1 digest read verifies and removes section";
    EXPECT_TRUE((verified.GetCRC() == ini.GetCRC())) << "round-trip CRC";
    verified.WriteString("S", "K", "tampered");
    byte wrong[20]{};
    verified.WriteUUBlock("Digest", wrong, sizeof(wrong));
    verified.WriteCCFile(&file, false);
    CCINIClass mismatch;
    EXPECT_TRUE((mismatch.ReadCCFile(&file, true) == 2 && mismatch.Digested && !mismatch.Exists("Digest", nullptr))) << "mismatched digest retains parsed content, removes digest, returns 2";
    ini.WriteCCFile(&file, false);
    CCINIClass missing;
    EXPECT_TRUE((missing.ReadCCFile(&file, true) == 2 && !missing.Digested && value(missing,"S","K") == "value")) << "missing digest is status 2, content remains available";
    game::ResourceEnvironment files(temp.directory);
    game::ResourceScope scope({&files, nullptr, {}});
    ini.Clear(); EXPECT_TRUE((ini.LoadFromFile("CONFIG.INI") == 1)) << "case-insensitive loose INI lookup";
    const int count = ini.SectionIndex.Count();
    EXPECT_TRUE((ini.LoadFromFile("missing.ini") == 0 && ini.SectionIndex.Count() == count)) << "missing INI preserves existing data";
    CCFileClass output("written.ini");
    EXPECT_TRUE((ini.WriteCCFile(&output) == 14 && !output.HasHandle())) << "CCFile serializes and closes its own output";
    CCINIClass roundtrip;
    EXPECT_TRUE((roundtrip.LoadFromFile("written.ini") == 1 && value(roundtrip, "S", "K") == "value")) << "CCFile INI write/read round trip";
}
void check_live_indexes(INIClass& ini) {
    for (const auto& indexed : ini.SectionIndex) {
        bool live = false;
        for (auto* section : ini.Sections) if (section == indexed.Data) live = true;
        EXPECT_TRUE((live)) << "section index must only reference live sections";
    }
    for (auto* section : ini.Sections) for (const auto& indexed : section->EntryIndex) {
        bool live = false;
        for (auto* entry : section->Entries) if (entry == indexed.Data) live = true;
        EXPECT_TRUE((live)) << "entry index must only reference live entries";
    }
}
void duplicate_mutation_tests() {
    // The original retains the index lookup cache when appending without
    // growing the table. Deletion must remove that same selected object even
    // when sorting the new table would select another duplicate.
    INIClass ini;
    EXPECT_TRUE((load(ini, "[S]\nA=first\n[S]\nA=second\n") == 1)) << "duplicate section fixture";
    const std::string selected = ini.GetSection("S")->Entries.front()->Value;
    const std::string survivor = selected == "first" ? "second" : "first";
    EXPECT_TRUE((ini.WriteString("Other", "K", "V"))) << "append section after cached lookup";
    EXPECT_TRUE((ini.Clear("S"))) << "remove cached duplicate section";
    check_live_indexes(ini);
    EXPECT_TRUE((ini.SectionIndex.Count() == 2 && value(ini, "S", "A") == survivor)) << "duplicate section survivor remains readable";
    EXPECT_TRUE((value(ini, "Other", "K") == "V")) << "unrelated section survives deletion";

    for (int operation = 0; operation < 4; ++operation) {
        ini.Clear();
        EXPECT_TRUE((load(ini, "[S]\nA=first\nA=second\n") == 1)) << "duplicate key fixture";
        const std::string selected_value = ini.FindEntry("S", "A")->Value;
        const std::string remaining_value = selected_value == "first" ? "second" : "first";
        EXPECT_TRUE((ini.WriteString("S", "Other", "V"))) << "append key after cached lookup";
        if (operation == 0) EXPECT_TRUE((ini.Clear("S", "A"))) << "clear cached duplicate key";
        else EXPECT_TRUE((ini.WriteString("S", "A", operation == 1 ? "updated" : operation == 2 ? nullptr : ""))) << "replace or erase cached duplicate key";
        check_live_indexes(ini);
        std::vector<std::string> values;
        for (auto* entry : ini.GetSection("S")->Entries) values.emplace_back(entry->Value);
        const std::vector<std::string> expected = operation == 1
            ? std::vector<std::string>{remaining_value, "V", "updated"}
            : std::vector<std::string>{remaining_value, "V"};
        EXPECT_TRUE((values == expected)) << "mutation must replace only the cached duplicate, preserving insertion order";
        EXPECT_TRUE((ini.GetKeyCount("S") == int(expected.size()))) << "duplicate key index count";
        if (operation != 1) EXPECT_TRUE((value(ini, "S", "A") == remaining_value)) << "remaining duplicate key is readable";
        EXPECT_TRUE((value(ini, "S", "Other") == "V")) << "unrelated key survives mutation";
    }
}
void unit_tests() {
    CCINIClass ini;
    EXPECT_TRUE((load(ini, "") == 0)) << "empty input";
    EXPECT_TRUE((load(ini, "; only comments\n", true) == 1)) << "comment-only input";
    ini.Clear();
    EXPECT_TRUE((load(ini, "[General]\r\nA=first\nB=2\nA=last\nBlank=\nBool=yes indeed\nHex=$Af\nHex2=FFh\nOctal=010\nBad=no number\nRatio=12.5%\nPrecise=0.1\n[Empty]\n") == 1)) << "load text";
    EXPECT_TRUE((ini.GetKeyCount("General") == 10)) << "duplicate keys retained / blanks skipped";
    EXPECT_TRUE((!ini.Exists("general", "A") && !ini.Exists("Empty", nullptr))) << "case sensitivity / empty section";
    EXPECT_TRUE((std::string(ini.GetKeyName("General", 0)) == "A" && std::string(ini.GetKeyName("General", 2)) == "A")) << "insertion enumeration";
    EXPECT_TRUE((ini.ReadInteger("General", "Hex", -1) == 175 && ini.ReadInteger("General", "Hex2", -1) == 255)) << "hex conventions";
    EXPECT_TRUE((ini.ReadInteger("General", "Octal", -1) == 10 && ini.ReadInteger("General", "Bad", -1) == 0)) << "atoi conventions";
    EXPECT_TRUE((ini.ReadBool("General", "Bool", false))) << "boolean initial letter";
    EXPECT_TRUE((ini.ReadDouble("General", "Ratio", 0) == 0.125)) << "percent";
    EXPECT_TRUE((ini.ReadDouble("General", "Precise", 0) == double(0.1f))) << "float then double";
    char buffer[8] = "keep";
    EXPECT_TRUE((ini.ReadString("General", "A", "", buffer, 1) == 0 && std::string(buffer) == "keep")) << "small output unchanged";
    EXPECT_TRUE((ini.ReadString(nullptr, "A", "", buffer, 8) == 0 && std::string(buffer) == "keep")) << "null section unchanged";
    EXPECT_TRUE((ini.ReadString("General", "missing", buffer, buffer, 8) == 4 && std::string(buffer) == "keep")) << "aliased fallback";
    EXPECT_TRUE((buffer[5] == 0 && buffer[6] == 0 && buffer[7] == 0)) << "aliased fallback pads the complete output";
    std::memset(buffer, 'X', 8);
    EXPECT_TRUE((ini.ReadString("General", "missing", "ok", buffer, 8) == 2 &&
        !std::memcmp(buffer, "ok\0\0\0\0\0\0", 8))) << "read string preserves original strncpy padding";
    std::memset(buffer, 'X', 8);
    EXPECT_TRUE((ini.ReadString("General", "missing", nullptr, buffer, 8) == 0 && buffer[0] == 0 &&
        buffer[1] == 'X')) << "null fallback only clears the first byte";
    EXPECT_TRUE((ini.CurrentSection != nullptr)) << "section cache populated";
    const int count = ini.GetKeyCount("General"); ini.Reset();
    EXPECT_TRUE((!ini.CurrentSection && count == ini.GetKeyCount("General"))) << "Reset only clears cache";
    ini.Clear();
    EXPECT_TRUE((load(ini, "[S]\nA=1\nB=2\nC=3") == 1 && !ini.Exists("S", "C"))) << "initial final line without LF";
    EXPECT_TRUE((load(ini, "[S]\nA=4\nD=5") == 1 && value(ini,"S","D") == "5")) << "merge final line without LF";
    EXPECT_TRUE((std::string(ini.GetKeyName("S", 0)) == "B" && std::string(ini.GetKeyName("S", 1)) == "A")) << "update moves key to tail";
    ini.Clear();
    EXPECT_TRUE((load(ini, "; head\n[S]\n; key\nA\t= 1 ; inline\n\n[Empty]\n; tail\n", true) == 1)) << "comments load";
    EXPECT_TRUE((ini.Exists("Empty", nullptr))) << "keep empty section with comments";
    const auto encoded = save(ini);
    EXPECT_TRUE((encoded.find("; head\r\n[S]\r\n; key\r\nA       = 1 ; inline\r\n") == 0)) << "comment/column serialization";
    EXPECT_TRUE((ini.WriteString("S", "A", "2"))) << "comment preserving update";
    EXPECT_TRUE((save(ini).find("A       = 2 ; inline") != std::string::npos)) << "updated comment metadata";
    EXPECT_TRUE((ini.WriteString("S", "A", nullptr) && !ini.Exists("S", "A"))) << "deletion through WriteString";
    ini.Clear(); ini.Clear();
    EXPECT_TRUE((ini.Sections.empty() && !ini.LineComments && ini.SectionIndex.IndexTable == nullptr)) << "full cleanup";
    for (int i = 0; i < 64; ++i) { EXPECT_TRUE((load(ini, "[S]\nK=V\n") == 1)) << "repeated load"; ini.Clear(); }
    int destination = 19;
    IndexClass<int, int> index;
    EXPECT_TRUE((!index.TryGet(7, destination) && destination == 19)) << "query output on miss";
    EXPECT_TRUE((static_cast<const IndexClass<int,int>&>(index).FetchIndex(7) == 0)) << "stable missing reference";
    CRCEngine crc; const char raw[] = "General";
    const int whole = crc(raw, 7); CRCEngine split;
    split(raw, 2); EXPECT_TRUE((split(raw + 2, 5) == whole)) << "CRC streaming boundaries";
    // Pointer identity cache is intentional: mutation of the same section buffer
    // only takes effect after Reset, as it does in the original implementation.
    ini.WriteString("A", "K", "one"); ini.WriteString("B", "K", "two");
    char section[] = "A"; EXPECT_TRUE((value(ini, section, "K") == "one")) << "pointer cache setup";
    section[0] = 'B'; EXPECT_TRUE((value(ini, section, "K") == "one")) << "pointer identity cache";
    ini.Reset(); EXPECT_TRUE((value(ini, section, "K") == "two")) << "cache reset takes effect";
    EXPECT_TRUE((ini.ReadCCFile(nullptr, true) == 0)) << "null digest input is a parse failure";
}

TEST(Ini, TextLifecycleAndMutation) {
    const auto [argc, argv] = ra2::test::arguments();
            unit_tests();
            file_tests();
            duplicate_mutation_tests();
            if (argc > 2 && std::string(argv[1]) == "--reference") {
                game::ResourceEnvironment files(argv[2]);
                game::mount_language_md(files); game::mount_language(files);
                game::ResourceScope scope({&files, nullptr, {}});
                EXPECT_TRUE((MixFileClass::Bootstrap())) << "reference bootstrap";
                CCINIClass ini;
                EXPECT_TRUE((ini.LoadFromFile("rulesmd.ini") == 1)) << "MIX rulesmd.ini read";
                EXPECT_TRUE((ini.GetKeyCount("InfantryTypes") > 0 && ini.Exists("E1", "Strength"))) << "real INI types/values";
                std::cout << "rulesmd.ini: " << ini.SectionIndex.Count() << " sections, InfantryTypes=" << ini.GetKeyCount("InfantryTypes") << ", E1.Strength=" << ini.ReadInteger("E1","Strength",-1) << '\n';
            }
}

namespace {
std::optional<int> legacy_command(int argc, char** argv) {
    if (argc > 2 && std::string(argv[1]) == "--snapshot") {
                CCINIClass ini; int status = 0;
                const bool with_comments = std::string(argv[2]) == "comments";
                for (int i = 3; i < argc; ++i) {
                    std::ifstream stream(argv[i], std::ios::binary);
                    EXPECT_TRUE((bool(stream))) << "snapshot file missing";
                    const std::string text((std::istreambuf_iterator<char>(stream)), {});
                    status = load(ini, text, with_comments);
                }
                snapshot(ini, status); return 0;
            }
    return std::nullopt;
}
const ra2::test::CommandRegistration command(legacy_command);
}
