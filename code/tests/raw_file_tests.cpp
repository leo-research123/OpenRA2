#include "support/test_support.hpp"
#include "yrpp/RawFileClass.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winioctl.h>
#endif

namespace fs = std::filesystem;
namespace {

struct Temp {
    fs::path path = fs::temp_directory_path() / ("ra2-rawfile-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Temp() { fs::create_directories(path); }
    ~Temp() { std::error_code ignored; fs::remove_all(path, ignored); }
    std::string name(const fs::path& leaf) const {
        const auto utf8 = (path / leaf).u8string();
        return {utf8.begin(), utf8.end()};
    }
};
void ownership_and_errors() {
    static_assert(!std::is_copy_constructible_v<RawFileClass>);
    static_assert(!std::is_copy_assignable_v<RawFileClass>);
    Temp temp;
    RawFileClass empty;
    EXPECT_TRUE((!empty.HasHandle() && !empty.GetFileName() && !empty.SkipCDCheck)) << "default object state";
    EXPECT_TRUE((!empty.Open() && !empty.DeleteFile() && empty.GetFileSize() == -1 && empty.Seek(0) == -1)) << "unnamed operations must return failure";
    std::array<char, 4> data{};
    EXPECT_TRUE((empty.ReadBytes(data.data(), 4) == 0 && !empty.HasHandle())) << "failed lazy open";
    auto name = temp.name("file.bin");
    RawFileClass file(name.c_str());
    EXPECT_TRUE((!file.SkipCDCheck)) << "named constructor must initialize the YR base field";
    EXPECT_TRUE((file.GetFileName() == name.c_str())) << "EA constructor must borrow the filename";
    EXPECT_TRUE((!file.Exists() && !file.Exists(true) && !file.Open() && !file.HasHandle())) << "missing file must not be reported available or open";
    EXPECT_TRUE((file.SetFileName(name.c_str()) && file.GetFileName() != name.c_str())) << "SetFileName must own a copy";
    EXPECT_TRUE((file.SetFileName(file.GetFileName()) && name == file.GetFileName())) << "self SetFileName use-after-free";
    EXPECT_TRUE((!file.Open(static_cast<FileAccessMode>(99)) && !file.HasHandle())) << "invalid access mode";
    EXPECT_TRUE((file.CreateFile() && !file.HasHandle())) << "Create must close its handle";
    EXPECT_TRUE((file.Exists() && !file.HasHandle())) << "probe must close temporary handle";
    EXPECT_TRUE((file.Open(FileAccessMode::Read))) << "open existing file";
    EXPECT_TRUE((file.Exists() && file.HasHandle())) << "probe must preserve an existing handle";
    EXPECT_TRUE((file.WriteBytes(data.data(), 1) == 0)) << "write to read-only handle must fail without retrying forever";
    file.Close(); file.Close();
    EXPECT_TRUE((file.SetFileName(nullptr) == nullptr && !file.GetFileName())) << "null name must release owned name";
    EXPECT_TRUE((file.OpenEx(name.c_str(), FileAccessMode::Write))) << "reopen after null name";
    file.Close();
    EXPECT_TRUE((file.DeleteFile() && !file.Exists() && !file.DeleteFile())) << "delete status";
    const auto directory = temp.name(".");
    RawFileClass folder(directory.c_str());
    EXPECT_TRUE((!folder.Exists() && !folder.Exists(true) && !folder.Open())) << "directory must not be opened as a regular file";
}
void io_and_bias() {
    Temp temp;
    const auto name = temp.name(fs::path(u8"中文文件.bin"));
    std::vector<char> payload(100003);
    for (size_t i = 0; i < payload.size(); ++i) payload[i] = char((i * 31) % 251);
    {
        RawFileClass file(name.c_str());
        FileClass& api = file;
        EXPECT_TRUE((api.WriteBytes(payload.data(), int32_t(payload.size())) == int32_t(payload.size()) &&
            !api.HasHandle())) << "lazy write / close";
        EXPECT_TRUE((api.GetFileSize() == int32_t(payload.size()) && !api.HasHandle())) << "lazy size / close";
        std::vector<char> readback(payload.size() + 10);
        EXPECT_TRUE((api.ReadBytes(readback.data(), int32_t(readback.size())) == int32_t(payload.size()) &&
            !api.HasHandle() && std::equal(payload.begin(), payload.end(), readback.begin()))) << "large read, short EOF, UTF-8 path and virtual dispatch";
        EXPECT_TRUE((api.Open(FileAccessMode::Read))) << "open for biased reads";
        file.Bias(17, 11);
        EXPECT_TRUE((file.FilePointer == 17 && api.GetFileSize() == 11 && api.Seek(0) == 0)) << "bias setup";
        std::array<char, 32> part{};
        EXPECT_TRUE((api.ReadBytes(part.data(), 32) == 11 && std::equal(part.begin(), part.begin() + 11,
            payload.begin() + 17) && api.ReadBytes(part.data(), 1) == 0)) << "bias read limit";
        EXPECT_TRUE((api.Seek(-2, FileSeekMode::End) == 9 && api.ReadBytes(part.data(), 32) == 2 &&
            part[0] == payload[26] && part[1] == payload[27])) << "bias FileSeekMode::End";
        EXPECT_TRUE((api.Seek(-15, FileSeekMode::Current) == 0 && api.Seek(100, FileSeekMode::Set) == 11)) << "bias seek clamps";
        file.Bias(3, 4);
        EXPECT_TRUE((file.FilePointer == 20 && api.GetFileSize() == 4 && api.Seek(0) == 0 &&
            api.ReadBytes(part.data(), 32) == 4 && part[0] == payload[20])) << "nested additive bias";
        file.Bias(0);
        EXPECT_TRUE((file.FilePointer == 0 && file.FileSize == -1 && api.Seek(0, FileSeekMode::Set) == 0 &&
            api.GetFileSize() == int32_t(payload.size()))) << "bias reset";
        // Destructor must close this handle and free the SetFileName allocation.
        EXPECT_TRUE((api.SetFileName(name.c_str()))) << "owned name for destructor";
    }
    // An independent reader verifies bytes written by RawFile, without RawFile's read path.
    std::ifstream disk(fs::path(std::u8string(name.begin(), name.end())), std::ios::binary);
    const std::vector<char> actual{std::istreambuf_iterator<char>(disk), std::istreambuf_iterator<char>()};
    EXPECT_TRUE((actual == payload)) << "physical payload differs";
    disk.close();
    RawFileClass file(name.c_str());
    EXPECT_TRUE((file.Open(FileAccessMode::Read | FileAccessMode::Write) && file.GetFileSize() == int(payload.size()))) << "YR read/write mode preserves existing contents";
    EXPECT_TRUE((file.Seek(0, FileSeekMode::End) == int(payload.size()))) << "seek to end before extending";
    char appended[] = "abcd";
    EXPECT_TRUE((file.WriteBytes(appended, 4) == 4 && file.GetFileSize() == int(payload.size()) + 4)) << "write must extend known bias length";
    const uint32_t timestamp = (uint32_t(2024 - 1980) << 25) | (6u << 21) | (15u << 16) |
        (12u << 11) | (34u << 5) | 28u;
    EXPECT_TRUE((file.SetFileTime(timestamp) && file.GetFileTime() == timestamp)) << "DOS timestamp round trip";
    file.Close();
    EXPECT_TRUE((!file.SetFileTime(timestamp) && file.GetFileTime() == 0)) << "closed timestamp semantics";
    EXPECT_TRUE((file.DeleteFile())) << "destructor left a blocking handle";
}
void oversized_file() {
    Temp temp;
    const auto name = temp.name("sparse.bin");
    { std::ofstream file(temp.path / "sparse.bin", std::ios::binary); }
#ifdef _WIN32
    HANDLE sparse = CreateFileW((temp.path / "sparse.bin").c_str(), GENERIC_WRITE, 0,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    DWORD returned = 0;
    const bool marked = sparse != INVALID_HANDLE_VALUE &&
        DeviceIoControl(sparse, FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, &returned, nullptr);
    if (sparse != INVALID_HANDLE_VALUE) CloseHandle(sparse);
    EXPECT_TRUE((marked)) << "cannot mark oversized fixture sparse";
#endif
    fs::resize_file(temp.path / "sparse.bin", uint64_t(UINT32_MAX) + 2);
    RawFileClass file(name.c_str());
    EXPECT_TRUE((file.GetFileSize() == -1 && !file.HasHandle())) << "large file size must not wrap to a small value";
}
}

TEST(RawFile, Contracts) {
    ownership_and_errors(); io_and_bias(); oversized_file();
}
