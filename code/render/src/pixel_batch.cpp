#include "pixel_batch.hpp"
#include <limits>
#include <algorithm>
#include <stdexcept>

namespace render {
namespace {
constexpr int PixelBatchKind = 13;
constexpr int PlainWarpFlag = 0x40000;
// Keeps pixel - destination inside signed 32-bit shader arithmetic.
constexpr std::int32_t DestinationLimit = std::int32_t(1) << 30;
constexpr std::size_t NoRun = std::numeric_limits<std::size_t>::max();
// Bins of the destination pixels that can pass draw_pixel's source, clip and
// target tests; right/bottom are exclusive. Plain integer code stays cheap in
// unoptimized builds, where this runs for every packet of every frame.
struct BinRect {
    int left = 0, top = 0, right = 0, bottom = 0;
};
bool visible_bins(const PixelParameters& p, BinRect& bins) {
    std::int64_t left = p[4], top = p[5], right = std::int64_t(p[4]) + p[2], bottom = std::int64_t(p[5]) + p[3];
    if (left < p[6]) left = p[6];
    if (top < p[7]) top = p[7];
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right > std::int64_t(p[6]) + p[8]) right = std::int64_t(p[6]) + p[8];
    if (bottom > std::int64_t(p[7]) + p[9]) bottom = std::int64_t(p[7]) + p[9];
    if (right > p[0]) right = p[0];
    if (bottom > p[1]) bottom = p[1];
    if (left >= right || top >= bottom) return false;
    bins = {int(left) / PixelBinSize, int(top) / PixelBinSize, int(right - 1) / PixelBinSize + 1,
            int(bottom - 1) / PixelBinSize + 1};
    return true;
}
}
bool pixel_local_packet(const PixelParameters& p, int width, int height) {
    // Kinds 0..9 except laser pairs, and the light-gated copy (14), touch only
    // their own pixel. Plain warp reads neighboring pixels and stays serial.
    const int kind = p[19];
    return ((kind >= 0 && kind <= 9) || kind == 14) && (p[13] & PlainWarpFlag) == 0 && p[0] == width &&
           p[1] == height && p[2] >= 0 && p[3] >= 0 && p[4] > -DestinationLimit && p[4] < DestinationLimit &&
           p[5] > -DestinationLimit && p[5] < DestinationLimit;
}
PixelBatch append_pixel_run(std::vector<PixelParameters>& words, std::size_t first, std::size_t end,
                            std::vector<std::int32_t>& schedule, PixelRunScratch& scratch) {
    if (first >= end || end > words.size()) throw std::invalid_argument("pixel run range");
    const int width = words[first][0], height = words[first][1];
    BinRect area{width, height, 0, 0};
    scratch.bounds.resize(end - first);
    for (auto i = first; i < end; ++i) {
        BinRect b;
        visible_bins(words[i], b);
        scratch.bounds[i - first] = {b.left, b.top, b.right, b.bottom};
        if (b.left >= b.right) continue;
        area.left = std::min(area.left, b.left);
        area.top = std::min(area.top, b.top);
        area.right = std::max(area.right, b.right);
        area.bottom = std::max(area.bottom, b.bottom);
    }
    PixelBatch batch{first, end, std::int32_t(words.size())};
    PixelParameters header{width, height};
    header[19] = PixelBatchKind;
    if (area.left < area.right) {
        const int columns = area.right - area.left, rows = area.bottom - area.top;
        const auto table = schedule.size(), tiles = std::size_t(columns) * rows;
        schedule.resize(table + tiles * 2, 0);
        for (const auto& b : scratch.bounds)
            for (int y = b[1]; y < b[3]; ++y)
                for (int x = b[0]; x < b[2]; ++x)
                    ++schedule[table + ((y - area.top) * columns + x - area.left) * 2 + 1];
        scratch.cursors.resize(tiles);
        std::size_t next = schedule.size();
        for (std::size_t i = 0; i < tiles; ++i) {
            schedule[table + i * 2] = scratch.cursors[i] = std::int32_t(next);
            next += std::size_t(schedule[table + i * 2 + 1]);
            if (next > std::size_t(std::numeric_limits<std::int32_t>::max())) throw std::length_error("pixel run bins");
        }
        schedule.resize(next);
        for (auto i = first; i < end; ++i) {
            const auto& b = scratch.bounds[i - first];
            for (int y = b[1]; y < b[3]; ++y)
                for (int x = b[0]; x < b[2]; ++x)
                    schedule[scratch.cursors[(y - area.top) * columns + x - area.left]++] = std::int32_t(i);
        }
        header[2] = columns;
        header[3] = rows;
        header[4] = std::int32_t(table);
        header[5] = area.left;
        header[6] = area.top;
        batch.columns = columns;
        batch.rows = rows;
    }
    words.push_back(header);
    return batch;
}

