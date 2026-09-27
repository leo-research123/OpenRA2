#pragma once
#include "api/type_resources.hpp"
#include "yrpp/CCINIClass.h"
#include "yrpp/ObjectTypeClass.h"
namespace game {
const TypeResourceServices& type_resources() noexcept;
void type_resource_result(TypeResourceStatus) noexcept;
CCINIClass& type_art_ini() noexcept;
TheaterType type_theater() noexcept;
bool type_image_filename(const ObjectTypeClass&, char*, std::size_t, bool new_theater);
// Building-specific 0x0045F230 naming: independent of ObjectType.NewTheater.
bool building_shape_filename(const char* base, bool theater_extension,
    char* output, std::size_t capacity);
bool load_building_owned_shape(const char* base, SHPStruct*& image, bool& owned);
bool load_building_cached_shape(const char* base, bool theater_extension,
    SHPStruct*& image, char* resolved = nullptr, std::size_t capacity = 0);
bool load_owned_type_shape(const char*, SHPStruct*&);
bool read_type_sound(CCINIClass&, const char*, const char*, int&);
}
