#[compute]
#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout(set = 0, binding = 0, std430) buffer Colors { uint output_colors[]; };
layout(set = 0, binding = 1, std430) buffer Depth { uint depth_values[]; };
layout(set = 0, binding = 2, std430) buffer Lighting { uint light_intensities[]; };
layout(rgba8, set = 0, binding = 3) uniform writeonly image2D target;
layout(push_constant, std430) uniform Frame {
    int width;
    int height;
    int clear;
    int pad;
} frame;

const uint CLEAR_COLOR = 0xFF000000u;
const uint CLEAR_DEPTH = 0xFFFFu;
// Original ABuffer constructor 0x00410CE0. This resets the baseline only;
// shroud/fog and AlphaShape must then update it through map_abuffer.glsl.
const uint NEUTRAL_LIGHT = 127u;

void clear_pixel(int pixel_index) {
    output_colors[pixel_index] = CLEAR_COLOR;
    depth_values[pixel_index] = CLEAR_DEPTH;
    light_intensities[pixel_index] = NEUTRAL_LIGHT;
}

void present_pixel(ivec2 position, int pixel_index) {
    // Buffer words store red in the low byte. Presentation forces opaque alpha.
    uint color = output_colors[pixel_index];
    imageStore(target, position, vec4(
        color & 0xFFu, (color >> 8) & 0xFFu, (color >> 16) & 0xFFu, 255u) / 255.0);
}

void main() {
    ivec2 position = ivec2(gl_GlobalInvocationID.xy);
    if (position.x >= frame.width || position.y >= frame.height) {
        return;
    }
    int pixel_index = position.y * frame.width + position.x;
    if (frame.clear != 0) {
        clear_pixel(pixel_index);
    } else {
        present_pixel(position, pixel_index);
    }
}
