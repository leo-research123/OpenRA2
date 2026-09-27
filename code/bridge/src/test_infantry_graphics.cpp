// Resource decoding / atlas preparation for the existing test scene.
#include "bridge/test_infantry.hpp"
#include "api/images.hpp"
#include "api/filesystem.hpp"
#include "yrpp/FileFormats/SHP.h"
#include "yrpp/MixFileClass.h"
#include "yrpp/CCFileClass.h"
#include "yrpp/CCINIClass.h"
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <algorithm>
#include <memory>
#include <sstream>
#include <stdexcept>


namespace {
std::vector<uint8_t> read_resource(const char* name) {
    CCFileClass file(name);
    if (!file.Open(FileAccessMode::Read)) throw std::runtime_error(std::string("Missing resource: ") + name);
    const int size = file.GetFileSize();
    if (size < 0 || size > 64 * 1024 * 1024) throw std::runtime_error("Invalid image resource size");
    std::vector<uint8_t> bytes(static_cast<size_t>(size));
    if (size && file.ReadBytes(bytes.data(), size) != size)
        throw std::runtime_error(std::string("Truncated resource: ") + name);
    return bytes;
}


struct ShapeUnloadScope {
    ~ShapeUnloadScope() noexcept { Unload_All_Shapes(); }
};
void destroy_graphics_resources(game::ResourceHandle* resources) noexcept {
    if (!resources) return;
    Unload_All_Shapes();
    game::destroy_resources(resources);
}
}

godot::String RA2TestInfantry::resource_directory() const {
    auto* os = godot::OS::get_singleton();
    godot::String path = game_data_path_;
    if (path.is_empty()) path = os->get_environment("RA2_GAME_DATA");
    for (const auto& argument : os->get_cmdline_user_args())
        if (argument.begins_with("--game-data=")) path = argument.substr(12);
    if (path.is_empty()) path = os->has_feature("editor")
        ? "res://../../out/reference/RA2MDddcompact" : os->get_executable_path().get_base_dir();
    if (!path.is_absolute_path()) path = godot::String("res://").path_join(path);
    return godot::ProjectSettings::get_singleton()->globalize_path(path);
}

void RA2TestInfantry::load_graphics() {
    // This test owns a short resource session while preparing the atlas. No
    // second file lookup/cache model, and no game objects created for the demo.
    const auto directory = resource_directory().utf8();
    game::ResourceHandle* resources = nullptr;
    std::string error;
    if (!game::create_resources(directory.get_data(), resources, error)) throw std::runtime_error(error);
    const std::unique_ptr<game::ResourceHandle, decltype(&destroy_graphics_resources)>
        files(resources, destroy_graphics_resources);
    if (!game::with_resources(*files, [](void* context) {
        static_cast<RA2TestInfantry*>(context)->load_graphics_in_context();
    }, this, error)) throw std::runtime_error(error);
}

