#pragma once
#include <cstdint>
namespace game {
class ResourceHandle;
struct IniHandle;
// Owned original INI storage, queried synchronously with borrowed string views.
// No exception propagates through any entry below.
bool load_ini(ResourceHandle&, const char* filename, IniHandle*&) noexcept;
void destroy_ini(IniHandle*) noexcept;
bool ini_section_count(const IniHandle&, std::uint32_t&) noexcept;
bool ini_section_name(const IniHandle&, std::uint32_t, const char*&) noexcept;
bool ini_entry_count(IniHandle&, const char* section, std::uint32_t&) noexcept;
bool ini_entry(IniHandle&, const char* section, std::uint32_t, const char*& key, const char*& value) noexcept;
bool ini_exists(IniHandle&, const char* section, const char* key, bool& value) noexcept;
bool ini_string(IniHandle&, const char* section, const char* key, char*, std::uint32_t capacity) noexcept;
bool ini_integer(IniHandle&, const char* section, const char* key, int fallback, int& value) noexcept;
bool ini_bool(IniHandle&, const char* section, const char* key, bool fallback, bool& value) noexcept;
bool ini_double(IniHandle&, const char* section, const char* key, double fallback, double& value) noexcept;
bool ini_preview(IniHandle&, int& width, int& height, std::uint8_t* rgb, std::uint32_t capacity) noexcept;
bool resource_package_count(const ResourceHandle&, std::uint32_t&) noexcept;
bool resource_package(const ResourceHandle&, std::uint32_t, const char*& name, std::uint32_t& entries) noexcept;
bool mount_map_resources(ResourceHandle&) noexcept;
bool localized_string(const ResourceHandle&, const char* label, std::uint16_t*, std::uint32_t capacity,
                       std::uint32_t& length) noexcept;
void unload_shapes() noexcept;
}
