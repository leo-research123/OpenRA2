// YR 0x46B280 / 0x46C840: register the projectile and update its type's delay.
#include "yrpp/BulletClass.h"
#include <algorithm>
#include <bit>
#include <new>

void BulletClass::InitScalable() {
    try {
        ScalableBullets.AddItem(this);
    } catch (const std::bad_alloc&) {
        // Failed growth leaves Count unchanged and still updates the type's delay.
        // Native vector allocation must not propagate an exception to the host.
        ScalableBullets.IsInitialized = true;
    }
    const int extra = std::max(ScalableBullets.Count - 5, 0) / 3;
    Type->ScaledSpawnDelay = std::bit_cast<int>(unsigned(Type->SpawnDelay) + unsigned(extra));
}
