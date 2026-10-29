#pragma once
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include "api/resource_queries.hpp"

// Godot lifetime wrapper over the actual INI object; no copied dictionary/model.
// Parsing owns all strings, so this result survives resource-environment teardown.
class RA2INI : public godot::RefCounted {
    GDCLASS(RA2INI, godot::RefCounted)
protected:
    static void _bind_methods();

public:
    ~RA2INI() override;
    godot::PackedStringArray get_section_names() const;
    godot::PackedStringArray get_key_names(const godot::String& section);
    bool has_section(const godot::String& section);
    bool has_key(const godot::String& section, const godot::String& key);
    godot::String get_string(const godot::String& section, const godot::String& key,
                             const godot::String& fallback) noexcept;
    // ICU-decoded text. Missing or invalid values clear output and report failure.
    // No exceptions cross the host boundary.
    bool try_get_string(const godot::String& section, const godot::String& key, godot::Dictionary output) noexcept;
    int get_integer(const godot::String& section, const godot::String& key, int fallback);
    bool get_bool(const godot::String& section, const godot::String& key, bool fallback);
    double get_double(const godot::String& section, const godot::String& key, double fallback);
    // Map thumbnail from [Preview] Size and the LZO-packed [PreviewPack] RGB
    // block. Missing or malformed data returns a null reference.
    godot::Ref<godot::Image> get_preview_image() noexcept;

private:
    friend class RA2Core;
    void initialize_text_encoding() noexcept;
    bool decode_text(const char* text, godot::String& output) const noexcept;
    game::IniHandle* ini_ = nullptr;
    char text_encoding_[64] = "UTF-8";
};
