#pragma once
class TacticalClass;
namespace game {
struct PresentationContext;
void build_rubber_band(PresentationContext&, const TacticalClass&);
void build_rally_points(PresentationContext&, const TacticalClass&, bool foreground) noexcept;
}
