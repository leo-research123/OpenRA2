// Compile-only original x86 MS ABI checks. Native timer state stays outside.
#include "yrpp/CCToolTip.h"
#include "yrpp/MouseClass.h"
#include "yrpp/UnitClass.h"
#include "yrpp/InfantryTypeClass.h"
#include "yrpp/TacticalClass.h"
#include <cstddef>
static_assert(sizeof(ToolTip)==0x1C);
static_assert(sizeof(ToolTipManagerData)==0x210);
static_assert(sizeof(ToolTipManager)==0x260);
static_assert(sizeof(CCToolTip)==0x268);
static_assert(offsetof(ToolTipManager,CurrentToolTip)==0x4);
static_assert(offsetof(ToolTipManager,CurrentMousePosition)==0x10);
static_assert(offsetof(ToolTipManager,CurrentToolTipData)==0x18);
static_assert(offsetof(ToolTipManager,ToolTipDelay)==0x228);
static_assert(offsetof(ToolTipManager,ToolTips)==0x234);
static_assert(offsetof(ToolTipManager,ToolTipIndex)==0x24C);
static_assert(offsetof(CCToolTip,FullRedraw)==0x260);
static_assert(offsetof(CCToolTip,Delay)==0x264);
static_assert(offsetof(TechnoTypeClass,HasTurretTooltips)==0x806);
static_assert(offsetof(InfantryTypeClass,UseOwnName)==0xECA);
static_assert(offsetof(UnitClass,ToolTipText)==0x6E8);
static_assert(offsetof(TechnoClass,CurrentTurretNumber)==0x124);
static_assert(offsetof(TechnoClass,CurrentWeaponNumber)==0x138);
static_assert(offsetof(ScrollClass,unknown_byte_554A)==0x555A);
static_assert(offsetof(TacticalClass,field_D8)==0xD8);
