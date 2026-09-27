#include "file_system.hpp"
#if !defined(RA2_FILES_GAME) && !defined(RA2_YRPP_GAME)
#include "yrpp/CCFileClass.h"
#include <new>
#endif
#include <algorithm>
#include <cerrno>
#include <climits>
#include <ctime>
#include <system_error>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace game {
#ifndef RA2_FILES_GAME
void prepare_mix_file() {}
bool file_read_aborted() noexcept { return false; }
void clear_file_read_error() noexcept {}
#ifndef RA2_YRPP_GAME
CCFileClass* ConstructFile(void* storage, const char* name) { return new (storage) CCFileClass(name); }
void DestroyFile(CCFileClass* file) { file->~CCFileClass(); }
#endif
#endif

namespace {
thread_local FileSystem* selected = nullptr;
#if defined(RA2_FILES_GAME) || defined(RA2_YRPP_GAME)
FileSystem original_files;
#endif
FileResult failure(unsigned error) noexcept {
#ifdef _WIN32
    const bool missing = error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
#else
    const bool missing = error == ENOENT || error == ENOTDIR;
#endif
    return {missing ? FileStatus::missing : FileStatus::io_error, error};
}
unsigned last_error() noexcept {
#ifdef _WIN32
    return GetLastError();
#else
    return errno;
#endif
}
std::string upper(std::string text) {
    for (auto& c : text) if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
    return text;
}
}
FileSystem& current_file_system() noexcept {
#if defined(RA2_FILES_GAME) || defined(RA2_YRPP_GAME)
    return original_files;
#else
    static FileSystem fallback;
    return selected ? *selected : fallback;
#endif
}
FileSystemScope::FileSystemScope(FileSystem* service) noexcept : previous_(selected) { selected = service; }
FileSystemScope::~FileSystemScope() { selected = previous_; }

FileResult FileSystem::resolve(const char* name, bool missing_leaf, std::filesystem::path& output) const noexcept {
    if (!name || !*name) return {FileStatus::invalid, ENOENT};
    try {
        std::string text(name);
        std::replace(text.begin(), text.end(), '\\', '/');
        auto relative = path_from_utf8(text);
        auto path = relative.is_absolute() ? relative.root_path() : root_;
        if (path.empty()) path = std::filesystem::current_path();
        auto parts = relative.relative_path();
        for (auto it = parts.begin(); it != parts.end(); ++it) {
            const auto exact = path / *it;
            if (std::filesystem::exists(exact)) { path = exact; continue; }
            if (!std::filesystem::is_directory(path)) return {FileStatus::missing, ENOENT};
            std::filesystem::path found;
            const auto wanted = upper(path_to_utf8(*it));
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                if (upper(path_to_utf8(entry.path().filename())) != wanted) continue;
                if (!found.empty()) return {FileStatus::ambiguous, EEXIST};
                found = entry.path();
            }
            if (found.empty()) {
                if (!missing_leaf || std::next(it) != parts.end()) return {FileStatus::missing, ENOENT};
                found = exact;
            }
            path = std::move(found);
        }
        output = std::move(path);
        return {};
    } catch (const std::filesystem::filesystem_error& e) { return failure(e.code().value()); }
      catch (...) { return {FileStatus::io_error, ENOMEM}; }
}

