#[compute]
#version 450

// One packet completes before the next packet starts. Within a packet, each
// invocation owns one destination pixel. Color/Z read-modify-write therefore
// needs neither atomics nor synchronization between workgroups.
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Buffer bindings and the 20-word packet layout are shared with the bridge.
// See README.md for field meanings, source formats and per-kind state access.
layout(set = 0, binding = 0, std430) readonly buffer Parameters { int packet_words[]; };
layout(set = 0, binding = 1, std430) readonly buffer Source { uint source_texels[]; };
layout(set = 0, binding = 2, std430) buffer Output { uint output_colors[]; };
layout(set = 0, binding = 3, std430) buffer Depth { uint depth_values[]; };
// Read-only in this consumer, not constant: map_abuffer.glsl implements the
// original dynamic producers. Native frame scheduling still needs to connect it.
layout(set = 0, binding = 4, std430) readonly buffer Lighting { uint light_intensities[]; };

// 256 unlit RGBA8 colors, followed by shade_count tables of 256 colors.
// The host prepares these tables using the original palette conversion.
layout(set = 0, binding = 5, std430) readonly buffer Palette { uint palette_colors[]; };
layout(push_constant, std430) uniform Batch { int packet_index; } batch;

// These aliases name the wire layout without eagerly loading a whole packet.
// Words 10/11/12/15 have different meanings for different drawing paths.
const int PACKET_WORD_COUNT = 20;
#define PACKET_WORD(slot) packet_words[batch.packet_index * PACKET_WORD_COUNT + (slot)]
#define TARGET_WIDTH                 PACKET_WORD(0)
#define TARGET_HEIGHT                PACKET_WORD(1)
#define SOURCE_WIDTH                 PACKET_WORD(2)
#define SOURCE_HEIGHT                PACKET_WORD(3)
#define DESTINATION_X                PACKET_WORD(4)
#define DESTINATION_Y                PACKET_WORD(5)
#define CLIP_X                       PACKET_WORD(6)
#define CLIP_Y                       PACKET_WORD(7)
#define CLIP_WIDTH                   PACKET_WORD(8)
#define CLIP_HEIGHT                  PACKET_WORD(9)
#define PALETTE_LIGHTING_ENABLED     (PACKET_WORD(10) & 1)
#define SHAPE_TINT_RGB565            (uint(PACKET_WORD(10)) >> 16u)
#define LIGHT_STRENGTH_FROM_SOURCE   PACKET_WORD(10)
#define PALETTE_INTENSITY            PACKET_WORD(11)
#define EFFECT_RGB                   PACKET_WORD(11)
#define LIGHT_FLAGS                  PACKET_WORD(11)
#define SHADE_COUNT                  PACKET_WORD(12)
#define EFFECT_OPACITY               PACKET_WORD(12)
#define LIGHT_STRENGTH               PACKET_WORD(12)
#define PACKED_DRAW_MODE             PACKET_WORD(13)
#define BASE_DEPTH                   PACKET_WORD(14)
#define PACKED_DEPTH_GRADIENT        PACKET_WORD(15)
#define WRAP_RESOURCE_DEPTH          PACKET_WORD(15)
#define STATE_ORIGIN_Y               PACKET_WORD(16)
#define REPEAT_FIRST_LIGHT_ROW       PACKET_WORD(17)
#define FIRST_VISIBLE_Y              PACKET_WORD(18)
#define DRAW_KIND                    PACKET_WORD(19)

const int DRAW_SHAPE_OR_TILE = 0;
const int DRAW_COPY = 1;
const int DRAW_FILL = 2;
const int DRAW_INDEXED = 3;
const int DRAW_SELECTION_LINE = 4;
const int DRAW_SPOTLIGHT = 5;
const int DRAW_DEPTH_GLOW = 6;
const int DRAW_DEPTH_ALPHA = 7;
const int DRAW_PARTICLE = 8;

