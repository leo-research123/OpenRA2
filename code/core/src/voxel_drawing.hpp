#pragma once
#include "api/voxel_drawing.hpp"
#include <array>
#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
class BSurface;
namespace game {
std::uint64_t next_voxel_drawing_identity() noexcept;
struct VoxelGpuBuffer {
    std::uint64_t identity = next_voxel_drawing_identity();
    std::vector<std::uint32_t> words;
    VoxelBufferView view() const noexcept { return {identity, words.data(), std::uint32_t(words.size())}; }
};
class VoxelImageHandle {
public:
    std::uint64_t identity = next_voxel_drawing_identity();
    VoxelImageKind kind = VoxelImageKind::raster;
    int width = 0, height = 0, left = 0, top = 0;
    bool buffered = false;
    std::vector<VoxelSectionView> sections;
    std::vector<std::shared_ptr<const VoxelGpuBuffer>> sources;
    std::shared_ptr<const VoxelGpuBuffer> palette, columns;
    std::array<std::uint32_t, 256> lighting{};
    std::array<std::int32_t, 10> shadow{};
    std::shared_ptr<const VoxelImageHandle> mask;
    std::vector<VoxelLayerView> layers;
    std::vector<std::shared_ptr<const VoxelImageHandle>> children;
    std::vector<std::uint32_t> reference_pixels;
};
// World-owned resource decoding, never keyed by a projected matrix or frame.
struct VoxelGpuAssets {
    std::map<std::pair<const void*, unsigned>, std::shared_ptr<const VoxelGpuBuffer>> sections;
    std::map<const void*, std::shared_ptr<const VoxelGpuBuffer>> shadows;
    std::shared_ptr<const VoxelGpuBuffer> palette;
    void clear() {
        sections.clear();
        shadows.clear();
        palette.reset();
    }
};
std::shared_ptr<VoxelImageHandle> make_voxel_composite(const VoxelImageHandle&, const RectangleStruct&);
// Mutable drawing scratch never crosses the host boundary. Published masks
// and cropped composites come from the immutable content cache below.
struct VoxelCompositeScratch {
    std::vector<unsigned char> pixels;
    BSurface* surface = nullptr;
    std::shared_ptr<VoxelImageHandle> image;
    ~VoxelCompositeScratch();
};
class VoxelCompositeCache {
    struct Layer {
        std::uint64_t image;
        Point2D position;
        RectangleStruct clip;
    };
    struct Key {
        std::vector<Layer> layers;
        RectangleStruct rect;
        std::uint64_t hash;
    };
    struct View {
        const VoxelImageHandle& image;
        RectangleStruct rect;
        std::uint64_t hash;
    };
    struct Hash {
        using is_transparent = void;
        std::size_t operator()(const Key& k) const noexcept { return std::size_t(k.hash); }
        std::size_t operator()(const View& k) const noexcept { return std::size_t(k.hash); }
    };
    struct Equal {
        using is_transparent = void;
        bool operator()(const Key&, const Key&) const noexcept;
        bool operator()(const Key&, const View&) const noexcept;
        bool operator()(const View& a, const Key& b) const noexcept { return (*this)(b, a); }
    };
    struct Entry {
        std::shared_ptr<VoxelImageHandle> image;
        std::uint64_t used;
        std::size_t bytes;
    };
    std::unordered_map<Key, Entry, Hash, Equal> entries_;
    std::uint64_t serial_ = 0;
    std::size_t bytes_ = 0;

public:
    std::shared_ptr<VoxelImageHandle> get(const VoxelImageHandle&, const RectangleStruct&);
    void clear() {
        entries_.clear();
        bytes_ = 0;
    }
};
}
