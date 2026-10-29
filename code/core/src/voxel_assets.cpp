// Generation counter for voxel objects replaced by the type loaders. It is kept
// apart from the voxel renderer so type loading does not depend on drawing code.
#include "building_voxel.hpp"
#include <atomic>

namespace game {
namespace {
std::atomic<std::uint64_t> revision{1};
}

std::uint64_t voxel_resource_revision() noexcept { return revision.load(std::memory_order_relaxed); }

void voxel_assets_changed() noexcept { revision.fetch_add(1, std::memory_order_relaxed); }
}
