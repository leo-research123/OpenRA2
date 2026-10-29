#version 450

// Material page generation. A resident background instance is drawn into one
// world-space page of map_resolve.vert/.frag: the page keeps, per pixel, which record
// and palette index is on top, how many shadows lie over it and its world Z.
// Lighting is not evaluated here, so light and palette changes never redraw a
// page. Records use the renderer's 48-byte resident layout (renderer.cpp
// prepare_scene); instance i of a draw is the record whose id is entry first + i
// of the draw order (the instance's occupied slots), and that id is what the page
// stores.
layout(set = 0, binding = 0, std430) readonly buffer Records {
    int record_words[];
};
layout(set = 0, binding = 3, std430) readonly buffer Order {
    int order_words[];
};
layout(push_constant, std430) uniform Page {
    int first;
    int origin_x;
    int origin_y;
    int size;
} page;
layout(location = 0) flat out int record;

void main() {
    record = order_words[page.first + gl_InstanceIndex];
    int base = record * 12;
    int width = record_words[base + 2] & 0xFFFF, height = record_words[base + 2] >> 16;
    int px = record_words[base + 3] - page.origin_x, py = record_words[base + 4] - page.origin_y;
    int left = max(px, 0), top = max(py, 0);
    int right = min(px + width, page.size), bottom = min(py + height, page.size);
    if (right <= left || bottom <= top) {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);  // Outside the clip volume.
        return;
    }
    int corner = gl_VertexIndex;
    int x = (corner == 1 || corner == 2 || corner == 4) ? right : left;
    int y = (corner == 2 || corner == 4 || corner == 5) ? bottom : top;
    gl_Position = vec4(2.0 * float(x) / float(page.size) - 1.0, 2.0 * float(y) / float(page.size) - 1.0, 0.0, 1.0);
}
