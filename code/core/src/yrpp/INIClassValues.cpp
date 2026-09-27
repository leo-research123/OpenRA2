// Copyright 2025 Electronic Arts Inc.
// SPDX-License-Identifier: GPL-3.0-or-later
// INIClass value conversion adapted from pinned CnC_Renegade WWLib ini.cpp;
// YR tuple/time/escaped-UTF16 differences calibrated at 529880/529CA0/52A760/
// 528E00/528F00.
#include "yrpp/CCINIClass.h"
#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <string>

int* INIClass::Read2Integers(int* output, const char* section, const char* key, int* fallback) {
    if (!output) return nullptr;
    int values[2] = {fallback ? fallback[0] : 0, fallback ? fallback[1] : 0};
    char text[64]{};
    if (ReadString(section, key, "", text, sizeof(text)))
        std::sscanf(text, "%d,%d", &values[0], &values[1]);
    std::copy_n(values, 2, output);
    return output;
}
Point2D* INIClass::ReadPoint2D(Point2D& output, const char* section, const char* key, Point2D& fallback) {
    int defaults[2] = {fallback.X, fallback.Y}, values[2];
    Read2Integers(values, section, key, defaults);
    output = {values[0], values[1]};
    return &output;
}
bool INIClass::Write2Integers(const char* section, const char* key, int* values) {
    if (!values) return false;
    char text[64];
    std::snprintf(text, sizeof(text), "%d,%d", values[0], values[1]);
    return WriteString(section, key, text);
}
int* INIClass::Read3Integers(int* output, const char* section, const char* key, int* fallback) {
    if (!output) return nullptr;
    int values[3] = {fallback ? fallback[0] : 0, fallback ? fallback[1] : 0, fallback ? fallback[2] : 0};
    char text[64]{};
    if (ReadString(section, key, "", text, sizeof(text)))
        std::sscanf(text, "%d,%d,%d", &values[0], &values[1], &values[2]);
    std::copy_n(values, 3, output);
    return output;
}
CoordStruct* INIClass::ReadPoint3D(CoordStruct& output, const char* section, const char* key, CoordStruct& fallback) {
    int defaults[3] = {fallback.X, fallback.Y, fallback.Z}, values[3];
    Read3Integers(values, section, key, defaults);
    output = {values[0], values[1], values[2]};
    return &output;
}
int* INIClass::Read4Integers(int* output, const char* section, const char* key, int* fallback) {
    if (!output) return nullptr;
    int values[4] = {fallback ? fallback[0] : 0, fallback ? fallback[1] : 0,
        fallback ? fallback[2] : 0, fallback ? fallback[3] : 0};
    char text[64]{};
    if (section && key && ReadString(section, key, "0,0,0,0", text, sizeof(text)))
        std::sscanf(text, "%d,%d,%d,%d", &values[0], &values[1], &values[2], &values[3]);
    std::copy_n(values, 4, output);
    return output;
}
byte* INIClass::Read3Bytes(byte* output, const char* section, const char* key, byte* fallback) {
    if (!output) return nullptr;
    int values[3] = {fallback ? fallback[0] : 0, fallback ? fallback[1] : 0, fallback ? fallback[2] : 0};
    char defaults[64], text[64]{};
    std::snprintf(defaults, sizeof(defaults), "%d,%d,%d", values[0], values[1], values[2]);
    if (ReadString(section, key, defaults, text, sizeof(text)))
        std::sscanf(text, "%d,%d,%d", &values[0], &values[1], &values[2]);
    for (int i = 0; i < 3; ++i) output[i] = static_cast<byte>(values[i]);
    return output;
}
bool INIClass::Write3Bytes(const char* section, const char* key, byte* values) {
    if (!values) return false;
    char text[64];
    std::snprintf(text, sizeof(text), "%d,%d,%d", int(values[0]), int(values[1]), int(values[2]));
    return WriteString(section, key, text);
}
ColorStruct* INIClass::ReadColor(ColorStruct* output, const char* section, const char* key, const ColorStruct& fallback) {
    if (!output) return nullptr;
    char defaults[64], text[64]{};
    std::snprintf(defaults, sizeof(defaults), "%d,%d,%d", int(fallback.R), int(fallback.G), int(fallback.B));
    if (ReadString(section, key, defaults, text, sizeof(text))) {
        // Unlike Read3Bytes, target 474C70 explicitly initializes missing fields.
        int red = 0, green = 0, blue = 0;
        std::sscanf(text, "%d,%d,%d", &red, &green, &blue);
        output->R = static_cast<byte>(red);
        output->G = static_cast<byte>(green);
        output->B = static_cast<byte>(blue);
    } else *output = fallback;
    return output;
}
bool INIClass::WriteColor(const char* section, const char* key, const ColorStruct& color) {
    byte bytes[3] = {color.R, color.G, color.B};
    return Write3Bytes(section, key, bytes);
}
int INIClass::ReadTime(const char* section, const char* key, int fallback) {
    const auto* entry = ReadEntry(section, key);
    if (!entry || !entry->Value) return fallback;
    int hours = 0, minutes = 0, seconds = 0;
    std::sscanf(entry->Value, "%02d:%02d:%02d", &hours, &minutes, &seconds);
    const std::uint32_t frames = 60u * (std::uint32_t(seconds) + 60u * (std::uint32_t(minutes) + 60u * std::uint32_t(hours)));
    return std::bit_cast<std::int32_t>(frames);
}
bool INIClass::WriteTime(const char* section, const char* key, int value) {
    char text[64];
    std::snprintf(text, sizeof(text), "%02d:%02d:%02d", value / 216000, value / 3600 % 60, value / 60 % 60);
    return WriteString(section, key, text);
}

