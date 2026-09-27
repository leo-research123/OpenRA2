// YRpp FileClass::ReadWholeFile, calibrated against gamemd 0x4A3890.
// YRpp upstream CCFileClass.h only provided an address jump, not this body.
#include "yrpp/Memory.h"
#include "yrpp/FileClass.h"

void* FileClass::ReadWholeFile() {
    // Preserve Exists -> Size -> Allocate -> Read. The original does not reject
    // zero sizes or use the Read return value to discard a short-read buffer.
    if (!this->Exists(false)) return nullptr;
    const int size = this->GetFileSize();
    void* data = YRMemory::Allocate(static_cast<std::size_t>(size));
    if (data) {
        try { this->ReadBytes(data, size); }
        catch (...) { YRMemory::Deallocate(data); throw; }
    }
    return data;
}

// Non-template interface helpers; bodies retained from the corresponding header.

FileClass::FileClass() : SkipCDCheck(false), padding_5{}
{}

DWORD FileClass::GetFileTime()
{ return 0; }

bool FileClass::SetFileTime(DWORD)
{ return false; }
