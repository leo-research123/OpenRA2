#include "voxel_drawing.hpp"
#include <atomic>
#include <algorithm>
namespace game {
std::uint64_t next_voxel_drawing_identity() noexcept {
    static std::atomic<std::uint64_t> next{1};
    return next.fetch_add(1, std::memory_order_relaxed);
}
std::uint64_t voxel_image_identity(const VoxelImageHandle& image) noexcept { return image.identity; }
bool read_voxel_image(const VoxelImageHandle& image, VoxelImageView& out) noexcept {
    out = {};
    out.identity = image.identity;
    out.kind = image.kind;
    out.width = image.width;
    out.height = image.height;
    out.left = image.left;
    out.top = image.top;
    out.buffered = image.buffered;
    out.sections = image.sections.data();
    out.section_count = std::uint32_t(image.sections.size());
    out.lighting = image.lighting.data();
    if (image.palette) out.palette = image.palette->view();
    if (image.columns) out.columns = image.columns->view();
    out.reference_pixels = image.reference_pixels.data();
    out.reference_count = std::uint32_t(image.reference_pixels.size());
    out.shadow = image.shadow.data();
    out.mask = image.mask.get();
    out.layers = image.layers.data();
    out.layer_count = std::uint32_t(image.layers.size());
    return out.width > 0 && out.height > 0 && out.width <= 256 && out.height <= 256;
}
std::shared_ptr<VoxelImageHandle> make_voxel_composite(const VoxelImageHandle& source, const RectangleStruct& rect) {
    auto result = std::make_shared<VoxelImageHandle>();
    result->kind = VoxelImageKind::composite;
    result->width = rect.Width;
    result->height = rect.Height;
    result->children = source.children;
    result->layers = source.layers;
    for (auto& layer : result->layers) {
        layer.position.X -= rect.X;
        layer.position.Y -= rect.Y;
        layer.clip.X -= rect.X;
        layer.clip.Y -= rect.Y;
    }
    if (!result->layers.empty() && std::all_of(result->layers.begin(), result->layers.end(), [](const auto& layer) {
            return !layer.image->reference_pixels.empty();
        })) {
        result->reference_pixels.resize(std::size_t(rect.Width) * rect.Height);
        for (int y = 0; y < rect.Height; ++y)
            for (int x = 0; x < rect.Width; ++x)
                for (const auto& layer : result->layers) {
                    const auto& image = *layer.image;
                    const int sx = x - layer.position.X, sy = y - layer.position.Y;
                    if (sx < 0 || sy < 0 || sx >= image.width || sy >= image.height || x < layer.clip.X ||
                        y < layer.clip.Y || x >= layer.clip.X + layer.clip.Width ||
                        y >= layer.clip.Y + layer.clip.Height)
                        continue;
                    const auto index = image.reference_pixels[std::size_t(sy) * image.width + sx] & 0xFFu;
                    if (index) result->reference_pixels[std::size_t(y) * rect.Width + x] = 0x1000000u | index;
                }
    }
    return result;
}
}

#include "yrpp/Surface.h"
#include <algorithm>
namespace game {
VoxelCompositeScratch::~VoxelCompositeScratch() { BSurface::Destroy(surface); }
namespace {
bool equal_rect(const RectangleStruct& a, const RectangleStruct& b) noexcept {
    return a.X == b.X && a.Y == b.Y && a.Width == b.Width && a.Height == b.Height;
}
}
bool VoxelCompositeCache::Equal::operator()(const Key& a, const Key& b) const noexcept {
    if (a.hash != b.hash || !equal_rect(a.rect, b.rect) || a.layers.size() != b.layers.size()) return false;
    for (std::size_t i = 0; i < a.layers.size(); ++i) {
        const auto& x = a.layers[i];
        const auto& y = b.layers[i];
        if (x.image != y.image || x.position.X != y.position.X || x.position.Y != y.position.Y ||
            !equal_rect(x.clip, y.clip))
            return false;
    }
    return true;
}
bool VoxelCompositeCache::Equal::operator()(const Key& a, const View& b) const noexcept {
    if (a.hash != b.hash || !equal_rect(a.rect, b.rect) || a.layers.size() != b.image.layers.size()) return false;
    for (std::size_t i = 0; i < a.layers.size(); ++i) {
        const auto& x = a.layers[i];
        const auto& y = b.image.layers[i];
        if (x.image != y.image->identity || x.position.X != y.position.X || x.position.Y != y.position.Y ||
            !equal_rect(x.clip, y.clip))
            return false;
    }
    return true;
}
std::shared_ptr<VoxelImageHandle> VoxelCompositeCache::get(const VoxelImageHandle& source,
                                                           const RectangleStruct& rect) {
    std::uint64_t hash = 0xCBF29CE484222325ull;
    const auto mix = [&](std::uint64_t value) {
        hash ^= value;
        hash *= 0x100000001B3ull;
        hash ^= hash >> 29;
    };
    const auto bounds = [&](const RectangleStruct& r) {
        mix(std::uint32_t(r.X));
        mix(std::uint32_t(r.Y));
        mix(std::uint32_t(r.Width));
        mix(std::uint32_t(r.Height));
    };
    bounds(rect);
    mix(source.layers.size());
    for (const auto& layer : source.layers) {
        mix(layer.image->identity);
        mix(std::uint32_t(layer.position.X));
        mix(std::uint32_t(layer.position.Y));
        bounds(layer.clip);
    }
    const auto used = ++serial_;
    if (const auto found = entries_.find(View{source, rect, hash}); found != entries_.end()) {
        found->second.used = used;
        return found->second.image;
    }
    const auto bytes = std::size_t(rect.Width) * rect.Height * 4;
    while (!entries_.empty() && (entries_.size() >= 512 || bytes_ + bytes > 16 * 1024 * 1024)) {
        const auto oldest = std::min_element(entries_.begin(), entries_.end(), [](const auto& a, const auto& b) {
            return a.second.used < b.second.used;
        });
        bytes_ -= oldest->second.bytes;
        entries_.erase(oldest);
    }
    Key key{{}, rect, hash};
    key.layers.reserve(source.layers.size());
    for (const auto& layer : source.layers)
        key.layers.push_back({layer.image->identity, layer.position, layer.clip});
    auto result = make_voxel_composite(source, rect);
    entries_.emplace(std::move(key), Entry{result, used, bytes});
    bytes_ += bytes;
    return result;
}
}
