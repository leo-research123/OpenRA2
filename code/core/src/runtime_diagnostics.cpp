#include "api/runtime_diagnostics.hpp"
#include "runtime_diagnostics.hpp"
#include "runtime_crash.hpp"
#include "api/version.hpp"
#include "yrpp/TagClass.h"
#include "yrpp/TriggerClass.h"
#include "yrpp/TriggerTypeClass.h"
#include "yrpp/Unsorted.h"
#include "yrpp/Randomizer.h"
#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <mutex>
#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif
#ifndef RA2_DIAGNOSTICS_BUILD
#define RA2_DIAGNOSTICS_BUILD "unknown"
#endif

namespace game {
struct RuntimeTagRecord {
    const TagClass* address = nullptr;
    unsigned id = 0;
    bool alive = false;
    int persistence = -1;
    char type[32]{}, trigger[32]{};
};
struct RuntimeLogState {
    std::mutex mutex;
    std::atomic<bool> enabled{false};
    std::FILE* file = nullptr;
    std::FILE* session_file = nullptr;
    char path[4096]{}, resources[1024]{}, map[260]{}, player[32]{}, run[96]{};
    char session_directory[4096]{};
    std::uint64_t sequence = 0, world = 0, tick = 0;
    std::size_t bytes = 0;
    int mode = -1, difficulty = -1;
    RuntimeTagRecord tags[2048]{};
    ~RuntimeLogState() {
        if (file) std::fclose(file);
        if (session_file) std::fclose(session_file);
    }
};
static RuntimeLogState runtime_log;
static constexpr std::size_t runtime_log_limit = 2u * 1024u * 1024u;
#if defined(__aarch64__) || defined(_M_ARM64)
static constexpr const char* runtime_log_arch = "arm64";
#elif defined(__x86_64__) || defined(_M_X64)
static constexpr const char* runtime_log_arch = "x86_64";
#elif defined(__i386__) || defined(_M_IX86)
static constexpr const char* runtime_log_arch = "x86";
#else
static constexpr const char* runtime_log_arch = "unknown";
#endif
static void copy_log_text(char* dest, std::size_t capacity, const char* source) noexcept {
    std::size_t i = 0;
    if (source)
        for (; i + 1 < capacity && source[i]; ++i) {
            const auto c = static_cast<unsigned char>(source[i]);
            dest[i] = c < 0x20 || c == '"' || c == '\\' ? '_' : static_cast<char>(c);
        }
    dest[i] = 0;
}
static void stop_runtime_log() noexcept {
    runtime_log.enabled.store(false, std::memory_order_relaxed);
    if (runtime_log.file) {
        std::fclose(runtime_log.file);
        runtime_log.file = nullptr;
    }
}
static bool write_runtime_line(const char* line, std::size_t size) noexcept {
    if (runtime_log.session_file) {
        if (std::fwrite(line, 1, size, runtime_log.session_file) != size ||
            std::fflush(runtime_log.session_file) != 0) {
            std::fclose(runtime_log.session_file);
            runtime_log.session_file = nullptr;
        }
    }
    if (!runtime_log.file) return false;
    if (std::fwrite(line, 1, size, runtime_log.file) != size || std::fflush(runtime_log.file) != 0) {
        stop_runtime_log();
        return false;
    }
    runtime_log.bytes += size;
    return true;
}
static bool runtime_log_regular_or_missing(const char* path) noexcept {
    try {
        std::error_code error;
        const auto status = std::filesystem::symlink_status(std::filesystem::u8path(path), error);
        return error == std::errc::no_such_file_or_directory ||
               (!error && (status.type() == std::filesystem::file_type::not_found ||
                           status.type() == std::filesystem::file_type::regular));
    } catch (...) {
        return false;
    }
}
static bool rotate_runtime_log() noexcept {
    if (runtime_log.file) {
        std::fclose(runtime_log.file);
        runtime_log.file = nullptr;
    }
    char source[4120], dest[4120];
    // Reject directories, devices and symlinks before renaming any segment.
    // fopen(path, "rb") can succeed for directories on POSIX systems.
    for (int i = 0; i < 4; ++i) {
        if (i)
            std::snprintf(source, sizeof(source), "%s.%d", runtime_log.path, i);
        else
            std::snprintf(source, sizeof(source), "%s", runtime_log.path);
        if (!runtime_log_regular_or_missing(source)) {
            stop_runtime_log();
            return false;
        }
    }
    // Four bounded segments retain the previous session and recent history.
    for (int i = 2; i >= 0; --i) {
        if (i)
            std::snprintf(source, sizeof(source), "%s.%d", runtime_log.path, i);
        else
            std::snprintf(source, sizeof(source), "%s", runtime_log.path);
        std::snprintf(dest, sizeof(dest), "%s.%d", runtime_log.path, i + 1);
        if (auto* existing = std::fopen(source, "rb")) {
            std::fclose(existing);
            std::remove(dest);
            if (std::rename(source, dest) != 0) {
                stop_runtime_log();
                return false;
            }
        }
    }
    runtime_log.file = std::fopen(runtime_log.path, "wb");
    runtime_log.bytes = 0;
    if (!runtime_log.file) {
        stop_runtime_log();
        return false;
    }
    char header[2048];
    const int size = std::snprintf(
        header, sizeof(header),
        "runtime_context_v1 run=%s version=%s build=%s compiled=\"%s %s\" arch=%s pointer_bits=%u "
        "resources=\"%s\" map=\"%s\" mode=%d difficulty=%d player=\"%s\" world=%llu max_segment_bytes=%zu segments=4\n",
        runtime_log.run, project_version, RA2_DIAGNOSTICS_BUILD, __DATE__, __TIME__, runtime_log_arch,
        unsigned(sizeof(void*) * 8), runtime_log.resources, runtime_log.map, runtime_log.mode, runtime_log.difficulty,
        runtime_log.player, static_cast<unsigned long long>(runtime_log.world), runtime_log_limit);
    return size > 0 &&
           write_runtime_line(header, std::size_t(size) < sizeof(header) ? std::size_t(size) : sizeof(header) - 1);
}
static void write_runtime_event(const char* event, const char* detail) noexcept {
    char line[4096];
    const auto sequence = ++runtime_log.sequence;
    runtime_crash_sequence(sequence, runtime_log.world, runtime_log.tick);
    const int size = std::snprintf(line, sizeof(line), "run=%s seq=%llu frame=%d world=%llu tick=%llu event=%s %s\n",
                                   runtime_log.run, static_cast<unsigned long long>(sequence), Unsorted::CurrentFrame,
                                   static_cast<unsigned long long>(runtime_log.world),
                                   static_cast<unsigned long long>(runtime_log.tick), event, detail);
    if (size <= 0) return;
    const auto count = std::size_t(size) < sizeof(line) ? std::size_t(size) : sizeof(line) - 1;
    if (runtime_log.bytes + count > runtime_log_limit && !rotate_runtime_log()) return;
    write_runtime_line(line, count);
}
bool configure_runtime_log(const char* path, const char* resources) noexcept {
    try {
        const std::lock_guard lock(runtime_log.mutex);
        if (runtime_log.file) write_runtime_event("session_end", path ? "reason=reconfigure" : "reason=close");
        stop_runtime_log();
        if (runtime_log.session_file) {
            std::fclose(runtime_log.session_file);
            runtime_log.session_file = nullptr;
        }
        prepare_runtime_crash_session(nullptr);
        runtime_log.session_directory[0] = 0;
        if (!path) return true;
        if (!*path || std::strlen(path) >= sizeof(runtime_log.path)) return false;
        std::error_code error;
        const auto parent = std::filesystem::u8path(path).parent_path();
        if (!parent.empty()) std::filesystem::create_directories(parent, error);
        if (error) return false;
        std::snprintf(runtime_log.path, sizeof(runtime_log.path), "%s", path);
        copy_log_text(runtime_log.resources, sizeof(runtime_log.resources), resources);
        runtime_log.map[0] = runtime_log.player[0] = 0;
        runtime_log.mode = runtime_log.difficulty = -1;
        runtime_log.world = runtime_log.tick = runtime_log.sequence = 0;
        for (auto& tag : runtime_log.tags)
            tag = {};
        const auto now =
            std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();
#if defined(_WIN32)
        const int pid = _getpid();
#else
        const int pid = getpid();
#endif
        static unsigned serial = 0;
        std::snprintf(runtime_log.run, sizeof(runtime_log.run), "%lld-%d-%u", static_cast<long long>(now), pid,
                      ++serial);
        const auto session = parent / "sessions" / runtime_log.run;
        std::filesystem::create_directories(session, error);
        if (!error) {
            std::snprintf(runtime_log.session_directory, sizeof(runtime_log.session_directory), "%s",
                          session.string().c_str());
            runtime_log.session_file = std::fopen((session / "events.log").string().c_str(), "wb");
            prepare_runtime_crash_session(session.string().c_str());
        }
        if (!rotate_runtime_log()) return false;
        runtime_log.enabled.store(true, std::memory_order_relaxed);
        write_runtime_event("session_begin", "clock=unix_microseconds projectile_log=projectiles.log");
        return runtime_log.enabled.load(std::memory_order_relaxed);
    } catch (...) {
        runtime_log.enabled.store(false, std::memory_order_relaxed);
        return false;
    }
}
bool runtime_log_enabled() noexcept { return runtime_log.enabled.load(std::memory_order_relaxed); }
const char* runtime_log_session_directory() noexcept { return runtime_log.session_directory; }
void runtime_log_event(const char* event, const char* format, ...) noexcept {
    if (!runtime_log_enabled()) return;
    try {
        char detail[2048];
        va_list args;
        va_start(args, format);
        const int size = std::vsnprintf(detail, sizeof(detail), format, args);
        va_end(args);
        if (size < 0) return;
        const std::lock_guard lock(runtime_log.mutex);
        if (runtime_log.file) write_runtime_event(event, detail);
    } catch (...) {
    }
}
void runtime_log_map(const char* filename, int mode, int difficulty, const char* player, std::uint64_t world) noexcept {
    if (!runtime_log_enabled()) return;
    try {
        const std::lock_guard lock(runtime_log.mutex);
        copy_log_text(runtime_log.map, sizeof(runtime_log.map), filename);
        copy_log_text(runtime_log.player, sizeof(runtime_log.player), player);
        runtime_log.mode = mode;
        runtime_log.difficulty = difficulty;
        runtime_log.world = world;
        runtime_log.tick = 0;
        char detail[512];
        std::snprintf(detail, sizeof(detail), "map=\"%s\" mode=%d difficulty=%d player=\"%s\"", runtime_log.map, mode,
                      difficulty, runtime_log.player);
        if (runtime_log.file) write_runtime_event("map_context", detail);
    } catch (...) {
    }
}
void runtime_log_tick(std::uint64_t world, std::uint64_t tick, bool completed) noexcept {
    if (!runtime_log_enabled()) return;
    try {
        const std::lock_guard lock(runtime_log.mutex);
        runtime_log.world = world;
        runtime_log.tick = tick;
        runtime_crash_sequence(runtime_log.sequence, world, tick);
        if (runtime_log.file && tick % 120 == 0) write_runtime_event(completed ? "tick_complete" : "tick_begin", "");
    } catch (...) {
    }
}
static RuntimeTagRecord& runtime_tag_record(const TagClass* tag) noexcept {
    return runtime_log.tags[(reinterpret_cast<std::uintptr_t>(tag) >> 4) % 2048];
}
void runtime_log_tag_created(const TagClass& tag) noexcept {
    if (!runtime_log_enabled()) return;
    try {
        const std::lock_guard lock(runtime_log.mutex);
        auto& saved = runtime_tag_record(&tag);
        saved = {};
        saved.address = &tag;
        saved.id = tag.UniqueID;
        saved.alive = true;
        if (tag.Type) {
            saved.persistence = int(tag.Type->Persistence);
            copy_log_text(saved.type, sizeof(saved.type), tag.Type->ID);
        }
        if (tag.FirstTrigger && tag.FirstTrigger->Type)
            copy_log_text(saved.trigger, sizeof(saved.trigger), tag.FirstTrigger->Type->ID);
    } catch (...) {
    }
    runtime_log_tag("tag_created", &tag, -1);
}
void runtime_log_tag(const char* event, const TagClass* tag, int kind, int index, int count) noexcept {
    if (!runtime_log_enabled()) return;
    try {
        const std::lock_guard lock(runtime_log.mutex);
        const auto& saved = runtime_tag_record(tag);
        const bool known = saved.address == tag;
        char detail[512];
        std::snprintf(detail, sizeof(detail),
                      "tag=0x%llX known=%d alive=%d id=%u type=\"%s\" trigger=\"%s\" initial_persistence=%d kind=%d "
                      "index=%d count=%d",
                      static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(tag)), known,
                      known && saved.alive, known ? saved.id : 0, known ? saved.type : "<unknown>",
                      known ? saved.trigger : "<unknown>", known ? saved.persistence : -1, kind, index, count);
        if (runtime_log.file) write_runtime_event(event, detail);
    } catch (...) {
    }
}
void runtime_log_tag_deleted(const TagClass* tag) noexcept {
    runtime_log_tag("tag_deleted", tag, -1);
    if (!runtime_log_enabled()) return;
    try {
        const std::lock_guard lock(runtime_log.mutex);
        auto& saved = runtime_tag_record(tag);
        if (saved.address == tag) saved.alive = false;
    } catch (...) {
    }
}
}

