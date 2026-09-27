#pragma once
#include "yrpp/BasicStructures.h"
#include <cstdint>
class FileClass;
class BSurface;
class PCX;
namespace game {
struct PCXValue {
    BSurface* surface;
    BytePalette palette;
};
std::uint32_t pcx_hash(const char* const* key);
void grow_pcx(PCX* pcx);
void shrink_pcx(PCX* pcx);
bool erase_pcx(PCX* pcx, const char* const* key, PCXValue* output);

}
