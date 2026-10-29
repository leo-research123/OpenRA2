#include "runtime_diagnostics.hpp"
#include "yrpp/CCFileClass.h"
#include "yrpp/Randomizer.h"
#include <cstdio>
#include <filesystem>
#include <atomic>
#if defined(__APPLE__)
#include <CommonCrypto/CommonDigest.h>
#endif

namespace game {
void runtime_log_resource(const char* filename) noexcept {
    if (!runtime_log_enabled() || !filename || !*runtime_log_session_directory()) return;
    try {
        CCFileClass source(filename);
        const int size = source.GetFileSize();
        if (size < 0 || !source.Open(FileAccessMode::Read)) {
            runtime_log_event("resource_snapshot", "name=\"%.240s\" available=0", filename);
            return;
        }
        const auto folder = std::filesystem::u8path(runtime_log_session_directory()) / "resources";
        std::error_code error;
        std::filesystem::create_directories(folder, error);
        const auto name = std::filesystem::u8path(filename).filename();
        static std::atomic<std::uint64_t> snapshot_sequence{0};
        char prefix[32];
        std::snprintf(prefix, sizeof(prefix), "%llu-", static_cast<unsigned long long>(++snapshot_sequence));
        const auto snapshot = std::string(prefix) + name.string();
        auto* destination = error ? nullptr : std::fopen((folder / snapshot).string().c_str(), "wb");
        char buffer[65536], hex[65]{};
        int remaining = size;
        bool complete = destination != nullptr;
#if defined(__APPLE__)
        CC_SHA256_CTX hash;
        CC_SHA256_Init(&hash);
#endif
        while (remaining > 0) {
            const int requested = remaining < int(sizeof(buffer)) ? remaining : int(sizeof(buffer));
            const int count = source.ReadBytes(buffer, requested);
            if (count <= 0 || count > requested) {
                complete = false;
                break;
            }
            if (destination && std::fwrite(buffer, 1, count, destination) != std::size_t(count)) complete = false;
#if defined(__APPLE__)
            CC_SHA256_Update(&hash, buffer, count);
#endif
            remaining -= count;
        }
        source.Close();
        if (destination && std::fclose(destination) != 0) complete = false;
#if defined(__APPLE__)
        unsigned char digest[CC_SHA256_DIGEST_LENGTH];
        CC_SHA256_Final(digest, &hash);
        for (unsigned i = 0; i < sizeof(digest); ++i)
            std::snprintf(hex + i * 2, 3, "%02x", digest[i]);
#endif
        runtime_log_event("resource_snapshot",
                          "name=\"%.240s\" snapshot=\"resources/%.280s\" available=1 bytes=%d complete=%d sha256=%s",
                          filename, snapshot.c_str(), size, complete && remaining == 0,
                          remaining == 0 ? hex : "incomplete");
    } catch (...) {
        runtime_log_event("resource_snapshot", "name=\"%.240s\" available=0 error=exception", filename);
    }
}
void runtime_log_rng(const Randomizer& scenario, const Randomizer& global) noexcept {
    if (!runtime_log_enabled()) return;
    for (int stream = 0; stream < 2; ++stream) {
        const auto& random = stream ? global : scenario;
        runtime_log_event("rng_context", "stream=%s disabled=%d next1=%d next2=%d", stream ? "global" : "scenario",
                          random.unknown_00, random.Next1, random.Next2);
        for (unsigned first = 0; first < 0xFA; first += 10) {
            char table[160]{};
            unsigned written = 0;
            for (unsigned i = first; i < first + 10 && i < 0xFA; ++i)
                written +=
                    static_cast<unsigned>(std::snprintf(table + written, sizeof(table) - written, "%s0x%08X",
                                                        i == first ? "" : ",", static_cast<unsigned>(random.Table[i])));
            runtime_log_event("rng_table", "stream=%s first=%u values=%s", stream ? "global" : "scenario", first,
                              table);
        }
    }
}
}
