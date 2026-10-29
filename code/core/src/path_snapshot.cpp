#include "path_snapshot.hpp"
#include "yrpp/AStarClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/DriveLocomotionClass.h"
#include "yrpp/MapClass.h"
#include <algorithm>
#include <array>
#include <thread>
#include <typeinfo>

namespace game {
#if !defined(RA2_YRPP_GAME) && !defined(RA2_IMAGE_GAME)
static constexpr int dx[8]{0, 1, 1, 1, 0, -1, -1, -1};
static constexpr int dy[8]{-1, -1, 0, 1, 1, 1, 0, -1};
static PathSnapshotCounters capture_counters;
void read_path_snapshot_counters(PathSnapshotCounters& out) noexcept { out = capture_counters; }

bool capture_path_snapshot(FootClass& foot, CellStruct destination, int offset, int mode, PathSnapshot& out) noexcept {
    if (offset || mode < 0 || mode > 2 || typeid(foot) != typeid(UnitClass) || foot.OnBridge || !foot.IsInPlayfield ||
        foot.Transporter || foot.IsTether || foot.IsAttackedByLocomotor || foot.ThreatAvoidanceValue() != 0)
        { ++capture_counters.profile; return false; }
    auto& unit = static_cast<UnitClass&>(foot);
    auto* drive = dynamic_cast<DriveLocomotionClass*>(static_cast<ILocomotion*>(foot.Locomotor));
    auto* type = unit.Type;
    if (!type || !drive || drive->Is_Piggybacking() || type->IsTrain || unit.FollowerCar || unit.IsFollowerCar ||
        (type->MovementZone != MovementZone::Normal && type->MovementZone != MovementZone::Crusher &&
         type->MovementZone != MovementZone::Destroyer) ||
         (type->SpeedType != SpeedType::Track && type->SpeedType != SpeedType::Wheel)) {
        ++capture_counters.profile; return false;
    }
    const auto start = CellClass::Coord2Cell(foot.GetDestination());
    const int width = std::abs(start.X - destination.X) + 5;
    const int height = std::abs(start.Y - destination.Y) + 5;
    // Longer routes retain the hierarchical finder until its input is isolated.
    if (start == destination || width > 64 || height > 64 || width * height > path_snapshot_cells) {
        ++capture_counters.extent; return false;
    }
    auto& map = MapClass::Instance;
    auto* first = map.TryGetCellAt(start);
    auto* last = map.TryGetCellAt(destination);
    if (!first || !last || !foot.IsInSameZoneAsCoords(last->GetCoords())) {
        ++capture_counters.zone; return false;
    }
    const int level = static_cast<signed char>(first->Level);
    if (first->ContainsBridge() || first->GetTunnel() || first->SlopeIndex ||
        last->ContainsBridge() || last->GetTunnel() || last->SlopeIndex ||
        static_cast<signed char>(last->Level) != level) {
        ++capture_counters.terrain; return false;
    }
    out.origin = {short(std::min(start.X, destination.X) - 2), short(std::min(start.Y, destination.Y) - 2)};
    out.start = start;
    out.destination = destination;
    out.width = width;
    out.height = height;
    out.level = level;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            auto* cell = map.TryGetCellAt(CellStruct{short(out.origin.X + x), short(out.origin.Y + y)});
            auto& cost = out.costs[y * width + x];
            cost = 0;
            if (!cell || !map.IsWithinUsableArea(cell, true)) continue;
            // Flat ordinary ground only: no worker may inspect a live bridge,
            // tunnel, ramp, linked object, locomotor or original singleton.
            if (cell->ContainsBridge() || cell->GetTunnel() || cell->SlopeIndex ||
                static_cast<signed char>(cell->Level) != level) continue;
            const auto move = foot.IsCellOccupied(cell, FacingType::None, level, nullptr, true);
            if (move == Move::OK) cost = 1000;
            else if (move == Move::MovingBlock) cost = mode == 2 ? 1000000 : 4000;
        }
    }
    out.costs[(start.Y - out.origin.Y) * width + start.X - out.origin.X] = 1000;
    if (!out.costs[(destination.Y - out.origin.Y) * width + destination.X - out.origin.X]) {
        ++capture_counters.destination; return false;
    }
    return true;
}

