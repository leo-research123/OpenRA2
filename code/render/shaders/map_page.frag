#version 450

// Page Z is world Z. Unlike a view-clipped draw, gradient and auxiliary depth
// start at the image's own first row and column, so an image spanning several
// pages gets one depth everywhere; the original restarted them at the first
// visible row. Resource Z is not wrapped to 16 bits.
layout(set = 0, binding = 0, std430) readonly buffer Records {
    int record_words[];
};
layout(set = 0, binding = 1) uniform usampler2DArray resource_pages;
uint resource_word(int index) {
    return texelFetch(resource_pages, ivec3(index & 1023, (index >> 10) & 1023, index >> 20), 0).r;
}
layout(push_constant, std430) uniform Page {
    int first;
    int origin_x;
    int origin_y;
    int size;
} page;
layout(set = 0, binding = 2, std430) readonly buffer ResourceMetadata {
    int resource_words[];
};
layout(location = 0) flat in int record;
layout(location = 0) out uint material;
layout(location = 1) out vec4 shadow;

const int ORIGINAL_SHAPE_DEPTH_FLAG = 0x10000;
const int AUXILIARY_DEPTH_FLAG = 0x20000;
const int DEPTH_NONE = 0;
const int BLEND_SHADOW = 1;
const uint SHAPE_TEXEL_OCCUPIED = 0x10000u;
// The record's source is a shape sequence frame: 8-bit planes named by a frame table entry
// (colour plane address, width, height, prefix plane address). Word 9 then names the SHA
// frame's entry and word 11 its offset (x | y << 16). Matches render::ShapePlanesFlag.
const int SHAPE_PLANES_FLAG = 0x80000;

uint resource_byte(int byte_index) { return (resource_word(byte_index >> 2) >> uint((byte_index & 3) * 8)) & 0xFFu; }

int word(int slot) { return record_words[record * 12 + slot]; }

void main() {
    ivec2 position = ivec2(gl_FragCoord.xy);
    int source_width = word(2) & 0xFFFF, source_height = word(2) >> 16;
    int destination_x = word(3) - page.origin_x, destination_y = word(4) - page.origin_y;
    ivec2 source_position = position - ivec2(destination_x, destination_y);
    int source_base = resource_words[word(0) * 4];
    int draw_mode = word(6) & 0xFFFFFF;
    bool planes = (draw_mode & SHAPE_PLANES_FLAG) != 0;
    int pitch = planes ? resource_words[word(0) * 4 + 1] : source_width;
    uint sample_word;
    if (planes) {
        uint index = resource_byte(source_base + source_position.y * pitch + source_position.x);
        sample_word = index != 0u ? (index | SHAPE_TEXEL_OCCUPIED) : 0u;
    } else {
        sample_word = resource_word(source_base + source_position.y * source_width + source_position.x);
    }
    if ((sample_word & SHAPE_TEXEL_OCCUPIED) == 0u) discard;
    uint metadata = uint(word(7));
    // World Z: map_raster.vert/.frag's resident formulas with the image's own world
    // rows. The original quantizes gradients on screen rows, so its rounding
    // followed the camera; here it is fixed in the world and equal across pages.
    int pixel_depth = 0xFFFF - 0x100000;
    if ((draw_mode & 0xFF) != DEPTH_NONE) {
        if ((draw_mode & ORIGINAL_SHAPE_DEPTH_FLAG) != 0) {
            int gradient = int((metadata >> 11) & 7u) - 1, adjust = word(8);
            int top = word(4), row = position.y + page.origin_y - top, phase, step, limit, delta, base;
            if (gradient == 2) {
                int last = top + source_height;
                base = adjust + 32768 - last + 1;
                if ((draw_mode & AUXILIARY_DEPTH_FLAG) == 0) {
                    base = 3 * (base / 3) - source_height / 3;
                    phase = 3 - source_height % 3;
                    if (phase == 3) {
                        phase = 0;
                        base += 1;
                    }
                }
                step = 1;
                limit = 3;
                delta = 1;
            } else {
                base = adjust + 32768 - top;
                int divisor = gradient == 1 ? 3 : 1;
                base = divisor * (base / divisor);
                phase = 0;
                step = gradient == 1 ? 2 : 1;
                limit = gradient == 1 ? 3 : 1;
                delta = -1;
            }
            if ((draw_mode & AUXILIARY_DEPTH_FLAG) != 0) {
                if (planes) {
                    int prefix_base = resource_words[word(0) * 4 + 3];
                    int prefix = prefix_base < 0 ? 0 : int(resource_byte(prefix_base + source_position.y * pitch));
                    int auxiliary_entry = word(9), offset = word(11);
                    int auxiliary_pitch = resource_words[auxiliary_entry * 4 + 1];
                    int ax = source_position.x - prefix + (offset & 0xFFFF), ay = source_position.y + (offset >> 16);
                    uint delta = resource_byte(resource_words[auxiliary_entry * 4] + ay * auxiliary_pitch + ax);
                    pixel_depth = base - (int(delta << 24) >> 24);
                } else {
                    int first = source_base + source_position.y * source_width;
                    int prefix = int((resource_word(first) >> 17) & 0xFFu);
                    uint auxiliary = resource_word(first + source_position.x - prefix);
                    pixel_depth = base - (int(auxiliary << 16) >> 24);
                }
            } else {
                pixel_depth = base + ((phase + row * step) / limit) * delta;
            }
        } else {
            pixel_depth = word(5) + int((sample_word >> 8) & 0xFFu);
        }
    }
    gl_FragDepth = float(clamp(pixel_depth + 0x100000, 0, 0xFFFFFF)) / 16777216.0;
    if (((draw_mode >> 8) & 0xFF) == BLEND_SHADOW) {
        // The shadow pipeline masks the material and adds one to the count.
        material = 0u;
        shadow = vec4(1.0 / 255.0);
        return;
    }
    // Record ids start at 1; 0 marks an empty pixel. An opaque write clears
    // the shadows below it.
    material = (uint(record) << 8) | (sample_word & 0xFFu);
    shadow = vec4(0.0);
}
