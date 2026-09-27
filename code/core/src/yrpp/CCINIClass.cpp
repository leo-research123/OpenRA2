// YRpp 9402d7da interfaces; original 4741F0 FileStraw wrapper calibrated.
#include "yrpp/CCINIClass.h"
#include "yrpp/Straws.h"
#include "yrpp/Pipes.h"
#include <memory>
#include <stdexcept>
#include <cstring>
#include <cwchar>
#include <algorithm>
#include "ini_runtime.hpp"

CCINIClass::CCINIClass() = default;
CCINIClass::~CCINIClass() = default;
int CCINIClass::ReadCCFile(FileClass* file, bool digest, bool loadComments) {
    if (!file) return 0;
    FileStraw input(*file);
    const int result = ReadStraw(input, loadComments);
    Digested = false;
    if (result && digest) {
        byte expected[20]{};
        const size_t length = ReadUUBlock("Digest", expected, sizeof(expected));
        if (!length) return 2;
        Clear("Digest");
        CalculateDigest();
        if (length != sizeof(expected) || std::memcmp(expected, Digest, sizeof(expected))) return 2;
    }
    return result;
}
int CCINIClass::WriteCCFile(FileClass* file, bool digest) {
    if (!file) throw std::invalid_argument("INI output file is null");
    const bool opened = !file->HasHandle();
    if (opened && !file->Open(FileAccessMode::Write))
        throw std::runtime_error("cannot open INI output file");
    FilePipe output(*file);
    // This wrapper opened the file above; transfer that lifetime to FilePipe.
    output.HasOpened = opened;
    if (!digest) return WritePipe(output);
    Clear("Digest");
    CalculateDigest();
    struct RemoveDigest {
        CCINIClass& ini;
        ~RemoveDigest() { ini.Clear("Digest"); }
    } remove{*this};
    if (!WriteUUBlock("Digest", Digest, sizeof(Digest)))
        throw std::runtime_error("cannot serialize INI digest");
    return WritePipe(output);
}
int CCINIClass::LoadFromFile(const char* filename, bool loadComments) {
    CCFileClass file(filename);
    return file.Exists() ? ReadCCFile(&file, false, loadComments) : 0;
}
CCINIClass* CCINIClass::LoadINIFile(const char* filename) {
    auto result = std::make_unique<CCINIClass>();
    result->LoadFromFile(filename);
    return result.release();
}
void CCINIClass::UnloadINIFile(CCINIClass*& ini) { delete ini; ini = nullptr; }

int CCINIClass::ReadStringtableEntry(const char* section, const char* key, wchar_t* output, size_t capacity) {
    if (!output || !capacity) return 0;
    char label[1024]{};
    ReadString(section, key, "", label, sizeof(label));
    const auto& runtime = game::ini_runtime();
    const wchar_t* source = nullptr;
    if (!runtime.stringtable(runtime.context, label, source) || !source)
        throw std::runtime_error("INI StringTable lookup failed");
    const size_t count = std::min(std::wcslen(source), capacity - 1);
    std::memmove(output, source, count * sizeof(wchar_t));
    output[count] = 0;
    return static_cast<int>(count);
}
