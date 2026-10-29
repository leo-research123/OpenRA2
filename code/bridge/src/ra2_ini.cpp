#include "bridge/ra2_ini.hpp"
#include <godot_cpp/core/class_db.hpp>
#include "text_encoding.hpp"
#include <string>
#include <vector>
#include <cstring>

RA2INI::~RA2INI() { game::destroy_ini(ini_); }
void RA2INI::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("get_section_names"), &RA2INI::get_section_names);
    ClassDB::bind_method(D_METHOD("get_key_names", "section"), &RA2INI::get_key_names);
    ClassDB::bind_method(D_METHOD("has_section", "section"), &RA2INI::has_section);
    ClassDB::bind_method(D_METHOD("has_key", "section", "key"), &RA2INI::has_key);
    ClassDB::bind_method(D_METHOD("get_string", "section", "key", "fallback"), &RA2INI::get_string, DEFVAL(""));
    ClassDB::bind_method(D_METHOD("try_get_string", "section", "key", "output"), &RA2INI::try_get_string);
    ClassDB::bind_method(D_METHOD("get_integer", "section", "key", "fallback"), &RA2INI::get_integer, DEFVAL(0));
    ClassDB::bind_method(D_METHOD("get_bool", "section", "key", "fallback"), &RA2INI::get_bool, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("get_double", "section", "key", "fallback"), &RA2INI::get_double, DEFVAL(0.0));
    ClassDB::bind_method(D_METHOD("get_preview_image"), &RA2INI::get_preview_image);
}
godot::PackedStringArray RA2INI::get_section_names() const {
    godot::PackedStringArray result;
    std::uint32_t sections=0;
    if (ini_) game::ini_section_count(*ini_, sections);
    for (std::uint32_t i=0; i<sections; ++i) {
        godot::String text;
        const char* section=nullptr;
        if (game::ini_section_name(*ini_, i, section) && decode_text(section, text)) result.append(text);
    }
    return result;
}
godot::PackedStringArray RA2INI::get_key_names(const godot::String& section) {
    const auto name = section.utf8();
    godot::PackedStringArray result;
    std::uint32_t entries=0;
    if (ini_) game::ini_entry_count(*ini_, name.get_data(), entries);
    for (std::uint32_t i=0; i<entries; ++i) {
        const char* key=nullptr; const char* value=nullptr;
        godot::String text;
        if (game::ini_entry(*ini_, name.get_data(), i, key, value) && decode_text(key, text)) result.append(text);
    }
    return result;
}
bool RA2INI::has_section(const godot::String& section) {
    const auto name = section.utf8();
    bool value=false;
    return ini_ && game::ini_exists(*ini_, name.get_data(), nullptr, value) && value;
}
bool RA2INI::has_key(const godot::String& section, const godot::String& key) {
    const auto name = section.utf8(), field = key.utf8();
    // Temporary UTF-8 buffers can reuse an address across calls. They are not
    // valid identities for the original caller-owned section-name cache.
    bool value=false;
    return ini_ && game::ini_exists(*ini_, name.get_data(), field.get_data(), value) && value;
}
void RA2INI::initialize_text_encoding() noexcept {
    // Only the host's presentation encoding changes; CCINIClass retains its
    // original bytes, object layout, lookup and gameplay parsing contracts.
    std::strcpy(text_encoding_, "GB18030");
    try {
        constexpr size_t limit = 65536;
        std::string sample;
        auto append = [&sample](const char* text) {
            if (!text) return;
            const auto length = std::strlen(text);
            // Omit packed ASCII data and UTF-8 edits from the legacy sample.
            // Preserve complete fields rather than splitting multibyte units.
            if (length >= limit - sample.size() || game::is_utf8_text(text, static_cast<int>(length))) return;
            sample.append(text, length);
            sample.push_back('\n');
        };
        std::uint32_t sections=0;
    if (ini_) game::ini_section_count(*ini_, sections);
    for (std::uint32_t i=0; i<sections; ++i) {
            const char* section=nullptr;
            if (!game::ini_section_name(*ini_, i, section)) continue;
            append(section);
            std::uint32_t entries=0;
            game::ini_entry_count(*ini_, section, entries);
            for (std::uint32_t k=0; k<entries; ++k) {
                const char* key=nullptr; const char* value=nullptr;
                if (game::ini_entry(*ini_, section, k, key, value)) { append(key); append(value); }
            }
        }
        game::TextEncoding encoding;
        if (game::detect_text_encoding(sample.data(), static_cast<int>(sample.size()), encoding) ==
            game::TextDecodeStatus::success)
            std::strcpy(text_encoding_, encoding.name);
        // Ambiguous short legacy text uses the existing Chinese-resource
        // default, through ICU's GB18030 converter (also accepts GBK).
    } catch (...) {
        // Allocation failure keeps the same explicit legacy fallback.
    }
}

