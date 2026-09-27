#pragma once

#include <windows.h>
#include "yrpp/Memory.h"
#pragma pack(push, 1)

class FieldClass
{
public:
    FieldClass() = default;

    /// VA: 0x004CB580.
    FieldClass(char* id, CHAR data)
        { JMP_THIS(0x4CB580); }

    /// VA: 0x004CB5E0.
    FieldClass(char* id, BYTE data)
        { JMP_THIS(0x4CB5E0); }

    /// VA: 0x004CB640.
    FieldClass(char* id, SHORT data)
        { JMP_THIS(0x4CB640); }

    /// VA: 0x004CB6A0.
    FieldClass(char* id, WORD data)
        { JMP_THIS(0x4CB6A0); }

    /// VA: 0x004CB700.
    FieldClass(char* id, LONG data)
        { JMP_THIS(0x4CB700); }

    /// VA: 0x004CB760.
    FieldClass(char* id, DWORD data)
        { JMP_THIS(0x4CB760); }

    /// VA: 0x004CB7C0.
    FieldClass(char* id, char* data)
        { JMP_THIS(0x4CB7C0); }

    /// VA: 0x004CB830.
    FieldClass(char* id, void* data, int length)
        { JMP_THIS(0x4CB830); }

    /// VA: 0x004CB890.
    ~FieldClass()
        { JMP_THIS(0x4CB890); }

    /// VA: 0x004CB8B0.
    void HostToNet()
        { JMP_THIS(0x4CB8B0); }

    /// VA: 0x004CB920.
    void NetToHost()
        { JMP_THIS(0x4CB920); }

    // Properties
    char ID[4];
    WORD DataType;
    WORD Size;
    void* Data;
    FieldClass* Next;
};
static_assert(sizeof(FieldClass) == 0x10);

class PacketClass
{
public:
    void AddField(FieldClass* pField)
    {
        pField->Next = this->Head;
        this->Head = pField;
    }

    template<typename T, typename... Args>
    void AddField(char* id, T data, Args... args)
    {
        auto pField = GameCreate<FieldClass>(id, data, args...);
        this->AddField(pField);
    }

    // Properties
    WORD Size;
    WORD ID;
    FieldClass* Head;
    FieldClass* Current;
};
static_assert(sizeof(PacketClass) == 0x0C);

#pragma pack(pop)
