#pragma once
#include <cstdint>
namespace render {
struct Spirv { const std::uint32_t* words = nullptr; std::uint32_t count = 0; };
struct ShaderProgram { Spirv compute{}, vertex{}, fragment{}; };
}
