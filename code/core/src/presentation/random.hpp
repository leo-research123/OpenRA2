#pragma once
class Randomizer;
namespace game {
// The presentation's own random stream. The original draws electric bolts (and lasers and radiation beams) with
// Randomizer::Global, the generator that also serves the simulation's presentation-like choices, so how often a
// picture is made changes later simulation results. The presentation draws with this stream instead; it never
// touches Randomizer::Global. It is seeded with a fixed constant, so a run draws the same jitter every time.
Randomizer& presentation_random() noexcept;
}
