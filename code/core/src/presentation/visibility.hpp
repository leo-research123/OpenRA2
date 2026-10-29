#pragma once
class ObjectClass;
namespace game {
// The objects the last tactical traversal found inside the view. This is the presentation's own
// record: ObjectClass::IsVisible (+0x99) is only read by the original DrawObjects,
// so the field stays in the layout but is no longer written. The traversal sets an
// object's entry before it decides on it; the spatial index and the second traversal pass read it.
class VisibleObjects {
public:
    void set(const ObjectClass*, bool visible);
    bool contains(const ObjectClass*) const noexcept;
    // An object that is destroyed leaves the set, so a reused address starts invisible.
    void forget(const ObjectClass*) noexcept;
    void clear() noexcept;
    unsigned size() const noexcept;
};
VisibleObjects& visible_objects() noexcept;

// The animations the drawing of a building keeps out of the ordinary animation pass. The original does this
// with AnimClass::Invisible, which only the drawing reads: DrawIt hides the slot animations of a building that is
// being sold (BState 0, Mission::Selling), and the voxel barrel animation is hidden once its building drew it.
// Nothing sets Invisible back except the barrel's temporary toggle, so an entry stays until the animation is
// destroyed (a replaced slot animation is a new object and starts visible). The field itself is no longer written.
class HiddenAnims {
public:
    void hide(const ObjectClass*);
    bool contains(const ObjectClass*) const noexcept;
    // An animation that is destroyed leaves the set, so a reused address starts visible.
    void forget(const ObjectClass*) noexcept;
    void clear() noexcept;
    unsigned size() const noexcept;
};
HiddenAnims& hidden_anims() noexcept;

// Redraw requests of the drawing itself. The only one is a building's door: while the door of an unloading factory is
// at its closed frame the original draws it and writes NeedsRedraw = true, so the next frame redraws the building. The
// drawing no longer writes game state; it registers the object here and the presenter turns the request into the
// invalidation notifications at the start of the next frame (the spatial index and the host's presentation revision).
class RedrawRequests {
public:
    void request(const ObjectClass*);
    // An object that is destroyed drops its request.
    void forget(const ObjectClass*) noexcept;
    // Applies and clears the pending requests (the notifications MarkForRedraw sends).
    void apply() noexcept;
    unsigned size() const noexcept;
};
RedrawRequests& redraw_requests() noexcept;
}

class UnitClass;
namespace game {
unsigned short unit_waterline(const UnitClass&);
void set_unit_waterline(const UnitClass&, unsigned short);
void forget_unit_waterline(const ObjectClass*) noexcept;
}
