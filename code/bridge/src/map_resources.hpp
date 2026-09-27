#pragma once
#include "api/map_view.hpp"
#include "type_drawing_packets.hpp"
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <array>
#include <map>
#include <memory>
#include <vector>
#include <tuple>

namespace game {
// Bridge-owned copies survive original TMP/PAL teardown and queued GPU work.
struct MapBufferData { std::uint64_t id=0; godot::PackedByteArray bytes; };
struct MapTileData { std::shared_ptr<MapBufferData> base,extra; };
struct MapPaletteData { std::shared_ptr<MapBufferData> buffer; int shades=53; };
struct MapDrawPacket {
    std::array<std::int32_t,20> parameters{};
    std::shared_ptr<MapBufferData> source,palette;
    bool lighting=false;
    bool cached_lighting=false;
};
struct MapLightingImage {RectangleStruct bounds{};std::shared_ptr<MapBufferData> source;};
struct MapFrameData {
    std::uint64_t generation=0;
    int width=0,height=0;
    std::vector<MapDrawPacket> packets;
    MapDrawStatistics stats{};
};
class MapResources {
public:
    MapDrawingContext begin_frame(MapFrameData& frame);
    void end_frame() noexcept { frame_=nullptr; }
    void clear();
    const char* error() const noexcept { return error_; }
    std::size_t decoded_tiles() const { return tiles_.size(); }
    std::size_t prepared_palettes() const { return palettes_.size(); }
    std::size_t decoded_lighting() const { return lighting_images_.size(); }
private:
    std::uint64_t next_id_=1;
    MapFrameData* frame_=nullptr;
    char error_[768]{};
    std::map<std::pair<const TMPImage*,bool>,MapTileData> tiles_;
    std::map<std::pair<const BytePalette*,std::array<int,5>>,MapPaletteData> palettes_;
    std::map<std::tuple<const SHPStruct*,int,const SHPStruct*,int,int,int>,std::shared_ptr<MapBufferData>> shapes_;
    std::map<WORD,std::shared_ptr<MapBufferData>> fills_;
    std::map<std::pair<const SHPStruct*,int>,MapLightingImage> lighting_images_;
    std::shared_ptr<MapBufferData> buffer(const std::vector<std::uint32_t>&);
    static DrawingStatus palette(void*,const BytePalette&,int,int,int,int,const DrawingPaletteHandle*&) noexcept;
    static DrawingStatus shape_palette(void*,const BytePalette&,int,const DrawingPaletteHandle*&) noexcept;
    static DrawingStatus color_scheme_palette(void*,const BytePalette&,int,const DrawingPaletteHandle*&) noexcept;
    enum class PaletteKind { terrain, shape, color_scheme };
    static DrawingStatus prepare_palette(void*,const BytePalette&,int,int,int,int,PaletteKind,const DrawingPaletteHandle*&) noexcept;
    static DrawingStatus tile(void*,const TileDrawingRequest&);
    static DrawingStatus shape(void*,const ShapeDrawingRequest&);
    static DrawingStatus indexed(void*,const IndexedDrawingRequest&);
    static DrawingStatus raster(void*,const RasterDrawingRequest&);
    static DrawingStatus lighting_shape(void*,const LightingShapeDrawingRequest&);
};
}
