#include "yrpp/AbstractTypeClass.h"
#include "yrpp/SwizzleManagerClass.h"

namespace game {
SwizzleManagerClass swizzler;
DynamicVectorClass<AbstractClass*> abstract_objects;
IndexClass<int, AbstractClass*> abstract_targets;
DynamicVectorClass<AbstractTypeClass*> abstract_types;
DynamicVectorClass<AbstractClass*> type_expiration_listeners;
DynamicVectorClass<AbstractClass*> trigger_expiration_listeners;
DynamicVectorClass<AbstractClass*> trigger_instance_expiration_listeners;
DynamicVectorClass<AbstractClass*> tag_expiration_listeners;
DynamicVectorClass<AbstractClass*> pending_deletes;
}

SwizzleManagerClass& SwizzleManagerClass::Instance = game::swizzler;

DynamicVectorClass<AbstractClass*>& AbstractClass::Array = game::abstract_objects;
IndexClass<int, AbstractClass*>& AbstractClass::TargetIndex = game::abstract_targets;
DynamicVectorClass<AbstractTypeClass*>& AbstractTypeClass::Array = game::abstract_types;
DynamicVectorClass<AbstractClass*>& AbstractClass::TypeExpirationListeners = game::type_expiration_listeners;
DynamicVectorClass<AbstractClass*>& AbstractClass::TriggerExpirationListeners = game::trigger_expiration_listeners;
DynamicVectorClass<AbstractClass*>& AbstractClass::TriggerInstanceExpirationListeners = game::trigger_instance_expiration_listeners;
DynamicVectorClass<AbstractClass*>& AbstractClass::TagExpirationListeners = game::tag_expiration_listeners;
DynamicVectorClass<AbstractClass*>& AbstractClass::PendingDeletes = game::pending_deletes;
