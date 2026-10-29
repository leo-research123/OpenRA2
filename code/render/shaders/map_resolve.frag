#version 450

// Lighting is map_raster.vert/.frag's resident path: the record's light sources
// (cell intensity and tint, converter slot) and the light field at this pixel.
// Shadows halve the color once per shadow over the material, as the raster
// shadow blend did.
layout(set = 0, binding = 0, std430) readonly buffer Records {
    int record_words[];
};
layout(set = 0, binding = 1) uniform usampler2DArray resource_pages;
uint resource_word(int index) {
    return texelFetch(resource_pages, ivec3(index & 1023, (index >> 10) & 1023, index >> 20), 0).r;
}
#ifndef RING_TILE
layout(set = 0, binding = 2, std430) readonly buffer Lighting {
    uint light_intensities[];
};
layout(set = 0, binding = 3, std430) readonly buffer Pages {
    int page_layers[];
};
#endif
layout(set = 0, binding = 4, std430) readonly buffer CellLights {
    int cell_light_words[];
};
layout(set = 0, binding = 5, std430) readonly buffer ConverterSlots {
    int converter_offsets[];
};
layout(set = 0, binding = 6) uniform usampler2DArray materials;
layout(set = 0, binding = 7) uniform sampler2DArray shadows;
layout(set = 0, binding = 8) uniform sampler2DArray depths;
layout(set = 0, binding = 9) uniform usampler2D light_ring;
#ifdef RING_TILE
layout(push_constant, std430) uniform Tile {
    int world_x;
    int world_y;
    int ring_x;
    int ring_y;
    int layer;
    int light_mask_x;
    int light_mask_y;
    int ring_width;
    int ring_height;
    int reserved0;
    int reserved1;
    int reserved2;
} tile;
// Packed texel of the lit background ring: x = world Z * 2^24 (0..2^24; bit 31: no pixel), y = the lit color
// as RGB8 (red in the low byte), the value the frame holds after the background.
layout(location = 0) out uvec4 packed_out;
#else
// Frame: world Z flag, camera y and target size; then the light mode (1:
// fragments read the light ring of map_light_page.comp instead of the
// ABuffer), camera, view and the ring's address mask (world & mask).
layout(set = 0, binding = 10, std430) readonly buffer FrameDepth {
    int world_depth;
    int camera_y;
    int target_width;
    int target_height;
    int light_mode;
    int light_camera_x;
    int light_camera_y;
    int light_view_x;
    int light_view_y;
    int light_view_width;
    int light_view_height;
    int light_mask_x;
    int light_mask_y;
    int frame_reserved0;
    int frame_reserved1;
    int frame_reserved2;
};
layout(push_constant, std430) uniform Resolve {
    int camera_x;
    int camera_y;
    int clip_x;
    int clip_y;
    int clip_width;
    int clip_height;
    int page_x;
    int page_y;
    int columns;
    int rows;
    int target_width;
    int target_height;
} view;
layout(location = 0) out vec4 color;

// The ABuffer value at a target pixel: the frame's ABuffer, or in light mode
// the world light ring (the clear value outside the view).
uint light_at(ivec2 p, int target_width_now) {
    if (light_mode == 0) return light_intensities[p.y * target_width_now + p.x];
    if (p.x < light_view_x || p.y < light_view_y || p.x >= light_view_x + light_view_width ||
        p.y >= light_view_y + light_view_height)
        return 127u;
    return texelFetch(light_ring, (p + ivec2(light_camera_x, light_camera_y)) & ivec2(light_mask_x, light_mask_y), 0).r;
}
#endif

const int PageSize = 256;
int record;
int word(int slot) { return record_words[record * 12 + slot]; }
// Record fields every lighting step uses, read once per pixel: metadata (word
// 7), the light sources (word 10) and the first word of the record's cell lights.
uint record_metadata;
uint record_light;
int cell_base;

uint rgba8_to_rgb565(uint value) { return ((value & 0xF8u) << 8) | ((value >> 5) & 0x7E0u) | ((value >> 19) & 0x1Fu); }
vec3 rgb565_color(uint color565) {
    uvec3 channel = uvec3(color565 >> 11, (color565 >> 5) & 0x3Fu, color565 & 0x1Fu);
    uvec3 maximum = uvec3(31u, 63u, 31u);
    return vec3((channel * 255u + maximum / 2u) / maximum) / 255.0;
}

const int PALETTE_CELL_TINT = 1;
const int PALETTE_SPRITE_TINT = 2;
const int PALETTE_CONVERTER = 3;
int cell_light(int slot) { return cell_light_words[cell_base + slot]; }
int intensity_source() { return int((record_light >> 20) & 7u); }
int palette_source() { return int((record_light >> 24) & 3u); }

