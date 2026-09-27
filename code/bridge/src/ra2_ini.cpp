#include "bridge/ra2_ini.hpp"
#include <godot_cpp/core/class_db.hpp>

void RA2INI::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("get_section_names"), &RA2INI::get_section_names);
    ClassDB::bind_method(D_METHOD("get_key_names", "section"), &RA2INI::get_key_names);
    ClassDB::bind_method(D_METHOD("has_section", "section"), &RA2INI::has_section);
    ClassDB::bind_method(D_METHOD("has_key", "section", "key"), &RA2INI::has_key);
    ClassDB::bind_method(D_METHOD("get_string", "section", "key", "fallback"), &RA2INI::get_string, DEFVAL(""));
    ClassDB::bind_method(D_METHOD("get_integer", "section", "key", "fallback"), &RA2INI::get_integer, DEFVAL(0));
    ClassDB::bind_method(D_METHOD("get_bool", "section", "key", "fallback"), &RA2INI::get_bool, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("get_double", "section", "key", "fallback"), &RA2INI::get_double, DEFVAL(0.0));
}
godot::PackedStringArray RA2INI::get_section_names() const {
    godot::PackedStringArray result;
    for (const auto* section : ini_.Sections) result.append(godot::String::utf8(section->Name));
    return result;
}
godot::PackedStringArray RA2INI::get_key_names(const godot::String& section) {
    const auto name = section.utf8();
    godot::PackedStringArray result;
    if (auto* node = ini_.GetSection(name.get_data()))
        for (const auto* entry : node->Entries) result.append(godot::String::utf8(entry->Key));
    return result;
}
bool RA2INI::has_section(const godot::String& section) {
    const auto name = section.utf8(); return ini_.Exists(name.get_data(), nullptr);
}
bool RA2INI::has_key(const godot::String& section, const godot::String& key) {
    const auto name = section.utf8(), field = key.utf8();
    // Temporary UTF-8 buffers can reuse an address across calls. They are not
    // valid identities for the original caller-owned section-name cache.
    ini_.Reset(); return ini_.Exists(name.get_data(), field.get_data());
}
godot::String RA2INI::get_string(const godot::String& section, const godot::String& key, const godot::String& fallback) {
    const auto name = section.utf8(), field = key.utf8(), def = fallback.utf8();
    char text[512]; ini_.Reset();
    ini_.ReadString(name.get_data(), field.get_data(), def.get_data(), text, sizeof(text));
    return godot::String::utf8(text);
}
int RA2INI::get_integer(const godot::String& section, const godot::String& key, int fallback) {
    const auto name = section.utf8(), field = key.utf8(); ini_.Reset();
    return ini_.ReadInteger(name.get_data(), field.get_data(), fallback);
}
bool RA2INI::get_bool(const godot::String& section, const godot::String& key, bool fallback) {
    const auto name = section.utf8(), field = key.utf8(); ini_.Reset();
    return ini_.ReadBool(name.get_data(), field.get_data(), fallback);
}
double RA2INI::get_double(const godot::String& section, const godot::String& key, double fallback) {
    const auto name = section.utf8(), field = key.utf8(); ini_.Reset();
    return ini_.ReadDouble(name.get_data(), field.get_data(), fallback);
}
