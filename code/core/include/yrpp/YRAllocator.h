// Memory allocation handler

#pragma once

// Shared core CRT allocation. Original buffers may cross this boundary only
// after the original allocation hooks are installed with the same shared CRT.

class MemoryBuffer
{
public:
    constexpr MemoryBuffer() noexcept = default;

    explicit MemoryBuffer(int size) noexcept;

    MemoryBuffer(void* pBuffer, int size) noexcept;

    // constexpr definitions must remain visible to callers. Copies borrow storage.
    constexpr MemoryBuffer(MemoryBuffer const& other) noexcept
        : Buffer(other.Buffer), Size(other.Size)
    { }

    MemoryBuffer(MemoryBuffer&& other) noexcept;

    ~MemoryBuffer() noexcept;

    MemoryBuffer& operator = (MemoryBuffer const& other) noexcept;

    MemoryBuffer& operator = (MemoryBuffer&& other) noexcept;

    void Clear() noexcept;

public:
    void* Buffer{ nullptr };
    int Size{ 0 };
    bool Allocated{ false };
};