namespace game {
void capture_render_failure_context(RenderFailureContext& output) noexcept {
    try {
        const std::lock_guard lock(runtime_log.mutex);
        std::snprintf(output.map, sizeof(output.map), "%s", runtime_log.map);
        std::snprintf(output.run, sizeof(output.run), "%s", runtime_log.run);
    } catch (...) {
    }
}
bool write_render_failure(const RenderFailureContext& context, const DrawingFailureInfo& failure, char* path,
                          std::uint32_t path_capacity) noexcept {
    if (path && path_capacity) path[0] = 0;
    if (!failure.id) return false;
    try {
        const std::lock_guard lock(runtime_log.mutex);
        char header[2048], request[4096], errors[2304];
        char stage[96], condition[768], source[192], detail[2048], object[512], core_error[1024], bridge_error[1024];
        copy_log_text(stage, sizeof(stage), failure.stage);
        copy_log_text(condition, sizeof(condition), failure.condition);
        copy_log_text(source, sizeof(source), failure.source);
        copy_log_text(detail, sizeof(detail), failure.detail);
        copy_log_text(object, sizeof(object), failure.object);
        copy_log_text(core_error, sizeof(core_error), context.core_error);
        copy_log_text(bridge_error, sizeof(bridge_error), context.bridge_error);
        std::snprintf(header, sizeof(header),
                      "failure=%llu stage=%s status=%s failure_frame=%d frame=%d world=%llu tick=%llu generation=%llu "
                      "render_sequence=%llu canvas=%d,%d camera=%d,%d core=%u core_status=%s end_frame=%u enqueue=%u "
                      "gpu_prepare=%u gpu_submit=%u gpu_complete=%u commands=%u resources=%u",
                      static_cast<unsigned long long>(failure.id), context.stage, drawing_status_name(failure.status),
                      failure.frame, context.frame, static_cast<unsigned long long>(context.world),
                      static_cast<unsigned long long>(context.tick),
                      static_cast<unsigned long long>(context.generation),
                      static_cast<unsigned long long>(context.render_sequence), context.width, context.height,
                      context.camera_x, context.camera_y, context.core, drawing_status_name(context.core_status),
                      context.end_frame, context.enqueue, context.gpu_prepare, context.gpu_submit, context.gpu_complete,
                      context.commands, context.resources);
        std::snprintf(request, sizeof(request),
                      "failure=%llu request=%llu origin=%s source=%s:%u rejected=\"%s\" values=\"%s\" object=\"%s\"",
                      static_cast<unsigned long long>(failure.id), static_cast<unsigned long long>(failure.request),
                      stage, source, failure.line, condition, detail, object);
        std::snprintf(errors, sizeof(errors), "failure=%llu core_error=\"%s\" bridge_error=\"%s\"",
                      static_cast<unsigned long long>(failure.id), core_error, bridge_error);
        char filename[4096]{};
        bool saved = false;
        if (runtime_log.session_directory[0]) {
            const int length =
                std::snprintf(filename, sizeof(filename), "%s/render-failure-%llu.txt", runtime_log.session_directory,
                              static_cast<unsigned long long>(failure.id));
            if (length > 0 && std::size_t(length) < sizeof(filename)) {
                if (auto* file = std::fopen(filename, "wx")) {
                    const bool written =
                        std::fprintf(
                            file,
                            "render_failure_v1 run=%s version=%s build=%s compiled=\"%s %s\" arch=%s map=\"%s\" "
                            "resources=\"%s\"\n%s\n%s\n%s\n",
                            (context.run[0] ? context.run : runtime_log.run), project_version, RA2_DIAGNOSTICS_BUILD,
                            __DATE__, __TIME__, runtime_log_arch, (context.map[0] ? context.map : runtime_log.map),
                            runtime_log.resources, header, request, errors) > 0;
                    const int closed = std::fclose(file);
                    saved = written && closed == 0;
                }
            }
        }
        // Each event fits the regular event limit; split the request payload so
        // long predicates cannot truncate the actual parameter values.
        if (runtime_log.enabled.load(std::memory_order_relaxed)) {
            write_runtime_event("render_failure", header);
            char origin[1536];
            std::snprintf(origin, sizeof(origin), "failure=%llu request=%llu origin=%s source=%s:%u rejected=\"%s\"",
                          static_cast<unsigned long long>(failure.id), static_cast<unsigned long long>(failure.request),
                          stage, source, failure.line, condition);
            write_runtime_event("render_failure_origin", origin);
            char values[2816];
            std::snprintf(values, sizeof(values), "failure=%llu values=\"%s\" object=\"%s\"",
                          static_cast<unsigned long long>(failure.id), detail, object);
            write_runtime_event("render_failure_values", values);
            write_runtime_event("render_failure_errors", errors);
            char result[256];
            std::snprintf(result, sizeof(result), "failure=%llu snapshot_saved=%d",
                          static_cast<unsigned long long>(failure.id), saved);
            write_runtime_event("render_failure_snapshot", result);
        }
        if (saved && path && path_capacity) {
            if (std::strlen(filename) < path_capacity) std::snprintf(path, path_capacity, "%s", filename);
        }
        return saved;
    } catch (...) {
        return false;
    }
}
}
