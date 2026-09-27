// Host state is scoped outside all game-visible objects.
#pragma once
#include "filesystem/bootstrap_stage.hpp"
#include "filesystem/file_system.hpp"
#include <cstdint>
#include <string>
#include <vector>
namespace game { class ResourceEnvironment; }
namespace game {
// Diagnostic output from the original file chain; owns no file or cache.
struct FileLocation {
    std::filesystem::path physical_file;
    uint64_t offset = 0;
    uint64_t size = 0;
    std::vector<std::string> source_chain;
    const uint8_t* memory = nullptr; // borrowed cached carrier, if any
};
struct ResourceContext {
    ResourceEnvironment* files = nullptr;
    FileLocation* source = nullptr;
    BootstrapObserver observer;
};
// Each host worker has one active context. Nested operations restore the caller.
const ResourceContext& current_context();
const ResourceContext* try_current_context() noexcept;
class ResourceScope {
public:
    explicit ResourceScope(ResourceContext context);
    // Observe a file operation without changing the selected file service.
    explicit ResourceScope(FileLocation& source);
    ~ResourceScope();
    ResourceScope(const ResourceScope&) = delete;
    ResourceScope& operator=(const ResourceScope&) = delete;
private:
    FileSystemScope platform_scope_;
    ResourceContext context_;
    const ResourceContext* previous_;
};
}
