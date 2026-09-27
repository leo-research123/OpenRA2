// Copyright 2025 Electronic Arts Inc.
// SPDX-License-Identifier: GPL-3.0-or-later
// Adapted from CnC_Renegade 3e00c3a1b97381bb28be89a35b856375e0629a08,
// Code/wwlib/ini.cpp, inisup.h, readline.cpp and trim.cpp. Unmodified source and
// notices: third_party/ea/wwlib.
#include <cfenv>
#include "yrpp/CCINIClass.h"
#include "yrpp/CRC.h"
#include "yrpp/Straws.h"
#include "yrpp/Pipes.h"
#include <algorithm>
#include <bit>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <string>

namespace {
using Comment = INIClass::INIComment;
using Entry = INIClass::INIEntry;
using Section = INIClass::INISection;
char* copy_string(const char* text) {
    if (!text) return nullptr;
    const auto count = std::strlen(text) + 1;
    auto* result = static_cast<char*>(std::malloc(count));
    if (!result) throw std::bad_alloc();
    std::memcpy(result, text, count);
    return result;
}
void free_comments(Comment* head) noexcept {
    while (head) { auto* next = head->Next; std::free(head->Value); delete head; head = next; }
}
struct CommentsDeleter { void operator()(Comment* head) const { free_comments(head); } };
using CommentsOwner = std::unique_ptr<Comment, CommentsDeleter>;
void append_comment(CommentsOwner& head, Comment*& tail, const char* text) {
    auto node = std::make_unique<Comment>();
    node->Value = copy_string(text);
    if (tail) tail->Next = node.get();
    else head.reset(node.get());
    tail = node.release();
}
int name_id(const char* text) {
    CRCEngine crc;
    return crc(text, static_cast<int>(std::strlen(text)));
}
bool whitespace(char c) { return c && static_cast<unsigned char>(c) <= 0x20; }
void trim(char* text) {
    auto* first = text;
    while (whitespace(*first)) ++first;
    auto length = std::strlen(first);
    while (length && whitespace(first[length - 1])) --length;
    std::memmove(text, first, length);
    text[length] = 0;
}
// 65D5C0 retains indentation, unlike the EA Read_Line final strtrim call.
int read_line(Straw& input, char* line, int capacity, bool& eof) {
    int used = 0;
    for (;;) {
        char c;
        if (input.Get(&c, 1) != 1) { eof = true; break; }
        if (c == '\n') break;
        if (c != '\r' && used + 1 < capacity) line[used++] = c;
    }
    line[used] = 0;
    return static_cast<int>(std::strlen(line));
}
bool section_line(const char* line) {
    while (whitespace(*line)) ++line;
    return *line == '[' && std::strchr(line, ']');
}
void section_name(char* line) {
    trim(line);
    if (line[0] == '[') {
        if (auto* end = std::strchr(line, ']')) *end = 0;
        std::memmove(line, line + 1, std::strlen(line));
    } else *line = 0;
}
const char* comment_columns(const char* line, int& before, int& after, int& comment) {
    before = after = comment = -1;
    int column = 0;
    const char* result = nullptr;
    for (const auto* p = line; *p; ++p) {
        if (before >= 0 && after < 0 && static_cast<unsigned char>(*p) > 0x20) after = column;
        if (*p == ';') { comment = column; result = p + 1; break; }
        if (*p == '\t') column = (column & ~7) + 8;
        else { if (*p == '=' && before < 0) before = column; ++column; }
    }
    before = std::max(0, before); after = std::max(0, after); comment = std::max(0, comment);
    return result;
}
char* split_entry(char* line) {
    if (auto* comment = std::strchr(line, ';')) { *comment = 0; trim(line); }
    if (!*line || *line == ';' || *line == '=') return nullptr;
    auto* divider = std::strchr(line, '=');
    if (!divider) return nullptr;
    *divider++ = 0;
    trim(line); trim(divider);
    return *line && *divider ? divider : nullptr;
}
std::unique_ptr<Section> make_section(const char* name) {
    auto section = std::make_unique<Section>(); section->Name = copy_string(name); return section;
}
std::unique_ptr<Entry> make_entry(const char* key, const char* value) {
    auto entry = std::make_unique<Entry>();
    entry->Key = copy_string(key); entry->Value = copy_string(value); return entry;
}
void append_entry(Section& section, std::unique_ptr<Entry> entry) {
    if (!section.EntryIndex.AddIndex(name_id(entry->Key), entry.get())) throw std::bad_alloc();
    section.Entries.AddTail(entry.release());
}
void append_section(INIClass& ini, std::unique_ptr<Section> section) {
    if (!ini.SectionIndex.AddIndex(name_id(section->Name), section.get())) throw std::bad_alloc();
    ini.Sections.AddTail(section.release());
}
}