// Packet depth modes differ from the public ShapeDepthMode enum values.
const int DEPTH_NONE = 0;
const int DEPTH_READ_WRITE = 1;
const int ORIGINAL_SHAPE_DEPTH_FLAG = 0x10000;
const int AUXILIARY_DEPTH_FLAG = 0x20000;

// The percentage describes transparency, not source opacity.
const int BLEND_SHADOW = 1;
const int BLEND_TRANSPARENT_25 = 2;
const int BLEND_TRANSPARENT_50 = 3;
const int BLEND_TRANSPARENT_75 = 4;

const uint SHAPE_TEXEL_OCCUPIED = 0x10000u;
const uint INDEXED_TEXEL_OCCUPIED = 0x1000000u;
const uint WORD_MASK = 0xFFFFu;
const uint NEUTRAL_LIGHT = 127u;
const uint OPAQUE_ALPHA = 0xFF000000u;

// After a packed right shift, remove bits that crossed RGB565 channel borders.
const uint RGB565_HALF_MASK = 0x7BEFu;
const uint RGB565_QUARTER_MASK = 0x39E7u;
const int LIGHT_DARKEN_FLAG = 0x1;
const int LIGHT_SKIP_RED_FLAG = 0x2; // Green/blue are this bit shifted by 1/2.

// Color helpers: RGBA8 words store red in the low byte; RGB565 stores red in
// bits 11..15. All blending retains the calibrated integer truncation order.
uint rgba8_to_rgb565(uint color) {
    return ((color & 0xF8u) << 8)
        | ((color >> 5) & 0x7E0u)
        | ((color >> 19) & 0x1Fu);
}

uint rgb565_to_rgba8(uint color) {
    uint red = (color >> 11) & 0x1Fu;
    uint green = (color >> 5) & 0x3Fu;
    uint blue = color & 0x1Fu;

    // Output expansion rounds to nearest, matching the CPU and reference PNGs.
    // Effect arithmetic below instead scales channels by 8/4/8 before mixing.
    return ((red * 255u + 15u) / 31u)
        | (((green * 255u + 31u) / 63u) << 8)
        | (((blue * 255u + 15u) / 31u) << 16)
        | OPAQUE_ALPHA;
}

uint blend_rgba8_as_rgb565(uint foreground, uint background, int blend_mode) {
    uint foreground565 = rgba8_to_rgb565(foreground);
    uint background565 = rgba8_to_rgb565(background);
    uint blended565;

    if (blend_mode == BLEND_SHADOW) {
        blended565 = (background565 >> 1) & RGB565_HALF_MASK;
    } else if (blend_mode == BLEND_TRANSPARENT_50) {
        blended565 = ((foreground565 >> 1) & RGB565_HALF_MASK)
            + ((background565 >> 1) & RGB565_HALF_MASK);
    } else if (blend_mode == BLEND_TRANSPARENT_25) {
        blended565 = 3u * ((foreground565 >> 2) & RGB565_QUARTER_MASK)
            + ((background565 >> 2) & RGB565_QUARTER_MASK);
    } else {
        blended565 = ((foreground565 >> 2) & RGB565_QUARTER_MASK)
            + 3u * ((background565 >> 2) & RGB565_QUARTER_MASK);
    }
    return rgb565_to_rgba8(blended565);
}

uint depth_alpha_color(int output_index, int state_index) {
    uint background565 = rgba8_to_rgb565(output_colors[output_index]);
    uint light_intensity = light_intensities[state_index] & WORD_MASK;
    uint opacity = uint(EFFECT_OPACITY);
    uint packed_rgb = uint(EFFECT_RGB);
    uvec3 source_rgb = uvec3(
        packed_rgb & 0xFFu, (packed_rgb >> 8) & 0xFFu, (packed_rgb >> 16) & 0xFFu);
    uvec3 destination_rgb = uvec3(
        ((background565 >> 11) & 0x1Fu) * 8u,
        ((background565 >> 5) & 0x3Fu) * 4u,
        (background565 & 0x1Fu) * 8u);

    // Floor the weighted terms separately, then apply ABuffer, including 127.
    uvec3 source_part = (opacity * source_rgb) >> 8u;
    uvec3 destination_part = ((256u - opacity) * destination_rgb) >> 8u;
    uvec3 lit_rgb = (light_intensity * (source_part + destination_part)) >> 7u;
    return rgb565_to_rgba8(
        (((lit_rgb.r >> 3u) << 11u) | ((lit_rgb.g >> 2u) << 5u) | (lit_rgb.b >> 3u))
        & WORD_MASK);
}

