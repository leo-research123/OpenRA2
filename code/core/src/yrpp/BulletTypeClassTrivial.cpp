// SPDX-License-Identifier: GPL-3.0-or-later
// YRpp 9402d7da trivial bodies; OpenTS 44fac744 Create_Bullet/Set_Bullet_Data.
// Copyright 2025 Electronic Arts Inc.; Copyright 2026 OpenTS contributors.
// EA Section 7 terms: third_party/opents/LICENSE.md.
#include "yrpp/BulletTypeClass.h"
#include "yrpp/BulletClass.h"

// 0x0046C870: confirmed constant return, not an unimplemented stub.
bool BulletTypeClass::SpawnAtMapCoords(CellStruct* pMapCoords,HouseClass* pOwner) { return false; }

// 0x0046C880: confirmed constant return, not an unimplemented stub.
ObjectClass* BulletTypeClass::CreateObject(HouseClass* owner) { return nullptr; }

// OpenTS Create_Bullet / Set_Bullet_Data; YR 0x46B050 creates the COM
// instance and then configures it. This is distinct from CreateObject's
// original, intentionally-null map-placement factory above.
BulletClass* YRPP_FASTCALL BulletTypeClass::CreateBullet(AbstractClass* target,TechnoClass* owner,
        int damage,WarheadTypeClass* warhead,int speed,bool bright) {
    auto* bullet=BulletClass::Create();
    if(bullet)bullet->Construct(this,target,owner,damage,warhead,speed,bright);
    return bullet;
}
