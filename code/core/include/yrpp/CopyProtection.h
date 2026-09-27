#pragma once

#include "yrpp/platform/ABI.h"
#include "yrpp/GeneralDefinitions.h"

namespace CopyProtection
{
    /*
    Shared Memory for Copy Protection Interprocess.
    */
    /// Global VA: 0x0089F75C.
    DEFINE_REFERENCE(char*, _mProtectedData, 0x89F75C);

    /// VA: 0x0049F5C0.
    bool IsLauncherRunning() JMP_STD(0x49F5C0);

    /// VA: 0x0049F620.
    bool NotifyLauncher() JMP_STD(0x49F620);

    /// VA: 0x0049F920.
    bool CheckVersion() JMP_STD(0x49F920);

    /// VA: 0x0049F7A0.
    bool CheckProtectedData() JMP_STD(0x49F7A0);

    /// VA: 0x0049F740.
    void YRPP_FASTCALL DispatchLauncherMessage(MSG* pMsg) JMP_STD(0x49F740);

    /// VA: 0x0049F8B0.
    LSTATUS ShutDown() JMP_STD(0x49F8B0);
}
