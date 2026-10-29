#pragma once
// Internal parsing helpers; never expose STL at the original-class boundary.
#include "scenario_object_ini.hpp"
#include "type_registry.hpp"
#include <limits>
#include <bit>
#include <cstdint>
#include <cstring>
#include <cstdlib>
namespace game {
// Original ahtoi (0x412610): consume hex digits until the first non-hex,
// preserving 32-bit wrap and signed ID bits used by SwizzleManagerClass.
inline LONG scenario_hex_identity(const char* text) noexcept {
    std::uint32_t value = 0;
    for (; text && *text; ++text) {
        const unsigned char c = *text;
        const int digit = c >= '0' && c <= '9'   ? c - '0'
                          : c >= 'A' && c <= 'F' ? c - 'A' + 10
                          : c >= 'a' && c <= 'f' ? c - 'a' + 10
                                                 : -1;
        if (digit < 0) break;
        value = value * 16u + unsigned(digit);
    }
    return std::bit_cast<LONG>(value);
}

// Original waypoint-name conversion 0x763690: A=0, Z=25, AA=26.
inline bool scenario_waypoint(std::string_view text, int& result) {
    if (text.empty()) {
        result = -1;
        return true;
    }
    unsigned value = 0;
    for (unsigned char c : text) {
        if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
        if (c < 'A' || c > 'Z' || value > (unsigned(std::numeric_limits<int>::max()) - 26) / 26) return false;
        value = value * 26 + unsigned(c - 'A' + 1);
    }
    result = int(value) - 1;
    return true;
}
inline std::string scenario_text(CCINIClass& ini, const char* section, const char* key) {
    char buffer[4096]{};
    ini.ReadString(section, key, "", buffer, sizeof(buffer));
    return buffer;
}
inline bool script_token(const char*& cursor, std::string& value) {
    if (!cursor || !*cursor) return false;
    const char* end = cursor;
    while (*end && *end != ',')
        ++end;
    value = std::string(scenario_trim(std::string_view(cursor, std::size_t(end - cursor))));
    cursor = *end ? end + 1 : end;
    return true;
}
inline bool script_integer(const char*& cursor, int& value) {
    std::string token;
    return script_token(cursor, token) && scenario_number(token, value);
}
template <class T>
T* scenario_reference(const std::string& id) {
    if (id == "-1" || INIClass::IsBlankValue(id.c_str())) return nullptr;
    if (id.size() >= 3) return T::FindOrAllocate(id.c_str());
    int index = -1;
    return scenario_number(id, index) && index >= 0 && index < T::Array.Count ? T::Array[index] : nullptr;
}

// Token transport shared by original-class record readers. Field meanings stay
// in TActionClass/TEventClass. Checked input retains the native status contract;
// legacy input consumes the caller's CRT strtok state and atoi conventions.
class ScriptFieldReader {
    const char** cursor_;
    std::string token_;

public:
    explicit ScriptFieldReader(const char** cursor = nullptr) : cursor_(cursor) {}
    bool checked() const noexcept { return cursor_ != nullptr; }
    bool token(const char*& value) {
        if (!checked()) {
            value = std::strtok(nullptr, ",");
            return value != nullptr;
        }
        if (!script_token(*cursor_, token_)) {
            value = nullptr;
            return false;
        }
        value = token_.c_str();
        return true;
    }
    bool number(const char* text, int& value) const {
        if (checked()) return scenario_number(text, value);
        value = std::atoi(text);
        return true;
    }
    bool integer(int& value) {
        const char* text;
        return token(text) && number(text, value);
    }
    bool waypoint(const char* text, int& value) const {
        if (checked()) return scenario_waypoint(text, value);
        // 0x763690 reads at most two alphabetic characters.
        const auto letter = [](unsigned char c) {
            if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
            return c >= 'A' && c <= 'Z' ? int(c - 'A') : -1;
        };
        value = letter(*text);
        if (value >= 0)
            if (const int second = letter(text[1]); second >= 0) value = 26 * (value + 1) + second;
        return true;
    }
    template <class T>
    T* reference(const char* text) const {
        if (checked()) return scenario_reference<T>(text);
        const int index = std::atoi(text);
        if (index == -1) return nullptr;
        if (std::strlen(text) >= 3) return T::FindOrAllocate(text);
        return index >= 0 && index < T::Array.Count ? T::Array[index] : nullptr;
    }
};
}