uint light_effect_color(int strength, int output_index) {
    uint background565 = rgba8_to_rgb565(output_colors[output_index]);
    ivec3 rgb = ivec3(
        int((background565 >> 11) & 0x1Fu) * 8,
        int((background565 >> 5) & 0x3Fu) * 4,
        int(background565 & 0x1Fu) * 8);
    for (int channel = 0; channel < 3; ++channel) {
        if ((LIGHT_FLAGS & LIGHT_DARKEN_FLAG) != 0) {
            rgb[channel] = ((256 - strength) * rgb[channel]) >> 8;
        } else if ((LIGHT_FLAGS & (LIGHT_SKIP_RED_FLAG << channel)) == 0) {
            // Preserve unsigned multiplication followed by signed shift.
            rgb[channel] = min(255,
                rgb[channel] + (int(uint(strength) * uint(rgb[channel])) >> 8));
        }
    }
    return rgb565_to_rgba8(
        uint(((rgb.r >> 3) << 11) | ((rgb.g >> 2) << 5) | (rgb.b >> 3)));
}

uint particle_color(uint light_intensity) {
    // ParticleClass 0x0062CEC0: the core supplies interpolated RGB. Apply
    // ABuffer only below 127, then quantize without changing scene Z.
    uint packed_rgb = uint(EFFECT_RGB);
    uvec3 rgb = uvec3(
        packed_rgb & 0xFFu, (packed_rgb >> 8) & 0xFFu, (packed_rgb >> 16) & 0xFFu);
    if (light_intensity < NEUTRAL_LIGHT) {
        rgb = (rgb * light_intensity) >> 7;
    }
    return rgb565_to_rgba8(
        ((rgb.r >> 3) << 11) | ((rgb.g >> 2) << 5) | (rgb.b >> 3));
}

uint selection_line_color(uint source_sample, int state_index) {
    uint color565 = rgba8_to_rgb565(source_sample);
    uint light_intensity = light_intensities[state_index] & WORD_MASK;
    if (light_intensity != NEUTRAL_LIGHT) {
        // Expand, light and truncate each channel before packing it again.
        // Retain the final WORD mask: this path does not clamp the channels.
        uint red = (((((color565 >> 11) & 0x1Fu) * 8u * light_intensity) >> 7) >> 3) << 11;
        uint green = (((((color565 >> 5) & 0x3Fu) * 4u * light_intensity) >> 7) >> 2) << 5;
        uint red_green = red | green;
        uint blue = (((color565 & 0x1Fu) * 8u * light_intensity) >> 7) >> 3;
        color565 = red_green | blue;
    }
    return rgb565_to_rgba8(color565 & WORD_MASK);
}

int palette_pixel_depth(ivec2 source_position, ivec2 destination_position,
    uint source_sample, bool original_shape_depth) {
    int pixel_depth;
    if (original_shape_depth) {
        int phase = PACKED_DEPTH_GRADIENT & 0xFF;
        int step = (PACKED_DEPTH_GRADIENT >> 8) & 0xFF;
        int limit = (PACKED_DEPTH_GRADIENT >> 16) & 0xFF;
        if ((PACKED_DRAW_MODE & AUXILIARY_DEPTH_FLAG) != 0) {
            // RLE prefix overhang is stored at the first visible source
            // column. It relocates the auxiliary signed 8-bit Z sample.
            int left = max(CLIP_X - DESTINATION_X, 0);
            int prefix = int((source_texels[source_position.y * SOURCE_WIDTH + left] >> 17) & 0xFFu);
            uint auxiliary = source_texels[source_position.y * SOURCE_WIDTH + source_position.x - prefix];
            pixel_depth = BASE_DEPTH - (int(auxiliary << 16) >> 24);
        } else {
            // The top byte is a signed delta; division must precede it.
            pixel_depth = BASE_DEPTH
                + ((phase + (destination_position.y - FIRST_VISIBLE_Y) * step) / limit)
                * (PACKED_DEPTH_GRADIENT >> 24);
        }
    } else {
        // Indexed samples carry signed 16-bit Z; SHP/TMP carry unsigned
        // 8-bit resource Z. Keep the casts before arithmetic right shifts.
        pixel_depth = BASE_DEPTH + (DRAW_KIND == DRAW_INDEXED
            ? (int(source_sample << 8) >> 16) : int((source_sample >> 8) & 0xFFu));
        if (WRAP_RESOURCE_DEPTH != 0) {
            pixel_depth = int(uint(pixel_depth) & WORD_MASK);
        }
    }
    return pixel_depth;
}

