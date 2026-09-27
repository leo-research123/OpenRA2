/*
 * Powerup and terrain readers adapt EA REDALERT/RULES.CPP, revision
 * f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae.
 * Copyright 2020 Electronic Arts Inc. GPL-3.0-or-later with the additional
 * terms in third_party/ea/LICENSE.TXT. YR table shapes, fourth powerup token,
 * movement limits and read order calibrated at 673E80 / 674000. ColorAdd and
 * Movies are calibrated at 66D480 / 674550.
 */
#include "yrpp/RulesClass.h"
#include "yrpp/CCINIClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/Powerups.h"
#include "yrpp/AnimTypeClass.h"
#include "yrpp/Unsorted.h"
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>

namespace {
char* next_token(char*& cursor) {
    // strtok's comma-only semantics: consecutive delimiters do not produce
    // empty tokens. The cursor is local to this operation, not process-global.
    while (*cursor == ',') ++cursor;
    if (!*cursor) return nullptr;
    char* token = cursor;
    while (*cursor && *cursor != ',') ++cursor;
    if (*cursor) *cursor++ = '\0';
    return token;
}

void trim_token(char* text) {
    char* first = text;
    while (*first && static_cast<unsigned char>(*first) <= 0x20) ++first;
    if (first != text) std::memmove(text, first, std::strlen(first) + 1);
    // 727CF0 retains first after the shift and tests it while trimming the end.
    // Keep that detail; blindly normalizing all whitespace changes lookups.
    for (size_t length = std::strlen(text); length && *first; --length) {
        if (static_cast<unsigned char>(text[length - 1]) > 0x20) break;
        text[length - 1] = '\0';
    }
}

int weight_from_text(const char* text) {
    while (*text && static_cast<unsigned char>(*text) <= 0x20) ++text;
    const bool negative = *text == '-';
    if (*text == '+' || *text == '-') ++text;
    std::uint32_t value = 0;
    while (*text >= '0' && *text <= '9') value = value * 10u + static_cast<unsigned>(*text++ - '0');
    return std::bit_cast<std::int32_t>(negative ? 0u - value : value);
}

float movement_cost(CCINIClass* ini, const char* section, const char* key) {
    // YR caps the upper bound only, and performs a second read below that bound.
    return ini->ReadDouble(section, key, 1.0) >= 1.0
        ? 1.0f : static_cast<float>(ini->ReadDouble(section, key, 1.0));
}
}

bool RulesClass::Read_ColorAdd(CCINIClass* ini) {
    if (!ini || !ini->GetSection("ColorAdd")) return false;
    const int count = ini->GetKeyCount("ColorAdd");
    // The EXE writes past ColorAdd for oversized sections. Reject before any
    // mutation instead of corrupting the following Rules fields.
    if (count > 16) return false;
    for (int i = 0; i < count; ++i) {
        byte fallback[3] = {}, color[3];
        ini->Read3Bytes(color, "ColorAdd", ini->GetKeyName("ColorAdd", i), fallback);
        ColorAdd[i] = ColorStruct(color[0], color[1], color[2]);
    }
    return true;
}

bool YRPP_STDCALL RulesClass::Read_LandCharacteristics(CCINIClass* ini) {
    if (!ini) return false;
    constexpr const char* sections[] = {"Clear", "Road", "Water", "Rock", "Wall", "Tiberium",
        "Beach", "Rough", "Ice", "Railroad", "Tunnel", "Weeds"};
    for (int i = 0; i < 12; ++i) {
        const char* section = sections[i];
        if (!ini->GetSection(section)) continue;
        auto& ground = GroundType::Array[i];
        ground.Cost[3] = movement_cost(ini, section, "Hover");
        ground.Cost[0] = movement_cost(ini, section, "Foot");
        ground.Cost[1] = movement_cost(ini, section, "Track");
        ground.Cost[2] = movement_cost(ini, section, "Wheel");
        ground.Cost[4] = 1.0f;
        ground.Cost[5] = movement_cost(ini, section, "Float");
        ground.Cost[6] = movement_cost(ini, section, "Amphibious");
        ground.Cost[7] = movement_cost(ini, section, "FloatBeach");
        ground.Buildable = ini->ReadBool(section, "Buildable", false);
    }
    return true;
}

bool YRPP_STDCALL RulesClass::Read_Powerups(CCINIClass* ini) {
    if (!ini || !ini->GetSection("Powerups")) return false;
    for (int i = 0; i < 19; ++i) {
        char text[128];
        if (!ini->ReadString("Powerups", Powerups::Effects[i], "0,NONE", text, sizeof(text))) continue;
        char* cursor = text;
        if (char* token = next_token(cursor)) {
            trim_token(token);
            Powerups::Weights[i] = weight_from_text(token);
        }
        if (char* token = next_token(cursor)) {
            trim_token(token);
            Powerups::Anims[i] = AnimTypeClass::FindIndex(token);
        }
        if (char* token = next_token(cursor)) {
            trim_token(token);
            if (!_strcmpi(token, "yes")) Powerups::Naval[i] = true;
            else if (!_strcmpi(token, "no")) Powerups::Naval[i] = false;
        }
        if (char* token = next_token(cursor)) {
            if (std::strchr(token, '%')) Powerups::Arguments[i] = std::atof(token) * 0.01;
            else {
                trim_token(token);
                Powerups::Arguments[i] = std::atof(token);
            }
        }
    }
    return true;
}

bool YRPP_STDCALL RulesClass::Read_Movies(CCINIClass* ini) {
    if (!ini || !ini->GetSection("Movies")) return false;
    const int count = ini->GetKeyCount("Movies");
    for (int i = 0; i < count; ++i) {
        char name[32];
        if (!ini->ReadString("Movies", ini->GetKeyName("Movies", i), "<none>", name, sizeof(name)) ||
                MovieInfo::FindIndex(name) != -1) continue;
        const size_t length = std::strlen(name) + 1;
        auto* copy = static_cast<char*>(YRMemory::AllocateOnce(length));
        // A failed strdup/append in the EXE can insert null or lose the name.
        // Preserve earlier insertions but report failure and release this name.
        if (!copy) return false;
        std::memcpy(copy, name, length);
        bool appended = false;
        const bool initialized = MovieInfo::Array.IsInitialized;
        try {
            appended = MovieInfo::Array.AddItem(copy);
        } catch (const std::bad_alloc&) {
            // std::allocator throws instead of returning null as the original
            // vector allocator did. Restore its guard before reporting failure.
            MovieInfo::Array.IsInitialized = initialized;
        }
        if (!appended) {
            YRMemory::Deallocate(copy);
            return false;
        }
    }
    return true;
}
