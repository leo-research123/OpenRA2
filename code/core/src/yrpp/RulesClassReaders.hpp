#pragma once
// Internal helpers shared by the original RulesClass section readers.
#include "yrpp/CCINIClass.h"
#include "yrpp/AbstractTypeClass.h"
#include "rules_runtime.hpp"
#include <new>
#include <stdexcept>
#include <bit>
#include <cstdint>
#include <cstring>

// These readers deliberately observe the target's persistent rounding mode.
#if defined(__clang__)
#pragma STDC FENV_ACCESS ON
#elif defined(_MSC_VER)
#pragma fenv_access(on)
#endif

namespace {
inline bool read_rule_guid(const char* text,GUID& result) {
    // Canonical rule CLSID; invalid input retains the previous value (0x527920).
    if(std::strlen(text)!=38 || text[0]!='{' || text[37]!='}' || text[9]!='-' || text[14]!='-' || text[19]!='-' || text[24]!='-')return false;
    GUID value{};
    const auto hex=[&](unsigned start,unsigned count,std::uint32_t& output){output=0;for(unsigned i=0;i<count;++i){const unsigned char c=text[start+i];unsigned n;
        if(c>='0'&&c<='9')n=c-'0';else if(c>='a'&&c<='f')n=c-'a'+10;else if(c>='A'&&c<='F')n=c-'A'+10;else return false;output=(output<<4)|n;}return true;};
    std::uint32_t part;if(!hex(1,8,part))return false;value.Data1=part;
    if(!hex(10,4,part))return false;value.Data2=static_cast<WORD>(part);if(!hex(15,4,part))return false;value.Data3=static_cast<WORD>(part);
    for(unsigned i=0;i<8;++i){if(!hex(i<2?20+2*i:25+2*(i-2),2,part))return false;value.Data4[i]=static_cast<BYTE>(part);}
    result=value;return true;
}
inline int rule_integer(double value) {
    game::rules_integer_rounding();
    // MSVC x86 _ftol returns a signed 64-bit result in EDX:EAX; the caller
    // retains EAX. Invalid/out-of-int64 conversions produce the indefinite
    // integer (whose low 32 bits are zero). Avoid host float-to-int UB.
    if (!(value >= -9223372036854775808.0 && value < 9223372036854775808.0)) return 0;
    const auto bits = static_cast<std::uint32_t>(static_cast<std::int64_t>(value));
    return std::bit_cast<std::int32_t>(bits);
}
inline int rule_scaled_integer(double value, unsigned multiplier) {
    // Evaluate before _ftol changes the control word, as in the original.
    // Its persistent 53-bit/toward-zero mode controls subsequent products.
    return rule_integer(value * static_cast<double>(multiplier));
}
inline int read_rule_distance(CCINIClass& ini, const char* section, const char* key, int fallback) {
    const double value = ini.ReadDouble(section, key, -1.0);
    return value == -1.0 ? fallback : rule_scaled_integer(value, 256);
}
inline int read_rule_percentage(CCINIClass& ini, const char* section, const char* key, int fallback) {
    int value = ini.ReadInteger(section, key, -1);
    if (value == -1) return fallback;
    if (value >= 100) return 255;
    return value <= 0 ? 0 : value * 256 / 100;
}
inline void read_rule_shape(const char* name, SHPStruct*& result) {
    const auto& runtime = game::rules_runtime();
    if (!runtime.shape || !runtime.shape(runtime.context, name, result))
        throw std::runtime_error("RulesClass shape dependency failed");
}
inline int rule_decimal(const char* text) {
    while (*text && static_cast<unsigned char>(*text) <= 0x20) ++text;
    const bool negative = *text == '-';
    if (*text == '+' || *text == '-') ++text;
    std::uint32_t value = 0;
    while (*text >= '0' && *text <= '9') value = value * 10u + static_cast<unsigned>(*text++ - '0');
    return std::bit_cast<std::int32_t>(negative ? 0u - value : value);
}
inline void read_rule_sound(CCINIClass& ini, const char* section, const char* key, int& value) {
    char name[128];
    if (!ini.ReadString(section, key, "", name, sizeof(name))) return;
    const auto& runtime = game::rules_runtime();
    if (runtime.defer_sound_references) {
        if (!runtime.retain_sound_reference ||
            !runtime.retain_sound_reference(runtime.context, &ini, section, key))
            throw std::runtime_error("RulesClass deferred sound retention failed");
        return;
    }
    int index = -1;
    if (!game::rules_sound_index(name, index))
        throw std::runtime_error("RulesClass sound lookup dependency failed");
    if (index != -1) value = index;
}
template<class T>
void read_rule_type(CCINIClass& ini, const char* section, const char* key,
        AbstractType kind, T*& value) {
    char name[128];
    if (ini.ReadString(section, key, "", name, sizeof(name))) {
        AbstractTypeClass* result = nullptr;
        if (!game::rules_resolve_type(kind, name, result))
            throw std::runtime_error("RulesClass type resolution dependency failed");
        value = static_cast<T*>(result);
    }
}
inline char* next_rule_token(char*& cursor) {
    while (*cursor == ',') ++cursor;
    if (!*cursor) return nullptr;
    char* result = cursor;
    while (*cursor && *cursor != ',') ++cursor;
    if (*cursor) *cursor++ = '\0';
    return result;
}
template<class T>
void read_rule_type_list(CCINIClass& ini, const char* section, const char* key,
        AbstractType kind, TypeList<T*>& value) {
    char text[128];
    if (!ini.ReadString(section, key, "", text, sizeof(text))) {
        // Target returns a value copy of the fallback, then assigns it. Even
        // a borrowed input buffer becomes an owned copy after this operation.
        value = TypeList<T*>(value);
        return;
    }
    TypeList<T*> replacement;
    char* cursor = text;
    while (char* token = next_rule_token(cursor)) {
        AbstractTypeClass* result = nullptr;
        if (!game::rules_resolve_type(kind, token, result))
            throw std::runtime_error("RulesClass type resolution dependency failed");
        auto* item = static_cast<T*>(result);
        if (item && !replacement.AddItem(item)) throw std::bad_alloc();
    }
    value = replacement;
}
inline void read_rule_integer_list(CCINIClass& ini, const char* section, const char* key,
        TypeList<int>& value) {
    char text[512];
    if (!ini.ReadString(section, key, "", text, sizeof(text))) {
        value = TypeList<int>(value);
        return;
    }
    TypeList<int> replacement;
    char* cursor = text;
    while (char* token = next_rule_token(cursor))
        if (!replacement.AddItem(rule_decimal(token))) throw std::bad_alloc();
    value = replacement;
}
inline void read_rule_sound_list(CCINIClass& ini, const char* section, const char* key,
        TypeList<int>& value) {
    char text[128];
    if (!ini.ReadString(section, key, "", text, sizeof(text))) {
        value = TypeList<int>(value);
        return;
    }
    const auto& runtime = game::rules_runtime();
    if (runtime.defer_sound_references) {
        if (!runtime.retain_sound_reference ||
            !runtime.retain_sound_reference(runtime.context, &ini, section, key))
            throw std::runtime_error("RulesClass deferred sound retention failed");
        return;
    }
    TypeList<int> replacement;
    char* cursor = text;
    while (char* token = next_rule_token(cursor)) {
        const auto& runtime = game::rules_runtime();
        bool exists = false;
        int index = -1;
        if (!runtime.sound_list_entry || !runtime.sound_list_entry(runtime.context, token, exists, index))
            throw std::runtime_error("RulesClass sound registration dependency failed");
        if (exists && !replacement.AddItem(index)) throw std::bad_alloc();
    }
    value = replacement;
}
inline void read_rule_prerequisites(CCINIClass& ini, const char* section, const char* key,
        TypeList<int>& value) {
    char text[128];
    if (!ini.ReadString(section, key, "", text, sizeof(text))) {
        value = TypeList<int>(value);
        return;
    }
    TypeList<int> replacement;
    char* cursor = text;
    while (char* token = next_rule_token(cursor)) {
        constexpr const char* categories[] = {"POWER", "FACTORY", "BARRACKS", "RADAR", "TECH", "PROC"};
        int index = -1;
        bool category = false;
        for (int i = 0; i < 6; ++i)
            if (!_strcmpi(token, categories[i])) { category = true; index = -1 - i; break; }
        if (!category) {
            const auto& runtime = game::rules_runtime();
            if (!runtime.find_index || !runtime.find_index(runtime.context, AbstractType::BuildingType, token, index))
                throw std::runtime_error("RulesClass prerequisite lookup dependency failed");
        }
        if ((category || index != -1) && !replacement.AddItem(index)) throw std::bad_alloc();
    }
    value = replacement;
}
inline void load_rule_types(CCINIClass* ini, AbstractType kind) {
    int count = 0;
    for (int index = 0;; ++index) {
        if (!game::rules_type_count(kind, count))
            throw std::runtime_error("RulesClass type enumeration dependency failed");
        if (index >= count) break;
        AbstractTypeClass* item = nullptr;
        if (!game::rules_type_at(kind, index, item))
            throw std::runtime_error("RulesClass type enumeration dependency failed");
        item->LoadFromINI(ini);
    }
}
}