int palette_intensity() {
    int source = intensity_source();
    if (source == 0) return int(uint(word(6)) >> 24);
    int packed = cell_light(0);  // Normal | Terrain << 16
    int intensity = source == 1   ? packed >> 16
                    : source == 2 ? int(uint(packed) >> 16)
                    : source == 3 ? (packed << 16) >> 16
                    : source == 4 ? packed & 0xFFFF
                                  : 1000;
    return min(254, 261 * max(intensity, 0) / 2048);
}
int shade_count() {
    int source = palette_source();
    if (source == PALETTE_CELL_TINT) return cell_light(1) & 0xFFFF;
    if (source == PALETTE_SPRITE_TINT) return (cell_light(1) >> 16) & 0xFFFF;
    return int(record_metadata & 0xFFu) + 1;
}
// The level factors keep the integer division: forming them with one reciprocal
// per pixel measured about 3% slower on the resolve pass, where every tinted
// background pixel pays for it.
uint tinted_color565(uint entry, int level, int shades, int factors) {
    int last = shades - 1;
    uvec3 scaled;
    for (int c = 0; c < 3; ++c) {
        int factor = cell_light(factors + c);
        int level_factor = last != 0 ? (2 * factor * level) / last : factor;
        uint product = ((entry >> uint(8 * c)) & 0xFFu) * uint(level_factor);
        scaled[c] = min(product >> 16, 255u);
    }
    return ((scaled.r & 248u) << 8) | ((scaled.g & 252u) << 3) | ((scaled.b & 248u) >> 3);
}
uint palette_color565(uint palette_index, ivec2 position) {
    bool lit = (record_metadata & 0x100u) != 0u;
    int shades = shade_count();
    uint shade = 0u;
    if (lit) {
#ifdef RING_TILE
        // The tile pass is only used while no pixel takes its light from the first visible row (0x200), which
        // follows the camera; position is a world position and the light is addressed by it.
        uint light = texelFetch(light_ring, position & ivec2(tile.light_mask_x, tile.light_mask_y), 0).r;
#else
        int row = position.y;
        if ((record_metadata & 0x200u) != 0u) {
            int first_visible = max(word(4) - view.camera_y, view.clip_y);
            row = (first_visible % view.target_height + view.target_height) % view.target_height;
        }
        uint light = light_at(ivec2(position.x, row), view.target_width);
#endif
        uint maximum_shade = uint(shades - 1);
        shade = min(maximum_shade, maximum_shade * uint(palette_intensity()) * min(light, 255u) / 32258u);
    }
    int source = palette_source();
    int palette_base = word(1);
    if (source == PALETTE_CELL_TINT || source == PALETTE_SPRITE_TINT) {
        if (!lit) return rgba8_to_rgb565(resource_word(palette_base + int(palette_index)));
        if (palette_index == 0u) return 0u;
        return tinted_color565(resource_word(palette_base + 256 + int(palette_index)), int(shade), shades,
                               source == PALETTE_CELL_TINT ? 2 : 5);
    }
    int base = source == PALETTE_CONVERTER ? converter_offsets[palette_base] : palette_base;
    if (lit) palette_index += 256u + shade * 256u;
    return rgba8_to_rgb565(resource_word(base + int(palette_index)));
}

void main() {
#ifdef RING_TILE
    ivec2 local = ivec2(gl_FragCoord.xy) - ivec2(tile.ring_x, tile.ring_y);
    ivec2 position = ivec2(tile.world_x, tile.world_y) + local;
    ivec3 at = ivec3(local, tile.layer);
    uint material = texelFetch(materials, at, 0).r;
    if (material == 0u) {
        packed_out = uvec4(0x80000000u, 0u, 0u, 0u);
        return;
    }
    uint depth = uint(texelFetch(depths, at, 0).r * 16777216.0);
#else
    ivec2 position = ivec2(gl_FragCoord.xy);
    ivec2 world = position + ivec2(view.camera_x, view.camera_y);
    ivec2 page = world >> 8;  // floor(world / PageSize)
    ivec2 cell = page - ivec2(view.page_x, view.page_y);
    if (cell.x < 0 || cell.y < 0 || cell.x >= view.columns || cell.y >= view.rows) discard;
    int layer = page_layers[cell.y * view.columns + cell.x];
    if (layer < 0) discard;
    ivec3 at = ivec3(world & (PageSize - 1), layer);
    uint material = texelFetch(materials, at, 0).r;
    if (material == 0u) discard;
    gl_FragDepth = texelFetch(depths, at, 0).r;
#endif
    record = int(material >> 8);
    record_metadata = uint(word(7));
    record_light = uint(word(10));
    cell_base = int(record_light & 0x3FFFFu) * 8;
    uint color565 = palette_color565(material & 0xFFu, position) | (record_metadata >> 16);
    vec3 rgb = rgb565_color(color565);
    int count = int(texelFetch(shadows, at, 0).r * 255.0 + 0.5);
    for (int i = 0; i < count; ++i)
        rgb = floor(rgb * 127.5 + 0.5) / 255.0;
#ifdef RING_TILE
    uvec3 channel = uvec3(rgb * 255.0 + 0.5);
    packed_out = uvec4(depth, channel.r | (channel.g << 8) | (channel.b << 16), 0u, 0u);
#else
    color = vec4(rgb, 1.0);
#endif
}
