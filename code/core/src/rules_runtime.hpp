#pragma once
#include "api/rules_runtime.hpp"
#include <cfenv>

namespace game {
const RulesRuntimeServices* default_rules_runtime() noexcept;
const RulesRuntimeServices& rules_runtime();
inline bool rules_integer_rounding_is_known_active() noexcept {
#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
    // Some x86 CRTs hide mixed x87/SSE modes in fegetround. Keep the original
    // update and evaluation order when the standard query cannot prove both.
    return false;
#else
    return std::fegetround() == FE_TOWARDZERO;
#endif
}
// Fixed target 7C5F00 leaves its integer-conversion rounding mode active.
void rules_integer_rounding() noexcept;
bool rules_resolve_type(AbstractType type, const char* name, AbstractTypeClass*& result);
bool rules_sound_index(const char* name, int& result);
bool rules_type_count(AbstractType type, int& result);
bool rules_type_at(AbstractType type, int index, AbstractTypeClass*& result);
// Advanced command bar positions are stored in TabClass, the original owner.
void bind_command_bar_runtime(RulesRuntimeServices&) noexcept;
}
