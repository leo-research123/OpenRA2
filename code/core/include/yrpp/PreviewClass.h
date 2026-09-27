#pragma once

#include "yrpp/YRPPCore.h"

class DSurface;
class INIClass;

class PreviewClass
{
public:

    /// VA: 0x006406E0.
    PreviewClass() JMP_THIS(0x6406E0);
    /// VA: 0x006406F0.
    ~PreviewClass() JMP_THIS(0x6406F0);

    /// VA: 0x00640710.
    void DrawStartPoints(HWND hWnd) JMP_THIS(0x640710);
    /// VA: 0x00640A40.
    bool DrawMap(DSurface& surface) JMP_THIS(0x640A40);
    /// VA: 0x00641140.
    void GeneratePreviewImage() JMP_THIS(0x641140);
    /// VA: 0x006418B0.
    bool WritePreviewPack(INIClass& ini) const JMP_THIS(0x6418B0);
    /// VA: 0x00641B00.
    bool ReadPreviewPack(INIClass& ini) JMP_THIS(0x641B00);
    /// VA: 0x00641DB0.
    bool ReadPCXPreview(const char* lpFile) JMP_THIS(0x641DB0);
    /// VA: 0x00641EE0.
    bool ReadPreview(const char* lpFile) JMP_THIS(0x641EE0); // Stupid WestWood Optimization Here
    /// VA: 0x00642130.
    void* CreatePalettedPreview(int nResolution, int& BytesWritten) JMP_THIS(0x642130);
    /// VA: 0x006425F0.
    void CreatePreviewSurface(void* pData) JMP_THIS(0x6425F0);

    DSurface* ImageSurface;
};
