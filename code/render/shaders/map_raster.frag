#version 450

// Ports map_draw.comp draw_pixel() for kinds 0..3 with row origin 0 and no
// plain warp; the host routes every other packet to the compute path.
// Fixed-function state does the rest:
// - Depth: D32F is the only scene Z. Production uses world Z / 0x1000000;
//   legacy fixtures keep Z / 0x10000. Compute paths sample it read-only.
//   Original SHP hides equal depth (LESS); other resources accept it (LEQUAL).
// - Color: RGBA8 holding expanded RGB565. Opaque writes are exact. A shadow
//   outputs zero with destination factor 0.5, a plain multiply. Translucency
//   outputs the original masked source term and adds the destination weighed
//   by the blend constant. Readers truncate the 8-bit result to RGB565: within
//   one step of the original for shadows and 25%/50% transparency, two steps
//   for 75% transparency, whose 3 * (d >> 2) depends on the destination's low
//   bits (README.md, Raster path).
layout(set = 0, binding = 1, std430) readonly buffer Arena {
    uint arena_words[];
};
layout(set = 0, binding = 2, std430) readonly buffer Lighting {
    uint light_intensities[];
};
// Frame: world Z flag, camera y and target size; then the light mode (1:
// fragments read the light ring of map_light_page.comp instead of the
// ABuffer), camera, view and the ring's address mask (world & mask).
layout(set = 0, binding = 4, std430) readonly buffer FrameDepth {
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
layout(set = 0, binding = 7) uniform usampler2D light_ring;
layout(location = 0) flat in ivec4 v_source;
layout(location = 1) flat in ivec4 v_palette;
layout(location = 2) flat in ivec4 v_depth;
layout(location = 3) flat in ivec4 v_extra;
layout(location = 4) flat in ivec4 v_tint;
layout(location = 5) flat in ivec4 v_plane;
layout(location = 0) out vec4 color;

#define SOURCE_BASE v_source.x
#define SOURCE_WIDTH v_source.y
#define DESTINATION v_source.zw
#define PALETTE_BASE v_palette.x
#define SHADES v_palette.y
#define INTENSITY v_palette.z
#define FLAGS v_palette.w
#define PACKED_DRAW_MODE v_depth.x
#define DEPTH_BASE v_depth.y
#define GRADIENT v_depth.z
#define FIRST_VISIBLE_Y v_depth.w
#define AUXILIARY_COLUMN v_extra.x
#define DEPTH_OFFSET v_extra.y
#define LIGHT_ROW v_extra.z
#define SHAPE_TINT_RGB565 uint(v_extra.w)
#define TARGET_WIDTH v_tint.w
#define PLANES ((FLAGS & 256) != 0)
#define PREFIX_BASE v_plane.x
#define AUXILIARY_BASE v_plane.y
#define AUXILIARY_PITCH v_plane.z
#define AUXILIARY_OFFSET v_plane.w
#define DRAW_KIND ((FLAGS >> 4) & 3)
#define PALETTE_SOURCE ((FLAGS >> 6) & 3)

const int DRAW_SHAPE_OR_TILE = 0;
const int DRAW_COPY = 1;
const int DRAW_FILL = 2;
const int DRAW_INDEXED = 3;
const int DEPTH_NONE = 0;
const int ORIGINAL_SHAPE_DEPTH_FLAG = 0x10000;
const int AUXILIARY_DEPTH_FLAG = 0x20000;
const int BLEND_SHADOW = 1;
const int BLEND_TRANSPARENT_25 = 2;
const int BLEND_TRANSPARENT_50 = 3;
const int PALETTE_CELL_TINT = 1;
const int PALETTE_SPRITE_TINT = 2;
const uint SHAPE_TEXEL_OCCUPIED = 0x10000u;
const uint INDEXED_TEXEL_OCCUPIED = 0x1000000u;
const uint WORD_MASK = 0xFFFFu;

// The array is a virtual linear address space of 1024 x 1024 pages.
// Images can cross page boundaries; descriptors retain their exact dimensions.
#ifdef RESIDENT_RESOURCES
layout(set = 0, binding = 9) uniform usampler2DArray resource_pages;
uint resource_word(int index) {
    return texelFetch(resource_pages, ivec3(index & 1023, (index >> 10) & 1023, index >> 20), 0).r;
}
#else
uint resource_word(int index) { return arena_words[index]; }
#endif

// Byte i of the resident address space (shape sequence planes).
uint resource_byte(int byte_index) { return (resource_word(byte_index >> 2) >> uint((byte_index & 3) * 8)) & 0xFFu; }

uint rgba8_to_rgb565(uint value) { return ((value & 0xF8u) << 8) | ((value >> 5) & 0x7E0u) | ((value >> 19) & 0x1Fu); }

// The RGBA8 attachment value of an RGB565 word, as map_draw.comp
// rgb565_to_rgba8() expands it.
vec4 rgb565_color(uint color565) {
    uvec3 channel = uvec3(color565 >> 11, (color565 >> 5) & 0x3Fu, color565 & 0x1Fu);
    uvec3 maximum = uvec3(31u, 63u, 31u);
    return vec4(vec3((channel * 255u + maximum / 2u) / maximum), 255.0) / 255.0;
}

// The ABuffer value at a target pixel: the frame's ABuffer, or in light mode
// the world light ring (the clear value outside the view).
uint light_at(ivec2 p, int target_width_now) {
    if (light_mode == 0) return light_intensities[p.y * target_width_now + p.x];
    if (p.x < light_view_x || p.y < light_view_y || p.x >= light_view_x + light_view_width ||
        p.y >= light_view_y + light_view_height)
        return 127u;
    return texelFetch(light_ring, (p + ivec2(light_camera_x, light_camera_y)) & ivec2(light_mask_x, light_mask_y), 0).r;
}

// Row limits are 1 or 3 and fragments lie at or below the first visible row,
// so the numerator is non-negative and the divisor a constant in practice. Any
// other input takes the general signed division, as before.
int gradient_steps(int numerator, int limit) {
    if (numerator >= 0) {
        if (limit == 3) return int(uint(numerator) / 3u);
        if (limit == 1) return numerator;
    }
    return numerator / limit;
}

// map_draw.comp palette_pixel_depth() for kinds 0 and 3; the vertex stage
// resolved the base and gradient.
int palette_pixel_depth(ivec2 source_position, ivec2 destination_position, uint source_sample) {
    if ((PACKED_DRAW_MODE & ORIGINAL_SHAPE_DEPTH_FLAG) != 0) {
        if ((PACKED_DRAW_MODE & AUXILIARY_DEPTH_FLAG) != 0) {
            if (PLANES) {
                // The prefix plane holds the transparent run length left at the first
                // visible column; the SHA pointer lags that far behind. Deltas are
                // signed bytes of the SHA frame at the offset the host resolved.
                int row = source_position.y * SOURCE_WIDTH;
                int prefix = PREFIX_BASE < 0 ? 0 : int(resource_byte(PREFIX_BASE + row + AUXILIARY_COLUMN));
                int ax = source_position.x - prefix + (AUXILIARY_OFFSET & 0xFFFF);
                int ay = source_position.y + (AUXILIARY_OFFSET >> 16);
                uint delta = resource_byte(AUXILIARY_BASE + ay * AUXILIARY_PITCH + ax);
                return DEPTH_BASE - (int(delta << 24) >> 24);
            }
            int row = SOURCE_BASE + source_position.y * SOURCE_WIDTH;
            int prefix = int((resource_word(row + AUXILIARY_COLUMN) >> 17) & 0xFFu);
            uint auxiliary = resource_word(row + source_position.x - prefix);
            return DEPTH_BASE - (int(auxiliary << 16) >> 24);
        }
        int phase = GRADIENT & 0xFF, step = (GRADIENT >> 8) & 0xFF, limit = (GRADIENT >> 16) & 0xFF;
        return DEPTH_BASE +
               gradient_steps(phase + (destination_position.y - FIRST_VISIBLE_Y) * step, limit) * (GRADIENT >> 24);
    }
    // Indexed samples carry signed 16-bit Z; SHP/TMP unsigned 8-bit Z.
    int pixel_depth =
        DEPTH_BASE + (DRAW_KIND == DRAW_INDEXED ? (int(source_sample << 8) >> 16) : int((source_sample >> 8) & 0xFFu));
    if ((FLAGS & 2) != 0) pixel_depth = int(uint(pixel_depth) & WORD_MASK);
    return pixel_depth;
}

// LightConvertClass::BuildColorTable(palette, ..., shades, tint, no ignored
// indexes, mode 2, no MMX) at one index and level, from the PAL entry (R | G << 8
// | B << 16) and the cell's tint factors: the same 32-bit wrapping products,
// truncating divisions and RGB565 packing as LightConvertClassPalette.cpp.
//
// The quotient is formed with one reciprocal per pixel instead of three integer
// divisions. The numerator 2 * factor * level stays below 2^23: the tint
// channels are clamped to 0..1000 and the light factor is at most 65536
// (PrepareCellTint), the level is at most last, and last is 26 or 52, so the
// numerator is at most 2 * 65536 * 52 = 6,815,744, float(n) is exact and
// n * (1 / last) is within one of the integer quotient; two corrections settle
// it. Anything outside that range (or last < 3) keeps the integer division.
uint tinted_color565(uint entry, int level, int shades) {
    int last = shades - 1;
    float reciprocal = last >= 3 ? 1.0 / float(last) : 0.0;
    uvec3 scaled;
    for (int c = 0; c < 3; ++c) {
        int factor = v_tint[c];
        int level_factor = factor;
        if (last != 0) {
            int numerator = 2 * factor * level;
            if (last >= 3 && uint(numerator) < 0x800000u) {
                uint divisor = uint(last);
                uint quotient = uint(float(numerator) * reciprocal);
                if ((quotient + 1u) * divisor <= uint(numerator)) ++quotient;
                if (quotient * divisor > uint(numerator)) --quotient;
                level_factor = int(quotient);
            } else {
                level_factor = numerator / last;
            }
        }
        uint product = ((entry >> uint(8 * c)) & 0xFFu) * uint(level_factor);
        scaled[c] = min(product >> 16, 255u);
    }
    return ((scaled.r & 248u) << 8) | ((scaled.g & 252u) << 3) | ((scaled.b & 248u) >> 3);
}

// map_draw.comp shaded_palette_index() with row origin 0, then the palette
// word. Cell-tinted palettes hold the unlit row and then the PAL entries.
uint palette_color565(uint source_sample, ivec2 destination_position) {
    uint palette_index = source_sample & 0xFFu;
    bool lit = (FLAGS & 1) != 0;
    uint shade = 0u;
    if (lit) {
        int row = LIGHT_ROW >= 0 ? LIGHT_ROW : destination_position.y;
        uint maximum_shade = uint(SHADES - 1);
        shade = min(maximum_shade, maximum_shade * uint(INTENSITY) *
                                       min(light_at(ivec2(destination_position.x, row), TARGET_WIDTH), 255u) / 32258u);
    }
    int source = PALETTE_SOURCE;
    if (source == PALETTE_CELL_TINT || source == PALETTE_SPRITE_TINT) {
        if (!lit) return rgba8_to_rgb565(resource_word(PALETTE_BASE + int(palette_index)));
        if (palette_index == 0u) return 0u;
        return tinted_color565(resource_word(PALETTE_BASE + 256 + int(palette_index)), int(shade), SHADES);
    }
    if (lit) palette_index += 256u + shade * 256u;
    return rgba8_to_rgb565(resource_word(PALETTE_BASE + int(palette_index)));
}

void main() {
    ivec2 destination_position = ivec2(gl_FragCoord.xy);
    ivec2 source_position = destination_position - DESTINATION;
    uint source_sample;
    if (PLANES) {
        // 8-bit colour plane: the palette index; zero is transparent.
        uint index = resource_byte(SOURCE_BASE + source_position.y * SOURCE_WIDTH + source_position.x);
        source_sample = index != 0u ? (index | SHAPE_TEXEL_OCCUPIED) : 0u;
    } else {
        source_sample = resource_word(
            SOURCE_BASE + (DRAW_KIND == DRAW_FILL ? 0 : source_position.y * SOURCE_WIDTH + source_position.x));
    }
    if (DRAW_KIND == DRAW_COPY || DRAW_KIND == DRAW_FILL) {
        color = rgb565_color(rgba8_to_rgb565(source_sample));
        return;
    }
    if (DRAW_KIND == DRAW_SHAPE_OR_TILE && (source_sample & SHAPE_TEXEL_OCCUPIED) == 0u) {
        discard;
    }
    if (DRAW_KIND == DRAW_INDEXED &&
        ((source_sample & INDEXED_TEXEL_OCCUPIED) == 0u || (source_sample & 0xFFu) == 0u)) {
        discard;
    }
    int pixel_depth = int(WORD_MASK);
    if ((PACKED_DRAW_MODE & 0xFF) != DEPTH_NONE) {
        pixel_depth = palette_pixel_depth(source_position, destination_position, source_sample);
    }
    // World Z uses an exact integer range in D32F; all paths share this
    // normalization. Legacy test packets keep the original 16-bit convention.
    gl_FragDepth = (FLAGS & 4) != 0 ? float(clamp(pixel_depth + DEPTH_OFFSET, 0, 0xFFFFFF)) / 16777216.0
                                    : float(clamp(pixel_depth, 0, int(WORD_MASK))) / 65536.0;
    if (((PACKED_DRAW_MODE >> 8) & 0xFF) == BLEND_SHADOW) {
        color = vec4(0.0);
        return;
    }
    uint color565 = palette_color565(source_sample, destination_position) | SHAPE_TINT_RGB565;
    int blend_mode = (PACKED_DRAW_MODE >> 8) & 0xFF;
    if (blend_mode >= BLEND_TRANSPARENT_25) {
        // The original source term, exactly as map_draw.comp masks it
        // (3 * (p >> 2), p >> 1 or p >> 2 per channel); the pipeline adds the
        // destination weighed by the blend constant.
        uvec3 channel = uvec3(color565 >> 11, (color565 >> 5) & 0x3Fu, color565 & 0x1Fu);
        uvec3 term = blend_mode == BLEND_TRANSPARENT_25   ? 3u * (channel >> 2u)
                     : blend_mode == BLEND_TRANSPARENT_50 ? channel >> 1u
                                                          : channel >> 2u;
        color = vec4(vec3(term) / vec3(31.0, 63.0, 31.0), 0.0);
        return;
    }
    color = rgb565_color(color565);
}
