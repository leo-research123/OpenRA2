// PCX object fields and dictionary operations adapted from fixed YRpp
// 9402d7da Dictionary.h. YR-specific minimum size, deletion ordering and float
// thresholds calibrated by 6B9450/6B9530/6BAC40/6BAEF0/6BAFA0.
// No general-purpose parallel image cache is introduced.
#include "yrpp/Memory.h"
#include "yrpp/PCX.h"
#include "PCXHelpers.hpp"
#include "filesystem/file_system.hpp"
#include "yrpp/CCFileClass.h"
#include <memory>
#include "yrpp/Surface.h"
#include "yrpp/Drawing.h"
#include <bit>
#include <cstring>

namespace {
struct PCXNode { char* key; game::PCXValue value; PCXNode* next; };
// The game's one-pointer string used by this dictionary is a simple owned
// string, not the unrelated reference-counted WWString implementation.
void assign_key(char*& destination, const char* source) {
    if ((!destination && !source) || (destination && source && std::strcmp(destination, source) == 0)) return;
    if (destination) YRMemory::Deallocate(destination);
    const char* text = source ? source : "";
    destination = static_cast<char*>(YRMemory::Allocate(std::strlen(text) + 1));
    std::strcpy(destination, text);
}
class Key {
public:
    explicit Key(const char* text) {
        const char* source = text ? text : "";
        data = static_cast<char*>(YRMemory::Allocate(std::strlen(source) + 1));
        std::strcpy(data, source);
    }
    ~Key() { if (data) YRMemory::Deallocate(data); }
    void lower() { for (char* p = data; *p; ++p) if (*p >= 'A' && *p <= 'Z') *p += 'a' - 'A'; }
    char* data;
};
PCXNode** table(PCX* object) { return static_cast<PCXNode**>(object->Buffer); }
std::uint32_t YRPP_FASTCALL pcx_hash_callback(const char* const* key, void*) {
    return game::pcx_hash(key);
}
std::uint32_t hash(PCX* object, const char* const* key) {
    // Keep the actual per-object callback and its ECX calling convention.
    using Hash = std::uint32_t (YRPP_FASTCALL*)(const char* const*, void*);
    return reinterpret_cast<Hash>(object->HashFunction)(key, nullptr) & ((1u << (object->TableBits & 31)) - 1u);
}
bool equal(const char* a, const char* b) { return (!a && !b) || (a && b && std::strcmp(a, b) == 0); }
PCXNode* find(PCX* object, const char* const* key) {
    if (!object->Count) return nullptr;
    for (auto* node = table(object)[hash(object, key)]; node; node = node->next)
        if (equal(node->key, *key)) return node;
    return nullptr;
}
void delete_node(PCXNode* node) {
    if (node->key) YRMemory::Deallocate(node->key);
    node->key = nullptr;
    YRMemory::Deallocate(node);
}
void resize(PCX* object, bool grow) {
    const auto old_size = object->TableSize;
    auto** old = table(object);
    object->TableSize = grow ? old_size * 2 : old_size / 2;
    object->TableBits += grow ? 1u : 0xffffffffu;
    object->Buffer = YRMemory::Allocate(sizeof(PCXNode*) * object->TableSize);
    std::memset(object->Buffer, 0, sizeof(PCXNode*) * object->TableSize);
    for (std::uint32_t i = 0; i < old_size; ++i) {
        for (auto* node = old[i]; node;) {
            const auto bucket = hash(object, &node->key);
            auto* first = table(object)[bucket];
            table(object)[bucket] = node;
            auto* next = node->next;
            node->next = first;
            node = next;
        }
    }
    YRMemory::Deallocate(old);
}
}

#ifndef RA2_IMAGE_GAME
namespace { PCX pcx_instance; }
PCX& PCX::Instance = pcx_instance;
#endif

PCX::PCX() {
    ShrinkThreshold = 0.2;
    ExpandThreshold = 0.8;
    MinTableSize = 128;
    TableSize = 128;
    TableBits = 7;
    Log2Size = 0;
    Count = 0;
    KeepSize = false;
    Buffer = YRMemory::Allocate(sizeof(PCXNode*) * TableSize);
    std::memset(Buffer, 0, sizeof(PCXNode*) * TableSize);
    HashFunction = reinterpret_cast<void*>(&pcx_hash_callback);
}
PCX::~PCX() {
    char* key = nullptr;
    while (Count) {
        std::uint32_t bucket = 0;
        while (bucket < TableSize && !table(this)[bucket]) ++bucket;
        if (bucket == TableSize) break;
        auto* node = table(this)[bucket];
        assign_key(key, node->key);
        const auto value = node->value.surface;
        const float percent = static_cast<float>(double(Count - 1u) / double(std::bit_cast<std::int32_t>(TableSize)));
        auto* next = node->next;
        delete_node(node);
        table(this)[bucket] = next;
        --Count;
        if (percent <= ShrinkThreshold) game::shrink_pcx(this);
        if (value) BSurface::Destroy(value);
    }
    if (key) YRMemory::Deallocate(key);
    for (std::uint32_t i = 0; i < TableSize; ++i) {
        for (auto* node = table(this)[i]; node;) {
            auto* next = node->next;
            delete_node(node);
            node = next;
        }
        table(this)[i] = nullptr;
    }
    Count = 0;
    while (TableSize > std::uint32_t(MinTableSize) && !KeepSize) game::shrink_pcx(this);
    YRMemory::Deallocate(Buffer); // target leaves the pointer value in the destroyed object
}

