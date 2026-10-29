#include "images/drawing_resources.hpp"
#include <unordered_map>
#include <atomic>
namespace game {
namespace {
std::uint64_t generation = 1;
std::unordered_map<const SHPStruct*, std::uint64_t>* values = nullptr;
auto& versions() {
    // Original resources may be destroyed by other translation units during
    // process teardown. The empty registry's lifetime includes that teardown.
    if (!values) values = new std::unordered_map<const SHPStruct*, std::uint64_t>;
    return *values;
}
}
bool get_shape_resource_version(const SHPStruct* shape, std::uint64_t& version) noexcept {
    version = 0;
    if (!shape) return false;
    try {
        auto [entry, inserted] = versions().try_emplace(shape, 0);
        if (inserted) entry->second = ++generation;
        version = entry->second;
        return true;
    } catch (...) {
        ++generation;
        return false;
    }
}
std::uint64_t shape_resource_generation() noexcept { return generation; }
void invalidate_shape_resource(const SHPStruct* shape) noexcept {
    ++generation;
    // No insertion or allocation during resource destruction.
    if (values) values->erase(shape);
}
std::uint64_t allocate_raster_content_identity() noexcept {
    static std::atomic<std::uint64_t> next{1};
    return next.fetch_add(1, std::memory_order_relaxed);
}
}
