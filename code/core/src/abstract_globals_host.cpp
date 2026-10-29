#include "yrpp/AbstractTypeClass.h"
#include "yrpp/SwizzleManagerClass.h"
#include "yrpp/AirstrikeClass.h"
#include "yrpp/DiskLaserClass.h"
#include "yrpp/EMPulseClass.h"
#include "yrpp/FoggedObjectClass.h"
#include "yrpp/TemporalClass.h"
// WaveClass.h relies on its includer for these.
#include "yrpp/CellClass.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/WaveClass.h"

namespace game {
SwizzleManagerClass swizzler;
DynamicVectorClass<AbstractClass*> abstract_objects;
IndexClass<int, AbstractClass*> abstract_targets;
DynamicVectorClass<AbstractTypeClass*> abstract_types;
DynamicVectorClass<AbstractClass*> type_expiration_listeners;
DynamicVectorClass<AbstractClass*> trigger_expiration_listeners;
DynamicVectorClass<AbstractClass*> trigger_instance_expiration_listeners;
DynamicVectorClass<AbstractClass*> tag_expiration_listeners;
DynamicVectorClass<AbstractClass*> script_expiration_listeners;
DynamicVectorClass<AbstractClass*> pending_deletes;
DynamicVectorClass<AbstractClass*> animation_expiration_listeners;
// Registries of original classes that native code does not construct yet.
// They stay empty; scenario teardown still visits them in the original order.
DynamicVectorClass<WaveClass*> waves;
DynamicVectorClass<EMPulseClass*> empulses;
DynamicVectorClass<DiskLaserClass*> disk_lasers;
DynamicVectorClass<TemporalClass*> temporals;
DynamicVectorClass<AirstrikeClass*> airstrikes;
DynamicVectorClass<FoggedObjectClass*> fogged_objects;
}

SwizzleManagerClass& SwizzleManagerClass::Instance = game::swizzler;

DynamicVectorClass<AbstractClass*>& AbstractClass::Array = game::abstract_objects;
IndexClass<int, AbstractClass*>& AbstractClass::TargetIndex = game::abstract_targets;
DynamicVectorClass<AbstractTypeClass*>& AbstractTypeClass::Array = game::abstract_types;
DynamicVectorClass<AbstractClass*>& AbstractClass::TypeExpirationListeners = game::type_expiration_listeners;
DynamicVectorClass<AbstractClass*>& AbstractClass::TriggerExpirationListeners = game::trigger_expiration_listeners;
DynamicVectorClass<AbstractClass*>& AbstractClass::TriggerInstanceExpirationListeners =
    game::trigger_instance_expiration_listeners;
DynamicVectorClass<AbstractClass*>& AbstractClass::ScriptExpirationListeners = game::script_expiration_listeners;
DynamicVectorClass<AbstractClass*>& AbstractClass::TagExpirationListeners = game::tag_expiration_listeners;
DynamicVectorClass<AbstractClass*>& AbstractClass::PendingDeletes = game::pending_deletes;
DynamicVectorClass<AbstractClass*>& AbstractClass::AnimExpirationListeners = game::animation_expiration_listeners;
DynamicVectorClass<WaveClass*>& WaveClass::Array = game::waves;
DynamicVectorClass<EMPulseClass*>& EMPulseClass::Array = game::empulses;
DynamicVectorClass<DiskLaserClass*>& DiskLaserClass::Array = game::disk_lasers;
DynamicVectorClass<TemporalClass*>& TemporalClass::Array = game::temporals;
DynamicVectorClass<AirstrikeClass*>& AirstrikeClass::Array = game::airstrikes;
DynamicVectorClass<FoggedObjectClass*>& FoggedObjectClass::Array = game::fogged_objects;
