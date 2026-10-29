#include "techno_spatial.hpp"
#include "rules_runtime.hpp"
#include "yrpp/TechnoClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/DriveLocomotionClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/WeaponTypeClass.h"
#include "yrpp/WarheadTypeClass.h"
#include "yrpp/SessionClass.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <vector>

#if !defined(RA2_YRPP_GAME) && !defined(RA2_IMAGE_GAME)
namespace game {
namespace {
constexpr int bucket_width = 8 * 256, region_width = 4 * bucket_width;
constexpr int coordinate_limit = 0x100000;
constexpr auto absent = std::numeric_limits<std::uint32_t>::max();
struct ThreatBucket {
    std::vector<std::uint32_t> slots;
    std::array<unsigned, 64> houses{};
    std::uint64_t mask = 0;
};
struct ThreatRegion {
    int x = 0, y = 0;
    std::array<ThreatBucket, 16> buckets;
    std::array<unsigned, 64> houses{};
    std::uint64_t mask = 0;
};
struct ThreatSlot {
    TechnoClass* object = nullptr;
    TechnoTypeClass* type = nullptr;
    std::uint64_t order = 0;
    std::uint32_t region = absent, bucket = 0, offset = 0;
    unsigned house = 0;
    bool dirty = false, unsafe = false;
};
struct ThreatIndex {
    std::unordered_map<TechnoClass*, std::uint32_t> identities;
    std::unordered_map<std::uint64_t, std::uint32_t> region_ids;
    std::unordered_map<TechnoTypeClass*, unsigned> types;
    std::vector<ThreatSlot> slots;
    std::vector<ThreatRegion> regions;
    std::vector<std::uint32_t> free_slots, dirty;
    std::vector<HouseClass*> houses;
    std::array<unsigned, 64> house_counts{};
    std::uint64_t sequence = 0;
    unsigned unsafe = 0;
    bool failed = false;

