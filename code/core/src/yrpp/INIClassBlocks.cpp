// Copyright 2025 Electronic Arts Inc.
// SPDX-License-Identifier: GPL-3.0-or-later
// Adapted from CnC_Renegade 3e00c3a1 WWLib ini.cpp, base64.cpp and b64pipe.cpp.
// YR 526E80/526FB0: 70-character lines, insertion-order enumeration and 128-byte
// ReadString buffers. Decoder packet boundaries are raw groups of four bytes.
#include "yrpp/CCINIClass.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>

namespace {
constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
int decode_character(unsigned char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}
size_t decode_packet(const char* text, size_t length, byte* output, size_t capacity) {
    if (!length || !capacity) return 0;
    std::uint32_t packet = 0;
    unsigned count = 0;
    for (size_t i = 0; i < length; ++i) {
        const auto c = static_cast<unsigned char>(text[i]);
        if (c == '=') break;
        const int code = decode_character(c);
        if (code < 0) continue;
        packet |= std::uint32_t(code) << (18 - count * 6);
        ++count;
    }
    // WWLib emits the first byte even for an incomplete/invalid packet.
    const auto size = std::min(capacity, size_t(count > 2 ? count - 1 : 1));
    for (size_t i = 0; i < size; ++i) output[i] = byte(packet >> (16 - 8 * i));
    return size;
}
}

bool INIClass::WriteUUBlock(const char* section, void* data, size_t length) {
    if (!section || !data || !length || length > size_t(std::numeric_limits<int>::max())) return false;
    // Snapshot the name because Clear can destroy a borrowed section->Name.
    try {
        const std::string name(section);
        Clear(name.c_str());
        const auto* bytes = static_cast<const byte*>(data);
        char line[71];
        size_t used = 0;
        int index = 1;
        const auto flush = [&]() {
            line[used] = 0;
            char key[32];
            std::snprintf(key, sizeof(key), "%d", index++);
            const bool success = WriteString(name.c_str(), key, line);
            used = 0;
            return success;
        };
        for (size_t i = 0; i < length; i += 3) {
            const auto count = std::min(size_t(3), length - i);
            const std::uint32_t packet = (std::uint32_t(bytes[i]) << 16)
                | (count > 1 ? std::uint32_t(bytes[i + 1]) << 8 : 0)
                | (count > 2 ? std::uint32_t(bytes[i + 2]) : 0);
            const char encoded[4] = {alphabet[(packet >> 18) & 63], alphabet[(packet >> 12) & 63],
                count > 1 ? alphabet[(packet >> 6) & 63] : '=', count > 2 ? alphabet[packet & 63] : '='};
            for (char c : encoded) {
                line[used++] = c;
                if (used == 70 && !flush()) return false;
            }
        }
        return !used || flush();
    } catch (const std::bad_alloc&) { return false; }
}
size_t INIClass::ReadUUBlock(const char* section, void* data, size_t capacity) {
    if (!section || !data || !capacity) return 0;
    auto* output = static_cast<byte*>(data);
    char packet[4];
    size_t pending = 0, total = 0;
    const int count = GetKeyCount(section);
    for (int i = 0; i < count; ++i) {
        char text[128]{};
        const int length = ReadString(section, GetKeyName(section, i), "=", text, sizeof(text));
        for (int j = 0; j < length; ++j) {
            packet[pending++] = text[j];
            if (pending == 4) {
                total += decode_packet(packet, pending, output + total, capacity - total);
                pending = 0;
            }
        }
    }
    return total + decode_packet(packet, pending, output + total, capacity - total);
}
