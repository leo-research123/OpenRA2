#pragma once
#include "api/rules_runtime.hpp"

namespace game {
const RulesRuntimeServices* default_rules_runtime() noexcept;
const RulesRuntimeServices& rules_runtime();
// Fixed target 7C5F00 leaves its integer-conversion rounding mode active.
void rules_integer_rounding() noexcept;
bool rules_resolve_type(AbstractType type, const char* name, AbstractTypeClass*& result);
bool rules_sound_index(const char* name, int& result);
bool rules_type_count(AbstractType type, int& result);
bool rules_type_at(AbstractType type, int index, AbstractTypeClass*& result);
}
