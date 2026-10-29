#pragma once
// The aircraft shadow-height cycle (plan section 3). The original AircraftClass::DrawIt (YR 0x004144B0) lowers
// an aircraft to the ground height (or the bridge height) around its shadow draw, so that the locomotor's
// Shadow_Matrix and the Z adjustment are computed for the ground, and then raises it again. The two SetHeight
// calls re-insert the aircraft into its cell, which is a game-state effect: a parked aircraft is discovered for
// the current player, its owner's power/radar recheck and discovery flags are set, the cell's object list is
// reordered and the occupation bits are marked.
//
// The cycle is therefore a step of the render phase, performed for the aircraft the original draw reaches, in
// draw order, whether or not a picture is made. The presenter does not touch the aircraft's height: it reads the
// shadow inputs the step sampled while the aircraft was at the ground height.
#include "yrpp/BasicStructures.h"
#include "yrpp/Drawing.h"
#include "yrpp/Matrix3D.h"
class AircraftClass;
namespace game {
struct TacticalWorldBounds;
struct AircraftShadowSample {
    const AircraftClass* aircraft = nullptr;
    Matrix3D local;      // Locomotor->Shadow_Matrix at the ground height
    VoxelIndexKey key;   // the key Shadow_Matrix returned
    int depth = 0;       // GetZAdjustment() at the ground height
    bool raised = false; // the aircraft is on a bridge: the shadow lies at the bridge height
    bool has_shadow = false;
    Point2D shadow_delta{};  // Locomotor->Shadow_Point() at the ground height
};
struct AircraftCycleEnvironment {
    bool window_active = false, debug_map = false, fog_of_war = false, memory_fog = false;
    int level_height = 104;
};
// The environment the traversal of this round runs in (view window, debug map, fog of war).
AircraftCycleEnvironment aircraft_cycle_environment() noexcept;
// True when the original draw of this round reaches the shadow cycle of the aircraft: the object traversal
// accepts it (inside the region, not hidden by memory fog, on screen), DrawIfVisible accepts it and
// AircraftClass::DrawIt gets past the anchor, the voxel and the cell checks. `frame` receives the walked frame
// the anchor computed. The presenter and the step use this one function.
bool aircraft_reaches_shadow_cycle(const AircraftClass&, const AircraftCycleEnvironment&, const TacticalWorldBounds& region,
                                   int& frame);
// Performs the cycle for every aircraft of the layers in traversal order and records the samples.
void run_aircraft_shadow_cycle();
const AircraftShadowSample* aircraft_shadow_sample(const AircraftClass*) noexcept;
}
