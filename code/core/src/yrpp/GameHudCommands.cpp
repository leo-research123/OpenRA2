// Original advanced command bar commands, 0x00730F30 / 0x00731A30 / 0x00731AF0.
#include "yrpp/Unsorted.h"
#include "yrpp/TechnoClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/DisplayClass.h"
#include "yrpp/EventClass.h"
#include "yrpp/StringTable.h"
#include <bit>
#include <new>

#if !defined(RA2_YRPP_GAME)
namespace {
int last_cheer_frame = -100;
}
int& Game::LastCheerFrame = last_cheer_frame;
void Game::BeginAttackMove() noexcept {
    const int count = ObjectClass::CurrentObjects.Count;
    if (!count) ShowMessage(StringTable::LoadString("MSG:NothingSelected"), 240);
    for (int i = 0; i < count; ++i) {
        auto* object = ObjectClass::CurrentObjects[i];
        if (object && (object->AbstractFlags & AbstractFlags::Techno) != AbstractFlags::None &&
            !static_cast<TechnoClass*>(object)->CanAttackOnTheMove()) {
            ShowMessage(StringTable::LoadString("MSG:AttackMoveUnsupported"), 240);
            return;
        }
    }
    AttackMoveMode = true;
}
void Game::ToggleBeacon() noexcept {
    if (!Unsorted::MuteSWLaunches) DisplayClass::Instance.SetBeaconMode(-1);
}
void Game::CheerSelectedHouse() noexcept {
    if (Unsorted::MuteSWLaunches) return;
    const int next = std::bit_cast<int>(unsigned(LastCheerFrame) + unsigned(RulesClass::Instance->MaximumCheerRate));
    if (next >= Unsorted::CurrentFrame) return;
    EventClass event;
    ::new (&event) EventClass(HouseClass::CurrentPlayer->ArrayIndex, EventType::AllCheer);
    EventClass::OutList.Add(event);
    // The original advances its cooldown even when the outgoing queue is full.
    LastCheerFrame = Unsorted::CurrentFrame;
}
#endif
