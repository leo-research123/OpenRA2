#pragma once

#include <cstdint>

// Minimal, no-device implementation of the original WinModemClass.
// Serial hardware, dialing, and asynchronous send/receive are NOT implemented:
// opening always fails and Get_Modem_Status always reports no carrier.
// The initial virtual methods follow the operations identified at 774120,
// 774410, 7748A0 and 7748B0 in fixed gamemd (SHA-256 7b8a0685...).
// Only core-owned instances are supported. This partial declaration does not
// reproduce the EXE's 0x2088 layout or its complete virtual interface: never
// overlay it on *B72D20 or pass these objects to original WinModem methods.
class WinModemClass {
public:
    static constexpr int CarrierDetect = 0x80;
    static constexpr std::intptr_t InvalidHandle = -1;

    // Optional borrowed core instance. The connection owner selects it;
    // construction alone neither publishes an instance nor implies a device.
    // This storage is independent of the original game's B72D20 global.
    static WinModemClass* Instance;

    WinModemClass() noexcept;
    virtual ~WinModemClass() noexcept;
    WinModemClass(const WinModemClass&) = delete;
    WinModemClass& operator=(const WinModemClass&) = delete;

    virtual std::intptr_t Open_Serial_Port(const char* port, unsigned baud,
        unsigned char parity, unsigned char data_bits, unsigned char stop_bits) noexcept;
    virtual int Set_Dial_Type(int type) noexcept;
    // Returns signal bits, not a signed error code. CarrierDetect is the bit
    // Scenario uses; zero means no carrier even when an instance exists.
    virtual int Get_Modem_Status() noexcept;

    void Close_Serial_Port() noexcept;
    std::intptr_t Get_Port_Handle() const noexcept;

private:
    int DialType;
};
