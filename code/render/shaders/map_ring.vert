#version 450

// Lit background ring copy: one quad over the view clip fetches the lit color
// and world Z of every pixel from the lit background ring (map_resolve.vert/.frag
// compiled with RING_TILE fills it, one world tile at a time) at (world & mask)
// and writes them as the frame's color and depth. Pixels without a background
// pixel are left as the frame clear made them.
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

void main() {
    int corner = gl_VertexIndex;
    int x = (corner == 1 || corner == 2 || corner == 4) ? view.clip_x + view.clip_width : view.clip_x;
    int y = (corner == 2 || corner == 4 || corner == 5) ? view.clip_y + view.clip_height : view.clip_y;
    gl_Position = vec4(2.0 * float(x) / float(view.target_width) - 1.0,
                       2.0 * float(y) / float(view.target_height) - 1.0, 0.0, 1.0);
}
