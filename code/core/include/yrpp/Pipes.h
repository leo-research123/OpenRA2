// YRpp stream interfaces.
#pragma once

#include "yrpp/YRAllocator.h"
#include "yrpp/FileClass.h"
#include <cstring>

class Pipe
{
public:
    explicit Pipe() = default;

    virtual ~Pipe()
    {
        if (this->ChainTo)
            this->ChainTo->ChainFrom = this->ChainFrom;

        if (this->ChainFrom)
            this->ChainFrom->Put_To(this->ChainTo);

        this->ChainFrom = nullptr;
        this->ChainTo = nullptr;
    }

    virtual int Flush()
    {
        if (this->ChainTo)
            return this->ChainTo->Flush();

        return 0;
    }

    virtual int End() { return(Flush()); }

    virtual void Put_To(Pipe* pPipe)
    {
        if (this->ChainTo != pPipe)
        {
            if (pPipe && pPipe->ChainFrom)
            {
                pPipe->ChainFrom->Put_To(nullptr);
                pPipe->ChainFrom = nullptr;
            }

            if (this->ChainTo)
            {
                this->ChainTo->ChainFrom = nullptr;
                this->ChainTo->Flush();
            }

            this->ChainTo = pPipe;
            if (this->ChainTo)
                this->ChainTo->ChainFrom = this;
        }
    }

    void Put_To(Pipe& pipe) { Put_To(&pipe); }

    virtual int Put(void const* source, int length)
    {
        if (this->ChainTo)
            this->ChainTo->Put(source, length);

        return length;
    }

    Pipe* ChainTo { nullptr };
    Pipe* ChainFrom { nullptr };

private:
    Pipe(Pipe& rvalue) = delete;
    Pipe& operator=(Pipe const& pipe) = delete;
};

class BufferPipe final : public Pipe
{
public:
    explicit BufferPipe() = delete;
    explicit BufferPipe(void* pBuffer, int nLength) : Pipe {}, Buffer { pBuffer,nLength }
    {
    }

    virtual ~BufferPipe() override final
    {
    }

    virtual int Put(void const* pSource, int nLength) override final
    {
        if (this->Buffer.Buffer && pSource && nLength > 0 && this->Index < this->Buffer.Size)
        {
            if (this->Buffer.Size)
            {
                int nResidue = this->Buffer.Size - this->Index;
                if (nLength >= nResidue)
                    nLength = nResidue;
            }
            if (nLength > 0)
                memcpy((char*)this->Buffer.Buffer + this->Index, pSource, nLength);

            this->Index += nLength;
            return nLength;
        }

        return 0;
    }

    MemoryBuffer Buffer;
    int Index = 0;

private:
    BufferPipe(BufferPipe& rvalue) = delete;
    BufferPipe& operator=(BufferPipe const& pipe) = delete;
};

class FilePipe : public Pipe {
public:
    explicit FilePipe(FileClass& file) : File(&file) {}
    ~FilePipe() override;
    int Put(const void* input, int length) override;
    int End() override;
    FileClass* File;
    bool HasOpened = false;
};
// LCW block stream, 0 = compress, 1 = decompress; block size 1..64000.
// Malformed compressed input returns -1 and latches Counter = -1.
class LCWPipe final : public Pipe {
public:
    explicit LCWPipe(int control, int blockSize);
    ~LCWPipe() override;
    int Flush() override;
    int Put(const void* buffer, int length) override;
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
class LZOPipe final : public Pipe {
public:
    explicit LZOPipe(int control, int blockSize);
    ~LZOPipe() override;
    int Flush() override;
    int Put(const void* buffer, int length) override;
    int Control;
    int Counter;
    void* Buffer;
    void* Buffer2;
    int BlockSize;
    int SafetyMargin;
    short BlockHeader_CompCount;
    short BlockHeader_UncompCount;
};
