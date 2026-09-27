// MemoryBuffer implementation moved from YRpp YRAllocator.h; see yrpp/SOURCE.json.
// All targets use the same core CRT contract; ownership flags remain upstream.
#include "yrpp/YRAllocator.h"
#include <cstddef>
#include "yrpp/Memory.h"

MemoryBuffer::MemoryBuffer(int size) noexcept : MemoryBuffer(nullptr, size) {}

MemoryBuffer::MemoryBuffer(void *pBuffer, int size) noexcept : Buffer(pBuffer), Size(size) {
    if (!this->Buffer && this->Size > 0) {
        this->Buffer = YRMemory::Allocate(static_cast<size_t>(size));
        this->Allocated = true;
    }
}

MemoryBuffer::MemoryBuffer(MemoryBuffer &&other) noexcept
    : Buffer(other.Buffer), Size(other.Size), Allocated(other.Allocated) {
    other.Allocated = false;
}

MemoryBuffer::~MemoryBuffer() noexcept {
    if (this->Allocated) {
        YRMemory::Deallocate(this->Buffer);
    }
}

MemoryBuffer &MemoryBuffer::operator=(MemoryBuffer const &other) noexcept {
    if (this != &other) {
        MemoryBuffer tmp(static_cast<MemoryBuffer &&>(*this));
        this->Buffer = other.Buffer;
        this->Size = other.Size;
    }

    return *this;
}

MemoryBuffer &MemoryBuffer::operator=(MemoryBuffer &&other) noexcept {
    *this = other;
    auto const allocated = other.Allocated;
    other.Allocated = false;
    this->Allocated = allocated;
    return *this;
}

void MemoryBuffer::Clear() noexcept {
    MemoryBuffer tmp(static_cast<MemoryBuffer &&>(*this));
    this->Buffer = nullptr;
    this->Size = 0;
}
