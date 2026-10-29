// Building control: supplied 0x0045FA90, 0x0043DA80 and 0x00458810.
// Sparse spans follow the fixed, already-vendored EA Mission Editor/XCC
// vxl_file.cpp (Olaf van der Spek; see third-party notices). Raster traversal
// and lighting are calibrated to 0x00756590 / 0x00757980 in the supplied EXE.
// Existing VoxLib/MotLib own resources; the host owns only derived surfaces.
#include "presentation/builders/unit.hpp"
#include "presentation/builders/building.hpp"
#include "presentation/context.hpp"
#include "building_voxel.hpp"
#include "voxel_drawing.hpp"
#include "techno_drawing.hpp"
#include "yrpp/AircraftClass.h"
#include "yrpp/BuildingClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/Surface.h"
#include "yrpp/Memory.h"
#include "yrpp/YRMath.h"
#include "yrpp/Matrix3DArithmetic.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <string>
#include <limits>
#include <cctype>
#include <atomic>
namespace game {
namespace {
constexpr double pi = 3.14159265358979323846;
// Matrix and float-store arithmetic is shared with the original Matrix3D module
// (x87 control word 0x0E7F: double operations and float stores truncate).
Matrix3D rotation(int axis, float angle) {
    auto m = Matrix3D::GetIdentity();
    if (axis == 0)
        m.RotateX(angle);
    else if (axis == 1)
        m.RotateY(angle);
    else
        m.RotateZ(angle);
    return m;
}
Matrix3D multiply(const Matrix3D& a, const Matrix3D& b) { return drawing_matrix_product(a, b); }
Vector3D<float> transform(const Matrix3D& m, Vector3D<float> p) {
    Vector3D<float> out;
    Matrix3D::MatrixApply(&out, &m, &p);
    return out;
}
unsigned direction(unsigned raw) { return ((raw >> 10) + 1u) / 2u & 31u; }
bool ready(const VoxelStruct& v, bool apply_hva = true) {
    return v.VXL && !v.VXL->Initialized && v.VXL->CountHeaders && v.VXL->BodyData &&
           (!apply_hva || (v.HVA && !v.HVA->LoadedFailed && v.HVA->Matrixes && v.HVA->FrameCount > 0 &&
                           v.HVA->LayerCount >= int(v.VXL->CountHeaders)));
}
bool finite(const Matrix3D& m) {
    for (auto& r : m.row)
        for (unsigned i = 0; i < 4; ++i)
            if (!std::isfinite(r[i]) || std::abs(r[i]) > 100000) return false;
    return true;
}
DrawingStatus copy_surface(const std::vector<std::uint32_t>& pixels, Vector3D<float> lo, Vector3D<float> hi, float cx,
                           float cy, VoxelSurface& out, bool allow_empty = false) {
    // 0x754649..0x75470F retains four pixels of padding around the projected
    // bounds. PlainBlit's row-depth phase depends on this full rectangle.
    const int width = int(double(hi.X) - lo.X) + 8, height = int(double(hi.Y) - lo.Y) + 8;
    const int left = 128 - width / 2, top = 128 - height / 2;
    if (width <= 0 || height <= 0 || left < 0 || top < 0 || left + width > 256 || top + height > 256)
        return DrawingStatus::unsupported;
    if (!allow_empty && std::none_of(pixels.begin(), pixels.end(), [](auto pixel) { return pixel != 0; }))
        return DrawingStatus::skipped;
    out.width = width;
    out.height = height;
    out.offset = {int(cx) - width / 2, int(cy) - height / 2};
    out.pixels.resize(std::size_t(width) * height);
    for (int y = 0; y < height; ++y)
        std::copy_n(pixels.data() + (top + y) * 256 + left, width, out.pixels.data() + y * width);
    return DrawingStatus::drawn;
}
struct Sample {
    unsigned x, y, z;
    std::uint8_t color, normal;
};
bool decode(const VoxLib& v, const VoxelSectionTailer& t, std::vector<Sample>& out) {
    const unsigned nx = std::uint8_t(t.size_X), ny = std::uint8_t(t.size_Y), nz = std::uint8_t(t.size_Z);
    if (!nx || !ny || !nz) return false;
    const auto begin = reinterpret_cast<std::uintptr_t>(v.BodyData);
    const auto end = begin + v.TotalSize;
    auto range = [&](const void* p, std::size_t n) {
        auto q = reinterpret_cast<std::uintptr_t>(p);
        return q >= begin && q <= end && n <= end - q;
    };
    if (!range(t.span_start_off, std::size_t(nx) * ny * 4) || !range(t.span_end_off, std::size_t(nx) * ny * 4) ||
        !range(t.span_data_off, 0))
        return false;
    for (unsigned y = 0; y < ny; ++y)
        for (unsigned x = 0; x < nx; ++x) {
            std::int32_t first, last;
            std::memcpy(&first, reinterpret_cast<const char*>(t.span_start_off) + 4 * (y * nx + x), 4);
            std::memcpy(&last, reinterpret_cast<const char*>(t.span_end_off) + 4 * (y * nx + x), 4);
            if (first == -1 && last == -1) continue;
            if (first < 0 || last < first) return false;
            const auto base = reinterpret_cast<std::uintptr_t>(t.span_data_off);
            if (std::uint64_t(last) >= end - base) return false;
            const auto* p = t.span_data_off + first;
            const auto* limit = t.span_data_off + last + 1;
            unsigned z = 0;
            while (z < nz) {
                if (limit - p < 3) return false;
                const unsigned skip = *p++, count = *p++;
                if ((!skip && !count) || skip > nz - z || count > nz - z - skip ||
                    limit - p < std::ptrdiff_t(2 * count + 1))
                    return false;
                z += skip;
                for (unsigned k = 0; k < count; ++k) {
                    out.push_back({x, y, z++, p[0], p[1]});
                    p += 2;
                    if (out.size() > 16777216) return false;
                }
                if (*p++ != count) return false;
            }
            if (p != limit) return false;
        }
    return true;
}
// Decode immutable traversal once per section and corner, independently of
// object matrices, HVA frames and projected image state.
std::shared_ptr<const VoxelGpuBuffer> gpu_section(VoxelGpuAssets& assets, const VoxLib& v, const VoxelSectionTailer& t,
                                                  unsigned corner) {
    const auto key = std::make_pair(static_cast<const void*>(&t), corner);
    if (auto found = assets.sections.find(key); found != assets.sections.end()) return found->second;
    std::vector<Sample> samples;
    if (!decode(v, t, samples)) return {};
    const bool reverse_x = corner == 0 || corner == 1 || corner == 4 || corner == 5;
    const bool reverse_y = corner == 0 || corner == 3 || corner == 4 || corner == 7;
    std::stable_sort(samples.begin(), samples.end(), [&](const auto& a, const auto& b) {
        if (a.y != b.y) return reverse_y ? a.y > b.y : a.y < b.y;
        if (a.x != b.x) return reverse_x ? a.x > b.x : a.x < b.x;
        return corner < 4 ? a.z < b.z : a.z > b.z;
    });
    auto buffer = std::make_shared<VoxelGpuBuffer>();
    auto& words = buffer->words;
    words = {0, unsigned(samples.size())};
    unsigned lx = UINT32_MAX, ly = UINT32_MAX;
    for (unsigned i = 0; i < samples.size(); ++i) {
        const auto& sample = samples[i];
        if (sample.x != lx || sample.y != ly) {
            words.push_back(i);
            words.push_back(0);
            ++words[0];
            lx = sample.x;
            ly = sample.y;
        }
        ++words.back();
    }
    for (const auto& sample : samples) {
        words.push_back(sample.x | (sample.y << 8) | (sample.z << 16) | (unsigned(sample.color) << 24));
        words.push_back(sample.normal);
    }
    assets.sections.emplace(key, buffer);
    return buffer;
}
DrawingStatus gpu_bounds(Vector3D<float> lo, Vector3D<float> hi, float cx, float cy, VoxelSurface& out) {
    const int width = int(double(hi.X) - lo.X) + 8, height = int(double(hi.Y) - lo.Y) + 8, left = 128 - width / 2,
              top = 128 - height / 2;
    if (width <= 0 || height <= 0 || left < 0 || top < 0 || left + width > 256 || top + height > 256)
        return DrawingStatus::unsupported;
    out.width = width;
    out.height = height;
    out.offset = {int(cx) - width / 2, int(cy) - height / 2};
    out.gpu->width = width;
    out.gpu->height = height;
    out.gpu->left = left;
    out.gpu->top = top;
    return DrawingStatus::drawn;
}
// Normals are exact float constants extracted from the supplied executable.
#include "building_voxel_normals.inc"
Vector3D<float> normalized(Vector3D<float> v) {
    double len = std::sqrt(double(v.X) * v.X + double(v.Y) * v.Y + double(v.Z) * v.Z);
    if (len > 0) {
        v.X = matrix_store_float(v.X / len);
        v.Y = matrix_store_float(v.Y / len);
        v.Z = matrix_store_float(v.Z / len);
    }
    return v;
}
double dot(Vector3D<float> a, Vector3D<float> b) { return double(a.Z) * b.Z + double(a.Y) * b.Y + double(a.X) * b.X; }
Vector3D<float> voxel_light_direction() {
    const float xy = std::bit_cast<float>(std::uint32_t(0xbf3504e6));
    return transform(rotation(1, 0.78537499904632568359375f), {xy, xy, 0});
}
// 0x00753D00 calls orthogonal inverse 0x005AFC20 before 0x007586F0.
// That inverse transposes the basis even in the mixed scaled-barrel case.
// HVA is deliberately absent: the original builds this table BEFORE the
// per-limb/per-frame transforms. These are target constants, not a new light.
std::array<std::uint8_t, 256> make_lighting(unsigned mode, const Matrix3D& scene) {
    std::array<std::uint8_t, 256> table{};
    const auto world = voxel_light_direction();
    Vector3D<float> light{matrix_store_float(double(scene.row[0][0]) * world.X + double(scene.row[1][0]) * world.Y +
                                             double(scene.row[2][0]) * world.Z),
                          matrix_store_float(double(scene.row[0][1]) * world.X + double(scene.row[1][1]) * world.Y +
                                             double(scene.row[2][1]) * world.Z),
                          matrix_store_float(double(scene.row[0][2]) * world.X + double(scene.row[1][2]) * world.Y +
                                             double(scene.row[2][2]) * world.Z)};
    const auto half = normalized(Vector3D<float>{light.X, light.Y, matrix_store_float(double(light.Z) + 1)});
    if (mode > 0 && mode <= 4)
        for (unsigned i = 0; i < normal_counts[mode]; ++i) {
            const auto n = normal_tables[mode][i];
            const double diffuse = std::max(0.0, dot(n, light)), h = dot(n, half);
            const float specular = matrix_store_float(std::max(0.0, h / (3.0 - 2.0 * h)));
            table[i] = std::uint8_t(std::clamp(int((diffuse + specular) * 16), 0, 255));
        }
    table[253] = table[254] = table[255] = 16;
    return table;
}
std::uint8_t lit_color(const Sample& s, const std::array<std::uint8_t, 256>& table, const VoxelPalette& p) {
    if (!s.color) return 0;
    return p.lighting[std::min(unsigned(table[s.normal]), p.sections - 1) * 256 + s.color];
}
}
namespace {
struct VoxelPartsSink {
    BuildingVoxelPart* parts;
    unsigned capacity, count = 0;
    ObjectTypeClass* type;
};
void bind_voxel_collection(TechnoDrawing& drawing, VoxelPartsSink& sink) {
    drawing.context = &sink;
    drawing.palette = [](void*, const TechnoClass&, TechnoPalette, CellClass*, HouseClass*,
                         const DrawingPaletteHandle*& output) noexcept {
        output = nullptr;
        return DrawingStatus::drawn;
    };
    drawing.voxel = [](void* p, const TechnoVoxelRequest& r) noexcept {
        auto& sink = *static_cast<VoxelPartsSink*>(p);
        if (sink.count >= sink.capacity) return DrawingStatus::invalid_argument;
        auto& part = sink.parts[sink.count++];
        part.resource = r.resource;
        part.local = r.local_matrix ? *r.local_matrix : r.matrix;
        part.frame = unsigned(r.frame);
        part.use_buffer = r.use_buffer;
        part.shadow = r.shadow;
        part.cache_key = r.cache_key;
        part.shadow_layer = r.shadow_layer;
        part.half_shadow = r.half_shadow;
        part.camera_applied = !r.local_matrix;
        part.barrel = r.resource == &sink.type->BarrelVoxel;
        for (const auto& barrel : sink.type->ChargerBarrels)
            if (r.resource == &barrel) part.barrel = true;
        return DrawingStatus::drawn;
    };
}
}
// Legacy internal inspection adapters. Algorithms execute the same original
// methods used by the map; these functions only collect their borrowed output.
bool unit_voxel_parts(UnitClass& unit, BuildingVoxelPart* parts, unsigned capacity, unsigned& count) noexcept {
    count = 0;
    if (!parts || capacity < 4 || !unit.Type || !unit.Locomotor) return false;
    auto* effective = unit.Unloading && unit.Type->Harvester && unit.Type->UnloadingClass ? unit.Type->UnloadingClass : unit.Type;
    VoxelPartsSink sink{parts, capacity, 0, effective};
    TechnoDrawing drawing;
    drawing.inspect_voxel_parts = true;
    bind_voxel_collection(drawing, sink);
    const auto status =
        with_techno_drawing(
            drawing,
            [](void* p, game::PresentationContext& ctx) {
                const auto& unit = *static_cast<UnitClass*>(p);
                ctx.unit_type = unit.Unloading && unit.Type->Harvester && unit.Type->UnloadingClass ? unit.Type->UnloadingClass : unit.Type;
                build_unit_vxl(ctx, unit, {}, {}, 1000, 0);
            },
            &unit);
    count = sink.count;
    return techno_drawing_complete(status) && count != 0;
}
bool load_voxel_palette(VoxelPalette& out) noexcept {
    try {
        out = {};
        CCFileClass file("VOXELS.VPL");
        if (!file.Open(FileAccessMode::Read)) return false;
        struct Header {
            std::uint32_t first, last, count, reserved;
        } h{};
        std::uint8_t pal[768];
        bool ok = file.ReadBytes(&h, sizeof(h)) == sizeof(h) && h.first <= h.last && h.last < 256 && h.count > 0 &&
                  h.count <= 128 && file.ReadBytes(pal, sizeof(pal)) == sizeof(pal);
        if (ok) {
            out.lighting.resize(h.count * 256);
            ok = file.ReadBytes(out.lighting.data(), int(out.lighting.size())) == int(out.lighting.size());
        }
        file.Close();
        if (!ok) {
            out = {};
            return false;
        }
        out.sections = h.count;
        return true;
    } catch (...) {
        out = {};
        return false;
    }
}
Matrix3D building_voxel_camera() noexcept {
    return multiply(rotation(0, matrix_store_float(-pi / 3)), rotation(2, matrix_store_float(-pi / 4)));
}
bool building_voxel_plan(const BuildingClass& building, BuildingVoxelPlan& output) noexcept {
    output.count = 0;
    output.mixed = false;
    output.barrel_after_shp = false;
    if (!building.Type) return false;
    VoxelPartsSink sink{output.parts.data(), unsigned(output.parts.size()), 0, building.Type};
    TechnoDrawing drawing;
    drawing.inspect_voxel_parts = true;
    bind_voxel_collection(drawing, sink);
    const auto status = with_techno_drawing(
        drawing,
        [](void* p, game::PresentationContext& ctx) {
            build_building_upper(ctx, *static_cast<BuildingClass*>(p), {}, {});
        },
        const_cast<BuildingClass*>(&building));
    output.count = sink.count;
    // These are presentation-inspection labels, not an independent draw path.
    output.mixed = building.Type->BarrelAnimIsVoxel && !building.Type->TurretAnimIsVoxel;
    output.barrel_after_shp = output.mixed && ((direction(building.PrimaryFacing.Current().Raw) + 28) % 32) <= 16;
    return techno_drawing_complete(status) && output.count != 0;
}
DrawingStatus composite_unit_voxel(BSurface& target, Point2D point, const RectangleStruct& clip,
                                   const VoxelSurface& source) noexcept {
    if (target.BytesPerPixel != 1 || !target.Buffer.Buffer || target.Width <= 0 || target.Height <= 0 ||
        std::int64_t(target.Width) * target.Height > target.Buffer.Size || source.width < 0 || source.height < 0 ||
        std::uint64_t(source.width) * source.height > source.pixels.size())
        return DrawingStatus::invalid_argument;
    const int left = clip.X + point.X + source.offset.X, top = clip.Y + point.Y + source.offset.Y;
    const int x0 = std::max({left, clip.X, 0}), y0 = std::max({top, clip.Y, 0});
    const int x1 = std::min({left + source.width, clip.X + clip.Width, target.Width});
    const int y1 = std::min({top + source.height, clip.Y + clip.Height, target.Height});
    auto* pixels = static_cast<unsigned char*>(target.Buffer.Buffer);
    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x) {
            const auto index =
                static_cast<unsigned char>(source.pixels[std::size_t(y - top) * source.width + x - left]);
            if (index) pixels[std::size_t(y) * target.Width + x] = index;
        }
    return DrawingStatus::drawn;
}
DrawingStatus raster_voxel_shadow_columns(const std::int32_t* spans, unsigned nx, unsigned ny,
                                          const Vector3D<float>* points, Point2D& offset, float cx, float cy,
                                          Surface* exclusion, std::uint8_t* pixels,
                                          std::uint32_t pixel_count) noexcept {
    try {
        if (!spans || !points || !pixels || pixel_count < 0x10001 || !nx || !ny || nx > 255 || ny > 255)
            return DrawingStatus::invalid_argument;
        if (!std::isfinite(cx) || !std::isfinite(cy) || std::abs(cx) > 100000 || std::abs(cy) > 100000)
            return DrawingStatus::invalid_argument;
        for (unsigned i = 0; i < 4; ++i)
            if (!std::isfinite(points[i].X) || !std::isfinite(points[i].Y) || std::abs(points[i].X) > 100000 ||
                std::abs(points[i].Y) > 100000)
                return DrawingStatus::invalid_argument;
        const double ox = double(offset.X) + cx, oy = double(offset.Y) + cy;
        if (ox < std::numeric_limits<int>::min() || ox > std::numeric_limits<int>::max() ||
            oy < std::numeric_limits<int>::min() || oy > std::numeric_limits<int>::max())
            return DrawingStatus::unsupported;
        const auto sx = std::uint16_t(int((double(points[2].X) + 128 - cx) * 256)),
                   sy = std::uint16_t(int((double(points[2].Y) + 128 - cy) * 256));
        const auto xx = std::uint16_t(int((double(points[1].X) - points[2].X) / nx * 256)),
                   xy = std::uint16_t(int((double(points[1].Y) - points[2].Y) / nx * 256));
        const auto yx = std::uint16_t(int((double(points[3].X) - points[2].X) / ny * 256)),
                   yy = std::uint16_t(int((double(points[3].Y) - points[2].Y) / ny * 256));
        // The original truncates after adding the centroid to the integer offset,
        // then narrows each sampled coordinate to an unsigned 16-bit word.
        offset.X = int(double(offset.X) + cx);
        offset.Y = int(double(offset.Y) + cy);
        for (unsigned y = 0; y < ny; ++y)
            for (unsigned x = 0; x < nx; ++x) {
                if (spans[y * nx + x] == -1) continue;
                const unsigned px = std::uint16_t(sx + int(x) * xx + int(y) * yx) >> 8,
                               py = std::uint16_t(sy + int(x) * xy + int(y) * yy) >> 8;
                const unsigned index = py * 256 + px;
                if (exclusion) {
                    Point2D first{int(std::uint16_t(px + offset.X)), int(std::uint16_t(py + offset.Y))};
                    Point2D second{first.X + 1, first.Y};
                    const bool clear_first = exclusion->GetPixel(&first) == 0;
                    const bool clear_second = exclusion->GetPixel(&second) == 0;
                    pixels[index] = clear_first;
                    pixels[index + 1] = clear_second;
                } else {
                    pixels[index] = 1;
                    pixels[index + 1] = 1;
                }
            }
        return DrawingStatus::drawn;
    } catch (...) {
        return DrawingStatus::backend_failure;
    }
}
static DrawingStatus render_voxel(const BuildingVoxelPart& part, const VoxelPalette& palette, VoxelSurface& out,
                                  VoxelGpuAssets* gpu_assets) noexcept {
    try {
        out = {};
        if (!part.resource || !ready(*part.resource, part.apply_hva) || !palette.sections ||
            palette.lighting.size() != palette.sections * 256)
            return DrawingStatus::unavailable;
        if (gpu_assets) out.gpu = std::make_shared<VoxelImageHandle>();
        const auto& v = *part.resource->VXL;
        const auto* h = part.apply_hva ? part.resource->HVA : nullptr;
        const auto camera = part.camera_applied ? Matrix3D::GetIdentity() : building_voxel_camera();
        if (part.shadow) {
            // 0x753F90 / 0x756860 project occupied XY columns from the section's
            // bottom plane. Shadow spans always cover the sample and its right
            // neighbour; flattening the full model before the camera is different.
            const int layer = part.shadow_layer;
            if (layer < 0 || unsigned(layer) >= v.CountHeaders) return DrawingStatus::invalid_argument;
            const int limb = v.HeaderData[layer].limb_number;
            if (limb < 0 || unsigned(limb) >= v.CountTailers) return DrawingStatus::invalid_argument;
            const auto& t = v.TailerData[limb];
            const unsigned nx = std::uint8_t(t.size_X), ny = std::uint8_t(t.size_Y);
            if (!nx || !ny) return DrawingStatus::invalid_argument;
            const auto matrix = multiply(multiply(camera, part.local), (h ? h->GetLayerMatrix(layer, 0) : Matrix3D::GetIdentity()));
            if (!finite(matrix)) return DrawingStatus::invalid_argument;
            Vector3D<float> points[4], lo{1e9f, 1e9f, 0}, hi{-1e9f, -1e9f, 0};
            // 0x754C00 derives the shadow displacement from the same light vector.
            const float displacement = matrix_store_float(double(voxel_light_direction().X) * -6);
            for (unsigned i = 0; i < 4; ++i) {
                auto p = transform(matrix, t.Bounds[i]);
                if (part.half_shadow) {
                    p.X = matrix_store_float(double(p.X) * 0.5);
                    p.Y = matrix_store_float(double(p.Y) * 0.5);
                }
                p.X = matrix_store_float(double(p.X) + displacement);
                p.Y = -p.Y;
                points[i] = p;
                lo.X = std::min(lo.X, p.X);
                lo.Y = std::min(lo.Y, p.Y);
                hi.X = std::max(hi.X, p.X);
                hi.Y = std::max(hi.Y, p.Y);
            }
            if (hi.X - lo.X > 250 || hi.Y - lo.Y > 250) return DrawingStatus::unsupported;
            const float cx = matrix_store_float((double(lo.X) + hi.X) * 0.5),
                    cy = matrix_store_float((double(lo.Y) + hi.Y) * 0.5);
            if (gpu_assets) {
                auto& image = *out.gpu;
                image.kind = VoxelImageKind::shadow;
                auto found = gpu_assets->shadows.find(&t);
                if (found == gpu_assets->shadows.end()) {
                    std::vector<Sample> validation;
                    if (!decode(v, t, validation)) return DrawingStatus::invalid_argument;
                    auto columns = std::make_shared<VoxelGpuBuffer>();
                    columns->words.reserve(nx * ny);
                    for (unsigned i = 0; i < nx * ny; ++i)
                        columns->words.push_back(t.span_end_off[i] != -1);
                    found = gpu_assets->shadows.emplace(&t, std::move(columns)).first;
                }
                image.columns = found->second;
                image.shadow = {int(std::uint16_t(int((double(points[2].X) + 128 - cx) * 256))),
                                int(std::uint16_t(int((double(points[2].Y) + 128 - cy) * 256))),
                                int(std::uint16_t(int((double(points[1].X) - points[2].X) / nx * 256))),
                                int(std::uint16_t(int((double(points[1].Y) - points[2].Y) / nx * 256))),
                                int(std::uint16_t(int((double(points[3].X) - points[2].X) / ny * 256))),
                                int(std::uint16_t(int((double(points[3].Y) - points[2].Y) / ny * 256))),
                                int(nx),
                                int(ny),
                                int(double(part.shadow_offset.X) + cx),
                                int(double(part.shadow_offset.Y) + cy)};
                return gpu_bounds(lo, hi, cx, cy, out);
            }
            std::vector<Sample> samples;
            if (!decode(v, t, samples)) return DrawingStatus::invalid_argument;
            std::vector<std::uint8_t> columns(0x10001);
            auto offset = part.shadow_offset;
            const auto status =
                raster_voxel_shadow_columns(t.span_end_off, nx, ny, points, offset, cx, cy, part.shadow_exclusion,
                                            columns.data(), std::uint32_t(columns.size()));
            if (status != DrawingStatus::drawn) return status;
            std::vector<std::uint32_t> pixels(256 * 256);
            for (unsigned i = 0; i < pixels.size(); ++i)
                if (columns[i]) pixels[i] = 0x1000001u;
            return copy_surface(pixels, lo, hi, cx, cy, out, true);
        }

        struct Section {
            const VoxelSectionTailer* t;
            Matrix3D local;
            std::array<Vector3D<float>, 8> points;
            std::vector<Sample> samples;
            unsigned corner = 0;
        };
        std::vector<Section> sections;
        Vector3D<float> lo{1e9f, 1e9f, 1e9f}, hi{-1e9f, -1e9f, -1e9f};
        if (part.voxel_index < -1 || (part.voxel_index >= 0 && unsigned(part.voxel_index) >= v.CountHeaders))
            return DrawingStatus::invalid_argument;
        const unsigned first = part.voxel_index < 0 ? 0 : unsigned(part.voxel_index),
                       last = part.voxel_index < 0 ? v.CountHeaders : first + 1;
        for (unsigned i = first; i < last; ++i) {
            const int limb = v.HeaderData[i].limb_number;
            if (limb < 0 || unsigned(limb) >= v.CountTailers) return DrawingStatus::invalid_argument;
            Section s;
            s.t = &v.TailerData[limb];
            s.local = part.local * (h ? h->GetLayerMatrix(i, part.frame) : Matrix3D::GetIdentity());
            if (!finite(s.local)) return DrawingStatus::invalid_argument;
            const auto matrix =
                multiply(multiply(camera, part.local), (h ? h->GetLayerMatrix(i, part.frame) : Matrix3D::GetIdentity()));
            for (unsigned k = 0; k < 8; ++k) {
                auto p = transform(matrix, s.t->Bounds[k]);
                p.Y = -p.Y;
                s.points[k] = p;
                if (k && p.Z < s.points[s.corner].Z) s.corner = k;
                if (!std::isfinite(p.X) || !std::isfinite(p.Y) || !std::isfinite(p.Z))
                    return DrawingStatus::invalid_argument;
                lo.X = std::min(lo.X, p.X);
                lo.Y = std::min(lo.Y, p.Y);
                lo.Z = std::min(lo.Z, p.Z);
                hi.X = std::max(hi.X, p.X);
                hi.Y = std::max(hi.Y, p.Y);
                hi.Z = std::max(hi.Z, p.Z);
            }
            if (!gpu_assets && !decode(v, *s.t, s.samples)) return DrawingStatus::invalid_argument;
            sections.push_back(std::move(s));
        }
        const auto lighting =
            make_lighting(std::uint8_t(sections.front().t->NormalsMode), multiply(camera, part.local));
        if (hi.X - lo.X > 250 || hi.Y - lo.Y > 250 || hi.Z - lo.Z > 250) return DrawingStatus::unsupported;
        const Vector3D<float> center{matrix_store_float((double(lo.X) + hi.X) * 0.5),
                                     matrix_store_float((double(lo.Y) + hi.Y) * 0.5),
                                     matrix_store_float((double(lo.Z) + hi.Z) * 0.5)};
        if (gpu_assets) {
            if (!gpu_assets->palette) {
                auto p = std::make_shared<VoxelGpuBuffer>();
                p->words.assign(palette.lighting.begin(), palette.lighting.end());
                gpu_assets->palette = std::move(p);
            }
            out.gpu->palette = gpu_assets->palette;
            out.gpu->buffered = part.use_buffer;
            std::copy(lighting.begin(), lighting.end(), out.gpu->lighting.begin());
        }
        std::vector<std::uint32_t> pixels(gpu_assets ? 0 : 256 * 256);
        std::vector<unsigned char> depth(gpu_assets ? 0 : 256 * 256);
        constexpr int corners[8][8] = {
            {0, 3, 1, 4, 1, 1, -1, -1}, {1, 2, 0, 5, 1, 0, -1, 1}, {2, 1, 3, 6, 0, 0, 1, 1}, {3, 0, 2, 7, 0, 1, 1, -1},
            {4, 7, 5, 0, 1, 1, -1, -1}, {5, 6, 4, 1, 1, 0, -1, 1}, {6, 5, 7, 2, 0, 0, 1, 1}, {7, 4, 6, 3, 0, 1, 1, -1}};
        auto maximum_z = [](const auto& s) {
            float z = s.points[0].Z;
            for (const auto& p : s.points)
                z = std::max(z, p.Z);
            return z;
        };
        std::stable_sort(sections.begin(), sections.end(),
                         [&](const auto& a, const auto& b) { return maximum_z(a) < maximum_z(b); });
        if (part.use_buffer) std::reverse(sections.begin(), sections.end());
        for (auto& s : sections) {
            const auto* c = corners[s.corner];
            const auto origin = s.points[c[0]];
            int start[3], step[3][3];
            const unsigned sizes[3] = {std::uint8_t(s.t->size_X), std::uint8_t(s.t->size_Y), std::uint8_t(s.t->size_Z)};
            for (unsigned axis = 0; axis < 3; ++axis) {
                const float o = axis == 0   ? origin.X
                                : axis == 1 ? origin.Y
                                            : origin.Z,
                            mid = axis == 0   ? center.X
                                  : axis == 1 ? center.Y
                                              : center.Z;
                start[axis] = int((double(o) + 128 - mid) * 256);
                for (unsigned d = 0; d < 3; ++d) {
                    const auto& q = s.points[c[d + 1]];
                    float end = axis == 0 ? q.X : axis == 1 ? q.Y : q.Z;
                    step[d][axis] = int((double(end) - o) / sizes[d] * 256);
                }
            }
            if (gpu_assets) {
                auto source = gpu_section(*gpu_assets, v, *s.t, s.corner);
                if (!source) return DrawingStatus::invalid_argument;
                VoxelSectionView section;
                section.source = source->view();
                for (int a = 0; a < 3; ++a)
                    section.parameters[a] = start[a];
                for (int d = 0; d < 3; ++d)
                    for (int a = 0; a < 3; ++a)
                        section.parameters[3 + d * 3 + a] = step[d][a];
                section.parameters[12] = int(center.Z);
                section.parameters[13] = int(sizes[0] | (sizes[1] << 8) | (sizes[2] << 16));
                section.parameters[14] = int(s.corner);
                out.gpu->sections.push_back(section);
                out.gpu->sources.push_back(std::move(source));
                continue;
            }
            std::stable_sort(s.samples.begin(), s.samples.end(), [&](const auto& a, const auto& b) {
                if (a.y != b.y) return c[7] > 0 ? a.y < b.y : a.y > b.y;
                if (a.x != b.x) return c[6] > 0 ? a.x < b.x : a.x > b.x;
                return s.corner < 4 ? a.z < b.z : a.z > b.z;
            });
            auto packed_xy = [](int x, int y) {
                return std::uint32_t(std::uint16_t(x)) | (std::uint32_t(std::uint16_t(y)) << 16);
            };
            unsigned last_x = UINT32_MAX, last_y = UINT32_MAX, cursor_z = 0;
            std::uint32_t xy = 0;
            for (const auto& sample : s.samples) {
                unsigned ijk[3] = {c[4] ? sizes[0] - 1 - sample.x : sample.x, c[5] ? sizes[1] - 1 - sample.y : sample.y,
                                   s.corner >= 4 ? sizes[2] - 1 - sample.z : sample.z};
                unsigned p[3];
                for (unsigned a = 0; a < 3; ++a)
                    p[a] = std::uint16_t(start[a] + int(ijk[0]) * step[0][a] + int(ijk[1]) * step[1][a] +
                                         int(ijk[2]) * step[2][a]) >>
                           8;
                if (!part.use_buffer) {
                    // 0x007DF9C0 / 0x007DFAE0 add packed X/Y words: X carries into Y.
                    // A skipped run uses a separately accumulated pair, unlike dense voxels.
                    if (last_x != sample.x || last_y != sample.y) {
                        xy = packed_xy(start[0], start[1]) + ijk[0] * packed_xy(step[0][0], step[0][1]) +
                             ijk[1] * packed_xy(step[1][0], step[1][1]);
                        cursor_z = 0;
                        last_x = sample.x;
                        last_y = sample.y;
                    }
                    const int skip = int(ijk[2] - cursor_z);
                    xy += packed_xy(skip * step[2][0], skip * step[2][1]);
                    p[0] = (xy >> 8) & 255;
                    p[1] = xy >> 24;
                    xy += packed_xy(step[2][0], step[2][1]);
                    cursor_z = ijk[2] + 1;
                }
                const auto color = lit_color(sample, lighting, palette);
                const auto index = p[1] * 256 + p[0];
                if (part.use_buffer && p[2] <= depth[index]) continue;
                // Original depth-enabled span routine writes the sample and its right-hand
                // neighbor together after testing the first pixel (0x00757279..0x00757294).
                const auto delta = std::int16_t(-int(p[2]) + 128 - int(center.Z));
                const auto packed = 0x1000000u | (std::uint32_t(std::uint16_t(delta)) << 8) | color;
                pixels[index] = packed;
                depth[index] = p[2];
                if (part.use_buffer && p[0] < 255) {
                    pixels[index + 1] = packed;
                    depth[index + 1] = p[2];
                }
            }
        }
        if (gpu_assets) {
            if (std::none_of(out.gpu->sources.begin(), out.gpu->sources.end(),
                             [](const auto& source) { return source->words[1] != 0; }))
                return DrawingStatus::skipped;
            return gpu_bounds(lo, hi, center.X, center.Y, out);
        }
        return copy_surface(pixels, lo, hi, center.X, center.Y, out);
    } catch (...) {
        out = {};
        return DrawingStatus::backend_failure;
    }
}
DrawingStatus render_building_voxel(const BuildingVoxelPart& part, const VoxelPalette& palette,
                                    VoxelSurface& out) noexcept {
    return render_voxel(part, palette, out, nullptr);
}
DrawingStatus prepare_building_voxel(const BuildingVoxelPart& part, const VoxelPalette& palette, VoxelGpuAssets& assets,
                                     VoxelSurface& out) noexcept {
    return render_voxel(part, palette, out, &assets);
}
std::uint64_t building_voxel_signature(const BuildingClass& b) noexcept {
    std::uint64_t h = 1469598103934665603ull;
    auto add = [&](std::uint64_t v) {
        h ^= v;
        h *= 1099511628211ull;
    };
    add(voxel_resource_revision());
    add(b.PrimaryFacing.Current().Raw);
    add(b.BarrelFacing.Current().Raw);
    add(b.TurretAnimFrame);
    add(unsigned(b.CurrentMission));
    add(unsigned(b.QueuedMission));
    add(b.Animation.Value);
    add(b.Health);
    add(b.Type && b.Type->UseBuffer);
    add(std::bit_cast<std::uint32_t>(b.TurretRecoil.TravelSoFar));
    add(std::bit_cast<std::uint32_t>(b.BarrelRecoil.TravelSoFar));
    add(unsigned(b.TurretRecoil.State));
    add(unsigned(b.BarrelRecoil.State));
    return h;
}
}
