#pragma once
#include <cstdint>
struct SHPStruct;
namespace game {
// Synchronous resource-thread queries. The borrowed address is never retained
// by a queued draw. A successful version identifies one lifetime/content;
// failure must invalidate a host cache. No exception crosses this boundary.
bool get_shape_resource_version(const SHPStruct*, std::uint64_t& version) noexcept;
std::uint64_t shape_resource_generation() noexcept;
}
