#[compute]
#version 450

// Native nonrotating ABuffer, consecutive immutable lighting SHP requests.
// Each invocation owns one destination pixel and applies its spatial bin's
// requests in submission order. This preserves overlapping integer operations
// without one dispatch/barrier per cell. The host barriers before consumers.
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout(set = 0, binding = 0, std430) readonly buffer Parameters { int words[]; };
layout(set = 0, binding = 1, std430) readonly buffer Atlas { uint texels[]; };
layout(set = 0, binding = 2, std430) buffer Lighting { uint lights[]; };
layout(set = 0, binding = 3, std430) readonly buffer Bins { int bins[]; };
layout(push_constant, std430) uniform Batch { int width; int height; int columns; int bin_offset; } batch;

void main() {
    ivec2 at = ivec2(gl_GlobalInvocationID.xy);
    if (at.x >= batch.width || at.y >= batch.height) return;
    // Must match game::LightingBinSize. Bin entries are absolute word offsets.
    int tile = (at.y / 32) * batch.columns + at.x / 32;
    int first = bins[batch.bin_offset + tile * 2];
    int count = bins[batch.bin_offset + tile * 2 + 1];
    if (count == 0) return;
    int destination = at.y * batch.width + at.x;
    uint value = lights[destination];
    for (int n = 0; n < count; ++n) {
        int p = bins[first + n] * 20;
        ivec2 local = at - ivec2(words[p + 4], words[p + 5]);
        ivec2 clip = ivec2(words[p + 6], words[p + 7]);
        if (local.x < 0 || local.y < 0 || local.x >= words[p + 2] || local.y >= words[p + 3]
            || at.x < clip.x || at.y < clip.y
            || at.x >= clip.x + words[p + 8] || at.y >= clip.y + words[p + 9]) continue;
        uint sample_value = texels[words[p + 17] + local.y * words[p + 2] + local.x];
        if ((sample_value & 0x10000u) == 0u) continue;
        uint source = sample_value & 0xFFu;
        int operation = words[p + 19];
        if (operation == 1) {
            // 0x47EFE0: copy shroud except sentinel 254; zero is covered.
            if (source != 254u) value = source;
        } else if (operation == 2) {
            // 0x47F250: sources >127 skip without modifying the old word.
            if (source <= 127u) value = uint(max(0, int(value & 0xFFFFu) + int(source) - 127));
        } else if (operation == 3) {
            // 0x420F40 / 0x421350 use 0x420960's table. Truncate and clamp
            // EACH multiplication; combining overlapping AlphaShapes changes it.
            value = min(255u, (value & 0xFFFFu) * source / 127u);
        }
    }
    lights[destination] = value;
}
