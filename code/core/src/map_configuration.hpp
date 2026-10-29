#pragma once
class RulesClass;
class SessionClass;
namespace game {
class MapWorld;
// Native campaign startup counterpart of the Rules-to-Session copy at
// 0x0052D1FB..0x0052D207. Explicit multiplayer options retain their priority.
void initialize_campaign_superweapon_option(const RulesClass&, SessionClass&) noexcept;
// Binds the rules services of a native world around operation: art shapes,
// color schemes built from the world's unit palette, deferred sound fields,
// and the registries RulesClass::Init (0x668BF0) clears. Exceptions from
// operation propagate after the previous services are restored.
bool with_map_rules_runtime(MapWorld&, void (*operation)(void*), void* argument);
}