uint shaded_palette_index(uint source_sample, ivec2 destination_position, int state_index) {
    uint palette_index = source_sample & 0xFFu;
    if (PALETTE_LIGHTING_ENABLED != 0) {
        if (REPEAT_FIRST_LIGHT_ROW != 0) {
            int first_row = ((FIRST_VISIBLE_Y - STATE_ORIGIN_Y) % TARGET_HEIGHT + TARGET_HEIGHT) % TARGET_HEIGHT;
            state_index = first_row * TARGET_WIDTH + destination_position.x;
        }
        // AlphaLightingRemapClass 0x00420140: WORD lighting is a byte-range
        // intensity, not opacity. The divisor is the original 254 * 127.
        // Host bounds (1..256 shades, 0..254 intensity) prevent overflow.
        uint maximum_shade = uint(SHADE_COUNT - 1);
        uint shade = min(maximum_shade,
            maximum_shade * uint(PALETTE_INTENSITY) * min(light_intensities[state_index], 255u) / 32258u);
        palette_index += 256u + shade * 256u;
    }
    return palette_index;
}

// Keep rejection and buffer writes in the entry point. Calculation helpers have
// a single return, preserving the original control flow when they are inlined.
void main() {
    ivec2 source_position = ivec2(gl_GlobalInvocationID.xy);
    if (source_position.x >= SOURCE_WIDTH || source_position.y >= SOURCE_HEIGHT) {
        return;
    }

    uint source_sample = source_texels[DRAW_KIND == DRAW_FILL
        ? 0 : source_position.y * SOURCE_WIDTH + source_position.x];
    if (DRAW_KIND == DRAW_SHAPE_OR_TILE && (source_sample & SHAPE_TEXEL_OCCUPIED) == 0u) {
        return;
    }
    if (DRAW_KIND == DRAW_INDEXED
        && ((source_sample & INDEXED_TEXEL_OCCUPIED) == 0u || (source_sample & 0xFFu) == 0u)) {
        return;
    }

    ivec2 destination_position = source_position + ivec2(DESTINATION_X, DESTINATION_Y);
    if (destination_position.x < CLIP_X || destination_position.y < CLIP_Y
        || destination_position.x >= CLIP_X + CLIP_WIDTH || destination_position.y >= CLIP_Y + CLIP_HEIGHT
        || destination_position.x < 0 || destination_position.y < 0
        || destination_position.x >= TARGET_WIDTH || destination_position.y >= TARGET_HEIGHT) {
        return;
    }
    int output_index = destination_position.y * TARGET_WIDTH + destination_position.x;
    if (DRAW_KIND == DRAW_COPY || DRAW_KIND == DRAW_FILL) {
        output_colors[output_index] = source_sample;
        return;
    }

    // Color uses screen coordinates. Z/ABuffer instead use a circular row
    // origin; the second modulo also wraps negative row offsets correctly.
    int state_row = ((destination_position.y - STATE_ORIGIN_Y) % TARGET_HEIGHT + TARGET_HEIGHT) % TARGET_HEIGHT;
    int state_index = state_row * TARGET_WIDTH + destination_position.x;
    if (DRAW_KIND == DRAW_DEPTH_ALPHA) {
        // DSurface 0x004BEAC0: equal depth is hidden; scene Z is read-only.
        if (EFFECT_OPACITY < 8 || BASE_DEPTH >= int(depth_values[state_index])
            || light_intensities[state_index] == 0u) {
            return;
        }
        output_colors[output_index] = depth_alpha_color(output_index, state_index);
        return;
    }
    if (DRAW_KIND == DRAW_SPOTLIGHT || DRAW_KIND == DRAW_DEPTH_GLOW) {
        // Spotlight ignores Z; depth glow rejects equal or farther depth.
        // Neither path reads ABuffer or writes scene Z.
        if (DRAW_KIND == DRAW_DEPTH_GLOW && BASE_DEPTH >= int(depth_values[state_index])) {
            return;
        }
        int strength = LIGHT_STRENGTH_FROM_SOURCE != 0
            ? int(source_sample & 0xFFu) : LIGHT_STRENGTH;
        if (strength == 0) {
            return;
        }
        output_colors[output_index] = light_effect_color(strength, output_index);
        return;
    }

    int depth_mode = PACKED_DRAW_MODE & 0xFF;
    int blend_mode = (PACKED_DRAW_MODE >> 8) & 0xFF;
    bool original_shape_depth = (PACKED_DRAW_MODE & ORIGINAL_SHAPE_DEPTH_FLAG) != 0;
    if (DRAW_KIND == DRAW_PARTICLE) {
        uint light_intensity = light_intensities[state_index] & WORD_MASK;
        if (BASE_DEPTH >= int(depth_values[state_index]) || light_intensity == 0u) {
            return;
        }
        output_colors[output_index] = particle_color(light_intensity);
        return;
    }
    if (DRAW_KIND == DRAW_SELECTION_LINE) {
        // DSurface 0x004BFD30: equal depth is hidden; scene Z is read-only.
        if (BASE_DEPTH >= int(depth_values[state_index]) || light_intensities[state_index] == 0u) {
            return;
        }
        output_colors[output_index] = selection_line_color(source_sample, state_index);
        return;
    }

    // SHP/TMP and indexed images share depth testing and palette lookup.
    if (depth_mode != DEPTH_NONE) {
        int pixel_depth = palette_pixel_depth(
            source_position, destination_position, source_sample, original_shape_depth);
        // Original SHP hides equal depth; the other resource path accepts it.
        if (original_shape_depth
            ? pixel_depth >= int(depth_values[state_index])
            : pixel_depth > int(depth_values[state_index])) {
            return;
        }
        if (depth_mode == DEPTH_READ_WRITE) {
            depth_values[state_index] = uint(pixel_depth) & WORD_MASK;
        }
    }
    // Shadows darken the destination and never look up a palette color.
    if (blend_mode == BLEND_SHADOW) {
        output_colors[output_index] = blend_rgba8_as_rgb565(0u, output_colors[output_index], blend_mode);
        return;
    }
    uint palette_index = shaded_palette_index(source_sample, destination_position, state_index);
    uint color = palette_colors[palette_index];
    // Include DRAW_INDEXED: original uncached VXL bitmaps reach PlainBlit via
    // 0x00706ED0 -> 0x004AF2A0 -> 0x004373B0. BlitTransXlatAlphaZRead<WORD>::
    // Blit_Copy_Tinted (0x00494C40) ORs the palette color with tint at 0x00494CD9.
    // type_drawing_packets.cpp::original_shape_mode packs tint only for the
    // original tint-capable blitter modes; shadows and translucency omit it.
    if (SHAPE_TINT_RGB565 != 0u) {
        color = rgb565_to_rgba8(rgba8_to_rgb565(color) | SHAPE_TINT_RGB565);
    }
    if (blend_mode >= BLEND_TRANSPARENT_25 && blend_mode <= BLEND_TRANSPARENT_75) {
        color = blend_rgba8_as_rgb565(color, output_colors[output_index], blend_mode);
    }
    output_colors[output_index] = color;
}
