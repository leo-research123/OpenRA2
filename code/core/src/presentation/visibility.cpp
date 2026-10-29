#include "presentation/visibility.hpp"
#include "tactical_spatial.hpp"
#include "map_world.hpp"
#include <algorithm>
#include <cstdint>
#include <unordered_set>
#include <vector>
namespace game {
namespace {
// The visible objects as an open-addressing hash set of addresses (linear probing, backward-shift deletion).
// The traversal sets and clears an entry for every object it reaches in every frame, so the set must not
// allocate per entry or chase node pointers: the table is one flat array that is reused across frames and grows
// only while the number of visible objects grows. nullptr marks an empty slot; a null object is kept apart.
class AddressSet {
public:
    // May throw std::bad_alloc while growing; the set is unchanged then.
    void insert(const ObjectClass* object) {
        if (!object) {
            has_null_ = true;
            return;
        }
        if ((count_ + 1) * 2 > slots_.size()) grow();
        for (auto i = home(object);; i = (i + 1) & mask_) {
            if (slots_[i] == object) return;
            if (!slots_[i]) {
                slots_[i] = object;
                ++count_;
                return;
            }
        }
    }
    bool contains(const ObjectClass* object) const noexcept {
        if (!object) return has_null_;
        if (!count_) return false;
        for (auto i = home(object); slots_[i]; i = (i + 1) & mask_)
            if (slots_[i] == object) return true;
        return false;
    }
    void erase(const ObjectClass* object) noexcept {
        if (!object) {
            has_null_ = false;
            return;
        }
        if (!count_) return;
        auto i = home(object);
        while (slots_[i] != object) {
            if (!slots_[i]) return;
            i = (i + 1) & mask_;
        }
        // Close the gap: move later members of the probe run back while that keeps them reachable from their home.
        for (auto j = (i + 1) & mask_; slots_[j]; j = (j + 1) & mask_) {
            const auto h = home(slots_[j]);
            if (((j - h) & mask_) >= ((j - i) & mask_)) {
                slots_[i] = slots_[j];
                i = j;
            }
        }
        slots_[i] = nullptr;
        --count_;
    }
    void clear() noexcept {
        std::fill(slots_.begin(), slots_.end(), nullptr);
        count_ = 0;
        has_null_ = false;
    }
    unsigned size() const noexcept { return unsigned(count_ + (has_null_ ? 1 : 0)); }

private:
    std::size_t home(const ObjectClass* object) const noexcept {
        // Objects are at least 16 byte aligned: drop the low bits, then spread the rest.
        return std::size_t((std::uintptr_t(object) >> 4) * 0x9E3779B97F4A7C15ull >> 20) & mask_;
    }
    void grow() {
        std::vector<const ObjectClass*> old(slots_.empty() ? 256 : slots_.size() * 2, nullptr);
        old.swap(slots_);
        mask_ = slots_.size() - 1;
        for (const auto* object : old) {
            if (!object) continue;
            auto i = home(object);
            while (slots_[i]) i = (i + 1) & mask_;
            slots_[i] = object;
        }
    }
    std::vector<const ObjectClass*> slots_;
    std::size_t mask_ = 0, count_ = 0;
    bool has_null_ = false;
};
AddressSet& storage() {
    // Never destroyed: objects are forgotten from their destructors, which can run after static teardown.
    static auto* objects = new AddressSet;
    return *objects;
}
}
void VisibleObjects::set(const ObjectClass* object, bool visible) {
    if (visible)
        storage().insert(object);
    else
        storage().erase(object);
}
bool VisibleObjects::contains(const ObjectClass* object) const noexcept { return storage().contains(object); }
void VisibleObjects::forget(const ObjectClass* object) noexcept { storage().erase(object); }
void VisibleObjects::clear() noexcept { storage().clear(); }
unsigned VisibleObjects::size() const noexcept { return storage().size(); }
namespace {
std::unordered_set<const ObjectClass*>& hidden_storage() {
    static auto* objects = new std::unordered_set<const ObjectClass*>;
    return *objects;
}
}
void HiddenAnims::hide(const ObjectClass* anim) { hidden_storage().insert(anim); }
bool HiddenAnims::contains(const ObjectClass* anim) const noexcept {
    try {
        return hidden_storage().find(anim) != hidden_storage().end();
    } catch (...) {
        return false;
    }
}
void HiddenAnims::forget(const ObjectClass* anim) noexcept {
    try {
        if (!hidden_storage().empty()) hidden_storage().erase(anim);
    } catch (...) {
    }
}
void HiddenAnims::clear() noexcept { hidden_storage().clear(); }
unsigned HiddenAnims::size() const noexcept { return unsigned(hidden_storage().size()); }
HiddenAnims& hidden_anims() noexcept {
    static HiddenAnims set;
    return set;
}
namespace {
std::unordered_set<const ObjectClass*>& redraw_storage() {
    static auto* objects = new std::unordered_set<const ObjectClass*>;
    return *objects;
}
}
void RedrawRequests::request(const ObjectClass* object) { redraw_storage().insert(object); }
void RedrawRequests::forget(const ObjectClass* object) noexcept {
    try {
        if (!redraw_storage().empty()) redraw_storage().erase(object);
    } catch (...) {
    }
}
void RedrawRequests::apply() noexcept {
    try {
        auto pending = std::move(redraw_storage());
        redraw_storage().clear();
        for (const auto* object : pending) tactical_spatial_dirty(object);
        if (!pending.empty()) map_object_changed();
    } catch (...) {
    }
}
unsigned RedrawRequests::size() const noexcept { return unsigned(redraw_storage().size()); }
RedrawRequests& redraw_requests() noexcept {
    static RedrawRequests set;
    return set;
}
VisibleObjects& visible_objects() noexcept {
    static VisibleObjects set;
    return set;
}
}

#include "yrpp/UnitClass.h"
#include <unordered_map>
namespace game {
namespace {
auto& waterlines() { static auto* values = new std::unordered_map<const ObjectClass*, unsigned short>; return *values; }
}
unsigned short unit_waterline(const UnitClass& unit) {
    const auto found = waterlines().find(&unit);
    return found == waterlines().end() ? unit.unknown_3CA : found->second;
}
void set_unit_waterline(const UnitClass& unit, unsigned short value) { waterlines()[&unit] = value; }
void forget_unit_waterline(const ObjectClass* object) noexcept { try { waterlines().erase(object); } catch (...) {} }
}
