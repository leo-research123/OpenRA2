#pragma once
#include "api/type_resources.hpp"
#include "yrpp/CCINIClass.h"
#include "yrpp/ObjectTypeClass.h"
namespace game {
const TypeResourceServices& type_resources() noexcept;
void type_resource_result(TypeResourceStatus) noexcept;
// Records the first failing type (or art file) of the current scope and reports failure.
void type_resource_failure(const char* name) noexcept;
// Name recorded by type_resource_failure in the current scope, or "".
const char* last_type_resource_failure() noexcept;
CCINIClass& type_art_ini() noexcept;
TheaterType type_theater() noexcept;
bool type_image_filename(const ObjectTypeClass&, char*, std::size_t, bool new_theater);
// Building-specific 0x0045F230 naming: independent of ObjectType.NewTheater.
bool building_shape_filename(const char* base, bool theater_extension, char* output, std::size_t capacity);
bool building_shape_filename(const char* base, bool theater_extension, TheaterType theater, char* output,
                             std::size_t capacity);
bool load_owned_type_shape(const char* filename, SHPStruct*& output);
bool read_type_sound(CCINIClass&, const char*, const char*, int&, std::size_t capacity = 512);
}
