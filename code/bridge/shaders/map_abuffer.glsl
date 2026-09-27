#[compute]
#version 450

// ABuffer producer used by TacticalClass::DrawShroud in native frame preparation.
// 127 is only the reset value;
// original shroud/fog and AlphaShape writes must precede map_draw consumers.
// Dispatch ordered packets with a barrier after each, including before drawing.
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(set = 0, binding = 0, std430) readonly buffer Parameters { int packet_words[]; };
layout(set = 0, binding = 1, std430) readonly buffer Source { uint source_texels[]; };
layout(set = 0, binding = 2, std430) buffer Lighting { uint light_intensities[]; };
layout(push_constant, std430) uniform Batch { int packet_index; } batch;

// Same geometry slots and 20-word stride as map_draw; word 19 selects an
// ABuffer operation, NOT a map_draw kind. This pass never binds color or Z.
const int PACKET_WORD_COUNT = 20;
#define PACKET_WORD(slot) packet_words[batch.packet_index * PACKET_WORD_COUNT + (slot)]
#define TARGET_WIDTH          PACKET_WORD(0)
#define TARGET_HEIGHT         PACKET_WORD(1)
#define SOURCE_WIDTH          PACKET_WORD(2)
#define SOURCE_HEIGHT         PACKET_WORD(3)
#define DESTINATION_X         PACKET_WORD(4)
#define DESTINATION_Y         PACKET_WORD(5)
#define CLIP_X                PACKET_WORD(6)
#define CLIP_Y                PACKET_WORD(7)
#define CLIP_WIDTH            PACKET_WORD(8)
#define CLIP_HEIGHT           PACKET_WORD(9)
#define STATE_ORIGIN_Y        PACKET_WORD(16)
#define ABUFFER_OPERATION     PACKET_WORD(19)

const int ABUFFER_RESET = 0;
const int ABUFFER_SHROUD = 1;
const int ABUFFER_FOG = 2;
const int ABUFFER_ALPHA_SHAPE = 3;
const uint NEUTRAL_LIGHT = 127u;
const uint SOURCE_PIXEL_COVERED = 0x10000u;
const uint SHROUD_SKIP_VALUE = 254u;

uint fog_intensity(uint previous, uint source_intensity) {
    // 0x0047F250: source > 127 skips; old == 127 copies source. The same
    // integer expression covers the copy case and overlapping fog pixels.
    return source_intensity > NEUTRAL_LIGHT ? previous
        : uint(max(0, int(previous) + int(source_intensity) - int(NEUTRAL_LIGHT)));
}

uint alpha_shape_intensity(uint previous, uint source_intensity) {
    // 0x00420960 builds the table used by 0x00420F40 / 0x00421350.
    // Exact for the original 0..255 inputs; division truncates BEFORE clamp.
    // Source zero is valid and darkens to zero; 254 is NOT a skip value here.
    return min(255u, previous * source_intensity / NEUTRAL_LIGHT);
}

void main() {
    ivec2 source_position = ivec2(gl_GlobalInvocationID.xy);
    if (source_position.x >= SOURCE_WIDTH || source_position.y >= SOURCE_HEIGHT) {
        return;
    }
    ivec2 destination_position = source_position + ivec2(DESTINATION_X, DESTINATION_Y);
    if (destination_position.x < CLIP_X || destination_position.y < CLIP_Y
        || destination_position.x >= CLIP_X + CLIP_WIDTH || destination_position.y >= CLIP_Y + CLIP_HEIGHT
        || destination_position.x < 0 || destination_position.y < 0
        || destination_position.x >= TARGET_WIDTH || destination_position.y >= TARGET_HEIGHT) {
        return;
    }
    int state_row = ((destination_position.y - STATE_ORIGIN_Y) % TARGET_HEIGHT + TARGET_HEIGHT) % TARGET_HEIGHT;
    int state_index = state_row * TARGET_WIDTH + destination_position.x;
    if (ABUFFER_OPERATION == ABUFFER_RESET) {
        // 0x00411330: dirty rectangles reset ABuffer without clearing color/Z.
        light_intensities[state_index] = NEUTRAL_LIGHT;
        return;
    }
    if (ABUFFER_OPERATION < ABUFFER_SHROUD || ABUFFER_OPERATION > ABUFFER_ALPHA_SHAPE) {
        return;
    }

    uint source_sample = source_texels[source_position.y * SOURCE_WIDTH + source_position.x];
    // The host supplies decoded SHP coverage; shroud 254 handles the cell
    // shape exterior. Pixel value zero must not imply missing coverage.
    if ((source_sample & SOURCE_PIXEL_COVERED) == 0u) {
        return;
    }
    uint source_intensity = source_sample & 0xFFu;
    if (ABUFFER_OPERATION == ABUFFER_SHROUD) {
        // 0x0047EFE0: copy SHROUD.SHP / FOG.SHP except the sentinel 254.
        if (source_intensity != SHROUD_SKIP_VALUE) {
            light_intensities[state_index] = source_intensity;
        }
        return;
    }
    // Original storage is WORD; valid producer/consumer intensities are 0..255.
    uint previous = light_intensities[state_index] & 0xFFFFu;
    if (ABUFFER_OPERATION == ABUFFER_FOG) {
        light_intensities[state_index] = fog_intensity(previous, source_intensity);
    } else {
        light_intensities[state_index] = alpha_shape_intensity(previous, source_intensity);
    }
}
