// SPDX-License-Identifier: GPL-3.0-or-later
// OpenTS 44fac744 techno.cpp Player_Assign_Mission / Player_Assign_Event;
// Electronic Arts / OpenTS, EA Section 7 terms: third_party/opents/LICENSE.md.
// YR 0x006FFBE0 / 0x006FFE00 add original planning-event submission/rejection.
#include "yrpp/TechnoClass.h"
#include "yrpp/EventClass.h"
#include "yrpp/CellClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/PlanningTokenClass.h"
#include "yrpp/Unsorted.h"
#include <new>
#if !defined(RA2_YRPP_GAME)
bool TechnoClass::ClickedMission(Mission mission,AbstractClass* target,AbstractClass* destination,CellClass* follow) {
    const bool attack_move=Game::IsAttackMoveMode() && CanAttackOnTheMove();
    if((mission==Mission::Move || mission==Mission::Attack) && attack_move)mission=Mission::AttackMove;
    if(PlanningNodeClass::PlanningModeActive) {
        // Preserve the original right-to-left target construction order.
        const TargetClass next(follow),where(destination),what(target),source(this);
        EventClass event;
        ::new(&event) EventClass(HouseClass::CurrentPlayer->ArrayIndex,source,mission,what,where,next);
        Game::PlanningManager_Submit(EventClass(event));
        return false;
    }
    if(Unsorted::MoveFeedback)switch(mission) {
        case Mission::Harvest:VoiceHarvest();break;
        case Mission::Attack:VoiceAttack(target);break;
        case Mission::Move:case Mission::AttackMove:VoiceMove();break;
        case Mission::Enter:VoiceEnter();break;
        case Mission::Capture:VoiceCapture();break;
        case Mission::Unload:VoiceDeploy();break;
        default:
            if(GetTechnoType()->VoiceSpecialAttack.Count>0) {
                const auto random=static_cast<unsigned>(Randomizer::Global.Random());
                auto& voices=GetTechnoType()->VoiceSpecialAttack;
                QueueVoice(voices[int(random%static_cast<unsigned>(voices.Count))]);
            }
            break;
    }
    const TargetClass where(destination),what(target),source(this);
    auto no_follow=source;no_follow.m_RTTI=0;
    EventClass event;
    ::new(&event) EventClass(HouseClass::CurrentPlayer->ArrayIndex,source,mission,what,where,no_follow);
    // Normal 0x00646E90 does not replace the constructor's Frame afterwards.
    EventClass::OutList.Add(event);
    return true;
}
bool TechnoClass::ClickedEvent(EventType type) {
    const TargetClass target(this);
    EventClass event;
    ::new(&event) EventClass(HouseClass::CurrentPlayer->ArrayIndex,type,target.m_ID,int(target.m_RTTI));
    if(PlanningNodeClass::PlanningModeActive) {
        Game::PlanningManager_RejectEvent(&event);
        return false;
    }
    EventClass::OutList.Add(event);
    return true;
}
#endif
