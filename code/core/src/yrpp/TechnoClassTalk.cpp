// Global speech-bubble state and reveal handoff, YR 0x70F120.
#include "yrpp/TechnoClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/RulesClass.h"
namespace {
TechnoClass* speaker = nullptr;
int bubble = 0;
SysTimerClass timer;
}
TechnoClass*& TechnoClass::TalkBubbleOwner = speaker;
int& TechnoClass::TalkBubbleType = bubble;
SysTimerClass& TechnoClass::TalkBubbleTimer = timer;
DWORD YRPP_FASTCALL TechnoClass::SetTalkBubble(TechnoClass* object, int value) {
    if (object && value) {
        TalkBubbleOwner = object;
        TalkBubbleType = value;
        TalkBubbleTimer.Start(static_cast<int>(RulesClass::Instance->TalkBubbleTime));
        auto at = object->GetCoords();
        MapClass::Instance.RevealArea1(&at, 2, HouseClass::CurrentPlayer, 0, 0, 0, 1, 1);
    } else {
        TalkBubbleOwner = nullptr;
        TalkBubbleType = 0;
        TalkBubbleTimer.Start(0);
    }
    return static_cast<DWORD>(TalkBubbleTimer.StartTime);
}
