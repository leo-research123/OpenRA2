// YRpp 9402d7da FileFormats/SHP.h, retaining the original object model.
// Frame directory access follows EA Mission Editor 6abf0f55 XCC
// cc_structures.h / shp_ts_file.h; GPL-3.0-or-later, Olaf van der Spek.
#include "filesystem/file_names.hpp"
#include "yrpp/Memory.h"
#include "yrpp/FileFormats/SHP.h"
#include "filesystem/file_system.hpp"
#include "yrpp/CCFileClass.h"
#include <memory>
#include "images/resource_state.hpp"
#include <bit>
#include <cstdint>

namespace {
void delete_shape(SHPStruct* shape) noexcept {
    shape->~SHPStruct();
    YRMemory::Deallocate(shape);
}
#ifdef RA2_IMAGE_GAME
// Preserve the EXE-owned state and initialization; indirect readers have not
// been exhaustively audited. No duplicate DLL storage.
using game::shp_head;
using game::shp_counter;
using game::shared_shape;
using game::shared_capacity;
using game::shared_index;
using game::invalid_frame_bounds;
#else
SHPReference* reference_head = nullptr;
std::uint32_t reference_counter = 1; // zero is the shared-cache sentinel
SHPStruct* shared_data = nullptr;
std::int32_t shared_size = 0;
std::int32_t shared_reference = 0;
RectangleStruct invalid_bounds{};
SHPReference*& shp_head() { return reference_head; }
std::uint32_t& shp_counter() { return reference_counter; }
SHPStruct*& shared_shape() { return shared_data; }
std::int32_t& shared_capacity() { return shared_size; }
std::int32_t& shared_index() { return shared_reference; }
RectangleStruct& invalid_frame_bounds() { return invalid_bounds; }
#endif
const SHPFrame* frame(const SHPStruct* shape, int index) {
    auto* data = const_cast<SHPStruct*>(shape)->GetData();
    if (!data || std::uint32_t(index) >= std::uint32_t(std::int32_t(data->Frames))) return nullptr;
    // Match the original's uint32 offset calculation; no pointer-to-array
    // subtraction or host-sized disk records are involved.
    auto address = reinterpret_cast<std::uintptr_t>(data) + 8u + std::uint32_t(index) * 24u;
    return reinterpret_cast<const SHPFrame*>(address);
}
void release_shared() {
    // 69E210/320 deliberately retain capacity/index when the pointer is null.
    if (auto* data = shared_shape()) {
        delete_shape(data);
        shared_shape() = nullptr;
        shared_capacity() = 0;
        shared_index() = 0;
    }
}
}

SHPReference::SHPReference(const char* filename) {
    Index = std::bit_cast<std::int32_t>(shp_counter()++);
    Filename = game::DuplicateName(filename);
    Loaded = false;
    Data = nullptr;
    Width = Height = Frames = 0;
    alignas(CCFileClass) std::byte file_storage[sizeof(CCFileClass)];
    std::unique_ptr<CCFileClass, decltype(&game::DestroyFile)> file(
        game::ConstructFile(file_storage, Filename), game::DestroyFile);
    if (file->Exists(false)) file->ReadBytes(this, 8);
    // The ignored _strcmpi(filename, "null.shp") has no resource-state effect.
    Type = 0xffff;
    Next = shp_head();
    Prev = nullptr;
    if (Next) Next->Prev = this;
    shp_head() = this;
    unknown_20 = 0;
}

SHPStruct::~SHPStruct() {
    if (Type != 0xffff) return;
    auto* reference = static_cast<SHPReference*>(this);
    Unload();
    if (reference->Next) reference->Next->Prev = reference->Prev;
    if (reference->Prev) reference->Prev->Next = reference->Next;
    else shp_head() = reference->Next;
    if (reference->Filename) {
        game::FreeName(reference->Filename);
        reference->Filename = nullptr;
    }
}

