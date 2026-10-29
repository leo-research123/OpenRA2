#pragma once

namespace game {
// RA2_VERIFY_DRAW_PURE: a drawing pass must not change game state. While one tactical Render runs, the
// scope keeps shallow raw-byte snapshots of the live game objects, the map cells, the houses, the
// scenario (including its random generator), Randomizer::Global and the layer lists, and reports every
// word that differs afterwards. No field is excluded as render-owned any more
// (ObjectClass::NeedsRedraw and AnimClass::Invisible were, until phases 4.3 and 4.4); the Aircraft FrozenStill / height temporary write is not, so a failure to restore it shows up. Snapshots are shallow by decision: heap data that objects point to
// (vector buffers, locomotor objects, type voxel caches) is not inspected.
//
// The check is off unless the environment variable is set; when off, a scope costs one flag test.
// Reports go to stderr as DRAW_PURE lines; a cumulative DRAW_PURE_TOTAL list is printed at exit.
class DrawPurityScope {
public:
    DrawPurityScope() noexcept;
    ~DrawPurityScope();
    DrawPurityScope(const DrawPurityScope&) = delete;
    DrawPurityScope& operator=(const DrawPurityScope&) = delete;

private:
    bool active_ = false;
};
}
