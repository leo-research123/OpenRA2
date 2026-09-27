// YRpp FileSystem interface.
#include "filesystem/file_names.hpp"
#include "yrpp/Memory.h"
#include "yrpp/FileSystem.h"
#include "yrpp/CRC.h"
#include "images/resource_state.hpp"
#include "filesystem/file_system.hpp"
#include "yrpp/CCFileClass.h"
#include <memory>
#include <new>
#include <cstring>

#ifndef RA2_IMAGE_GAME
namespace { BytePalette screen_palette{}, iso_palette{}; }
BytePalette& FileSystem::TEMPERAT_PAL = screen_palette;
BytePalette& FileSystem::ISOx_PAL = iso_palette;
#endif

namespace {
#ifdef RA2_IMAGE_GAME
using game::name_root;
#else
game::NameNode* root = nullptr;
game::NameNode*& name_root() { return root; }
#endif
std::int32_t name_crc(const char* name, char (&upper)[260]) {
    // Preserve the original fixed-name contract and the target CRT's locale.
    std::strcpy(upper, name);
    game::UppercaseName(upper);
    CRCEngine crc;
    return crc(upper, static_cast<int>(std::strlen(upper)));
}
game::NameNode* find_node(std::int32_t crc) {
    auto* node = name_root();
    while (node) {
        if (node->crc == crc && node->value) return node;
        node = node->crc < crc ? node->right : node->left;
    }
    return nullptr;
}
}

void* YRPP_FASTCALL FileSystem::LoadFile(const char* name, bool as_shape) {
    if (!name || std::strlen(name) >= 260) return nullptr;
    char upper[260];
    const auto crc = name_crc(name, upper);
    if (auto* found = find_node(crc)) return found->value;
    alignas(CCFileClass) std::byte file_storage[sizeof(CCFileClass)];
    std::unique_ptr<CCFileClass, decltype(&game::DestroyFile)> file(
        game::ConstructFile(file_storage, name), game::DestroyFile);
    if (!file->Exists(false)) return nullptr;
    auto* node = static_cast<game::NameNode*>(YRMemory::Allocate(sizeof(game::NameNode)));
    if (!node) throw std::bad_alloc();
    node->value = nullptr;
    node->crc = crc;
    game::insert_name_node(node, &name_root());
    void* value;
    if (as_shape || std::strstr(upper, ".SHP")) {
        void* storage = YRMemory::Allocate(sizeof(SHPReference));
        value = storage ? new (storage) SHPReference(name) : nullptr;
    } else {
        value = file->ReadWholeFile();
    }
    node->value = value;
    return value;
}

void* YRPP_FASTCALL FileSystem::LoadWholeFileEx(const char* name, bool& allocated) {
    allocated = false;
    if (void* cached = LoadFile(name, false)) return cached;
    alignas(CCFileClass) std::byte file_storage[sizeof(CCFileClass)];
    std::unique_ptr<CCFileClass, decltype(&game::DestroyFile)> file(
        game::ConstructFile(file_storage, name), game::DestroyFile);
    void* result = file->ReadWholeFile();
    allocated = result != nullptr;
    return result;
}

// Preserve existing YRpp convenience algorithms without emitting bodies in the
// header or calling GameCreate with an unverified compiler-native game vtable.
BytePalette* FileSystem::AllocatePalette(const char* name) {
    alignas(CCFileClass) std::byte file_storage[sizeof(CCFileClass)];
    std::unique_ptr<CCFileClass, decltype(&game::DestroyFile)> file(
        game::ConstructFile(file_storage, name), game::DestroyFile);
    auto* palette = static_cast<BytePalette*>(file->ReadWholeFile());
    if (palette) for (auto& color : palette->Entries) {
        color.R = BYTE(color.R << 2); color.G = BYTE(color.G << 2); color.B = BYTE(color.B << 2);
    }
    return palette;
}

namespace game {
void insert_name_node(NameNode* node, NameNode** link) {
    while (*link) link = (*link)->crc < node->crc ? &(*link)->right : &(*link)->left;
    *link = node;
    node->left = node->right = nullptr;
}
void destroy_name_nodes(NameNode* node) {
    if (!node) return;
    destroy_name_nodes(node->left);
    destroy_name_nodes(node->right);
    YRMemory::Deallocate(node); // values are owned and released by their consumers
}
} // namespace game

void FileSystem::ClearNameCache() noexcept {
    game::destroy_name_nodes(name_root());
    name_root() = nullptr;
}
void FileSystem::InvalidateName(const char* name) {
    if (!name || std::strlen(name) >= 260) return;
    char upper[260];
    if (auto* node = find_node(name_crc(name, upper))) node->value = nullptr;
}