    void mark(std::uint32_t id) {
        if (!slots[id].dirty) {
            dirty.push_back(id);
            slots[id].dirty = true;
        }
    }
    void add(TechnoClass* object) {
        const auto id = free_slots.empty() ? std::uint32_t(slots.size()) : free_slots.back();
        if (free_slots.empty())
            slots.emplace_back();
        else
            free_slots.pop_back();
        if (sequence == std::numeric_limits<std::uint64_t>::max()) {
            failed = true;
            return;
        }
        slots[id] = {};
        slots[id].object = object;
        slots[id].order = ++sequence;
        identities.emplace(object, id);
        mark(id);
    }
    void unlink(std::uint32_t id) {
        auto& slot = slots[id];
        if (slot.region != absent) {
            auto& region = regions[slot.region];
            auto& bucket = region.buckets[slot.bucket];
            const auto moved = bucket.slots.back();
            bucket.slots[slot.offset] = moved;
            slots[moved].offset = slot.offset;
            bucket.slots.pop_back();
            if (!--bucket.houses[slot.house]) bucket.mask &= ~(std::uint64_t{1} << slot.house);
            if (!--region.houses[slot.house]) region.mask &= ~(std::uint64_t{1} << slot.house);
            --house_counts[slot.house];
            slot.region = absent;
        }
        if (slot.type) {
            auto found = types.find(slot.type);
            if (!--found->second) types.erase(found);
            slot.type = nullptr;
        }
        if (slot.unsafe) {
            --unsafe;
            slot.unsafe = false;
        }
    }
    void refresh(std::uint32_t id) {
        auto& slot = slots[id];
        auto* object = slot.object;
        if (!object || object->LastLayer != Layer::Ground) {
            unlink(id);
            return;
        }
        // Include limbo and dead entries in the safety audit: the reference
        // evaluates weapon/cell queries before rejecting these entries.
        auto* type = object->GetTechnoType();
        const auto at = object->GetCoords();
        auto& map = MapClass::Instance;
        const auto* cell = map.TryGetCellAt(at);
        const auto kind = object->WhatAmI();
        if (!type || !object->Owner || type->Strength <= 0 || object->Health < 0 || object->Health > type->Strength ||
            (kind != AbstractType::Unit && kind != AbstractType::Infantry && kind != AbstractType::Building &&
             kind != AbstractType::Aircraft) ||
            type->CanDisguise || type->DisguiseWhenStill || object->Disguised || !map.TryGetCellAt(object->Location) ||
            !cell || object->OnBridge || cell->ContainsBridge() || at.X < 0 || at.Y < 0 || at.X > coordinate_limit ||
            at.Y > coordinate_limit || at.Z < -coordinate_limit || at.Z > coordinate_limit) {
            unlink(id);
            slot.unsafe = true;
            ++unsafe;
            return;
        }
        if (object->InLimbo) {
            unlink(id);
            return;
        }
        auto house = std::find(houses.begin(), houses.end(), object->Owner) - houses.begin();
        if (house == houses.size()) houses.push_back(object->Owner);
        if (house >= 64) {
            unlink(id);
            slot.unsafe = true;
            ++unsafe;
            return;
        }
        const int x = at.X / region_width, y = at.Y / region_width;
        const auto key = (std::uint64_t(unsigned(x)) << 32) | unsigned(y);
        auto found = region_ids.find(key);
        std::uint32_t region_id;
        if (found == region_ids.end()) {
            region_id = std::uint32_t(regions.size());
            regions.emplace_back();
            regions.back().x = x * region_width;
            regions.back().y = y * region_width;
            region_ids.emplace(key, region_id);
        } else
            region_id = found->second;
        auto& region = regions[region_id];
        const auto bucket_id = (at.Y % region_width / bucket_width) * 4 + at.X % region_width / bucket_width;
        if (slot.region == region_id && slot.bucket == unsigned(bucket_id) && slot.house == house && slot.type == type)
            return;
        unlink(id);
        auto& bucket = region.buckets[bucket_id];
        slot.region = region_id;
        slot.bucket = bucket_id;
        slot.offset = std::uint32_t(bucket.slots.size());
        slot.house = unsigned(house);
        bucket.slots.push_back(id);
        ++bucket.houses[house];
        bucket.mask |= std::uint64_t{1} << house;
        ++region.houses[house];
        region.mask |= std::uint64_t{1} << house;
        ++house_counts[house];
        slot.type = type;
        ++types[type];
    }
    void flush() {
        for (auto id : dirty) {
            slots[id].dirty = false;
            refresh(id);
        }
        dirty.clear();
    }
};
ThreatIndex index;
TechnoSpatialCounters counters;

bool conventional_weapon(const WeaponTypeClass* weapon) {
    if (!weapon) return true;
    auto* warhead = weapon->Warhead;
    return warhead && weapon->Projectile && !weapon->Spawner && !weapon->DrainWeapon && !warhead->IsLocomotor &&
           !warhead->Temporal && !warhead->Parasite && !warhead->MindControl;
}
struct ThreatBound {
    double constant = 0.0, distance = 0.0;
    int reach = 0, multiplier = 1;
    double upper(int lower) const {
        // Suppression only reduces positive values. Negative values, VHPScan
        // division and AllToHunt cannot exceed the final floor of one.
        return std::max(1.0, std::ceil(std::max(0.0, constant + distance * std::max(lower - reach, 0)) * multiplier));
    }
};
bool make_bound(TechnoClass& actor, ThreatBound& bound) {
    const auto* type = actor.GetTechnoType();
    const auto& rules = *RulesClass::Instance;
    const bool smart = actor.Owner->HasThreatNode;
    const double mine = smart ? type->MyEffectivenessCoefficient : rules.DumbMyEffectivenessCoefficient;
    const double theirs = smart ? type->TargetEffectivenessCoefficient : rules.DumbTargetEffectivenessCoefficient;
    const double special = smart ? type->TargetSpecialThreatCoefficient : rules.DumbTargetSpecialThreatCoefficient;
    const double health = smart ? type->TargetStrengthCoefficient : rules.DumbTargetStrengthCoefficient;
    bound.distance = smart ? type->TargetDistanceCoefficient : rules.DumbTargetDistanceCoefficient;
    if (!std::isfinite(mine) || !std::isfinite(theirs) || !std::isfinite(special) || !std::isfinite(health) ||
        !std::isfinite(bound.distance) || bound.distance >= 0 || !std::isfinite(rules.EnemyHouseThreatBonus))
        return false;
    double my_verses = 0, enemy_verses = 0, special_value = 0;
    double maximum_reach_magnitude = std::abs(double(type->GuardRange / 256));
    bound.reach = std::max(0, type->GuardRange / 256);
    for (int w = 0; w < TechnoTypeClass::MaxWeapons; ++w) {
        auto* weapon = actor.GetWeapon(w)->WeaponType;
        if (!weapon) continue;
        if (!conventional_weapon(weapon)) return false;
        // Suppression walks a rectangle around each target and may query
        // sparse off-map cells. Keep those InvalidCell writes in Array order.
        if (weapon->Supress && rules.FireSupress > 0) return false;
        bound.reach = std::max(bound.reach, weapon->Range / 256);
        maximum_reach_magnitude = std::max(maximum_reach_magnitude, std::abs(double(weapon->Range / 256)));
        for (double verse : weapon->Warhead->Verses) {
            if (!std::isfinite(verse)) return false;
            my_verses = std::max(my_verses, std::abs(verse));
        }
    }
    for (const auto& [enemy, count] : index.types) {
        if (!std::isfinite(enemy->SpecialThreatValue)) return false;
        special_value = std::max(special_value, std::abs(enemy->SpecialThreatValue));
    }
    // Covers elite, deployed, garrison and upgrade weapons without assuming
    // that GetWeapon returns a slot on the candidate's own type.
    for (auto* warhead : WarheadTypeClass::Array) {
        const double verse = warhead->Verses[int(type->Armor)];
        if (!std::isfinite(verse)) return false;
        enemy_verses = std::max(enemy_verses, std::abs(verse));
    }
    const double magnitude = std::abs(mine) * my_verses + std::abs(theirs) * enemy_verses +
                             std::abs(special) * special_value + std::abs(rules.EnemyHouseThreatBonus) +
                             std::max(0.0, health);
    // Reject conversion/wraparound domains, including negative VHPScan values.
    // Four score points dominate roundoff within this finite envelope.
    if (!std::isfinite(magnitude) || magnitude > 1e8 ||
        std::abs(bound.distance) * (4.0 * coordinate_limit + maximum_reach_magnitude) + magnitude + std::abs(health) +
                100000.0 >
            INT32_MAX / (type->VHPScan == 1 ? 2.0 : 1.0))
        return false;
    bound.constant = 100004.0 + magnitude;
    bound.multiplier = type->VHPScan == 1 ? 2 : 1;
    return true;
}
int lower_distance(const CoordStruct& origin, int x, int y, int width) {
    // Use the same monotone, quantized sqrt as ThreatCoeffients, NOT libm:
    // the original lookup can undershoot Euclidean distance by many leptons.
    // Z can only increase its nonnegative squared input. Retain edge slack.
    const int dx = std::max({x - origin.X, origin.X - (x + width - 1), 0});
    const int dy = std::max({y - origin.Y, origin.Y - (y + width - 1), 0});
    return std::max(0, int(Math::sqrt(double(dx) * dx + double(dy) * dy)) - 2);
}
struct ThreatVisit {
    double lower;
    std::uint32_t region;
    int bucket;
};
bool later(const ThreatVisit& a, const ThreatVisit& b) { return a.lower > b.lower; }
std::vector<ThreatVisit> visits;
}

void techno_spatial_register(TechnoClass* object) noexcept {
    if (index.failed) return;
    try {
        index.add(object);
    } catch (...) {
        index.failed = true;
    }
}
void techno_spatial_forget(TechnoClass* object) noexcept {
    if (index.failed) {
        // A failed partial update is never queried. Reset after world teardown.
        if (TechnoClass::Array.Count == 1) index = ThreatIndex{};
        return;
    }
    try {
        const auto found = index.identities.find(object);
        if (found == index.identities.end()) return;
        const auto id = found->second;
        index.unlink(id);
        index.slots[id].object = nullptr;
        index.slots[id].dirty = false;
        std::erase(index.dirty, id);
        index.free_slots.push_back(id);
        index.identities.erase(found);
        if (index.identities.empty()) index = ThreatIndex{};
    } catch (...) {
        index.failed = true;
    }
}
void techno_spatial_dirty(ObjectClass* object) noexcept {
    if (index.failed || !object || (object->AbstractFlags & ::AbstractFlags::Techno) == ::AbstractFlags::None) return;
    try {
        const auto found = index.identities.find(static_cast<TechnoClass*>(object));
        if (found != index.identities.end()) index.mark(found->second);
    } catch (...) {
        index.failed = true;
    }
}
bool try_spatial_ground_threat(TechnoClass& actor, unsigned flags, int mask, int zone, const CoordStruct* origin,
                               bool only_enemy, AbstractClass*& best, int& highest) noexcept {
    ++counters.queries;
    const auto run = [&]() -> bool {
        if (index.failed || index.identities.size() != std::size_t(TechnoClass::Array.Count) || !origin ||
            *origin == CoordStruct::Empty || origin->X < 0 || origin->Y < 0 || origin->X > coordinate_limit ||
            origin->Y > coordinate_limit || origin->Z < -coordinate_limit || origin->Z > coordinate_limit ||
            !rules_integer_rounding_is_known_active() || (flags & (3u | 0x100u | 0x800u | 0x8000u | 0x10000u)) ||
            actor.WhatAmI() != AbstractType::Unit || !actor.Owner || actor.Owner->AllToHunt ||
            MapClass::Instance.ZoneConnections.Count)
            return false;
        auto& unit = static_cast<UnitClass&>(actor);
        const auto* type = unit.Type;
        // Only the audited, read-only fire-admission path may be reordered.
        if (!type || type->DeployToFire || type->IsSimpleDeployer || type->DeploysInto || type->HasMultipleTurrets() ||
            type->IsGattling || type->CanDisguise || type->DisguiseWhenStill || actor.Transporter || actor.IsTether ||
            actor.SpawnManager || actor.BunkerLinkedItem || actor.TemporalImUsing ||
            !dynamic_cast<DriveLocomotionClass*>(static_cast<ILocomotion*>(unit.Locomotor)) || actor.OnBridge ||
            !MapClass::Instance.TryGetCellAt(actor.Location) || !MapClass::Instance.TryGetCellAt(actor.GetCoords()) ||
            MapClass::Instance.TryGetCellAt(actor.GetCoords())->ContainsBridge())
            return false;
        index.flush();
        if (index.unsafe) return false;
        ThreatBound bound;
        if (!make_bound(actor, bound)) return false;
        const bool friendly = type->AttackFriendlies || actor.Berzerk;
        const bool repair = actor.CombatDamage(-1) < 0;
        const bool require_discovery = SessionClass::Instance.GameMode == GameMode::Campaign &&
                                       actor.Owner->IsControlledByCurrentPlayer();
        const bool nearest = actor.CurrentMission == Mission::Hunt;
        const auto visit_lower = [&](int x, int y, int width) -> double {
            if (!nearest) return lower_distance(*origin, x, y, width);
            const double dx = std::max({x - origin->X, origin->X - (x + width - 1), 0});
            const double dy = std::max({y - origin->Y, origin->Y - (y + width - 1), 0});
            return dx * dx + dy * dy;
        };
        const auto evaluate = [&](TechnoClass* candidate, int& value) {
            ++counters.candidates;
            // These read-only rejections cannot affect RNG, InvalidCell or the
            // already-established rounding mode on the admitted fire path.
            if (candidate == &actor || candidate->InLimbo || !candidate->Health || !candidate->IsInPlayfield ||
                (candidate->CurrentMission != Mission::None && candidate->CurrentMissionControl()->NoThreat) ||
                (type->VHPScan == 2 && candidate->EstimatedHealth <= 0))
                return false;
            const auto* enemy = candidate->GetTechnoType();
            if (!enemy->LegalTarget || (!(unsigned(mask) & (1u << (unsigned(candidate->WhatAmI()) & 31))) &&
                                        (!(mask & 2) || !candidate->IsStrange())))
                return false;
            if (require_discovery && !candidate->IsOwnedByCurrentPlayer && !candidate->DiscoveredByCurrentPlayer &&
                candidate->WhatAmI() != AbstractType::Aircraft)
                return false;
            if (zone != -1 && MapClass::Instance.GetMovementZoneType(CellClass::Coord2Cell(candidate->GetCoords()),
                                                                     type->MovementZone, candidate->OnBridge) != zone)
                return false;
            ++counters.evaluations;
            return actor.CanAutoTargetObject(ThreatType(flags), mask, -1, candidate, &value, DWORD(zone), origin);
        };
        std::uint64_t houses = 0;
        for (unsigned h = 0; h < index.houses.size() && h < 64; ++h) {
            if (!index.house_counts[h]) continue;
            auto* owner = index.houses[h];
            const bool allied = actor.Owner->IsAlliedWith(owner);
            if ((!allied || repair || friendly) && (!only_enemy || owner->ArrayIndex == actor.Owner->EnemyHouseIndex) &&
                (!(flags & 0x4000) || allied))
                houses |= std::uint64_t{1} << h;
        }
        visits.clear();
        for (std::uint32_t r = 0; r < index.regions.size(); ++r) {
            const auto& region = index.regions[r];
            if (region.mask & houses)
                visits.push_back({visit_lower(region.x, region.y, region_width), r, -1});
        }
        std::make_heap(visits.begin(), visits.end(), later);
        auto* result = best;
        int score = highest;
        const int initial_score = highest;
        double nearest_distance = result ? hunt_distance_squared(*origin, result->GetCoords()) :
                                          std::numeric_limits<double>::infinity();
        std::uint64_t order = std::numeric_limits<std::uint64_t>::max();
        while (!visits.empty()) {
            std::pop_heap(visits.begin(), visits.end(), later);
            const auto visit = visits.back();
            visits.pop_back();
            if (nearest ? visit.lower > nearest_distance : bound.upper(int(visit.lower)) < score) break;
            const auto& region = index.regions[visit.region];
            if (visit.bucket < 0) {
                for (int b = 0; b < 16; ++b)
                    if (region.buckets[b].mask & houses) {
                        visits.push_back({visit_lower(region.x + b % 4 * bucket_width,
                                                      region.y + b / 4 * bucket_width, bucket_width),
                                          visit.region, b});
                        std::push_heap(visits.begin(), visits.end(), later);
                    }
                continue;
            }
            ++counters.buckets;
            for (auto id : region.buckets[visit.bucket].slots) {
                const auto& slot = index.slots[id];
                if (!(houses & (std::uint64_t{1} << slot.house))) continue;
                const auto at = slot.object->GetCoords();
                const auto candidate_upper = bound.upper(lower_distance(*origin, at.X, at.Y, 1));
                const double distance = nearest ? hunt_distance_squared(*origin, at) : 0;
                if (nearest ? distance > nearest_distance : candidate_upper < score) continue;
                int value = 0;
                if (evaluate(slot.object, value) &&
                    (nearest ? (!result || distance < nearest_distance ||
                                (distance == nearest_distance && slot.object->UniqueID < result->UniqueID)) :
                               (value > initial_score && (value > score || (value == score && slot.order < order))))) {
                    score = value;
                    result = slot.object;
                    order = slot.order;
                    nearest_distance = distance;
                }
            }
        }
        if (score == 1) ++counters.floor_ties;
        best = result;
        highest = score;
        return true;
    };
    try {
        if (run()) {
            ++counters.accelerated;
            return true;
        }
    } catch (...) {
        // The admitted evaluation path has no RNG or mutation. Allocation
        // failure is safe to retry through the original ordered traversal.
        index.failed = true;
    }
    ++counters.fallback;
    return false;
}
void read_techno_spatial_counters(TechnoSpatialCounters& out) noexcept { out = counters; }
void reset_techno_spatial_counters() noexcept { counters = {}; }
}
#endif
