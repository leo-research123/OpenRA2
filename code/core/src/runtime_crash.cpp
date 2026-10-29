#include "runtime_crash.hpp"
#include <atomic>
#include <cstdio>
#include <cstring>
#include <filesystem>
#if !defined(_WIN32)
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <spawn.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <sys/ucontext.h>
#else
#include <ucontext.h>
#endif
#include <dlfcn.h>
extern char** environ;
#endif
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <CommonCrypto/CommonDigest.h>
#endif

namespace game {
struct CrashContext {
    std::atomic<std::uint64_t> sequence{0}, world{0}, tick{0};
};
static CrashContext context;
static std::atomic<std::uint64_t> snapshots[8][7]{};
static std::atomic<std::uint64_t> published{0};
static std::atomic_flag publishing = ATOMIC_FLAG_INIT;
static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
static_assert(std::atomic<std::uint32_t>::is_always_lock_free);
// Versioned, fixed-width little-endian record; register slots follow platform ABI.
struct CrashRecord {
    char magic[8];
    std::uint32_t version, size, signal, signal_code, architecture, register_count;
    std::uint64_t fault_address, process, sequence, world, tick, subject;
    std::uint32_t phase, frame, id, reserved;
    std::uint64_t registers[34];
};
static char session_directory[4096]{};
#if !defined(_WIN32)
static int fatal_file = -1, request_pipe = -1, response_pipe = -1;
static pid_t reporter_process = -1;
static bool installed = false;
static std::atomic_flag handling = ATOMIC_FLAG_INIT;
static stack_t previous_stack{};
alignas(16) static unsigned char emergency_stack[65536];
static bool owns_stack = false;
static constexpr int fatal_signals[]{SIGABRT, SIGSEGV, SIGBUS, SIGILL, SIGFPE};
static struct sigaction previous[5];
static void fatal_handler(int signal, siginfo_t* info, void* raw) {
    if (!handling.test_and_set(std::memory_order_relaxed)) {
        CrashRecord record{};
        const char magic[8]{'R', 'A', '2', 'C', 'R', 'S', 'H', '2'};
        for (unsigned i = 0; i < 8; ++i)
            record.magic[i] = magic[i];
        record.version = 2;
        record.size = sizeof(record);
        record.signal = signal;
        record.signal_code = info ? info->si_code : 0;
        record.fault_address = info ? reinterpret_cast<std::uintptr_t>(info->si_addr) : 0;
        record.process = getpid();
        // Atomic slots prevent a data race when a worker faults during a main-thread checkpoint.
        for (unsigned attempt = 0; attempt < 8; ++attempt) {
            const auto token = published.load(std::memory_order_acquire);
            auto* saved = snapshots[token % 8];
            record.sequence = saved[0].load(std::memory_order_relaxed);
            record.world = saved[1].load(std::memory_order_relaxed);
            record.tick = saved[2].load(std::memory_order_relaxed);
            record.subject = saved[3].load(std::memory_order_relaxed);
            record.phase = static_cast<std::uint32_t>(saved[4].load(std::memory_order_relaxed));
            record.frame = static_cast<std::uint32_t>(saved[5].load(std::memory_order_relaxed));
            record.id = static_cast<std::uint32_t>(saved[6].load(std::memory_order_relaxed));
            record.reserved = published.load(std::memory_order_acquire) != token;
            if (!record.reserved) break;
        }
        auto* machine = static_cast<ucontext_t*>(raw);
#if defined(__APPLE__) && defined(__aarch64__)
        record.architecture = 1;
        record.register_count = 34;
        if (machine && machine->uc_mcontext) {
            const auto& state = machine->uc_mcontext->__ss;
            for (unsigned i = 0; i < 29; ++i)
                record.registers[i] = state.__x[i];
            record.registers[29] = state.__fp;
            record.registers[30] = state.__lr;
            record.registers[31] = state.__sp;
            record.registers[32] = state.__pc;
            record.registers[33] = state.__cpsr;
        }
#elif defined(__APPLE__) && defined(__x86_64__)
        record.architecture = 2;
        record.register_count = 21;
        if (machine && machine->uc_mcontext) {
            const auto& state = machine->uc_mcontext->__ss;
            const std::uint64_t values[]{
                state.__rax, state.__rbx, state.__rcx, state.__rdx,    state.__rdi, state.__rsi, state.__rbp,
                state.__rsp, state.__r8,  state.__r9,  state.__r10,    state.__r11, state.__r12, state.__r13,
                state.__r14, state.__r15, state.__rip, state.__rflags, state.__cs,  state.__fs,  state.__gs};
            for (unsigned i = 0; i < 21; ++i)
                record.registers[i] = values[i];
        }
#elif defined(__linux__) && defined(__x86_64__)
        record.architecture = 3;
        record.register_count = NGREG;
        if (machine)
            for (unsigned i = 0; i < NGREG && i < 34; ++i)
                record.registers[i] = machine->uc_mcontext.gregs[i];
#endif
        // No allocation, stdio, log lock, symbolization or object dereference here.
        if (fatal_file >= 0) {
            const auto* data = reinterpret_cast<const char*>(&record);
            std::size_t left = sizeof(record);
            while (left) {
                const auto count = write(fatal_file, data, left);
                if (count > 0) {
                    data += count;
                    left -= static_cast<std::size_t>(count);
                } else if (count < 0 && errno == EINTR)
                    continue;
                else
                    break;
            }
        }
        if (request_pipe >= 0) {
            const unsigned char request = 1;
            if (write(request_pipe, &request, 1) == 1 && response_pipe >= 0) {
                fd_set descriptors;
                FD_ZERO(&descriptors);
                FD_SET(response_pipe, &descriptors);
                timeval timeout{4, 0};
                // The reporter runs in a different process, even if this thread held a lock.
                if (select(response_pipe + 1, &descriptors, nullptr, nullptr, &timeout) > 0) {
                    unsigned char response;
                    (void)read(response_pipe, &response, 1);
                }
            }
        }
    }
    for (unsigned i = 0; i < 5; ++i)
        if (fatal_signals[i] == signal) {
            if (previous[i].sa_handler == SIG_IGN) {
                struct sigaction action{};
                action.sa_handler = SIG_DFL;
                sigemptyset(&action.sa_mask);
                sigaction(signal, &action, nullptr);
            } else
                sigaction(signal, &previous[i], nullptr);
            break;
        }
    // Re-deliver after restoring ownership to the host. Returning resumes its fatal dispatch.
    kill(getpid(), signal);
}
#endif
void runtime_crash_checkpoint(RuntimePhase phase, int frame, std::uintptr_t subject, std::uint32_t id) noexcept {
    if (publishing.test_and_set(std::memory_order_acquire)) return;
    const auto token = published.load(std::memory_order_relaxed) + 1;
    auto* saved = snapshots[token % 8];
    saved[0].store(context.sequence.load(std::memory_order_acquire), std::memory_order_relaxed);
    saved[1].store(context.world.load(std::memory_order_relaxed), std::memory_order_relaxed);
    saved[2].store(context.tick.load(std::memory_order_relaxed), std::memory_order_relaxed);
    saved[3].store(subject, std::memory_order_relaxed);
    saved[4].store(static_cast<unsigned>(phase), std::memory_order_relaxed);
    saved[5].store(static_cast<std::uint32_t>(frame), std::memory_order_relaxed);
    saved[6].store(id, std::memory_order_relaxed);
    published.store(token, std::memory_order_release);
    publishing.clear(std::memory_order_release);
}
void runtime_crash_sequence(std::uint64_t sequence, std::uint64_t world, std::uint64_t tick) noexcept {
    context.world.store(world, std::memory_order_relaxed);
    context.tick.store(tick, std::memory_order_relaxed);
    context.sequence.store(sequence, std::memory_order_release);
}
void stop_runtime_crash_capture() noexcept {
#if !defined(_WIN32)
    if (installed)
        for (unsigned i = 0; i < 5; ++i) {
            struct sigaction current{};
            sigaction(fatal_signals[i], nullptr, &current);
            if (current.sa_sigaction == fatal_handler) sigaction(fatal_signals[i], &previous[i], nullptr);
        }
    installed = false;
    if (owns_stack) {
        sigaltstack(&previous_stack, nullptr);
        owns_stack = false;
    }
    if (request_pipe >= 0) close(request_pipe);
    request_pipe = -1;
    if (response_pipe >= 0) close(response_pipe);
    response_pipe = -1;
    if (reporter_process > 0) {
        // An idle reporter receives EOF; bound shutdown and reap without waiting indefinitely.
        for (unsigned i = 0; i < 50; ++i) {
            if (waitpid(reporter_process, nullptr, WNOHANG) != 0) {
                reporter_process = -1;
                break;
            }
            usleep(1000);
        }
        if (reporter_process > 0) {
            kill(reporter_process, SIGTERM);
            (void)waitpid(reporter_process, nullptr, 0);
            reporter_process = -1;
        }
    }
#endif
}
bool prepare_runtime_crash_session(const char* directory) noexcept {
    stop_runtime_crash_capture();
#if !defined(_WIN32)
    if (fatal_file >= 0) close(fatal_file);
    fatal_file = -1;
#endif
    session_directory[0] = 0;
    if (!directory) return true;
    if (std::strlen(directory) >= sizeof(session_directory)) return false;
    try {
        std::snprintf(session_directory, sizeof(session_directory), "%s", directory);
        const auto path = std::filesystem::u8path(directory) / "native-fatal.bin";
#if !defined(_WIN32)
        fatal_file = open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
        if (fatal_file < 0) return false;
#endif
        const auto modules = std::filesystem::u8path(directory) / "modules.txt";
        auto* file = std::fopen(modules.string().c_str(), "wb");
        if (!file) return false;
#if defined(__APPLE__)
        for (std::uint32_t i = 0; i < _dyld_image_count(); ++i) {
            const auto* header = _dyld_get_image_header(i);
            const auto* name = _dyld_get_image_name(i);
            const auto* command = reinterpret_cast<const load_command*>(
                reinterpret_cast<const char*>(header) +
                (header->magic == MH_MAGIC_64 ? sizeof(mach_header_64) : sizeof(mach_header)));
            char uuid[33]{};
            for (std::uint32_t n = 0; n < header->ncmds; ++n) {
                if (command->cmd == LC_UUID) {
                    const auto* value = reinterpret_cast<const uuid_command*>(command);
                    for (unsigned j = 0; j < 16; ++j)
                        std::snprintf(uuid + j * 2, 3, "%02x", value->uuid[j]);
                }
                command =
                    reinterpret_cast<const load_command*>(reinterpret_cast<const char*>(command) + command->cmdsize);
            }
            std::fprintf(file, "base=0x%llX slide=0x%llX uuid=%s path=%s\n",
                         static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(header)),
                         static_cast<unsigned long long>(_dyld_get_image_vmaddr_slide(i)), uuid, name ? name : "");
            // Hash the game binary and host on the healthy startup path, never at a fault.
            if (name && (std::strstr(name, "ra2_core") || std::strstr(name, "Godot") || i == 0)) {
                if (auto* binary = std::fopen(name, "rb")) {
                    CC_SHA256_CTX hash;
                    CC_SHA256_Init(&hash);
                    char bytes[65536];
                    std::size_t count;
                    while ((count = std::fread(bytes, 1, sizeof(bytes), binary)))
                        CC_SHA256_Update(&hash, bytes, static_cast<CC_LONG>(count));
                    unsigned char result[CC_SHA256_DIGEST_LENGTH];
                    CC_SHA256_Final(result, &hash);
                    std::fclose(binary);
                    std::fprintf(file, "sha256=");
                    for (auto value : result)
                        std::fprintf(file, "%02x", value);
                    std::fprintf(file, " path=%s\n", name);
                }
            }
        }
#endif
        const bool ok = std::fclose(file) == 0;
#if !defined(_WIN32)
        handling.clear(std::memory_order_relaxed);
#endif
        return ok;
    } catch (...) {
        return false;
    }
}
bool configure_runtime_crash_capture(const char* reporter_path) noexcept {
#if !defined(_WIN32)
    if (installed) return true;
    if (fatal_file < 0) return false;
    try {
#if defined(__APPLE__)
        std::filesystem::path reporter;
        if (reporter_path && *reporter_path)
            reporter = std::filesystem::u8path(reporter_path);
        else {
            Dl_info library{};
            if (dladdr(reinterpret_cast<void*>(&configure_runtime_crash_capture), &library) && library.dli_fname)
                reporter = std::filesystem::u8path(library.dli_fname).parent_path() / "ra2_crash_reporter";
        }
        if (!reporter.empty() && access(reporter.c_str(), X_OK) == 0) {
            int requests[2], responses[2];
            if (pipe(requests) == 0) {
                if (pipe(responses) == 0) {
                    for (int descriptor : {requests[0], requests[1], responses[0], responses[1]})
                        fcntl(descriptor, F_SETFD, FD_CLOEXEC);
                    posix_spawn_file_actions_t actions;
                    posix_spawn_file_actions_init(&actions);
                    posix_spawn_file_actions_adddup2(&actions, requests[0], 300);
                    posix_spawn_file_actions_adddup2(&actions, responses[1], 301);
                    char pid[32];
                    std::snprintf(pid, sizeof(pid), "%d", getpid());
                    const auto executable = reporter.string();
                    const auto output = (std::filesystem::u8path(session_directory) / "native-threads.txt").string();
                    char* arguments[]{const_cast<char*>(executable.c_str()), pid, const_cast<char*>(output.c_str()),
                                      nullptr};
                    const int error =
                        posix_spawn(&reporter_process, executable.c_str(), &actions, nullptr, arguments, environ);
                    posix_spawn_file_actions_destroy(&actions);
                    close(requests[0]);
                    close(responses[1]);
                    if (!error) {
                        request_pipe = requests[1];
                        response_pipe = responses[0];
                        fcntl(request_pipe, F_SETNOSIGPIPE, 1);
                    } else {
                        reporter_process = -1;
                        close(requests[1]);
                        close(responses[0]);
                    }
                } else {
                    close(requests[0]);
                    close(requests[1]);
                }
            }
        }
#else
        (void)reporter_path;
#endif
        if (sigaltstack(nullptr, &previous_stack) == 0 && (previous_stack.ss_flags & SS_DISABLE)) {
            stack_t stack{};
            stack.ss_sp = emergency_stack;
            stack.ss_size = sizeof(emergency_stack);
            owns_stack = sigaltstack(&stack, nullptr) == 0;
        }
        struct sigaction action{};
        action.sa_sigaction = fatal_handler;
        sigemptyset(&action.sa_mask);
        action.sa_flags = SA_SIGINFO | SA_ONSTACK;
        unsigned count = 0;
        for (; count < 5; ++count)
            if (sigaction(fatal_signals[count], &action, &previous[count]) != 0) break;
        if (count != 5) {
            while (count) {
                --count;
                sigaction(fatal_signals[count], &previous[count], nullptr);
            }
            stop_runtime_crash_capture();
            return false;
        }
        installed = true;
        return true;
    } catch (...) {
        stop_runtime_crash_capture();
        return false;
    }
#else
    (void)reporter_path;
    return false;
#endif
}
}