INIClass::INIClass() = default;
INIClass::~INIClass() { Clear(); }
INIClass::INIEntry::~INIEntry() {
    std::free(Key); std::free(Value); std::free(CommentString); free_comments(Comments);
}
INIClass::INISection::~INISection() {
    std::free(Name);
    while (auto* entry = Entries.front()) delete entry;
    free_comments(Comments);
}
void INIClass::Reset() { CurrentSectionName = nullptr; CurrentSection = nullptr; }
bool INIClass::Clear(const char* section, const char* key) {
    Reset();
    if (!section) {
        while (auto* node = Sections.front()) delete node;
        SectionIndex.Clear(); free_comments(LineComments); LineComments = nullptr;
        return true;
    }
    auto* node = GetSection(section);
    if (!node) return true;
    if (!key) {
        SectionIndex.RemoveIndex(name_id(node->Name)); delete node;
    } else {
        Entry* entry = nullptr;
        if (node->EntryIndex.TryGet(name_id(key), entry) && entry) {
            node->EntryIndex.RemoveIndex(name_id(entry->Key)); delete entry;
        }
    }
    return true;
}
INIClass::INISection* INIClass::GetSection(const char* section) {
    Section* result = nullptr;
    if (section) SectionIndex.TryGet(name_id(section), result);
    return result;
}
INIClass::INIEntry* INIClass::FindEntry(const char* section, const char* key) {
    if (CurrentSectionName != section) {
        CurrentSection = GetSection(section);
        CurrentSectionName = CurrentSection ? const_cast<char*>(section) : nullptr;
    }
    Entry* result = nullptr;
    if (CurrentSection && key) CurrentSection->EntryIndex.TryGet(name_id(key), result);
    return result;
}
INIClass::INIEntry* INIClass::ReadEntry(const char* section, const char* key) {
    if (!section || !key) return nullptr;
    return FindEntry(section, key);
}
bool INIClass::Exists(const char* section, const char* key) {
    return key ? FindEntry(section, key) != nullptr : GetSection(section) != nullptr;
}
int INIClass::GetKeyCount(const char* section) {
    const auto* node = GetSection(section);
    return node ? node->EntryIndex.Count() : 0;
}
const char* INIClass::GetKeyName(const char* section, int index) {
    auto* node = GetSection(section);
    if (!node || index < 0 || index >= node->EntryIndex.Count()) return nullptr;
    for (auto* entry : node->Entries) if (index-- == 0) return entry->Key;
    return nullptr;
}
int INIClass::ReadString(const char* section, const char* key, const char* fallback,
    char* buffer, size_t size) {
    if (!buffer || size < 2 || !section || !key) return 0; // leaves output unchanged
    const auto* entry = ReadEntry(section, key);
    const char* source = entry && entry->Value ? entry->Value : fallback;
    if (!source) { *buffer = 0; return 0; }
    // YRpp GetString aliases default and destination. Copy before trimming.
    const size_t count = std::min(std::strlen(source), size - 1);
    std::memmove(buffer, source, count);
    // 528BD0 uses strncpy(..., size), including zero padding. Retain the
    // overlap-safe copy for GetString's aliased default/destination, then
    // pad before trimming so Scenario's serialized trailing bytes match.
    std::fill(buffer + count, buffer + size, char{});
    trim(buffer);
    return static_cast<int>(std::strlen(buffer));
}
bool INIClass::WriteString(const char* section, const char* key, const char* value) {
    if (!section || !key) return false;
    try {
        // Protect caller aliases to a replaced entry's key/value.
        const std::string saved_key(key), saved_value(value ? value : "");
        auto* node = GetSection(section);
        if (!node) {
            auto created = make_section(section); node = created.get(); append_section(*this, std::move(created));
        }
        Entry* previous = nullptr;
        node->EntryIndex.TryGet(name_id(saved_key.c_str()), previous);
        std::unique_ptr<Entry> old(previous);
        if (previous) {
            node->EntryIndex.RemoveIndex(name_id(previous->Key)); previous->Unlink();
        }
        // Original deletes an existing key for null/empty input, retaining its section.
        // Host also frees detached comments here (the target leaks that case).
        if (saved_value.empty()) return true;
        auto entry = make_entry(saved_key.c_str(), saved_value.c_str());
        if (old) {
            entry->Comments = std::exchange(old->Comments, nullptr);
            entry->CommentString = std::exchange(old->CommentString, nullptr);
            entry->PreIndentCursor = old->PreIndentCursor;
            entry->PostIndentCursor = old->PostIndentCursor;
            entry->CommentCursor = old->CommentCursor;
        }
        append_entry(*node, std::move(entry)); // updates move the key to the tail
        return true;
    } catch (const std::bad_alloc&) { return false; }
}
int INIClass::ReadStraw(Straw& source, bool loadComments) {
    Reset();
    const bool merge = SectionIndex.Count() > 0;
    if (merge) loadComments = false;
    CommentsOwner pending;
    Comment* tail = nullptr;
    try {
        CacheStraw input;
        input.Get_From(source);
        char line[512]; bool eof = false;
        for (;;) {
            read_line(input, line, sizeof(line), eof);
            if (eof) {
                if (loadComments) { free_comments(LineComments); LineComments = pending.release(); return 1; }
                return 0;
            }
            if (section_line(line)) break;
            if (loadComments) append_comment(pending, tail, line);
        }
        if (!merge) { free_comments(LineComments); LineComments = nullptr; }
        while (!eof) {
            section_name(line);
            if (merge) {
                const std::string section(line);
                while (!eof) {
                    read_line(input, line, sizeof(line), eof);
                    if (section_line(line)) break;
                    if (auto* value = split_entry(line)) {
                        if (!WriteString(section.c_str(), line, value)) return 0;
                    }
                }
            } else {
                auto section = make_section(line);
                section->Comments = pending.release(); tail = nullptr;
                while (!eof) {
                    read_line(input, line, sizeof(line), eof);
                    // Original first-load branch drops a last line without LF.
                    if (eof || section_line(line)) break;
                    if (loadComments) append_comment(pending, tail, line);
                    int before = 0, after = 0, column = 0;
                    std::unique_ptr<char, decltype(&std::free)> inline_comment(nullptr, &std::free);
                    if (loadComments) inline_comment.reset(copy_string(comment_columns(line, before, after, column)));
                    if (auto* value = split_entry(line)) {
                        auto entry = make_entry(line, value);
                        if (loadComments && tail) { std::free(tail->Value); tail->Value = nullptr; }
                        entry->Comments = pending.release(); tail = nullptr;
                        entry->CommentString = inline_comment.release();
                        entry->PreIndentCursor = before; entry->PostIndentCursor = after; entry->CommentCursor = column;
                        // YR appends every entry, including duplicate names/CRCs.
                        append_entry(*section, std::move(entry));
                    }
                }
                if (loadComments || !section->Entries.empty()) append_section(*this, std::move(section));
                else { pending.reset(); tail = nullptr; }
            }
        }
        free_comments(LineComments); LineComments = pending.release();
        return 1;
    } catch (const std::bad_alloc&) { Clear(); return 0; }
}
int INIClass::ReadInteger(const char* section, const char* key, int fallback) {
    const auto* entry = ReadEntry(section, key);
    if (!entry || !entry->Value || !*entry->Value) return fallback;
    const char* text = entry->Value;
    const size_t size = std::strlen(text);
    if (*text == '$' || text[size - 1] == 'h' || text[size - 1] == 'H') {
        unsigned value = std::bit_cast<unsigned>(fallback);
        std::sscanf(text, *text == '$' ? "$%x" : "%xh", &value);
        return std::bit_cast<int>(value);
    }
    // 32-bit accumulation matches the target atoi without host signed overflow.
    while (whitespace(*text)) ++text;
    const bool negative = *text == '-';
    if (*text == '-' || *text == '+') ++text;
    uint32_t value = 0;
    while (*text >= '0' && *text <= '9') value = value * 10 + uint32_t(*text++ - '0');
    return std::bit_cast<int32_t>(negative ? 0u - value : value);
}
bool INIClass::ReadBool(const char* section, const char* key, bool fallback) {
    const auto* entry = ReadEntry(section, key);
    if (!entry || !entry->Value) return fallback;
    switch (*entry->Value) {
        case '0': case 'f': case 'F': case 'n': case 'N': return false;
        case '1': case 't': case 'T': case 'y': case 'Y': return true;
        default: return fallback;
    }
}
double INIClass::ReadDouble(const char* section, const char* key, double fallback) {
    const auto* entry = ReadEntry(section, key);
    if (!entry || !entry->Value) return fallback;
    float value;
    // Target scans float, then widens. Failed scanf in the original reinterprets
    // an address-dependent stack slot; host returns the explicit default instead.
    // The bundled x86 CRT at 0x7CA530 rounds decimal-to-float to nearest
    // independently of _controlfp's rounding bits in all four modes.
    // Host scanf honors FE_TOWARDZERO left by _ftol-style rules readers;
    // without this scope .2 becomes the lower float and scales to 199.
    const int rounding=std::fegetround();
    if(rounding!=-1)std::fesetround(FE_TONEAREST);
    const int scanned=std::sscanf(entry->Value, "%f", &value);
    if(rounding!=-1)std::fesetround(rounding);
    if(scanned!=1)return fallback;
    return std::strchr(entry->Value, '%') ? double(value) * 0.01 : double(value);
}
bool INIClass::WriteInteger(const char* section, const char* key, int value, int format) {
    char text[64];
    if (format == 1 || format == 2) std::snprintf(text, sizeof(text), format == 1 ? "%Xh" : "$%X", std::bit_cast<unsigned>(value));
    else std::snprintf(text, sizeof(text), "%d", value);
    return WriteString(section, key, text);
}
bool INIClass::WriteBool(const char* section, const char* key, bool value) {
    return WriteString(section, key, value ? "yes" : "no");
}
bool INIClass::WriteDouble(const char* section, const char* key, double value) {
    char text[512]; std::snprintf(text, sizeof(text), "%f", value);
    return WriteString(section, key, text);
}
bool INIClass::IsBlankValue(const char* value) {
    if (!value) return false;
    std::string text(value);
    for (auto& c : text) if (c >= 'A' && c <= 'Z') c = char(c + 'a' - 'A');
    return text == "none" || text == "<none>";
}
int INIClass::WritePipe(Pipe& output) {
    Reset();
    int total = 0;
    const auto put = [&](const char* text) { total += output.Put(text, static_cast<int>(std::strlen(text))); };
    const auto comments = [&](Comment* node) {
        for (; node; node = node->Next) if (node->Value) { put(node->Value); put("\r\n"); }
    };
    const auto spaces = [&](int count) {
        const std::string padding(static_cast<size_t>(std::clamp(count, 0, 256)), ' '); put(padding.c_str());
    };
    for (auto* section : Sections) {
        if (total > 0 && !section->Comments) put("\r\n");
        comments(section->Comments); put("["); put(section->Name); put("]\r\n");
        for (auto* entry : section->Entries) {
            comments(entry->Comments);
            const int key_size = static_cast<int>(std::strlen(entry->Key));
            const int value_size = static_cast<int>(std::strlen(entry->Value));
            put(entry->Key);
            int padding = std::clamp(entry->PreIndentCursor - key_size, 0, 256);
            spaces(padding); put("=");
            const int after = std::clamp(entry->PostIndentCursor - padding - key_size - 1, 0, 256);
            spaces(after); padding += after;
            put(entry->Value);
            if (entry->CommentString) {
                spaces(entry->CommentCursor - padding - value_size - key_size - 1);
                put(";"); put(entry->CommentString);
            }
            put("\r\n");
        }
    }
    comments(LineComments);
    return total + output.End();
}

