#include "yrpp/Powerups.h"

namespace {
// Fixed YR data tables 7E523C / 81DA8C / 81DAD8; remaining tables are BSS.
const char* const effects[19] = {"Money", "Unit", "HealBase", "Cloak", "Explosion",
    "Napalm", "Squad", "Darkness", "Reveal", "Armor", "Speed", "Firepower", "ICBM",
    "Invulnerability", "Veteran", "IonStorm", "Gas", "Tiberium", "Pod"};
int weights[19] = {50, 20, 1, 3, 5, 5, 20, 1, 1, 10, 10, 10, 1, 3, 1, 1, 1, 1, 1};
int anims[19] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
double arguments[19]{};
bool naval[19]{};
}
const char* const (&Powerups::Effects)[19] = effects;
int (&Powerups::Weights)[19] = weights;
int (&Powerups::Anims)[19] = anims;
double (&Powerups::Arguments)[19] = arguments;
bool (&Powerups::Naval)[19] = naval;
