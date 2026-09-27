// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 waypoint.cpp, with YR 0x00763810 / 0x00763E20 lifecycle.
// Electronic Arts / OpenTS; EA Section 7: third_party/opents/LICENSE.md.
#include "yrpp/WaypointPathClass.h"
#include "yrpp/Notifications.h"
#include "yrpp/CRC.h"
#include <cstring>
#include <new>
#if !defined(RA2_YRPP_GAME)
namespace {DynamicVectorClass<WaypointPathClass*> paths;}
DynamicVectorClass<WaypointPathClass*>& WaypointPathClass::Array=paths;
#endif
WaypointPathClass::WaypointPathClass(int index) noexcept:AbstractClass(),CurrentWaypointIndex(-1),Waypoints() {
    if(static_cast<unsigned>(index)<12u) {
        try {Array.AddItem(this);}
        catch(const std::bad_alloc&) {
            // The original Vector allocation returns null, restores this flag,
            // then Add fails without registering the otherwise valid new path.
            Array.IsInitialized=true;
        }
    }
}
WaypointPathClass::~WaypointPathClass() {
#if defined(RA2_YRPP_GAME)
    AbstractClass::AnnounceExpiredPointer(this,true);
#else
    auto& receivers=PointerExpiredNotification::NotifyInvalidWaypoint.Array;
    for(int i=0;i<receivers.Count;++i)receivers[i]->PointerExpired(this,true);
#endif
    Array.Remove(this);
}
HRESULT YRPP_STDCALL WaypointPathClass::GetClassID(CLSID* id) {
    if(!id)return static_cast<HRESULT>(0x80004003u);
    constexpr DWORD words[]{0xF73125BAu,0x11D21054u,0x60007281u,0xB55B0508u};
    std::memcpy(id,words,sizeof(words));return 0;
}
void WaypointPathClass::ComputeCRC(CRCEngine& crc) const {
    AbstractClass::ComputeCRC(crc);
    crc(CurrentWaypointIndex);crc(Waypoints.Count);
}