void RA2TestInfantry::load_graphics_in_context() {
    if (!MixFileClass::Bootstrap()) throw std::runtime_error("Resource bootstrap failed");
    // Infantry graphics belong to the subsequent conquer package group. Only
    // mount the two packages needed by this independent graphics experiment.
    std::vector<std::unique_ptr<MixFileClass>> extra_mixes;
    // Declared after the packages, before SHP users: unload after local image
    // references die, before the extra packages are destroyed (also on failure).
    const ShapeUnloadScope unload_before_packages;
    for (const char* name : {"conqmd.mix", "conquer.mix"}) {
        CCFileClass file(name);
        if (file.Exists()) extra_mixes.push_back(std::make_unique<MixFileClass>(name));
    }
    CCINIClass art;
    if (art.LoadFromFile("artmd.ini") != 1 && art.LoadFromFile("art.ini") != 1)
        throw std::runtime_error("Missing artmd.ini / art.ini");
    char sequence[128]{};
    art.ReadString("GI", "Sequence", "", sequence, sizeof(sequence));
    if (!sequence[0]) throw std::runtime_error("[GI] Sequence is missing from art INI");
    char walk[128]{};
    art.ReadString(sequence, "Walk", "", walk, sizeof(walk));
    std::string numbers(walk);
    std::replace(numbers.begin(), numbers.end(), ',', ' ');
    std::istringstream values(numbers);
    if (!(values >> walk_start_ >> walk_count_ >> walk_stride_) || walk_start_ < 0 ||
        walk_count_ < 1 || walk_count_ > 64 || walk_stride_ < walk_count_)
        throw std::runtime_error("Invalid eight-direction Walk sequence");
    SHPReference shp("gi.shp");
    const auto pal = read_resource("unittem.pal");
    if (pal.size() != 768) throw std::runtime_error("unittem.pal must contain 256 RGB entries");
    std::string error;
    shp.Load();
    if (!shp.Loaded || !shp.Data) throw std::runtime_error("Cannot load gi.shp");
    auto& data = *shp.GetData();
    canvas_width_ = data.Width;
    canvas_height_ = data.Height;
    if (int64_t(walk_start_) + 7ll * walk_stride_ + walk_count_ > data.Frames)
        throw std::runtime_error("Walk sequence exceeds GI SHP frames");
    if (canvas_width_ < 1 || canvas_height_ < 1 || canvas_width_ > 512 || canvas_height_ > 512)
        throw std::runtime_error("GI canvas dimensions do not fit the test1 atlas");

    // One atlas contains every direction/phase. Cropped frames and their original
    // anchors are kept separately; each cell has a one-pixel transparent gutter.
    const int cell_width = canvas_width_ + 2, cell_height = canvas_height_ + 2;
    const int atlas_width = cell_width * 8, atlas_height = cell_height * walk_count_;
    godot::PackedByteArray atlas_bytes;
    atlas_bytes.resize(int64_t(atlas_width) * atlas_height);
    std::fill_n(atlas_bytes.ptrw(), atlas_bytes.size(), uint8_t(0));
    auto* destination = atlas_bytes.ptrw();
    frames_.clear(); frames_.reserve(size_t(walk_count_) * 8);
    for (int direction = 0; direction < 8; ++direction) {
        for (int phase = 0; phase < walk_count_; ++phase) {
            const int index = walk_start_ + direction * walk_stride_ + phase;
            const auto bounds = data.GetFrameBounds(index);
            const auto* payload = data.GetPixels(index);
            const bool compressed = data.HasCompression(index);
            std::vector<uint8_t> pixels;
            if (!payload || bounds.Width < 1 || bounds.Height < 1)
                throw std::runtime_error("GI Walk contains an empty frame");
            if (bounds.Width > canvas_width_ || bounds.Height > canvas_height_)
                throw std::runtime_error("GI frame dimensions do not fit the test1 atlas");
            if (!game::decode_shp_pixels(payload, bounds.Width, bounds.Height, compressed, pixels, error))
                throw std::runtime_error("GI frame " + std::to_string(index) + ": " + error);
            const int left = direction * cell_width + 1, top = phase * cell_height + 1;
            for (int y = 0; y < bounds.Height; ++y)
                std::copy_n(pixels.data() + y * bounds.Width, bounds.Width,
                    destination + (top + y) * atlas_width + left);
            frames_.push_back({index, bounds, float(left) / atlas_width, float(top) / atlas_height,
                float(bounds.Width) / atlas_width, float(bounds.Height) / atlas_height});
            const godot::Rect2 rect(bounds.X - canvas_width_ / 2, bounds.Y - canvas_height_ / 2,
                bounds.Width, bounds.Height);
            frame_envelope_ = frames_.size() == 1 ? rect : frame_envelope_.merge(rect);
        }
    }
    atlas_ = godot::ImageTexture::create_from_image(godot::Image::create_from_data(
        atlas_width, atlas_height, false, godot::Image::FORMAT_L8, atlas_bytes));
    godot::PackedByteArray palette_bytes;
    palette_bytes.resize(256 * 4);
    auto* colors = palette_bytes.ptrw();
    for (int i = 0; i < 256; ++i) {
        for (int channel = 0; channel < 3; ++channel)
            colors[i * 4 + channel] = uint8_t(pal[i * 3 + channel] << 2); // original PAL conversion
        colors[i * 4 + 3] = i ? 255 : 0;
    }
    palette_ = godot::ImageTexture::create_from_image(godot::Image::create_from_data(
        256, 1, false, godot::Image::FORMAT_RGBA8, palette_bytes));
    if (atlas_.is_null() || palette_.is_null() ||
        !renderer_.initialize(get_canvas_item(), atlas_->get_rid(), instance_count_, palette_->get_rid()))
        throw std::runtime_error("Cannot create the indexed sprite renderer");
}
