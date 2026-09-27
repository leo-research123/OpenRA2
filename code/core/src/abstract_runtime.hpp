#pragma once
#include <cstdint>
class AbstractClass;
namespace game {
// Original swizzle registration dependency; not a second pointer registry.
void register_original_abstract(std::uint32_t token, AbstractClass*) noexcept;
}