bool RA2INI::decode_text(const char* text, godot::String& output) const noexcept {
    output = godot::String{};
    if (!text) return false;
    try {
        const auto size = std::strlen(text);
        if (size > 65536) return false;
        game::TextEncoding encoding;
        std::strcpy(encoding.name, text_encoding_);
        std::vector<char> utf8(size * 4 + 1);
        int written = 0;
        if (game::decode_text_utf8(text, static_cast<int>(size), encoding, utf8.data(), static_cast<int>(utf8.size()),
                                   written) != game::TextDecodeStatus::success)
            return false;
        output = godot::String::utf8(utf8.data(), written);
        return true;
    } catch (...) {
        return false;
    }
}

godot::String RA2INI::get_string(const godot::String& section, const godot::String& key,
                                 const godot::String& fallback) noexcept {
    try {
        godot::Dictionary output;
        if (try_get_string(section, key, output)) return output["text"];
    } catch (...) {
    }
    return fallback;
}

bool RA2INI::try_get_string(const godot::String& section, const godot::String& key, godot::Dictionary output) noexcept {
    try {
        output.erase("text");
        const auto name = section.utf8(), field = key.utf8();
        char text[512]{};
        if (!ini_ || !game::ini_string(*ini_, name.get_data(), field.get_data(), text, sizeof(text))) return false;
        godot::String decoded;
        if (!decode_text(text, decoded)) return false;
        output["text"] = decoded;
        return true;
    } catch (...) {
        return false;
    }
}
int RA2INI::get_integer(const godot::String& section, const godot::String& key, int fallback) {
    const auto name = section.utf8(), field = key.utf8();
    int value=fallback;
    if (ini_) game::ini_integer(*ini_, name.get_data(), field.get_data(), fallback, value);
    return value;
}
bool RA2INI::get_bool(const godot::String& section, const godot::String& key, bool fallback) {
    const auto name = section.utf8(), field = key.utf8();
    bool value=fallback;
    if (ini_) game::ini_bool(*ini_, name.get_data(), field.get_data(), fallback, value);
    return value;
}
double RA2INI::get_double(const godot::String& section, const godot::String& key, double fallback) {
    const auto name = section.utf8(), field = key.utf8();
    double value=fallback;
    if (ini_) game::ini_double(*ini_, name.get_data(), field.get_data(), fallback, value);
    return value;
}
godot::Ref<godot::Image> RA2INI::get_preview_image() noexcept {
    try {
        int width=0, height=0;
        if (!ini_ || !game::ini_preview(*ini_, width, height, nullptr, 0)) return {};
        godot::PackedByteArray pixels;
        pixels.resize(width * height * 3);
        if (!game::ini_preview(*ini_, width, height, pixels.ptrw(), pixels.size())) return {};
        return godot::Image::create_from_data(width, height, false, godot::Image::FORMAT_RGB8, pixels);
    } catch (...) {
        return {};
    }
}
