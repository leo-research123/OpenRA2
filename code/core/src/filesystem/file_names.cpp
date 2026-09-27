#include "file_names.hpp"
#include "yrpp/Memory.h"
#include <cstring>

namespace game {
char* DuplicateName(const char* name) {
    if (!name) return nullptr;
    const auto size = std::strlen(name) + 1;
    auto* result = static_cast<char*>(YRMemory::AllocateBytes(size));
    if (result) std::memcpy(result, name, size);
    return result;
}
void FreeName(char* name) { YRMemory::Deallocate(name); }
#if !defined(RA2_FILES_GAME) && !defined(RA2_IMAGE_GAME)
void UppercaseName(char* name) {
    for (; *name; ++name) if (*name >= 'a' && *name <= 'z') *name -= 'a' - 'A';
}
#endif
}
