#pragma once
#include "yrpp/GeneralDefinitions.h"
class FootClass; class TechnoClass; class TechnoTypeClass; class HouseClass;
namespace game {
// Internal reconstruction helpers, not original executable entry points.
// The explicit appearance type avoids temporarily replacing the object's Type
// during presentation; Foot helpers retain its inherited locomotor adjustments.
int foot_z_adjustment(const FootClass&, const TechnoTypeClass&);
VisualType foot_visual_character(const FootClass&, VARIANT_BOOL, HouseClass*, const TechnoTypeClass&);
int techno_z_adjustment(const TechnoClass&, const TechnoTypeClass&);
VisualType techno_visual_character(const TechnoClass&, VARIANT_BOOL, HouseClass*, const TechnoTypeClass&);
}
