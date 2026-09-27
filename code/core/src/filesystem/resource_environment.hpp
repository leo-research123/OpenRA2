#pragma once

#include "yrpp/MixFileClass.h"
#include "filesystem/file_system.hpp"
#include "resource_context.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

class CCFileClass;

namespace game {

uint32_t filename_id(std::string_view name);

// One active host directory/service per process. Resource consumers must stop
// and close borrowed cache views before clear/destruction; mutations are serial.
class ResourceEnvironment {
public:
    explicit ResourceEnvironment(std::filesystem::path directory);
    ~ResourceEnvironment();
    ResourceEnvironment(const ResourceEnvironment&) = delete;
    ResourceEnvironment& operator=(const ResourceEnvironment&) = delete;

    // mount adopts the created object. Direct MixFileClass construction only
    // registers a non-owning node; that object's lifetime remains with its caller.
    MixFileClass& mount(std::string name);
    void unmount(MixFileClass& mix) noexcept;
    // Release adopted objects and detach all remaining nodes. Caller-owned MIX
    // metadata survives, but is no longer registered for lookup.
    void clear() noexcept;
    const List<MixFileClass>& mixes() const { return MixFileClass::MIXes; }
    const FileLocation& source_of(const MixFileClass& mix) const;
    const std::filesystem::path& directory() const { return directory_; }
    FileSystem& platform() { return platform_; }
private:
    friend class ::MixFileClass;
    friend class ::CCFileClass;
    struct OwnedMix {
        std::unique_ptr<MixFileClass> object;
        FileLocation source;
    };
    std::filesystem::path directory_;
    FileSystem platform_;
    // Allocation/source ledger only; lookup authority is MixFileClass::MIXes.
    std::vector<OwnedMix> objects_;
};
}
