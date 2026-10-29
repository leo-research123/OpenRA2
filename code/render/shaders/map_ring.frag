#version 450

layout(set = 0, binding = 0) uniform usampler2D ring;
layout(push_constant, std430) uniform Copy {
    int camera_x;
    int camera_y;
    int clip_x;
    int clip_y;
    int clip_width;
    int clip_height;
    int mask_x;
    int mask_y;
    int target_width;
    int target_height;
    int reserved0;
    int reserved1;
} view;
layout(location = 0) out vec4 color;

void main() {
    uvec2 texel =
        texelFetch(ring,
                   (ivec2(gl_FragCoord.xy) + ivec2(view.camera_x, view.camera_y)) & ivec2(view.mask_x, view.mask_y), 0)
            .rg;
    if ((texel.x & 0x80000000u) != 0u) discard;
    gl_FragDepth = float(texel.x) * (1.0 / 16777216.0);
    color = vec4(vec3(uvec3(texel.y, texel.y >> 8, texel.y >> 16) & 255u) / 255.0, 1.0);
}
