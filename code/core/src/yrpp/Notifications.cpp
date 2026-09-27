// Native storage for the existing original waypoint notification registry.
// The list and dispatch ordering correspond to YR 0x00B0F5F0 / 0x007258D0.
#include "yrpp/Notifications.h"
#if !defined(RA2_YRPP_GAME)
namespace {PointerExpiredNotification waypoint_receivers;}
PointerExpiredNotification& PointerExpiredNotification::NotifyInvalidWaypoint=waypoint_receivers;
#endif