// Non-template interface helpers; bodies retained from the corresponding header.

INIClass::INIClass(bool) : INIClass()
{ }

int INIClass::GetString(const char* pSection, const char* pKey, char* pBuffer,size_t szBufferSize)
{ return ReadString(pSection, pKey, pBuffer, pBuffer, szBufferSize); }

void INIClass::GetBool(const char* pSection, const char* pKey, bool& bValue)
{ bValue = ReadBool(pSection, pKey, bValue); }

void INIClass::GetInteger(const char* pSection, const char* pKey, int& nValue)
{ nValue = ReadInteger(pSection, pKey, nValue); }

void INIClass::GetDouble(const char* pSection, const char* pKey, double& nValue)
{ nValue = ReadDouble(pSection, pKey, nValue); }

int INIClass::ReadRate(const char* pSection, const char* pKey, int nDefault)
{
    double buffer = nDefault / 900.0;
    GetDouble(pSection, pKey, buffer);
    return static_cast<int>(buffer * 900.0);
}

void INIClass::GetRate(const char* pSection, const char* pKey, int& nValue)
{ nValue = ReadRate(pSection, pKey, nValue); }

bool INIClass::WriteRate(const char* pSection, const char* pKey, int nValue)
{
    double dValue = nValue / 900.0;
    return WriteDouble(pSection, pKey, dValue);
}

void INIClass::GetPoint2D(const char* pSection, const char* pKey, Point2D& value)
{ ReadPoint2D(value, pSection, pKey, value); }

void INIClass::GetPoint3D(const char* pSection, const char* pKey, CoordStruct& value)
{ ReadPoint3D(value, pSection, pKey, value); }

ColorStruct INIClass::ReadColor(const char* const pSection, const char* const pKey, ColorStruct const& defValue)
{
    ColorStruct outBuffer;
    this->ReadColor(&outBuffer, pSection, pKey, defValue);
    return outBuffer;
}

void INIClass::GetColor(const char* const pSection, const char* const pKey, ColorStruct& value)
{
    ReadColor(&value, pSection, pKey, value);
}

bool INIClass::IsBlank(const char *pValue)
{
    return IsBlankValue(pValue);
}
