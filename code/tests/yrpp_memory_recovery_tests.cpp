#include "support/test_support.hpp"
#include "yrpp/Memory.h"
#include "yrpp/FileClass.h"
#include "memory_testing.hpp"
#include "filesystem/file_names.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

int notifications = 0, permissions = 0, mode_reads = 0;
bool mode = false, fail_retry = false, change_mode = false;
std::size_t requested = 0, start_attempts = 0;
bool YRPP_CDECL recover(std::size_t size) noexcept {
    ++notifications;
    requested = size;
    if (change_mode) mode = false;
    if (fail_retry) game::fail_memory_allocation_after(1);
    return notifications <= permissions;
}
bool YRPP_CDECL read_mode() noexcept { ++mode_reads; return mode; }
void reset(int allowed = 1) {
    notifications = mode_reads = 0;
    permissions = allowed;
    mode = fail_retry = change_mode = false;
    requested = 0;
    start_attempts = game::memory_allocation_attempts();
    game::fail_memory_allocation_after(1);
}
void release(void* memory) {
    EXPECT_TRUE((memory != nullptr)) << "recovery must return a real CRT allocation";
    YRMemory::Deallocate(memory);
}
void observed(std::size_t attempts, int calls, std::size_t size) {
    EXPECT_TRUE((game::memory_allocation_attempts() - start_attempts == attempts)) << "allocation attempt count";
    EXPECT_TRUE((notifications == calls && requested == size)) << "notification count and original request size";
}
class File final : public FileClass {
public:
    int reads = 0;
    const char* GetFileName() const override { return "fixture"; }
    const char* SetFileName(const char*) override { return GetFileName(); }
    BOOL CreateFile() override { return 0; }
    BOOL DeleteFile() override { return 0; }
    bool Exists(bool) override { return true; }
    bool HasHandle() override { return false; }
    bool Open(FileAccessMode) override { return false; }
    bool OpenEx(const char*, FileAccessMode) override { return false; }
    int ReadBytes(void* buffer, int count) override {
        ++reads;
        EXPECT_TRUE((count == 5)) << "ReadWholeFile retains original size";
        std::memcpy(buffer, "data", 5);
        return count;
    }
    int Seek(int, FileSeekMode) override { return 0; }
    int GetFileSize() override { return 5; }
    int WriteBytes(void*, int) override { return 0; }
    void Close() override {}
    void CDCheck(DWORD, bool, const char*) override {}
};
void file_recovery() {
    File file;
    reset();
    auto* data = file.ReadWholeFile();
    EXPECT_TRUE((data && !std::memcmp(data, "data", 5) && file.reads == 1)) << "real FileClass resumes Read after recovery";
    release(data);
    observed(2, 1, 5);
    reset(0);
    EXPECT_TRUE((!file.ReadWholeFile() && file.reads == 1)) << "real FileClass returns null without Read when recovery declines";
    observed(1, 1, 5);
}
void core_contracts() {
    reset();
    EXPECT_TRUE((!YRMemory::Allocate(17))) << "standalone default has no recovery callback";
    observed(1, 0, 0);
    EXPECT_TRUE((!YRMemory::ConfigureFailureRecovery(recover, read_mode, 0))) << "zero limit rejected";
    EXPECT_TRUE((YRMemory::ConfigureFailureRecovery(recover, read_mode, 128))) << "register notifications, no allocator";
    EXPECT_TRUE((YRMemory::ConfigureFailureRecovery(recover, read_mode, 128))) << "same configuration is idempotent";
    EXPECT_TRUE((!YRMemory::ConfigureFailureRecovery(nullptr, read_mode, 128))) << "different recovery rejected";
    EXPECT_TRUE((!YRMemory::ConfigureFailureRecovery(recover, nullptr, 128))) << "different mode query rejected";
    EXPECT_TRUE((!YRMemory::ConfigureFailureRecovery(recover, read_mode, 127))) << "different limit rejected";
    file_recovery();

    reset(); release(YRMemory::Allocate(0)); observed(2, 1, 0);
    EXPECT_TRUE((!mode_reads)) << "New-style allocation ignores new-mode";
    reset(); release(YRMemory::AllocateChecked(37)); observed(2, 1, 37);
    reset();
    EXPECT_TRUE((!YRMemory::AllocateBytes(17))) << "malloc with new-mode off returns null"; observed(1, 0, 0);
    EXPECT_TRUE((mode_reads == 1)) << "malloc reads new-mode at entry";
    reset(2); mode = fail_retry = change_mode = true;
    EXPECT_TRUE((!YRMemory::AllocateBytes(17))) << "malloc returns null after handler eventually declines";
    observed(3, 3, 17);
    EXPECT_TRUE((mode_reads == 1)) << "malloc does not re-read new-mode between retries";
    reset(); mode = true; release(game::DuplicateName("name")); observed(2, 1, 5);
    reset(); EXPECT_TRUE((!game::DuplicateName("name"))) << "strdup-style name respects new-mode off"; observed(1, 0, 0);

    reset(); mode = true;
    EXPECT_TRUE((!YRMemory::AllocateOnce(19))) << "raw allocation never notifies recovery"; observed(1, 0, 0);
    reset(); mode = true;
    EXPECT_TRUE((!YRMemory::AllocateZeroed(19))) << "raw calloc never notifies recovery"; observed(1, 0, 0);
    reset(); mode = true;
    EXPECT_TRUE((!YRMemory::Reallocate(nullptr, 19))) << "null raw realloc makes only one raw attempt"; observed(1, 0, 0);
    auto* block = YRMemory::AllocateOnce(8);
    EXPECT_TRUE((block != nullptr)) << "initial realloc block"; std::memset(block, 0x5a, 8);
    reset(); mode = true;
    EXPECT_TRUE((!YRMemory::Reallocate(block, 19))) << "non-null raw realloc never notifies recovery"; observed(1, 0, 0);
    for (int i = 0; i < 8; ++i) EXPECT_TRUE((static_cast<unsigned char*>(block)[i] == 0x5a)) << "failed realloc retains data";
    release(block);
    reset(); mode = true;
    EXPECT_TRUE((!YRMemory::Allocate(129) && !YRMemory::AllocateBytes(129))) << "policy rejects oversized requests before CRT or handler";
    observed(0, 0, 0);
    game::fail_memory_allocation_after(0);
    release(YRMemory::AllocateOnce(129)); // Raw operation is not subject to New's limit.
    release(YRMemory::Allocate(128));
    EXPECT_TRUE((game::outstanding_memory_allocations() == 0)) << "recovery ownership is balanced";
}
void verify_checked_exit() {
    // _Exit avoids recursion through atexit if the checked path regresses.
    if (notifications != 2 || requested != 37 || game::memory_allocation_attempts() - start_attempts != 2)
        std::_Exit(99);
}
}

TEST(YrppMemoryRecovery, PolicyAndOwnership) {
    core_contracts();
}

namespace {
std::optional<int> legacy_command(int argc, char** argv) {
    if (argc == 2 && !std::strcmp(argv[1], "--checked-failure")) {
            EXPECT_TRUE((YRMemory::ConfigureFailureRecovery(recover, read_mode, 128))) << "checked recovery configuration";
            reset(1); fail_retry = true;
            std::atexit(verify_checked_exit);
            YRMemory::AllocateChecked(37);
            return 1;
        }
    return std::nullopt;
}
const ra2::test::CommandRegistration command(legacy_command);
}