int INIClass::ReadUnicodeString(const char* section, const char* key, const wchar_t* fallback, wchar_t* output, size_t capacity) {
    if (!output || !capacity) return 0;
    char text[20480]{};
    if (!ReadString(section, key, "", text, sizeof(text))) {
        // Preserve aliased default/output and reserve the terminator on all hosts.
        const std::wstring value(fallback ? fallback : L"");
        const auto count = std::min(value.size(), capacity - 1);
        std::copy_n(value.data(), count, output);
        output[count] = 0;
        return static_cast<int>(std::wcslen(output));
    }
    size_t count = 0;
    char* token = text;
    while (*token && count + 1 < capacity) {
        token += std::strspn(token, ",");
        if (!*token) break;
        auto* end = token + std::strcspn(token, ",");
        const bool more = *end != 0;
        *end = 0;
        unsigned value = 0;
        std::sscanf(token, "%x", &value);
        output[count++] = static_cast<wchar_t>(value & 0xffffu);
        if (!more) break;
        token = end + 1;
    }
    output[count] = 0;
    return static_cast<int>(std::wcslen(output));
}
bool INIClass::WriteUnicodeString(const char* section, const char* key, const wchar_t* value) {
    if (!value) return false;
    try {
        std::string encoded;
        const auto append = [&](std::uint32_t unit) {
            char text[8];
            std::snprintf(text, sizeof(text), "%x,", unsigned(unit));
            encoded += text;
        };
        for (; *value; ++value) {
            auto unit = static_cast<std::uint32_t>(*value);
            // wchar_t is UTF-16 on the target; wider hosts also accept Unicode scalars.
            if constexpr (sizeof(wchar_t) > 2) {
                if (unit > 0xffff && unit <= 0x10ffff) {
                    unit -= 0x10000;
                    append(0xd800 + (unit >> 10));
                    append(0xdc00 + (unit & 0x3ff));
                    continue;
                }
            }
            append(unit & 0xffffu);
        }
        return WriteString(section, key, encoded.c_str());
    } catch (const std::bad_alloc&) { return false; }
}