void SHPStruct::Load() {
    if (Type != 0xffff) return;
    auto* reference = static_cast<SHPReference*>(this);
    if (reference->Loaded) return;
    alignas(CCFileClass) std::byte file_storage[sizeof(CCFileClass)];
    std::unique_ptr<CCFileClass, decltype(&game::DestroyFile)> file(
        game::ConstructFile(file_storage, reference->Filename), game::DestroyFile);
    if (void* bytes = file->ReadWholeFile()) {
        reference->Data = static_cast<SHPStruct*>(bytes);
        reference->Loaded = true;
    }
}

void SHPStruct::Unload() {
    if (Type != 0xffff) return;
    auto* reference = static_cast<SHPReference*>(this);
    if (!reference->Loaded) return;
    if (reference->Data) delete_shape(reference->Data);
    reference->Data = nullptr;
    reference->Loaded = false;
}

SHPFile* SHPStruct::GetData() {
    if (Type != 0xffff) return reinterpret_cast<SHPFile*>(this);
    auto* reference = static_cast<SHPReference*>(this);
    if (reference->Loaded) return reinterpret_cast<SHPFile*>(reference->Data);
    if (shared_index() == reference->Index) return reinterpret_cast<SHPFile*>(shared_shape());
    alignas(CCFileClass) std::byte file_storage[sizeof(CCFileClass)];
    std::unique_ptr<CCFileClass, decltype(&game::DestroyFile)> file(
        game::ConstructFile(file_storage, reference->Filename), game::DestroyFile);
    if (!file->Exists(false)) { shared_index() = 0; return nullptr; }
    const int length = file->GetFileSize();
    if (!shared_shape()) {
        shared_capacity() = length < 0x300000 ? 0x300000 : length;
        shared_shape() = static_cast<SHPStruct*>(YRMemory::Allocate(std::size_t(shared_capacity())));
    } else if (shared_capacity() < length) {
        delete_shape(shared_shape());
        shared_shape() = static_cast<SHPStruct*>(YRMemory::Allocate(std::size_t(length)));
        shared_capacity() = length;
    }
    if (shared_shape()) file->ReadBytes(shared_shape(), length);
    shared_index() = reference->Index; // retained even when allocation failed
    return reinterpret_cast<SHPFile*>(shared_shape());
}

BYTE* SHPStruct::GetPixels(int index) const {
    const_cast<SHPStruct*>(this)->Load();
    // GetData may retry through the shared buffer if persistent allocation failed.
    auto* data = const_cast<SHPStruct*>(this)->GetData();
    if (!data || std::uint32_t(index) >= std::uint32_t(std::int32_t(data->Frames))) return nullptr;
    const auto address = reinterpret_cast<std::uintptr_t>(data) + 8u + std::uint32_t(index) * 24u;
    const auto* record = reinterpret_cast<const SHPFrame*>(address);
    if (!record || !record->Offset) return nullptr;
    return reinterpret_cast<BYTE*>(reinterpret_cast<std::uintptr_t>(data) + std::uint32_t(record->Offset));
}
RectangleStruct* SHPStruct::GetFrameBounds(RectangleStruct& output, int index) const {
    const auto* record = frame(this, index);
    output = record ? RectangleStruct{record->Left, record->Top, record->Width, record->Height}
                    : invalid_frame_bounds();
    return &output;
}
ColorStruct* SHPStruct::GetColor(ColorStruct& output, int index) const {
    const auto* record = frame(this, index);
    if (record) { output.R = record->Color.R; output.G = record->Color.G; output.B = record->Color.B; }
    else output.R = output.G = output.B = 0;
    return &output;
}
bool SHPStruct::HasCompression(int index) const {
    const auto* record = frame(this, index);
    return record && (record->Flags & 2) != 0;
}

void Initialize_Shape_Bounds() noexcept { invalid_frame_bounds() = {}; }
void Destroy_All_Shapes() {
    auto* current = shp_head();
    while (current) {
        auto* next = current->Next;
        delete_shape(current);
        current = next;
    }
    shp_head() = nullptr;
}
void Unload_All_Shapes() noexcept {
    for (auto* current = shp_head(); current; current = current->Next) current->Unload();
    release_shared();
}
void Unload_Shapes_Through(std::uint32_t threshold) noexcept {
    for (auto* current = shp_head(); current; current = current->Next)
        if (current->unknown_20 <= threshold) current->Unload();
    release_shared();
}
