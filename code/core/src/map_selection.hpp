#pragma once
class ScenarioClass;
namespace game {
// Resource view of MSChoice's numbered sections. Ownership never crosses a host ABI.
class MapChoices;
bool open_map_choices(const ScenarioClass&, MapChoices*&);
void close_map_choices(MapChoices*) noexcept;
int find_map_choice(const MapChoices&, const char* filename);
int map_choice_count(const MapChoices&) noexcept;
const char* map_choice_filename(const MapChoices&, int index) noexcept;
int map_choice_link_count(const MapChoices&, int index) noexcept;
int map_choice_link(const MapChoices&, int index, int link) noexcept;
}