bool PCX::LoadFile(const char* name, int format, int grayscale) {
    return Instance.GetSurface(name, nullptr) || Instance.ForceLoadFile(name, format, grayscale);
}
bool PCX::ForceLoadFile(const char* name, int format, int grayscale) {
    BytePalette palette{};
    game::PCXValue value;
    value.surface = nullptr;
    alignas(CCFileClass) std::byte file_storage[sizeof(CCFileClass)];
    std::unique_ptr<CCFileClass, decltype(&game::DestroyFile)> file(
        game::ConstructFile(file_storage, name), game::DestroyFile);
    auto* original = Read_PCX_File(file.get(), &palette, nullptr, 0);
    if (!original) return false;
    auto* result = original;
    if (format == 2) {
        WORD colors[256];
        for (int i = 0; i < 256; ++i) {
            const auto& c = palette.Entries[i];
            colors[i] = Drawing::RGB_To_Int(c.R, c.G, c.B);
        }
        RectangleStruct bounds;
        original->GetRect(&bounds);
        result = BSurface::Create(bounds.Width, bounds.Height, 2);
        auto* dest = static_cast<WORD*>(result->Lock(0, 0));
        auto* src = static_cast<BYTE*>(original->Lock(0, 0));
        const int count = std::bit_cast<std::int32_t>(std::uint32_t(bounds.Width) * std::uint32_t(bounds.Height));
        for (int i = 0; i < count; ++i) dest[i] = colors[src[i]];
        original->Unlock();
        result->Unlock();
        BSurface::Destroy(original);
    } else if (format == 1) {
        std::memcpy(&value.palette, &palette, 768);
    }
    if (grayscale && format == 1) {
        RectangleStruct bounds;
        result->GetRect(&bounds);
        auto* pixels = static_cast<BYTE*>(result->Lock(0, 0));
        const int count = std::bit_cast<std::int32_t>(std::uint32_t(bounds.Width) * std::uint32_t(bounds.Height));
        for (int i = 0; i < count; ++i) pixels[i] = palette.Entries[pixels[i]].R;
        result->Unlock();
    }
    value.surface = result;
    Key key(name);
    key.lower();
    if (auto* previous = find(this, &key.data)) {
        auto* surface = previous->value.surface;
        game::PCXValue removed;
        game::erase_pcx(this, &key.data, &removed);
        if (surface) BSurface::Destroy(surface);
    }
    auto* node = static_cast<PCXNode*>(YRMemory::Allocate(sizeof(PCXNode)));
    node->key = nullptr;
    node->value.surface = nullptr;
    assign_key(node->key, key.data);
    std::memcpy(&node->value, &value, sizeof(game::PCXValue));
    node->next = nullptr;
    const auto bucket = hash(this, &key.data);
    auto* first = table(this)[bucket];
    table(this)[bucket] = node;
    if (first) node->next = first;
    ++Count;
    if (double(Count) / double(TableSize) >= ExpandThreshold) game::grow_pcx(this);
    return true;
}
BSurface* PCX::GetSurface(const char* name, BytePalette* palette) {
    Key key(name); // target lookup does not lowercase; ForceLoadFile does
    const auto* node = find(this, &key.data);
    if (!node) return nullptr;
    if (palette) *palette = node->value.palette;
    return node->value.surface;
}
bool PCX::BlitToSurface(RectangleStruct* bounds, DSurface* target, BSurface* source, WORD transparent) {
    auto* dest = static_cast<WORD*>(target->Lock(0, 0));
    if (!dest) return false;
    const auto* src = static_cast<const WORD*>(source->Lock(0, 0));
    if (!src) { target->Unlock(); return false; }
    const int width = source->GetWidth();
    const int height = source->GetHeight();
    const int pitch = target->GetPitch() / 2;
    dest += bounds->X + pitch * bounds->Y;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) if (src[x] != transparent) dest[x] = src[x];
        if (width > 0) src += width;
        dest += pitch;
    }
    target->Unlock();
    source->Unlock();
    return true;
}

namespace game {
std::uint32_t pcx_hash(const char* const* key) {
    const auto length = *key ? std::uint32_t(std::strlen(*key)) : 0;
    auto result = length;
    for (std::uint32_t i = 0; i < length; ++i)
        result = std::rotl(result + i + std::uint32_t(std::int32_t(static_cast<signed char>((*key)[i]))), 8);
    return result;
}
void grow_pcx(PCX* object) { if (object->KeepSize != 1) resize(object, true); }
void shrink_pcx(PCX* object) {
    if (object->TableSize > std::uint32_t(object->MinTableSize) && object->KeepSize != 1) resize(object, false);
}
bool erase_pcx(PCX* object, const char* const* key, PCXValue* output) {
    if (!object->Count) return false;
    const float percent = static_cast<float>(double(object->Count - 1u) / double(std::bit_cast<std::int32_t>(object->TableSize)));
    const auto bucket = hash(object, key);
    auto** link = &table(object)[bucket];
    if (!*link) return false;
    while (*link && !equal((*link)->key, *key)) link = &(*link)->next;
    bool found = *link != nullptr;
    if (found) {
        auto* node = *link;
        std::memcpy(output, &node->value, sizeof(PCXValue));
        // Head removal decrements after deletion; interior removal before it.
        auto* next = node->next;
        if (link != &table(object)[bucket]) { *link = next; --object->Count; delete_node(node); }
        else { delete_node(node); *link = next; --object->Count; }
    }
    if (percent <= object->ShrinkThreshold) shrink_pcx(object);
    return found;
}
static_assert(sizeof(void*) != 4 || (sizeof(PCXNode) == 0x30c && sizeof(PCXValue) == 0x304));
}
