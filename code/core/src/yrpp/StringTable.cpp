/*
    XCC Utilities and Library
    Copyright (C) 2000  Olaf van der Spek  <olafvdspek@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

// Copyright (C) 2000 Olaf van der Spek <olafvdspek@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
// CSF record decoding adapted from XCC 70358b46858973426c1ecf204485cb2a88716217,
// misc/csf_file.cpp. YR's StringTable arrays, multi-value labels, whitespace,
// lookup and lifetime are calibrated to 734640..734F0C; see map resource records.
#include "yrpp/StringTable.h"
#include "yrpp/CCFileClass.h"
#include <algorithm>
#include <cstdlib>
#include <limits>
#include "yrpp/Memory.h"
#include <new>
#include <string>
#include <vector>

namespace {
CSFString* missing_strings = nullptr;
int max_label_len = 0, label_count = 0, value_count = 0, is_loaded = 0;
CSFLanguages language = CSFLanguages::US;
char* file_name = nullptr;
CSFLabel* labels = nullptr;
wchar_t** values = nullptr;
char** extra_values = nullptr;
constexpr CSFLanguage languages[] = {
    {CSFLanguages::US, "US", "us", "e"}, {CSFLanguages::UK, "UK", "uk", "e"},
    {CSFLanguages::German, "German", "ge", "g"}, {CSFLanguages::French, "French", "fr", "f"},
    {CSFLanguages::Spanish, "Spanish", "sp", "s"}, {CSFLanguages::Italian, "Italian", "it", "i"},
    {CSFLanguages::Japanese, "Japanese", "ja", "j"}, {CSFLanguages::Jabberwockie, "Jabberwockie", "jb", "e"},
    {CSFLanguages::Korean, "Korean", "ko", "k"}, {CSFLanguages::Chinese, "Chinese", "ch", "c"}
};
int compare(const char* lhs, const char* rhs) {
    for (;;) {
        unsigned char a = static_cast<unsigned char>(*lhs++), b = static_cast<unsigned char>(*rhs++);
        if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
        if (b >= 'A' && b <= 'Z') b += 'a' - 'A';
        if (a != b || !a) return int(a) - int(b);
    }
}
int YRPP_CDECL compare_labels(const void* lhs, const void* rhs) {
    return compare(static_cast<const CSFLabel*>(lhs)->Name, static_cast<const CSFLabel*>(rhs)->Name);
}
template<class T> T* allocate_zeroed(std::size_t count) {
    if (count > std::numeric_limits<std::size_t>::max() / sizeof(T)) return nullptr;
    auto* result = static_cast<T*>(YRMemory::Allocate(count * sizeof(T)));
    if (result) std::memset(result, 0, count * sizeof(T));
    return result;
}
template<class T> void free_values(T** table, int count) {
    if (table) for (int i = 0; i < count; ++i) YRMemory::Deallocate(table[i]);
    YRMemory::Deallocate(table);
}
// Owns the same original arrays temporarily, so rejected input cannot publish
// partial entries or poison IsLoaded. It is not another string-table model.
struct PendingTable {
    CSFHeader header{};
    CSFLabel* labels = nullptr;
    wchar_t** values = nullptr;
    char** extras = nullptr;
    int max_label = 0;
    ~PendingTable() {
        YRMemory::Deallocate(labels);
        free_values(values, header.NumValues);
        free_values(extras, header.NumValues);
    }
};
class Reader {
public:
    explicit Reader(const char* name) : file(name) {}
    bool open() {
        if (!file.Exists() || !file.Open(FileAccessMode::Read)) return false;
        remaining = file.GetFileSize();
        return remaining >= 24;
    }
    bool read(void* out, int size) {
        if (size < 0 || size > remaining) return false;
        if (size && file.ReadBytes(out, size) != size) return false;
        remaining -= size;
        return true;
    }
    bool integer(std::uint32_t& out) {
        unsigned char bytes[4];
        if (!read(bytes, 4)) return false;
        out = unsigned(bytes[0]) | (unsigned(bytes[1]) << 8) |
            (unsigned(bytes[2]) << 16) | (unsigned(bytes[3]) << 24);
        return true;
    }
    CCFileClass file;
    int remaining = 0;
};
bool parse(const char* name, PendingTable& table) {
    if (!name || !*name) return false;
    Reader in(name);
    if (!in.open()) return false;
    std::uint32_t h[6];
    for (auto& v : h) if (!in.integer(v)) return false;
    // Every label takes at least 12 bytes; every value takes at least 8.
    if (h[0] != CSF_SIGNATURE || !h[2] || !h[3] ||
        h[2] > unsigned(in.remaining / 12) || h[3] > unsigned(in.remaining / 8)) return false;
    table.header = {h[0], int(h[1]), int(h[2]), int(h[3]), h[4],
        h[1] < 2 ? CSFLanguages::US : static_cast<CSFLanguages>(h[5])};
    table.labels = allocate_zeroed<CSFLabel>(h[2]);
    table.values = allocate_zeroed<wchar_t*>(h[3]);
    table.extras = allocate_zeroed<char*>(h[3]);
    if (!table.labels || !table.values || !table.extras) return false;
    int value_index = 0;
    for (int i = 0; i < table.header.NumLabels; ++i) {
        std::uint32_t tag, count, length;
        if (!in.integer(tag) || tag != CSF_LABEL_SIGNATURE || !in.integer(count) ||
            !in.integer(length) || length >= sizeof(CSFLabel::Name) ||
            count > unsigned(table.header.NumValues - value_index)) return false;
        auto& label = table.labels[i];
        if (!in.read(label.Name, int(length))) return false;
        label.NumValues = int(count); label.FirstValueIndex = value_index;
        table.max_label = std::max(table.max_label, int(length));
        for (unsigned j = 0; j < count; ++j, ++value_index) {
            if (!in.integer(tag) || (tag != CSF_VALUE_SIGNATURE && tag != CSF_EXVALUE_SIGNATURE) ||
                !in.integer(length) || length > unsigned(in.remaining / 2)) return false;
            std::wstring text;
            text.reserve(length);
            bool terminated = false, start = true;
            wchar_t previous = 0;
            for (unsigned k = 0; k < length; ++k) {
                unsigned char bytes[2];
                if (!in.read(bytes, 2)) return false;
                const auto encoded = std::uint16_t(unsigned(bytes[0]) | (unsigned(bytes[1]) << 8));
                // Target inverts UTF-16 units up to an encoded zero, then uses
                // the decoded NUL terminator. Keep code units on wider hosts.
                if (!encoded || encoded == 0xffff) terminated = true;
                if (terminated) continue;
                const wchar_t c = static_cast<wchar_t>(std::uint16_t(~encoded));
                if (c == L' ' && (previous == L' ' || start)) continue;
                if (c == L'\n' || c == L'\t') {
                    if (previous == L' ') text.pop_back();
                    start = true;
                } else start = false;
                text.push_back(c); previous = c;
            }
            if (previous == L' ') text.pop_back();
            auto*& value = table.values[value_index];
            value = allocate_zeroed<wchar_t>(text.size() + 1);
            if (!value) return false;
            std::memcpy(value, text.c_str(), (text.size() + 1) * sizeof(wchar_t));
            if (tag == CSF_EXVALUE_SIGNATURE) {
                if (!in.integer(length) || length > unsigned(in.remaining)) return false;
                if (length) {
                    auto*& extra = table.extras[value_index];
                    extra = allocate_zeroed<char>(std::size_t(length) + 1);
                    if (!extra || !in.read(extra, int(length))) return false;
                }
            }
        }
    }
    return value_index == table.header.NumValues && in.remaining == 0;
}
}

CSFString*& StringTable::LastLoadedString = missing_strings;
int& StringTable::MaxLabelLen = max_label_len;
int& StringTable::LabelCount = label_count;
int& StringTable::ValueCount = value_count;
CSFLanguages& StringTable::Language = language;
int& StringTable::IsLoaded = is_loaded;
char*& StringTable::FileName = file_name;
CSFLabel*& StringTable::Labels = labels;
wchar_t**& StringTable::Values = values;
char**& StringTable::ExtraValues = extra_values;

bool StringTable::IsNone(const char* label) { return !compare(label, "none") || !compare(label, "<none>"); }
const CSFLanguage* YRPP_FASTCALL StringTable::GetLanguage(CSFLanguages id) {
    for (const auto& item : languages) if (item.Index == id) return &item;
    return nullptr;
}
const char* YRPP_FASTCALL StringTable::GetLanguageName(CSFLanguages id) {
    const auto* item = GetLanguage(id);
    return item ? item->Name : "unknown";
}
const wchar_t* YRPP_FASTCALL StringTable::LoadString(const char* label, char** extra, const char*, int) {
    if (extra) *extra = nullptr;
    if (!Labels) return L"***FATAL*** String Manager failed to initilaized properly";
    if (!label) label = "";
    int first = 0, last = LabelCount;
    while (first < last) {
        const int middle = first + (last - first) / 2;
        const auto& item = Labels[middle];
        const int order = compare(label, item.Name);
        if (order < 0) last = middle;
        else if (order > 0) first = middle + 1;
        else {
            if (!item.NumValues || item.FirstValueIndex < 0 || item.FirstValueIndex >= ValueCount) break;
            if (extra) *extra = ExtraValues[item.FirstValueIndex];
            return Values[item.FirstValueIndex];
        }
    }
    auto* missing = static_cast<CSFString*>(YRMemory::Allocate(sizeof(CSFString)));
    if (!missing) return L"MISSING:";
    new (missing) CSFString;
    std::wcscpy(missing->Text, L"MISSING:'");
    std::size_t n = 9;
    // Original swprintf has no bound. Preserve its format without overflowing
    // the 258-unit node for an invalid, overlong label.
    for (; *label && n < std::size(missing->Text) - 2; ++label) missing->Text[n++] = static_cast<unsigned char>(*label);
    missing->Text[n++] = L'\''; missing->Text[n] = 0;
    missing->PreviousEntry = LastLoadedString; LastLoadedString = missing;
    return missing->Text;
}
bool YRPP_FASTCALL StringTable::LoadFile(const char* name) {
    if (IsLoaded) return true;
    if (!name || !*name) return false;
    try {
        std::string normalized(name);
        const auto dot = normalized.find('.');
        if (dot != std::string::npos) normalized.resize(dot);
        normalized += ".csf";
        PendingTable table;
        if (!parse(normalized.c_str(), table)) return false;
        std::qsort(table.labels, std::size_t(table.header.NumLabels), sizeof(CSFLabel), compare_labels);
        Unload();
        Labels = table.labels; table.labels = nullptr;
        Values = table.values; table.values = nullptr;
        ExtraValues = table.extras; table.extras = nullptr;
        LabelCount = table.header.NumLabels; ValueCount = table.header.NumValues;
        MaxLabelLen = table.max_label; Language = table.header.Language;
        FileName = const_cast<char*>(name); IsLoaded = 1;
        // 73492F/734944 notify Session/MapSeed UI consumers. Those runtime
        // callbacks are outside this resource module and are not installed.
        return true;
    } catch (const std::bad_alloc&) { return false; }
}
bool YRPP_FASTCALL StringTable::ReadFile(const char* name) {
    if (!Labels || !Values || !ExtraValues) return false;
    try {
        PendingTable table;
        if (!parse(name, table) || table.header.NumLabels != LabelCount || table.header.NumValues != ValueCount) return false;
        std::memcpy(Labels, table.labels, std::size_t(LabelCount) * sizeof(CSFLabel));
        for (int i = 0; i < ValueCount; ++i) {
            YRMemory::Deallocate(Values[i]); YRMemory::Deallocate(ExtraValues[i]);
            Values[i] = table.values[i]; table.values[i] = nullptr;
            ExtraValues[i] = table.extras[i]; table.extras[i] = nullptr;
        }
        MaxLabelLen = std::max(MaxLabelLen, table.max_label);
        // Like the original, ReadFile does not sort or change file/language.
        return true;
    } catch (const std::bad_alloc&) { return false; }
}
void StringTable::Unload() {
    free_values(Values, ValueCount); Values = nullptr;
    free_values(ExtraValues, ValueCount); ExtraValues = nullptr;
    YRMemory::Deallocate(Labels); Labels = nullptr;
    while (LastLoadedString) {
        auto* item = LastLoadedString; LastLoadedString = item->PreviousEntry;
        item->~CSFString(); YRMemory::Deallocate(item);
    }
    IsLoaded = 0; // Target retains counts, language, filename and MaxLabelLen.
}
