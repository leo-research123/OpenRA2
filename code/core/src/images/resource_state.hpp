#pragma once
#include "yrpp/FileFormats/SHP.h"
#include <cstdint>

namespace game {
// Exact 5B3FF0 tree node, absent from the imported YRpp declarations. Values
// are borrowed. The tree owns nodes only, as demonstrated by 5B4080/5B4310.
struct NameNode {
    NameNode* left;
    NameNode* right;
    std::int32_t crc;
    void* value;
};
#ifdef RA2_IMAGE_GAME
// Original data-address seam only. Host storage is private to the owning .cpp.
NameNode*& name_root();
SHPReference*& shp_head();
std::uint32_t& shp_counter();
SHPStruct*& shared_shape();
std::int32_t& shared_capacity();
std::int32_t& shared_index();
RectangleStruct& invalid_frame_bounds();
#endif

void insert_name_node(NameNode* node, NameNode** link);
void destroy_name_nodes(NameNode* node);
}
