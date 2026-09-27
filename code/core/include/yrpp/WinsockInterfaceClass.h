#pragma once

#include "yrpp/YRPPCore.h"
#include "yrpp/ArrayClasses.h"
#include "yrpp/Helpers/CompileTime.h"

struct __declspec(align(4)) WinsockBufferType
{
    char Header[16];
    int BufferLen;
    char IsBroadcast;
    BYTE InUse;
    BYTE Allocated;
    char Bool1;
    char Bool2;
    int16_t SrcPort;
    int CRC;
    char Buffer[640];
};
static_assert(sizeof(WinsockBufferType) == 0x2A0);

class __declspec(align(4)) WinsockInterfaceClass
{
public:
    // Static
    /// Global VA: 0x00887628.
    DEFINE_REFERENCE(WinsockInterfaceClass*, Instance, 0x887628u)

    /// VA: 0x007B1DE0.
    bool Init()
        JMP_THIS(0x7B1DE0);

    /// VA: 0x007B1BC0.
    bool StartListening()
        JMP_THIS(0x7B1BC0);

    /// VA: 0x007B1C50.
    void StopListening()
        JMP_THIS(0x7B1C50);

    /// VA: 0x007B1CA0.
    void DiscardInBuffers()
        JMP_THIS(0x7B1CA0);

    /// VA: 0x007B1D10.
    void DiscardOutBuffers()
        JMP_THIS(0x7B1D10);

    // Properties
private:
    void* vftable;
public:
    DWORD MaxPacketSize;
    DynamicVectorClass<WinsockBufferType*> InBuffers;
    DynamicVectorClass<WinsockBufferType*> OutBuffers;
    DynamicVectorClass<WinsockBufferType*> AltOutBuffers;
    WinsockBufferType StaticInBuffer[128];
    WinsockBufferType StaticOutBuffer[128];
    WinsockBufferType StaticAltOutBuffer[128];
    DWORD StaticInBufferPos;
    DWORD StaticOutBufferPos;
    int StaticAltOutBufferPos;
    DWORD StaticInBuffersInUse;
    DWORD StaticOutBuffersInUse;
    DWORD StaticAltOutBuffersInUse;
    BYTE WinsockInitialised;
    int Socket;
    DWORD AlternateSocket;
    char ReceiveBuffer[640];
    int field_3F2F4;
    int NetCard;
};
static_assert(sizeof(WinsockInterfaceClass) == 0x3F2FC);
