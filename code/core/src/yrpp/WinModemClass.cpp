#include "yrpp/WinModemClass.h"

WinModemClass* WinModemClass::Instance = nullptr;

WinModemClass::WinModemClass() noexcept : DialType(0) {}

WinModemClass::~WinModemClass() noexcept {
    Close_Serial_Port();
    if (Instance == this) Instance = nullptr;
}

std::intptr_t WinModemClass::Open_Serial_Port(const char*, unsigned,
    unsigned char, unsigned char, unsigned char) noexcept {
    return InvalidHandle;
}

int WinModemClass::Set_Dial_Type(int type) noexcept {
    return DialType = type;
}

int WinModemClass::Get_Modem_Status() noexcept { return 0; }

void WinModemClass::Close_Serial_Port() noexcept {
    // No OS handle or buffers are acquired by the no-device implementation.
}

std::intptr_t WinModemClass::Get_Port_Handle() const noexcept {
    return InvalidHandle;
}
