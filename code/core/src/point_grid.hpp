#pragma once
#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace game {
// Private point index. Identity and exact coordinates stay in dense slots;
// crossing a bucket boundary is the only operation that changes membership.
class PointGrid {
    struct Point {
        int x = 0, y = 0;
        std::uint32_t offset = 0;
        bool live = false;
    };
    std::vector<Point> points;
    std::unordered_map<std::uint64_t, std::vector<std::uint32_t>> buckets;
    int shift;
    static int cell(int value, int shift) noexcept {
        const auto divisor = std::int64_t{1} << shift;
        return int(value >= 0 ? value / divisor : (std::int64_t(value) - divisor + 1) / divisor);
    }
    std::uint64_t key(int x, int y) const noexcept {
        return (std::uint64_t(std::uint32_t(cell(x, shift))) << 32) | std::uint32_t(cell(y, shift));
    }

public:
    explicit PointGrid(int bucket_shift = 10) : shift(bucket_shift) {}
    void clear() {
        points.clear();
        buckets.clear();
    }
    void erase(std::uint32_t id) {
        if (id >= points.size() || !points[id].live) return;
        auto& point = points[id];
        auto found = buckets.find(key(point.x, point.y));
        auto& bucket = found->second;
        const auto moved = bucket.back();
        bucket[point.offset] = moved;
        points[moved].offset = point.offset;
        bucket.pop_back();
        if (bucket.empty()) buckets.erase(found);
        point.live = false;
    }
    void set(std::uint32_t id, int x, int y) {
        if (id >= points.size()) points.resize(std::size_t(id) + 1);
        auto& point = points[id];
        if (point.live && point.x == x && point.y == y) return;
        const auto next = key(x, y);
        if (point.live && key(point.x, point.y) == next) {
            point.x = x;
            point.y = y;
            return;
        }
        erase(id);
        auto& bucket = buckets[next];
        bucket.push_back(id);
        point = {x, y, std::uint32_t(bucket.size() - 1), true};
    }
    // Half-open coordinates are widened so inclusive INT_MAX can be queried.
    void query(std::int64_t left, std::int64_t top, std::int64_t right, std::int64_t bottom,
               std::vector<std::uint32_t>& output) const {
        output.clear();
        if (left >= right || top >= bottom) return;
        const auto emit = [&](const auto& bucket) {
            // Write the hits in place instead of appending one by one: a query visits thousands of points.
            const auto first = output.size();
            output.resize(first + bucket.size());
            auto* hit = output.data() + first;
            std::size_t hits = 0;
            for (auto id : bucket) {
                const auto& p = points[id];
                if (p.x >= left && p.x < right && p.y >= top && p.y < bottom) hit[hits++] = id;
            }
            output.resize(first + hits);
        };
        const auto first_x = cell(int(std::clamp(left, std::int64_t(INT32_MIN), std::int64_t(INT32_MAX))), shift);
        const auto first_y = cell(int(std::clamp(top, std::int64_t(INT32_MIN), std::int64_t(INT32_MAX))), shift);
        const auto last_x = cell(int(std::clamp(right - 1, std::int64_t(INT32_MIN), std::int64_t(INT32_MAX))), shift);
        const auto last_y = cell(int(std::clamp(bottom - 1, std::int64_t(INT32_MIN), std::int64_t(INT32_MAX))), shift);
        if ((std::int64_t(last_x) - first_x + 1) * (std::int64_t(last_y) - first_y + 1) >
            std::int64_t(buckets.size()) * 4) {
            for (const auto& [key, bucket] : buckets)
                emit(bucket);
        } else
            for (auto y = std::int64_t(first_y); y <= last_y; ++y)
                for (auto x = std::int64_t(first_x); x <= last_x; ++x) {
                    const auto found = buckets.find((std::uint64_t(std::uint32_t(x)) << 32) | std::uint32_t(y));
                    if (found != buckets.end()) emit(found->second);
                }
    }
};
}
