#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

class FileClass;
class CCFileClass;

namespace game {
#ifdef _WIN32
using NativeFileHandle = void*;
inline NativeFileHandle invalid_file_handle() noexcept { return reinterpret_cast<void*>(-1); }
#else
using NativeFileHandle = int;
inline constexpr NativeFileHandle invalid_file_handle() noexcept { return -1; }
#endif
enum class FileStatus { success, missing, invalid, ambiguous, too_large, io_error };
struct FileResult {
    FileStatus status = FileStatus::success;
    std::uint32_t error = 0;
    explicit operator bool() const noexcept { return status == FileStatus::success; }
};
enum class FileMode : unsigned { read = 1, write = 2, read_write = 3 };
enum class FileCreation { existing, truncate, open_or_create };
enum class FileOrigin : unsigned { begin, current, end };

inline std::filesystem::path path_from_utf8(std::string_view text) {
    return std::filesystem::path(std::u8string(text.begin(), text.end()));
}
inline std::string path_to_utf8(const std::filesystem::path& path) {
    const auto text = path.u8string();
    return {text.begin(), text.end()};
}

// Physical file operations. MIX lookup remains in CCFileClass.
// Handles belong to RawFileClass/callers; this service never owns their lifetime.
class FileSystem {
public:
    FileSystem() = default;
    explicit FileSystem(std::filesystem::path root) : root_(std::move(root)) {}
    const std::filesystem::path& root() const noexcept { return root_; }
    FileResult resolve(const char* name, bool allow_missing_leaf,
        std::filesystem::path& path) const noexcept;
    FileResult open(const char* name, FileMode mode, FileCreation creation,
        NativeFileHandle& handle, unsigned share = 3, bool sequential = false) const noexcept;
    FileResult close(NativeFileHandle handle) const noexcept;
    FileResult read(NativeFileHandle handle, void* data, std::size_t count,
        std::size_t& transferred) const noexcept;
    FileResult write(NativeFileHandle handle, const void* data, std::size_t count,
        std::size_t& transferred) const noexcept;
    FileResult seek(NativeFileHandle handle, std::int64_t offset, FileOrigin origin,
        std::uint64_t& position) const noexcept;
    FileResult size(NativeFileHandle handle, std::uint64_t& bytes) const noexcept;
    FileResult remove(const char* name) const noexcept;
    FileResult get_time(NativeFileHandle handle, std::uint32_t& dos_time) const noexcept;
    FileResult set_time(NativeFileHandle handle, std::uint32_t dos_time) const noexcept;
private:
    std::filesystem::path root_;
};

// Link-selected file state and lifetime. Keep the existing game bindings.
// Original CD preparation before constructing the temporary MIX file.
void prepare_mix_file();
bool file_read_aborted() noexcept;
void clear_file_read_error() noexcept;
// storage belongs to the caller; DestroyFile releases contents, never storage.
CCFileClass* ConstructFile(void* storage, const char* name);
void DestroyFile(CCFileClass* file);

// A scope chooses only physical root/path policy, never an archive registry.
FileSystem& current_file_system() noexcept;
class FileSystemScope {
public:
    explicit FileSystemScope(FileSystem* service) noexcept;
    ~FileSystemScope();
    FileSystemScope(const FileSystemScope&) = delete;
    FileSystemScope& operator=(const FileSystemScope&) = delete;
private:
    FileSystem* previous_;
};
}

// Original-game callers can enter compiler-generated file virtual tables
// directly. Match compat/files/entries.inc failure returns at that boundary.
// Standalone callers retain the core's existing exception contract.
#ifdef RA2_FILES_GAME
#define RA2_FILE_TRY try
#define RA2_FILE_FAILURE(value) catch (...) { return value; }
#else
#define RA2_FILE_TRY
#define RA2_FILE_FAILURE(value)
#endif
