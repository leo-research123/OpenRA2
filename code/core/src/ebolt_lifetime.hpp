#pragma once
// The lifetime of electric bolts, as the steps of one tactical round of the render phase (plan 4.7). The original
// advances them while it draws (EBolt::DrawAll, YR 0x004C2830): per bolt, in reverse order, the start follows the
// owner, the bolt is drawn, its Random counter is incremented and its Lifetime halved, and a bolt whose Lifetime
// reaches zero is deleted (the destructor clears UnitClass::ElectricBolt, which FireElectricBolt reads). The
// state belongs to the game, so it advances once per round whatever is drawn; the draw only reads it.
namespace game {
// Before the draw of the round: each bolt with an owner moves its start to the owner's firing point.
void ebolt_follow_owners();
// After the draw of the round: Random++ (for a bolt that was drawn), Lifetime >>= 1, delete at zero.
void ebolt_advance();
}
