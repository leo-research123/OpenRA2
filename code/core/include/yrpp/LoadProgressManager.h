#pragma once

#include "yrpp/YRPP.h"

#include "yrpp/Helpers/CompileTime.h"

struct SHPStruct;
class Surface;

class LoadProgressManager {
public:
    /// Global VA: 0x00ABC9BC.
    DEFINE_REFERENCE(LoadProgressManager*, Instance, 0xABC9BCu)
    /// Global VA: 0x00B0FB88.
    DEFINE_REFERENCE(ConvertClass*, LoadScreenPal, 0xB0FB88u)
    /// Global VA: 0x00B0FB84.
    DEFINE_REFERENCE(BytePalette*, LoadScreenBytePal, 0xB0FB84u)

    // Returns Instance, allocating and registering it on first use; null when the allocation fails.
    /// VA: 0x00552A40.
    static LoadProgressManager* YRPP_FASTCALL GetOrCreate() { JMP_STD(0x552A40); }

    // Releases the Instance's resources (0x00554370), deletes it and clears Instance.
    /// VA: 0x00552AA0.
    static void YRPP_FASTCALL Destroy() { JMP_STD(0x552AA0); }

    // Only the message 4526 is handled: it releases ProgressSurface through DSurface::Hidden.
    /// VA: 0x00554400.
    virtual bool vt_entry_00(int message, int unused) { JMP_THIS(0x554400); }

    // Frees the owned graphics (shape buffers and objects, progress surface) and clears the pointers.
    /// VA: 0x00554370.
    void ReleaseGraphics() { JMP_THIS(0x554370); }

    /// VA: 0x00552D60.
    void Draw() { JMP_THIS(0x552D60); }

    DWORD field_4;
    DWORD field_8;
    RectangleStruct TitleBarRect;
    RectangleStruct LoadBarSHPRect;
    RectangleStruct LoadScreenSHPRect;
    wchar_t* LoadMessage;
    wchar_t* LoadBriefing;
    SHPStruct* TitleBarSHP;
    SHPStruct* LoadScreenSHP;
    SHPStruct* LoadBarSHP;
    bool TitleBarSHP_loaded;
    bool LoadScreenSHP_loaded;
    bool LoadBarSHP_loaded;
    DWORD field_54;
    DWORD field_58;
    DWORD field_5C;
    DSurface* ProgressSurface;
};