void solve_path_snapshot(const PathSnapshot& input, PathSnapshotResult& result) noexcept {
    result = {};
    std::fill_n(result.directions, 24, -1);
    const int count = input.width * input.height;
    if (input.width <= 0 || input.height <= 0 || count > path_snapshot_cells) return;
    const auto index = [&](CellStruct cell) {
        const int x = cell.X - input.origin.X, y = cell.Y - input.origin.Y;
        return x >= 0 && y >= 0 && x < input.width && y < input.height ? y * input.width + x : -1;
    };
    const int start = index(input.start), goal = index(input.destination);
    if (start < 0 || goal < 0 || start == goal || !input.costs[goal]) return;
    std::array<std::int64_t, path_snapshot_cells> distance;
    std::array<int, path_snapshot_cells> parent, direction, heap, position;
    distance.fill(std::int64_t{1} << 60);
    parent.fill(-1);
    position.fill(-1);
    int size = 0;
    const auto heuristic = [&](int cell) {
        return 1000 * std::max(std::abs(cell % input.width - goal % input.width),
                               std::abs(cell / input.width - goal / input.width));
    };
    const auto less = [&](int a, int b) {
        const auto fa = distance[a] + heuristic(a), fb = distance[b] + heuristic(b);
        return fa < fb || (fa == fb && a < b);
    };
    const auto swap = [&](int a, int b) {
        std::swap(heap[a], heap[b]); position[heap[a]] = a; position[heap[b]] = b;
    };
    const auto push = [&](int cell) {
        int p = position[cell];
        if (p < 0) { p = size++; heap[p] = cell; position[cell] = p; }
        while (p && less(heap[p], heap[(p - 1) / 2])) { swap(p, (p - 1) / 2); p = (p - 1) / 2; }
    };
    distance[start] = 0;
    push(start);
    while (size && result.expanded < path_snapshot_node_budget) {
        const int current = heap[0];
        swap(0, --size);
        position[current] = -2;
        int p = 0;
        while (2 * p + 1 < size) {
            int child = 2 * p + 1;
            if (child + 1 < size && less(heap[child + 1], heap[child])) ++child;
            if (!less(heap[child], heap[p])) break;
            swap(child, p); p = child;
        }
        ++result.expanded;
        if (current == goal) {
            int reverse[path_snapshot_cells];
            int length = 0;
            for (int n = goal; n != start; n = parent[n]) {
                if (n < 0 || length == path_snapshot_cells) return;
                reverse[length++] = direction[n];
            }
            result.length = std::min(length, 23);
            for (int i = 0; i < result.length; ++i) result.directions[i] = reverse[length - i - 1];
            result.found = result.length != 0;
            return;
        }
        for (int face = 0; face < 8; ++face) {
            const int x = current % input.width + dx[face], y = current / input.width + dy[face];
            if (x < 0 || y < 0 || x >= input.width || y >= input.height) continue;
            const int next = y * input.width + x;
            if (!input.costs[next] || position[next] == -2) continue;
            const auto cost = distance[current] + input.costs[next];
            if (cost >= distance[next]) continue;
            distance[next] = cost; parent[next] = current; direction[next] = face; push(next);
        }
    }
}

void solve_path_snapshot_batch(const PathSnapshot* inputs, PathSnapshotResult* results,
                               unsigned count, unsigned workers) noexcept {
    workers = std::clamp(workers, 1u, 4u);
    std::array<std::thread, 3> threads;
    const auto run = [&](unsigned lane) {
        for (unsigned i = lane; i < count; i += workers) solve_path_snapshot(inputs[i], results[i]);
    };
    for (unsigned lane = 1; lane < workers; ++lane) {
        try { threads[lane - 1] = std::thread(run, lane); }
        catch (...) { run(lane); }
    }
    run(0);
    for (auto& thread : threads) if (thread.joinable()) thread.join();
}

static FootClass* active_foot;
static const PathSnapshot* active_input;
static const PathSnapshotResult* active_result;
static std::uint64_t consumed = 0;
std::uint64_t path_snapshots_consumed() noexcept { return consumed; }
void activate_path_snapshot(FootClass* foot, const PathSnapshot* input, const PathSnapshotResult* result) noexcept {
    active_foot = foot; active_input = input; active_result = result;
}
bool consume_path_snapshot(FootClass& foot, CellStruct start, CellStruct end, int* directions,
                           PathFinderData*& result) noexcept {
    if (active_foot != &foot || !active_result || !active_result->found ||
        start != active_input->start || end != active_input->destination) return false;
    auto at = start;
    auto& map = MapClass::Instance;
    // Main-thread revalidation catches occupation changed by earlier results.
    for (int i = 0; i < active_result->length; ++i) {
        const int face = active_result->directions[i];
        const CellStruct next{short(at.X + dx[face]), short(at.Y + dy[face])};
        auto* cell = map.TryGetCellAt(next);
        if (!cell || cell->ContainsBridge() || cell->GetTunnel() || cell->SlopeIndex ||
            static_cast<signed char>(cell->Level) != active_input->level) return false;
        const auto move = foot.IsCellOccupied(cell, FacingType(face), active_input->level, map.TryGetCellAt(at), true);
        if (move != Move::OK && move != Move::MovingBlock) return false;
        at = next;
    }
    static int levels[24];
    static PathFinderData path;
    std::copy_n(active_result->directions, 24, directions);
    std::fill_n(levels, 24, active_input->level);
    path = {};
    path.StartCell = start;
    path.TotalDistance = active_result->length;
    path.PathLength = active_result->length + 1;
    path.Directions = directions;
    path.Levels = levels;
    result = &path;
    ++consumed;
    return true;
}
#endif
}
