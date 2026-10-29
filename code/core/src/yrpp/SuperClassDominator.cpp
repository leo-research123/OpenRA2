// Original psychic dominator and nuclear lighting phases.
#include "yrpp/SuperClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/IonBlastClass.h"
#include "yrpp/MapClass.h"
#include "yrpp/ScenarioClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/CellSpread.h"
#include "yrpp/FileFormats/SHP.h"
#include "x87_double.hpp"
#include <algorithm>
#include <bit>

namespace {
AnimClass* animation(AnimTypeClass* type, const CoordStruct& at) {
    auto* memory = YRMemory::Allocate(sizeof(AnimClass));
    return memory ? ::new (memory) AnimClass(type, at, 0, 1, 0x600, 0, false) : nullptr;
}
// 0x0053AF40 compares without spilling either x87 intermediate to double.
// The native non-x87 simulation uses the game's 53-bit, toward-zero mode.
bool fire_frame_reached(int frame, int frames, int percentage) noexcept {
#if defined(_MSC_VER) && defined(_M_IX86)
    const double factor = 0.01;
    unsigned short status;
    // clang-format off
    __asm fild frame
    __asm fidiv frames
    __asm fild percentage
    __asm fmul factor
    __asm fcompp
    __asm fnstsw status
            // clang-format on
            return (status & 0x0400u) == 0 &&
        (status & 0x4100u) != 0;
#elif defined(__i386__) || defined(__x86_64__)
    const double factor = 0.01;
    unsigned short status;
    __asm__ volatile("fildl %1; fidivl %2; fildl %3; fmull %4; fcompp; fnstsw %0"
                     : "=m"(status)
                     : "m"(frame), "m"(frames), "m"(percentage), "m"(factor)
                     : "st");
    return (status & 0x0400u) == 0 && (status & 0x4100u) != 0;
#else
    return game::x87_multiply(double(percentage), 0.01) <= game::x87_divide(double(frame), double(frames));
#endif
}
int scale(int value) { return std::bit_cast<int>(unsigned(value) * 1000u) / 100; }
}
void NukeFlash::FadeIn() {
    auto& scenario = *ScenarioClass::Instance;
    Status = NukeFlashStatus::FadeIn;
    Duration = 30;
    StartTime = Unsorted::CurrentFrame;
    scenario.AmbientTimer.Start(1);
    scenario.AmbientTarget = scenario.NukeAmbient;
    const auto tint = scenario.NukeLighting.Tint;
    ScenarioClass::RecalcLighting(scale(tint.Red), scale(tint.Green), scale(tint.Blue), true);
    MapClass::Instance.MarkNeedsRedraw(1);
}
void NukeFlash::FadeOut() {
    Status = NukeFlashStatus::FadeOut;
    Duration = 15;
    StartTime = Unsorted::CurrentFrame;
    ScenarioClass::UpdateLighting();
    MapClass::Instance.MarkNeedsRedraw(1);
}
void PsyDom::Start(HouseClass* owner, CellStruct coords) {
    auto& rules = *RulesClass::Instance;
    if (!rules.DominatorFirstAnim || !rules.DominatorSecondAnim) return;
    Owner = owner;
    Coords = coords;
    auto at = MapClass::Instance.GetCellAt(Coords)->GetCoords();
    at.Z += 750;
    Anim = animation(rules.DominatorFirstAnim, at);
    Status = PsychicDominatorStatus::FirstAnim;
    ScenarioClass::Instance->AmbientTimer.Start(1);
    ScenarioClass::UpdateLighting();
}
void PsyDom::Update() {
    switch (Status) {
        case PsychicDominatorStatus::FirstAnim:
            Status = PsychicDominatorStatus::Fire;
            break;
        case PsychicDominatorStatus::Fire:
            if (Anim && Anim->Type->GetImage() &&
                fire_frame_reached(Anim->Animation.Value, Anim->Type->GetImage()->Frames,
                                   RulesClass::Instance->DominatorFireAtPercentage)) {
                Status = PsychicDominatorStatus::SecondAnim;
                Fire();
            }
            break;
        case PsychicDominatorStatus::SecondAnim:
            if (Anim && Anim->Type->GetImage() && Anim->Type->GetImage()->Frames - Anim->Animation.Value <= 10)
                Status = PsychicDominatorStatus::Reset;
            break;
        case PsychicDominatorStatus::Reset:
            if (Anim && Anim->Type->GetImage() && Anim->Type->GetImage()->Frames - Anim->Animation.Value <= 1) {
                Status = PsychicDominatorStatus::Over;
                Coords = {-1, -1};
                Anim = nullptr;
                ScenarioClass::UpdateLighting();
            }
            break;
        case PsychicDominatorStatus::Over:
            if (ScenarioClass::Instance->AmbientTarget == ScenarioClass::Instance->AmbientCurrent)
                Status = PsychicDominatorStatus::Inactive;
            break;
        default:
            break;
    }
}
void PsyDom::Fire() {
    const auto at = MapClass::Instance.GetCellAt(Coords)->GetCoords();
    if (auto* memory = YRMemory::Allocate(sizeof(IonBlastClass))) {
        auto* blast = ::new (memory) IonBlastClass(at);
        blast->DisableIonBeam = true;
    }
    Anim = animation(RulesClass::Instance->DominatorSecondAnim, at);
    // The EXE passes the storm owner here, not the dominator capture owner.
    MapClass::DamageArea(at, RulesClass::Instance->DominatorDamage, nullptr, RulesClass::Instance->DominatorWarhead,
                         true, LightningStorm::Owner);
    const unsigned spread = unsigned(std::min(10, RulesClass::Instance->DominatorCaptureRange));
    DynamicVectorClass<TechnoClass*> captured;
    for (unsigned i = 0; i < CellSpread::NumCells(spread); ++i) {
        const auto offset = CellSpread::GetCell(i);
        auto* cell = MapClass::Instance.GetCellAt(CellStruct{short(Coords.X + offset.X), short(Coords.Y + offset.Y)});
        for (auto* object = cell->FindTechnoNearestTo(Point2D{}, false); object;) {
            if (object->CanBePermaMindControlled()) {
                if (object->MindControlledBy) object->MindControlledBy->CaptureManager->FreeUnit(object);
                object->SetOwningHouse(Owner, true);
                object->MindControlledByAUnit = true;
                auto visual = object->GetCoords();
                visual.Z += object->GetTechnoType()->MindControlRingOffset;
                object->MindControlRingAnim = animation(RulesClass::Instance->PermaControlledAnimationType, visual);
                if (object->MindControlRingAnim) object->MindControlRingAnim->SetOwnerObject(object);
                captured.AddItem(object);
            }
            auto* next = object->NextObject;
            object = next && (unsigned(next->AbstractFlags) & 1u) ? static_cast<TechnoClass*>(next) : nullptr;
        }
    }
    if (!Owner->IsControlledByCurrentPlayer())
        for (int i = captured.Count - 1; i >= 0; --i)
            captured[i]->QueueMission(Mission::Hunt, false);
}