FileResult FileSystem::open(const char* name, FileMode mode, FileCreation creation,
    NativeFileHandle& handle, unsigned share, bool sequential) const noexcept {
    handle = invalid_file_handle();
    if (!name || !*name) return {FileStatus::invalid, ENOENT};
    if (unsigned(mode) < 1 || unsigned(mode) > 3) return {FileStatus::invalid, EINVAL};
#if defined(_WIN32) && (defined(RA2_FILES_GAME) || defined(RA2_YRPP_GAME))
    const DWORD access = (unsigned(mode) & 1 ? GENERIC_READ : 0) | (unsigned(mode) & 2 ? GENERIC_WRITE : 0);
    const DWORD disposition = creation == FileCreation::existing ? OPEN_EXISTING : creation == FileCreation::truncate ? CREATE_ALWAYS : OPEN_ALWAYS;
    handle = CreateFileA(name, access, share, nullptr, disposition,
        FILE_ATTRIBUTE_NORMAL | (sequential ? FILE_FLAG_SEQUENTIAL_SCAN : 0), nullptr);
#else
    std::filesystem::path path;
    if (auto r = resolve(name, creation != FileCreation::existing, path); !r) return r;
#ifdef _WIN32
    const DWORD access = (unsigned(mode) & 1 ? GENERIC_READ : 0) | (unsigned(mode) & 2 ? GENERIC_WRITE : 0);
    const DWORD disposition = creation == FileCreation::existing ? OPEN_EXISTING : creation == FileCreation::truncate ? CREATE_ALWAYS : OPEN_ALWAYS;
    handle = CreateFileW(path.c_str(), access, share, nullptr, disposition,
        FILE_ATTRIBUTE_NORMAL | (sequential ? FILE_FLAG_SEQUENTIAL_SCAN : 0), nullptr);
#else
    (void)share; (void)sequential;
    int flags = mode == FileMode::read ? O_RDONLY : mode == FileMode::write ? O_WRONLY : O_RDWR;
    if (creation != FileCreation::existing) flags |= O_CREAT;
    if (creation == FileCreation::truncate) flags |= O_TRUNC;
    do { handle = ::open(path.c_str(), flags, 0666); } while (handle < 0 && errno == EINTR);
    if (handle >= 0) {
        struct stat info{};
        if (::fstat(handle, &info) || !S_ISREG(info.st_mode)) {
            const auto error = errno ? errno : EISDIR;
            ::close(handle); handle = invalid_file_handle(); return failure(error);
        }
    }
#endif
#endif
    return handle == invalid_file_handle() ? failure(last_error()) : FileResult{};
}
FileResult FileSystem::close(NativeFileHandle handle) const noexcept {
#ifdef _WIN32
    return CloseHandle(handle) ? FileResult{} : failure(last_error());
#else
    return ::close(handle) == 0 ? FileResult{} : failure(last_error());
#endif
}
FileResult FileSystem::read(NativeFileHandle handle, void* data, std::size_t count, std::size_t& actual) const noexcept {
    actual = 0;
    if ((!data && count) || count > INT32_MAX) return {FileStatus::invalid, EINVAL};
#ifdef _WIN32
    DWORD n = 0; const bool ok = ReadFile(handle, data, DWORD(count), &n, nullptr) != 0;
    actual = n; return ok ? FileResult{} : failure(last_error());
#else
    ssize_t n; do { n = ::read(handle, data, count); } while (n < 0 && errno == EINTR);
    if (n < 0) return failure(last_error());
    actual = n; return {};
#endif
}
FileResult FileSystem::write(NativeFileHandle handle, const void* data, std::size_t count, std::size_t& actual) const noexcept {
    actual = 0;
    if ((!data && count) || count > INT32_MAX) return {FileStatus::invalid, EINVAL};
#ifdef _WIN32
    DWORD n = 0; const bool ok = WriteFile(handle, data, DWORD(count), &n, nullptr) != 0;
    actual = n; return ok ? FileResult{} : failure(last_error());
#else
    ssize_t n; do { n = ::write(handle, data, count); } while (n < 0 && errno == EINTR);
    if (n < 0) return failure(last_error());
    actual = n; return {};
#endif
}
FileResult FileSystem::seek(NativeFileHandle handle, std::int64_t offset, FileOrigin origin, std::uint64_t& position) const noexcept {
    if (unsigned(origin) > 2) return {FileStatus::invalid, EINVAL};
#ifdef _WIN32
    LARGE_INTEGER distance, result; distance.QuadPart = offset;
    if (!SetFilePointerEx(handle, distance, &result, DWORD(origin))) return failure(last_error());
    position = result.QuadPart;
#else
    const auto result = ::lseek(handle, offset, int(origin));
    if (result < 0) return failure(last_error());
    position = result;
#endif
    return {};
}
FileResult FileSystem::size(NativeFileHandle handle, std::uint64_t& bytes) const noexcept {
#ifdef _WIN32
    LARGE_INTEGER result;
    if (!GetFileSizeEx(handle, &result)) return failure(last_error());
    bytes = result.QuadPart;
#else
    struct stat info{};
    if (::fstat(handle, &info)) return failure(last_error());
    bytes = info.st_size;
#endif
    return {};
}
FileResult FileSystem::remove(const char* name) const noexcept {
#if defined(_WIN32) && (defined(RA2_FILES_GAME) || defined(RA2_YRPP_GAME))
    return DeleteFileA(name) ? FileResult{} : failure(last_error());
#else
    std::filesystem::path path;
    if (auto r = resolve(name, false, path); !r) return r;
#ifdef _WIN32
    return DeleteFileW(path.c_str()) ? FileResult{} : failure(last_error());
#else
    return ::unlink(path.c_str()) == 0 ? FileResult{} : failure(last_error());
#endif
#endif
}
FileResult FileSystem::get_time(NativeFileHandle handle, std::uint32_t& packed) const noexcept {
#ifdef _WIN32
    BY_HANDLE_FILE_INFORMATION info; WORD date, time;
    if (!GetFileInformationByHandle(handle, &info) || !FileTimeToDosDateTime(&info.ftLastWriteTime, &date, &time)) return failure(last_error());
    packed = (std::uint32_t(date) << 16) | time;
#else
    struct stat info; std::tm parts{};
    if (::fstat(handle, &info)) return failure(last_error());
    if (!::gmtime_r(&info.st_mtime, &parts) || parts.tm_year < 80 || parts.tm_year > 207) return {FileStatus::invalid, EINVAL};
    packed = ((std::uint32_t(parts.tm_year - 80) << 9 | std::uint32_t(parts.tm_mon + 1) << 5 | std::uint32_t(parts.tm_mday)) << 16)
        | std::uint32_t(parts.tm_hour) << 11 | std::uint32_t(parts.tm_min) << 5 | std::uint32_t(parts.tm_sec) / 2;
#endif
    return {};
}
FileResult FileSystem::set_time(NativeFileHandle handle, std::uint32_t packed) const noexcept {
#ifdef _WIN32
    BY_HANDLE_FILE_INFORMATION info; FILETIME value;
    if (!GetFileInformationByHandle(handle, &info) || !DosDateTimeToFileTime(WORD(packed >> 16), WORD(packed), &value)
        || !SetFileTime(handle, &info.ftCreationTime, &value, &value)) return failure(last_error());
#else
    std::tm parts{};
    parts.tm_year = int(packed >> 25 & 127) + 80; parts.tm_mon = int(packed >> 21 & 15) - 1;
    parts.tm_mday = int(packed >> 16 & 31); parts.tm_hour = int(packed >> 11 & 31);
    parts.tm_min = int(packed >> 5 & 63); parts.tm_sec = int(packed & 31) * 2;
    const auto expected = parts; const auto time = ::timegm(&parts);
    if (time == std::time_t(-1) || parts.tm_year != expected.tm_year || parts.tm_mon != expected.tm_mon || parts.tm_mday != expected.tm_mday
        || parts.tm_hour != expected.tm_hour || parts.tm_min != expected.tm_min || parts.tm_sec != expected.tm_sec) return {FileStatus::invalid, EINVAL};
    const timespec times[] = {{time, 0}, {time, 0}};
    if (::futimens(handle, times)) return failure(last_error());
#endif
    return {};
}
}
