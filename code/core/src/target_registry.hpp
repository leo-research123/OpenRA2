#pragma once
class AbstractClass;
namespace game {
// For the concrete classes that originally register in TargetIndex, not all Abstracts.
// Callers ignore AddIndex's bool, as in the original. Allocation policy remains
// IndexClass/GameAllocator's contract; exceptions cannot cross these constructors.
void register_target_identity(AbstractClass&) noexcept;
void unregister_target_identity(AbstractClass&) noexcept;
}
