#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace game {
// Accepts the original GetPixels() pointer: the caller must supply a valid,
// readable frame for the duration of this call. Raw data uses width * height;
// RLE uses height rows, each prefixed with its uint16 byte length (including
// that prefix). No total payload size or file-boundary validation is provided.
// On reported failure indices remain unchanged.
bool decode_shp_pixels(const uint8_t* payload, int width, int height,
    bool compressed, std::vector<uint8_t>& indices, std::string& error);
}
