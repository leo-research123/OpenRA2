#include "filesystem/file_system.hpp"
// Copyright 2020 Electronic Arts Inc. See third_party/ea/LICENSE.TXT.
// REDALERT/CDFILE.CPP, f1f0d42bc2dcd06d5d1df943c6150ab34bf307ae.
// YR 47AE10/47AAB0/47AF10 retain the search order; physical root is a platform policy.
#include "yrpp/CCFileClass.h"
#include "filesystem/search_paths.hpp"
#include <string>

const char* CDFileClass::SetFileName(const char* filename) RA2_FILE_TRY {
    BufferIOFileClass::SetFileName(filename);
    if (!filename || IsDisabled || !game::file_search_paths() || BufferIOFileClass::Exists(false)) return this->GetFileName();
    for (auto* search = game::file_search_paths(); search; search = search->next) {
        const std::string path = std::string(search->path) + filename;
        BufferIOFileClass::SetFileName(path.c_str());
        if (BufferIOFileClass::Exists(false)) return this->GetFileName();
    }
    BufferIOFileClass::SetFileName(filename);
    return this->GetFileName();
} RA2_FILE_FAILURE(nullptr)
bool CDFileClass::Open(FileAccessMode access) RA2_FILE_TRY { return BufferIOFileClass::Open(access); } RA2_FILE_FAILURE(0)
bool CDFileClass::OpenEx(const char* filename, FileAccessMode access) RA2_FILE_TRY {
    CDFileClass::SetFileName(filename);
    return this->Open(access);
} RA2_FILE_FAILURE(0)

CDFileClass::CDFileClass() { }
CDFileClass::~CDFileClass() { }
