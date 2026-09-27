#pragma once
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include "yrpp/CCINIClass.h"

// Godot lifetime wrapper over the actual INI object; no copied dictionary/model.
// Parsing owns all strings, so this result survives resource-environment teardown.
class RA2INI : public godot::RefCounted {
    GDCLASS(RA2INI, godot::RefCounted)
protected:
    static void _bind_methods();
public:
    godot::PackedStringArray get_section_names() const;
    godot::PackedStringArray get_key_names(const godot::String& section);
    bool has_section(const godot::String& section);
    bool has_key(const godot::String& section, const godot::String& key);
    godot::String get_string(const godot::String& section, const godot::String& key, const godot::String& fallback);
    int get_integer(const godot::String& section, const godot::String& key, int fallback);
    bool get_bool(const godot::String& section, const godot::String& key, bool fallback);
    double get_double(const godot::String& section, const godot::String& key, double fallback);
private:
    friend class RA2Core;
    CCINIClass ini_;
};
