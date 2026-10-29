#version 450

// Background resolve: one quad over the view clip reads the visible material
// pages (map_page.vert/.frag) and writes the lit background color and its world Z.
//
// Compiled with RING_TILE (the map_ring_tile resource), the same lighting
// resolves one 256x256 world tile of a material page into the lit background
// ring instead: the quad is the tile's rectangle in the ring, the fragment
// stage writes the packed lit color and Z of every pixel of it (map_ring.vert/.frag
// copies the ring to the frame), and the light is read in world coordinates.
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

void main() {
    int corner = gl_VertexIndex;
    int x = tile.ring_x + ((corner == 1 || corner == 2 || corner == 4) ? 256 : 0);
    int y = tile.ring_y + ((corner == 2 || corner == 4 || corner == 5) ? 256 : 0);
    gl_Position =
        vec4(2.0 * float(x) / float(tile.ring_width) - 1.0, 2.0 * float(y) / float(tile.ring_height) - 1.0, 0.0, 1.0);
}
#else
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

void main() {
    int corner = gl_VertexIndex;
    int x = (corner == 1 || corner == 2 || corner == 4) ? view.clip_x + view.clip_width : view.clip_x;
    int y = (corner == 2 || corner == 4 || corner == 5) ? view.clip_y + view.clip_height : view.clip_y;
    gl_Position = vec4(2.0 * float(x) / float(view.target_width) - 1.0,
                       2.0 * float(y) / float(view.target_height) - 1.0, 0.0, 1.0);
}
#endif
