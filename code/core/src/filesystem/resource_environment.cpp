#include "resource_environment.hpp"
#include "yrpp/CRC.h"
#include "filesystem/resource_globals.hpp"
#include "filesystem/resource_startup.hpp"
#include <atomic>
#include "search_paths.hpp"
#include "filesystem/resource_context.hpp"


#include <algorithm>
#include <stdexcept>

namespace game {
uint32_t filename_id(std::string_view name) {
    CRCEngine crc;
    for (char c : name) {
        if (c >= 'a' && c <= 'z') c = char(c - 'a' + 'A');
        crc(c);
    }
    return static_cast<uint32_t>(crc());
}
namespace { std::atomic<ResourceEnvironment*> active_files{nullptr}; }

ResourceEnvironment::ResourceEnvironment(std::filesystem::path directory)
    : directory_(std::filesystem::absolute(std::move(directory))), platform_(directory_) {
    if (!std::filesystem::is_directory(directory_))
        throw std::runtime_error("game data directory does not exist: " + path_to_utf8(directory_));
    ResourceEnvironment* expected = nullptr;
    if (!active_files.compare_exchange_strong(expected, this))
        throw std::logic_error("a game resource environment is already active");
}

ResourceEnvironment::~ResourceEnvironment() {
    clear();
    active_files.store(nullptr);
}

MixFileClass& ResourceEnvironment::mount(std::string name) {
    OwnedMix record;
    game::ResourceScope scope({this, &record.source, {}});
    try { record.object = std::make_unique<MixFileClass>(name.c_str()); }
    catch (const std::bad_alloc&) { throw; }
    catch (const std::exception& e) { throw std::runtime_error(name + ": " + e.what()); }
    auto& result = *record.object;
    objects_.push_back(std::move(record));
    return result;
}
void ResourceEnvironment::unmount(MixFileClass& mix) noexcept {
    auto* pointer = &mix;
    std::erase_if(objects_, [pointer](const auto& record) { return record.object.get() == pointer; });
    game::forget_mix(pointer);
}
void ResourceEnvironment::clear() noexcept {
    // The host has stopped image users and unloaded the SHP module before
    // reaching this file-only teardown. No image traversal belongs here.
    game::shutdown_resource_files(*this);
    // Repeated Bootstrap/append failure can leave allocations outside slots.
    // Reclaim only here, at full host teardown, never inside original entry.
    objects_.clear();
    // 5B3B10 drains the non-owning global list by unlinking, without deleting
    // MIX objects. Direct constructor calls do not enter our allocation ledger:
    // detach their surviving nodes before the host directory can be reused.
    while (auto* remaining = MixFileClass::MIXes.front()) remaining->Unlink();
    MixFileClass::Array.Clear();
    MixFileClass::Array.CapacityIncrement = 10;
    MixFileClass::Maps.Clear(); MixFileClass::Maps.CapacityIncrement=10;
    MixFileClass::MULTIMD=nullptr;
    MixFileClass::SIDENC=nullptr;
    MixFileClass::Generics = {};
    game::disk_selection = 0;
    game::clear_file_search_paths();
}
const FileLocation& ResourceEnvironment::source_of(const MixFileClass& mix) const {
    for (const auto& record : objects_)
        if (record.object.get() == &mix) return record.source;
    throw std::logic_error("MIX does not belong to this file system");
}
}