void append_pixel_batches(std::vector<PixelParameters>& words, const std::vector<std::uint8_t>& eligible, int width,
                          int height, const PixelBatchOptions& options, std::vector<PixelBatch>& batches,
                          std::vector<std::int32_t>& schedule) {
    if (width <= 0 || height <= 0 || width > 8192 || height > 8192) throw std::invalid_argument("pixel batch viewport");
    if (eligible.size() > words.size()) throw std::invalid_argument("pixel batch eligibility");
    struct Run {
        std::size_t first, end;
        BinRect bins;
        bool visible;
    };
    std::vector<Run> runs;
    // Bin rectangle of each member; invisible members keep an empty rectangle.
    std::vector<BinRect> member_bins(eligible.size());
    const auto close = [&](std::size_t first, std::size_t end) {
        // A single packet keeps its own dispatch without bins.
        if (end - first < 2u) return;
        Run run{first, end, {std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), 0, 0}, false};
        for (auto n = first; n < end; ++n) {
            const auto& b = member_bins[n];
            if (b.left >= b.right) continue;
            run.visible = true;
            if (b.left < run.bins.left) run.bins.left = b.left;
            if (b.top < run.bins.top) run.bins.top = b.top;
            if (b.right > run.bins.right) run.bins.right = b.right;
            if (b.bottom > run.bins.bottom) run.bins.bottom = b.bottom;
        }
        runs.push_back(run);
    };
    std::size_t first = NoRun;
    for (std::size_t i = 0; i < eligible.size(); ++i) {
        const auto& p = words[i];
        const bool member = eligible[i] && pixel_local_packet(p, width, height);
        // Different row origins would map one pixel to different Z/ABuffer
        // entries, which other invocations may own.
        if (first != NoRun && (!member || p[16] != words[first][16] || i == options.split)) {
            close(first, i);
            first = NoRun;
        }
        if (!member) continue;
        if (first == NoRun) first = i;
        if (!visible_bins(p, member_bins[i])) member_bins[i] = {};
    }
    if (first != NoRun) close(first, eligible.size());
    // Resource words cover every packet, including the headers appended below.
    const auto total = words.size() + runs.size();
    if (total > std::size_t(std::numeric_limits<std::int32_t>::max() / 20))
        throw std::length_error("pixel batch parameters");
    schedule.assign(total * 2, 0);
    std::vector<std::int32_t> cursors;
    for (const auto& run : runs) {
        PixelParameters header{width, height};
        header[19] = PixelBatchKind;
        PixelBatch batch{run.first, run.end, std::int32_t(words.size())};
        if (run.visible) {
            const int columns = run.bins.right - run.bins.left, rows = run.bins.bottom - run.bins.top;
            const auto table = schedule.size();
            const auto tiles = std::size_t(columns) * rows;
            std::size_t next = table + tiles * 2;
            if (next > std::size_t(std::numeric_limits<std::int32_t>::max())) throw std::length_error("pixel bins");
            schedule.resize(next, 0);
            auto* entries = schedule.data() + table;
            for (auto n = run.first; n < run.end; ++n) {
                const auto& b = member_bins[n];
                for (int y = b.top; y < b.bottom; ++y)
                    for (int x = b.left; x < b.right; ++x)
                        ++entries[((y - run.bins.top) * columns + (x - run.bins.left)) * 2 + 1];
            }
            cursors.resize(tiles);
            for (std::size_t tile = 0; tile < tiles; ++tile) {
                entries[tile * 2] = std::int32_t(next);
                cursors[tile] = std::int32_t(next);
                next += std::size_t(entries[tile * 2 + 1]);
                if (next > std::size_t(std::numeric_limits<std::int32_t>::max()))
                    throw std::length_error("pixel bin references");
            }
            schedule.resize(next);
            auto* members = schedule.data();
            auto* cursor = cursors.data();
            // Ascending packet indices keep every pixel's original write order.
            for (auto n = run.first; n < run.end; ++n) {
                const auto& b = member_bins[n];
                for (int y = b.top; y < b.bottom; ++y)
                    for (int x = b.left; x < b.right; ++x)
                        members[cursor[(y - run.bins.top) * columns + (x - run.bins.left)]++] = std::int32_t(n);
            }
            header[2] = columns;
            header[3] = rows;
            header[4] = std::int32_t(table);
            header[5] = run.bins.left;
            header[6] = run.bins.top;
            batch.columns = columns;
            batch.rows = rows;
        }
        batches.push_back(batch);
        words.push_back(header);
    }
}
}
