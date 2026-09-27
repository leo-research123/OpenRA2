// File-class declarations calibrated to YR 1.001. Implementations retain the
// fixed EA file hierarchy; platform I/O is supplied by game::FileSystem.
#pragma once

#include "yrpp/RawFileClass.h"
#include "yrpp/YRAllocator.h"

class BufferIOFileClass : public RawFileClass
{
public:
    // Destructor
    virtual ~BufferIOFileClass() override; // VA: 0x00431B80
    // FileClass
    virtual const char* SetFileName(const char* pFileName) override; // VA: 0x00431E80
    virtual bool Exists(bool writeShared = false) override; // VA: 0x00431F10
    virtual bool HasHandle() override; // VA: 0x00431F30
    virtual bool Open(FileAccessMode access) override; // VA: 0x00431F70
    virtual bool OpenEx(const char* pFileName, FileAccessMode access) override; // VA: 0x00431F50
    virtual int ReadBytes(void* pBuffer, int nNumBytes) override; // VA: 0x004322A0
    virtual int Seek(int offset, FileSeekMode seek) override; // VA: 0x004324B0
    virtual int GetFileSize() override; // VA: 0x004325A0
    virtual int WriteBytes(void* pBuffer, int nNumBytes) override; // VA: 0x00432050
    virtual void Close() override; // VA: 0x004325C0

    // Constructor
    /// VA: 0x00431B20.
    BufferIOFileClass();
    // Missing nonvirtual original methods verified at 431BC0/431D90/431DD0.
    /// VA: 0x00431BC0.
    bool Cache(int size = 0, void* buffer = nullptr);
    /// VA: 0x00431D90.
    void Free();
    /// VA: 0x00431DD0.
    bool Commit();

    // Properties

public:
    // 431B20, 431BC0 and 431DD0: byte flags and pointer at x86 +30.
    bool IsAllocated{};
    bool IsOpen{};
    bool IsDiskOpen{};
    bool IsCached{};
    bool IsChanged{};
    bool UseBuffer{};
    int BufferRights{};
    void* IOBuffer{};
    int BufferSize{};
    int BufferPos{};
    int BufferFilePos{};
    int BufferChangeBeg{-1};
    int BufferChangeEnd{-1};
    int CachedFileSize{};
    int FilePos{};
    int TrueFileStart{};
};

//--------------------------------------------------------------------
// Files on a CD?
//--------------------------------------------------------------------
class CDFileClass : public BufferIOFileClass
{
public:
    // Destructor
    /// VA: unknown.
    virtual ~CDFileClass();
    // FileClass
    virtual const char* SetFileName(const char* pFileName) override; // VA: 0x0047AE10
    virtual bool Open(FileAccessMode access) override; // VA: 0x0047AAB0
    virtual bool OpenEx(const char* pFileName, FileAccessMode access) override; // VA: 0x0047AF10

    // Constructor
    /// VA: 0x0047AA30.
    CDFileClass();

    // Property

public:
    bool IsDisabled{};
};

//--------------------------------------------------------------------
// Files in MIXes
//--------------------------------------------------------------------
class CCFileClass : public CDFileClass
{
public:
    // Destructor
    /// VA: unknown.
    virtual ~CCFileClass();

    // FileClass
    virtual const char* SetFileName(const char* pFileName) override; // VA: 0x00473FC0
    virtual bool Exists(bool writeShared = false) override; // VA: 0x00473C50
    virtual bool HasHandle() override; // VA: 0x00473CD0
    virtual bool Open(FileAccessMode access) override; // VA: 0x00473D10
    virtual bool OpenEx(const char* pFileName, FileAccessMode access) override; // VA: 0x00401980
    virtual int ReadBytes(void* pBuffer, int nNumBytes) override; // VA: 0x00473B10
    virtual int Seek(int offset, FileSeekMode seek) override; // VA: 0x00473BA0
    virtual int GetFileSize() override; // VA: 0x00473C00
    virtual int WriteBytes(void* pBuffer, int nNumBytes) override; // VA: 0x00473AE0
    virtual void Close() override; // VA: 0x00473CE0
    virtual DWORD GetFileTime() override; // VA: 0x00473E50
    virtual bool SetFileTime(DWORD FileTime) override; // VA: 0x00473F00
    virtual void CDCheck(DWORD errorCode, bool lUnk, const char* pFilename) override; // VA: 0x00473AB0

    // Constructor
    /// VA: 0x004739F0.
    explicit CCFileClass(const char* pFileName = nullptr);

    // Properties

public:
    MemoryBuffer Buffer;
    DWORD Position{};
    DWORD Availablility{};
};

// TO BE CREATED WHEN NEEDED
// class RAMFileClass : public FileClass{/*...*/};
