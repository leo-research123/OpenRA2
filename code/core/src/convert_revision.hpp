#pragma once
#include <cstdint>

class ConvertClass;

namespace game {
// Content identity of original ConvertClass tables (PaletteData and
// FullColorData). Every write of a converter's tables assigns a fresh
// revision from one counter; revisions are never reused, so an address and a
// revision name one table content even after a destroyed converter's address
// is reused. Hosts cache converter copies by (address, revision) instead of
// comparing table contents. Converters not yet noted get a revision on first
// query. Allocation failure yields a fresh revision on every query, which
// only costs the host a copy.
std::uint64_t converter_revision(const ConvertClass&) noexcept;
// Called after the converter's tables were built or rebuilt.
void note_converter_tables(const ConvertClass&) noexcept;
// Called when the converter's tables are released.
void forget_converter(const ConvertClass&) noexcept;
// The latest revision assigned to any converter: changes whenever any
// converter's tables are written.
std::uint64_t converter_tables_generation() noexcept;
// Converters whose tables changed since the last take, in first-change order;
// forgotten converters are removed. output/count are borrowed until the next
// call into this module.
void take_changed_converters(const ConvertClass* const*& output, std::uint32_t& count) noexcept;
}
