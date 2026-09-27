#pragma once
class BulletClass;
namespace game {
// Diagnostic metadata only; no original object layout or gameplay state changes.
// These hooks never throw. Called synchronously by original Bullet methods.
void projectile_log_created(const BulletClass&) noexcept;
void projectile_log_event(const BulletClass&,const char* event) noexcept;
void projectile_log_contact(const BulletClass&,int x,int y,int z,int floor,int slope,
                            bool fell,bool rose,bool obstacle) noexcept;
}
