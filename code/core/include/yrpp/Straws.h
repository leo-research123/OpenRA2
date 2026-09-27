// YRpp stream interfaces.
#pragma once

#include "yrpp/YRAllocator.h"
#include "yrpp/FileClass.h"
#include <cstring>

class Straw
{
public:
    explicit Straw() = default;

    virtual ~Straw()
    {
        if (this->ChainTo)
            ChainTo->ChainFrom = this->ChainFrom;

        if (this->ChainFrom)
            ChainFrom->Get_From(ChainTo);

        this->ChainFrom = nullptr;
        this->ChainTo = nullptr;
    }

    virtual void Get_From(Straw* pStraw)
    {
        if (this->ChainTo != pStraw)
        {
            if (pStraw && pStraw->ChainFrom)
            {
                pStraw->ChainFrom->Get_From(nullptr);
                pStraw->ChainFrom = nullptr;
            }

            if (this->ChainTo)
                this->ChainTo->ChainFrom = nullptr;

            this->ChainTo = pStraw;
            if (this->ChainTo)
                this->ChainTo->ChainFrom = this;
        }
    }

    virtual int Get(void* pBuffer, int slen)
    {
        if (this->ChainTo)
            return this->ChainTo->Get(pBuffer, slen);

        return 0;
    }

    void Get_From(Straw& pipe) { Get_From(&pipe); }

    Straw* ChainTo { nullptr };
    Straw* ChainFrom { nullptr };

private:
    Straw(Straw& rvalue) = delete;
    Straw& operator=(Straw const& pipe) = delete;
};

class BufferStraw final : public Straw
{
public:
    explicit BufferStraw() = delete;
    explicit BufferStraw(void* pBuffer, int nLength) : Straw {}, Buffer { pBuffer,nLength }
    { }

    virtual ~BufferStraw() override final
    { }

    virtual int Get(void* pBuffer, int slen) override final
    {
        if (this->Buffer.Buffer && pBuffer && slen > 0 && this->Index < this->Buffer.Size)
        {
            if (this->Buffer.Size)
            {
                int nResidue = this->Buffer.Size - this->Index;
                if (slen > nResidue)
                    slen = nResidue;
            }

            if (slen > 0)
                memcpy(pBuffer, (char*)this->Buffer.Buffer + this->Index, slen);

            this->Index += slen;
            return slen;
        }
        return 0;
    }

    MemoryBuffer Buffer;
    int Index { 0 };

private:
    BufferStraw(BufferStraw& rvalue) = delete;
    BufferStraw& operator=(BufferStraw const& pipe) = delete;
};

// File/cache adapters from pinned EA WWLib xstraw/cstraw, target-calibrated.
class FileStraw : public Straw {
public:
    explicit FileStraw(FileClass& file) : File(&file) {}
    ~FileStraw() override;
    int Get(void* output, int length) override;
    FileClass* File;
    bool HasOpened = false;
};
class CacheStraw : public Straw {
public:
    explicit CacheStraw(int size = 4096);
    int Get(void* output, int length) override;
    MemoryBuffer Buffer;
    int Index = 0;
    int Length = 0;
};
// LCW block stream, 0 = compress, 1 = decompress; block size 1..64000.
// Malformed compressed input returns -1 and latches Counter = -1.
class LCWStraw final : public Straw {
public:
    explicit LCWStraw(int control, int blockSize);
    ~LCWStraw() override;
    int Get(void* buffer, int length) override;
    int Control;
    int Counter;
    void* Buffer;
    void* Buffer2;
    int BlockSize;
    int SafetyMargin;
    short BlockHeader_CompCount;
    short BlockHeader_UncompCount;
};

// Original WWLib/YR LZO block stream, same control and failure contract as LCW.
class LZOStraw final : public Straw {
public:
    explicit LZOStraw(int control, int blockSize);
    ~LZOStraw() override;
    int Get(void* buffer, int length) override;
    int Control;
    int Counter;
    void* Buffer;
    void* Buffer2;
    int BlockSize;
    int SafetyMargin;
    short BlockHeader_CompCount;
    short BlockHeader_UncompCount;
};
