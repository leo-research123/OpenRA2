#include "target_registry.hpp"
#include "yrpp/AbstractClass.h"
namespace game {
void register_target_identity(AbstractClass& object) noexcept {
    try { AbstractClass::TargetIndex.AddIndex(object.Fetch_ID(), &object); }
    catch (...) {} // Do not propagate through the concrete noexcept constructors.
}
void unregister_target_identity(AbstractClass& object) noexcept {
    // No allocation is performed by the calibrated sort/search/remove path.
    AbstractClass::TargetIndex.RemoveIndex(object.Fetch_ID());
}
}
