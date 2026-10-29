#version 450

// Hardware rasterization of the regular image packets: kind 0 SHP/TMP and
// kind 3 indexed (VXL bitmaps) with any blend, kind 1 copies and kind 2 fills.
// Each instance is one packet, drawn as the quad of its destination rectangle
// clipped to its clip rectangle and the target. Instances of one draw are
// consecutive packets with one pipeline state; the draw order is packet order,
// so blending and depth testing see every earlier write as map_draw.comp's
// ordered dispatches do.
//
// Everything constant over an instance is resolved here, once per vertex,
// and handed to the fragment stage as flat varyings: packet fields, the
// resident light sources (cell intensity, tint, converter slot), the depth
// gradient and the light row. Fragments read only their source sample, the
// ABuffer and the palette.
layout(set = 0, binding = 0, std430) readonly buffer Parameters {
    int packet_words[];
};
// Immediate packets: words 2 * packet and 2 * packet + 1 hold the source and
// palette word offsets. Resident draws: the schedule is the draw order, and
// instance i draws the record whose id is word first + i (an occupied slot of
// the record table bound as Parameters).
layout(set = 0, binding = 3, std430) readonly buffer Schedule {
    int schedule_words[];
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
// Resident scene lighting: 8 words per cell index (MapSceneLightChunk) and the
// arena word offset of each converter slot. Immediate packets do not read them.
layout(set = 0, binding = 5, std430) readonly buffer CellLights {
    int cell_light_words[];
};
layout(set = 0, binding = 6, std430) readonly buffer ConverterSlots {
    int converter_offsets[];
};
layout(push_constant, std430) uniform Run {
    int first_packet;
    int camera_x;
    int camera_y;
    int viewport;
    int clip_x;
    int clip_y;
    int clip_width;
    int clip_height;
} run;
#define RESIDENT (run.viewport == 1)
#define DYNAMIC_INSTANCE (run.viewport >= 2)

// Source base, source width, destination x, y (target pixels).
layout(location = 0) flat out ivec4 v_source;
// Palette base (converter slot resolved), shades, intensity, flags.
layout(location = 1) flat out ivec4 v_palette;
// Packed draw mode, depth base, packed gradient, first visible row.
layout(location = 2) flat out ivec4 v_depth;
// Auxiliary depth column, depth offset, light row (-1: the fragment's row),
// shape tint RGB565.
layout(location = 3) flat out ivec4 v_extra;
// Tint factors (red, green, blue) of a tinted palette, target width.
layout(location = 4) flat out ivec4 v_tint;
// Shape sequence planes (SHAPE_PLANES_FLAG): RLE prefix plane byte address (-1: none),
// SHA frame byte address, SHA frame width, SHA frame offset (x | y << 16).
layout(location = 5) flat out ivec4 v_plane;

const int ORIGINAL_SHAPE_DEPTH_FLAG = 0x10000;
const int AUXILIARY_DEPTH_FLAG = 0x20000;
// The kind 0 source is a shape sequence frame: 8-bit planes named by a frame
// table entry (render::ShapePlanesFlag). The entry holds the colour plane byte
// address, width, height and the prefix plane byte address.
const int SHAPE_PLANES_FLAG = 0x80000;
const int PALETTE_CELL_TINT = 1;
const int PALETTE_SPRITE_TINT = 2;
const int PALETTE_CONVERTER = 3;

#ifdef RESIDENT_RESOURCES
layout(set = 0, binding = 10, std430) readonly buffer ResourceMetadata {
    int resource_words[];
};
#endif
int packet;

// Match the renderer's 48-byte resident instance and 64-byte dynamic instance.
// Immediate packets keep their diagnostic 20-word format.
int packet_word(int slot) {
    if (DYNAMIC_INSTANCE) {
        int b = run.viewport == 3 ? packet_words[2 + packet] : packet * 16;
        switch (slot) {
            case 0:
#ifdef RESIDENT_RESOURCES
                return resource_words[packet_words[b] * 4];
#else
                return packet_words[b];
#endif
            case 1:
                return packet_words[b + 1];
            // The drawn size is the instance's own: a sequence frame may draw a narrower
            // part of its plane. The row pitch comes from the source (source_pitch()).
            case 2:
                return packet_words[b + 2] & 0xFFFF;
            case 3:
                return packet_words[b + 2] >> 16;
            case 4:
                return packet_words[b + 3];
            case 5:
                return packet_words[b + 4];
            case 6:
                return packet_words[b + 5];
            case 7:
                return packet_words[b + 6];
            case 8:
                return packet_words[b + 7];
            case 9:
                return packet_words[b + 8];
            case 10:
                return packet_words[b + 10] & ~2;
            case 11:
                return packet_words[b + 11];
            case 12:
                return packet_words[b + 12];
            case 13:
                return packet_words[b + 9] & 0xFFFFFF;
            case 14:
                return packet_words[b + 13];
            case 15:
                return packet_words[b + 14];
            case 17:
                return (packet_words[b + 10] >> 1) & 1;
            case 18:
                return packet_words[b + 15];
            case 19:
                return int(uint(packet_words[b + 9]) >> 24);
            default:
                return 0;
        }
    }
    if (!RESIDENT) return packet_words[packet * 20 + slot];
    int base = packet * 12;
    uint metadata = uint(packet_words[base + 7]);
    switch (slot) {
        case 0:
#ifdef RESIDENT_RESOURCES
            return resource_words[packet_words[base] * 4];
#else
            return packet_words[base];
#endif
        case 1:
            return packet_words[base + 1];
        case 2:
            return packet_words[base + 2] & 0xFFFF;
        case 3:
            return packet_words[base + 2] >> 16;
        case 4:
            return packet_words[base + 3];
        case 5:
            return packet_words[base + 4];
        case 6:
            return int((metadata >> 11) & 7u) - 1;
        case 7:
            return packet_words[base + 8];
        case 8:
            return int((metadata >> 10) & 1u);
        case 10:
            return int((metadata & 0xFFFF0000u) | ((metadata >> 8) & 1u));
        case 11:
            return int(uint(packet_words[base + 6]) >> 24);
        case 12:
            return int(metadata & 0xFFu) + 1;
        case 13:
            return packet_words[base + 6] & 0xFFFFFF;
        case 14:
            return packet_words[base + 5];
        case 15:
            return packet_words[base + 9];
        case 17:
            return int((metadata >> 9) & 1u);
        default:
            return 0;
    }
}

// Row pitch of the source: its metadata width (the drawn width for single images and
// fills). Dynamic instances and plane records name a source whose size differs from the drawn size.
int source_pitch(int drawn_width) {
#ifdef RESIDENT_RESOURCES
    if (DYNAMIC_INSTANCE) {
        int b = run.viewport == 3 ? packet_words[2 + packet] : packet * 16;
        if ((uint(packet_words[b + 9]) >> 24) != 2u) return resource_words[packet_words[b] * 4 + 1];
    } else if (RESIDENT && (packet_word(13) & SHAPE_PLANES_FLAG) != 0) {
        return resource_words[packet_words[packet * 12] * 4 + 1];
    }
#endif
    return drawn_width;
}

// Resident light sources (MapLightSource / MapPaletteSource): word 10 holds
// cell index | intensity source << 20 | palette source << 24.
uint light_word() { return RESIDENT ? uint(packet_words[packet * 12 + 10]) : 0u; }
int cell_light(int word) { return cell_light_words[int(light_word() & 0x3FFFFu) * 8 + word]; }
int intensity_source() { return int((light_word() >> 20) & 7u); }
int palette_source() { return int((light_word() >> 24) & 3u); }

// type_drawing_packets.cpp setup(): min(254, 261 * max(intensity, 0) / 2048),
// with the intensity the cell gives now when the packet took it from the cell.
int palette_intensity() {
    int source = intensity_source();
    if (source == 0) return packet_word(11);
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
    return packet_word(12);
}

void main() {
    packet = RESIDENT ? schedule_words[run.first_packet + gl_InstanceIndex] : run.first_packet + gl_InstanceIndex;
    int width = (RESIDENT || DYNAMIC_INSTANCE) ? target_width : packet_word(0);
    int height = (RESIDENT || DYNAMIC_INSTANCE) ? target_height : packet_word(1);
    int source_width = packet_word(2), source_height = packet_word(3);
    int pitch = source_pitch(source_width);
    int px = packet_word(4) - (RESIDENT ? run.camera_x : 0);
    int py = packet_word(5) - (RESIDENT ? run.camera_y : 0);
    int clip_x = RESIDENT ? run.clip_x : packet_word(6), clip_y = RESIDENT ? run.clip_y : packet_word(7);
    int clip_width = RESIDENT ? run.clip_width : packet_word(8),
        clip_height = RESIDENT ? run.clip_height : packet_word(9);
    int left = max(px, max(clip_x, 0)), top = max(py, max(clip_y, 0));
    int right = min(px + source_width, min(clip_x + clip_width, width));
    int bottom = min(py + source_height, min(clip_y + clip_height, height));
    if (right <= left || bottom <= top) {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);  // Outside the clip volume.
        return;
    }
    // Two triangles: (left, top), (right, top), (right, bottom) and
    // (left, top), (right, bottom), (left, bottom).
    int corner = gl_VertexIndex;
    int x = (corner == 1 || corner == 2 || corner == 4) ? right : left;
    int y = (corner == 2 || corner == 4 || corner == 5) ? bottom : top;
    gl_Position = vec4(2.0 * float(x) / float(width) - 1.0, 2.0 * float(y) / float(height) - 1.0, 0.0, 1.0);

    int source_base = (RESIDENT || DYNAMIC_INSTANCE) ? packet_word(0) : schedule_words[packet * 2];
    int palette_base = (RESIDENT || DYNAMIC_INSTANCE) ? packet_word(1) : schedule_words[packet * 2 + 1];
    int palette = palette_source();
    if (palette == PALETTE_CONVERTER) palette_base = converter_offsets[palette_base];
    int light = packet_word(10);
    bool lit = (light & 1) != 0;
    int kind = packet_word(19);
    int mode = packet_word(13);
    int first_visible = RESIDENT ? max(py, run.clip_y) : packet_word(18);

    // Depth: map_draw.comp palette_pixel_depth(); resident records derive the
    // original gradient from the visible rows here.
    int depth_base = packet_word(14) + (RESIDENT ? run.camera_y : 0);
    int gradient = packet_word(15);
    bool wrap = false;
    if ((mode & ORIGINAL_SHAPE_DEPTH_FLAG) != 0) {
        if (RESIDENT) {
            int shape = packet_word(6), adjust = packet_word(7), phase, step, limit, delta;
            if (shape == 2) {
                int last =
                    packet_word(8) != 0 ? py + source_height : min(py + source_height, run.clip_y + run.clip_height);
                int rows = last - first_visible;
                depth_base = adjust + 32768 - last + 1;
                if ((mode & 0x20000) == 0) {
                    depth_base = 3 * (depth_base / 3) - rows / 3;
                    phase = 3 - rows % 3;
                    if (phase == 3) {
                        phase = 0;
                        depth_base += 1;
                    }
                } else {
                    phase = 0;
                }
                step = 1;
                limit = 3;
                delta = 1;
            } else {
                depth_base = adjust + 32768 - first_visible;
                int divisor = shape == 1 ? 3 : 1;
                depth_base = divisor * (depth_base / divisor);
                phase = 0;
                step = shape == 1 ? 2 : 1;
                limit = shape == 1 ? 3 : 1;
                delta = -1;
            }
            gradient = phase | (step << 8) | (limit << 16) | (delta << 24);
        }
    } else {
        wrap = gradient != 0;
    }
    int light_row = -1;
    if (lit && packet_word(17) != 0) light_row = (first_visible % height + height) % height;
    bool planes = (DYNAMIC_INSTANCE || RESIDENT) && kind == 0 && (mode & SHAPE_PLANES_FLAG) != 0;
    int flags =
        (lit ? 1 : 0) | (wrap ? 2 : 0) | (world_depth != 0 ? 4 : 0) | (kind << 4) | (palette << 6) | (planes ? 256 : 0);
    ivec4 plane = ivec4(-1, 0, 0, 0);
#ifdef RESIDENT_RESOURCES
    if (planes) {
        // Dynamic instances: SHA entry in word 14, offset in word 15. Resident records: words 9 and 11.
        int b = RESIDENT ? packet * 12 : run.viewport == 3 ? packet_words[2 + packet] : packet * 16;
        plane.x = resource_words[packet_words[b] * 4 + 3];
        if ((mode & AUXILIARY_DEPTH_FLAG) != 0 && (mode & ORIGINAL_SHAPE_DEPTH_FLAG) != 0) {
            int aux = packet_words[b + (RESIDENT ? 9 : 14)];
            plane.y = resource_words[aux * 4];
            plane.z = resource_words[aux * 4 + 1];
            plane.w = packet_words[b + (RESIDENT ? 11 : 15)];
        }
    }
#endif
    int depth_offset = world_depth != 0 ? 0x100000 - (RESIDENT ? run.camera_y : camera_y) : 0;
    ivec3 tint = ivec3(0);
    if (palette == PALETTE_CELL_TINT || palette == PALETTE_SPRITE_TINT) {
        int factors = palette == PALETTE_CELL_TINT ? 2 : 5;
        tint = ivec3(cell_light(factors), cell_light(factors + 1), cell_light(factors + 2));
    }
    v_source = ivec4(source_base, pitch, px, py);
    v_palette = ivec4(palette_base, shade_count(), lit ? palette_intensity() : 0, flags);
    v_depth = ivec4(mode, depth_base, gradient, first_visible);
    v_extra = ivec4(max(clip_x - px, 0), depth_offset, light_row, int(uint(light) >> 16u));
    v_tint = ivec4(tint, width);
    v_plane = plane;
}
