#include "convert_revision.hpp"
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace game {
namespace {
std::uint64_t next_revision = 1;
std::unordered_map<const ConvertClass*, std::uint64_t>& revisions() {
    static std::unordered_map<const ConvertClass*, std::uint64_t> map;
    return map;
}
std::vector<const ConvertClass*> changed, taken;
}
std::uint64_t converter_revision(const ConvertClass& converter) noexcept {
    try {
        auto [found, inserted] = revisions().try_emplace(&converter, 0);
        if (inserted) found->second = next_revision++;
        return found->second;
    } catch (...) {
        return next_revision++;
    }
}
void note_converter_tables(const ConvertClass& converter) noexcept {
    const auto revision = next_revision++;
    try {
        revisions()[&converter] = revision;
        if (std::find(changed.begin(), changed.end(), &converter) == changed.end()) changed.push_back(&converter);
    } catch (...) {
        // Without a record the next query assigns another fresh revision.
        revisions().erase(&converter);
    }
}
void forget_converter(const ConvertClass& converter) noexcept {
    revisions().erase(&converter);
    changed.erase(std::remove(changed.begin(), changed.end(), &converter), changed.end());
}
std::uint64_t converter_tables_generation() noexcept { return next_revision - 1; }
void take_changed_converters(const ConvertClass* const*& output, std::uint32_t& count) noexcept {
    taken.swap(changed);
    changed.clear();
    output = taken.data();
    count = std::uint32_t(taken.size());
}
}
