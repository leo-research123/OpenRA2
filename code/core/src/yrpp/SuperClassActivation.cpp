#include "yrpp/SuperClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/MessageListClass.h"
#include "yrpp/RulesClass.h"
#include "yrpp/StringTable.h"
#include "x87_integer.hpp"
#include <bit>

namespace {
// This entry uses the original signed timer subtraction without the extra
// nonnegative clamp in the shared timer accessor.
int activation_time_left(const CDTimerClass& timer) {
    if (timer.StartTime == -1) return timer.TimeLeft;
    const int elapsed = std::bit_cast<int>(unsigned(Unsorted::CurrentFrame) - unsigned(timer.StartTime));
    return elapsed >= timer.TimeLeft ? 0 : std::bit_cast<int>(unsigned(timer.TimeLeft) - unsigned(elapsed));
}

// Preserve the caller's x87 rounding through the arithmetic, then apply the
// original _ftol int64 conversion. SSE evaluation can change the duration by one.
int charge_from_drain(double total, double left, double ratio) {
#if defined(_MSC_VER) && defined(_M_IX86)
    const unsigned short control = 0x0E7F;
    std::uint64_t result;
    // clang-format off
    __asm fld total
    __asm fld left
    __asm fdiv ratio
    __asm fsubp st(1), st(0)
    __asm fldcw control
    __asm fistp result
        // clang-format on
        return std::bit_cast<int>(static_cast<unsigned>(result));
#elif defined(__i386__) || defined(__x86_64__)
    const unsigned short control = 0x0E7F;
    std::uint64_t result;
    __asm__ volatile("fldl %1; fldl %2; fdivl %3; fsubrp; fldcw %4; fistpq %0"
                     : "=m"(result)
                     : "m"(total), "m"(left), "m"(ratio), "m"(control)
                     : "st");
    return std::bit_cast<int>(static_cast<unsigned>(result));
#else
    return game::x87_integer(total - left / ratio);
#endif
}

int drain_from_charge(double charged, double ratio) {
#if defined(_MSC_VER) && defined(_M_IX86)
    const unsigned short control = 0x0E7F;
    std::uint64_t result;
    // clang-format off
    __asm fld charged
    __asm fmul ratio
    __asm fldcw control
    __asm fistp result
        // clang-format on
        return std::bit_cast<int>(static_cast<unsigned>(result));
#elif defined(__i386__) || defined(__x86_64__)
    const unsigned short control = 0x0E7F;
    std::uint64_t result;
    __asm__ volatile("fldl %1; fmull %2; fldcw %3; fistpq %0"
                     : "=m"(result)
                     : "m"(charged), "m"(ratio), "m"(control)
                     : "st");
    return std::bit_cast<int>(static_cast<unsigned>(result));
#else
    return game::x87_integer(charged * ratio);
#endif
}

void activation_message(const char* key, int line) {
    const auto* text = StringTable::LoadString(key, nullptr, "D:\\ra2mdpost\\Ion.cpp", line);
    const int color = HouseClass::CurrentPlayer ? HouseClass::CurrentPlayer->ColorSchemeIndex : 3;
    MessageListClass::Instance.AddMessage(nullptr, 0, text, color, static_cast<TextPrintType>(0x4046), 150, false);
}
}

char SuperClass::CanFire() const {
    if (IsSuspended) return 0;
    return Type->UseChargeDrain ? ChargeDrainState != ::ChargeDrainState::Charging : IsReady;
}

void SuperClass::SetReadiness(bool ready) { IsReady = ready; }

char SuperClass::StopPreclickAnim(bool) {
    if (Animation) {
        Animation->RemainingIterations = 0;
        Animation = nullptr;
        AnimExpirationListeners.Remove(this);
    }
    if (AnimationGotInvalid) {
        AnimExpirationListeners.Remove(this);
        AnimationGotInvalid = false;
    }
    if (!IsPresent || IsReady || (IsSuspended && !Type->PreClick)) return 0;
    CameoChargeState = -1;
    RechargeTimer.Start(GetRechargeTime());
    if (Type->UseChargeDrain) ChargeDrainState = ::ChargeDrainState::Charging;
    return 1;
}

char SuperClass::ClickFire(bool isPlayer, const CellStruct& cell) {
    if (Type->UseChargeDrain) {
        if (Owner->IsControlledByHuman()) {
            if (ChargeDrainState == ::ChargeDrainState::Draining) {
                ChargeDrainState = ::ChargeDrainState::Ready;
                RechargeTimer.Start(charge_from_drain(GetRechargeTime(), activation_time_left(RechargeTimer),
                                                      RulesClass::Instance->ChargeToDrainRatio));
            } else if (ChargeDrainState == ::ChargeDrainState::Ready) {
                ChargeDrainState = ::ChargeDrainState::Draining;
                const int charged =
                    std::bit_cast<int>(unsigned(GetRechargeTime()) - unsigned(activation_time_left(RechargeTimer)));
                RechargeTimer.Start(drain_from_charge(charged, RulesClass::Instance->ChargeToDrainRatio));
                Launch(cell, isPlayer);
            }
        } else if (!Owner->FirestormActive) {
            Launch(cell, isPlayer);
        }
        return 0;
    }
    if ((!RechargeTimer.IsTicking() || !IsPresent || !IsReady) && !Type->PostClick) return 0;
    if (Type->Type == SuperWeaponType::LightningStorm && LightningStorm::HasDeferment()) {
        if (isPlayer) LightningStorm::PrintMessage();
        return 0;
    }
    if (Type->Type == SuperWeaponType::PsychicDominator && PsyDom::Active()) {
        if (isPlayer) PsyDom::PrintMessage();
        return 0;
    }
    Launch(cell, isPlayer);
    // Launch can change the superweapon's type and flags. Read them again.
    if (!Type->PostClick) IsReady = false;
    if (IsOneTime) {
        IsOneTime = false;
        if (!IsPresent) return 0;
        IsReady = false;
        IsPresent = false;
        ShowTimers.Remove(this);
        return 1;
    }
    if (Type->ManualControl) {
        CameoChargeState = -1;
        RechargeTimer.Start(GetRechargeTime());
        if (RechargeTimer.IsTicking()) {
            RechargeTimer.TimeLeft = activation_time_left(RechargeTimer);
            RechargeTimer.StartTime = -1;
        }
    } else if (!Type->PreClick && !Type->PostClick) {
        StopPreclickAnim(isPlayer);
    }
    return 0;
}

void LightningStorm::PrintMessage() { activation_message("Msg:LightningStormActive", 1123); }
void PsyDom::PrintMessage() { activation_message("Msg:DominatorActive", 1339); }

void SuperClass::CreateChronoAnim(CoordStruct coords) {
    if (Animation) {
        Animation->RemainingIterations = 0;
        Animation = nullptr;
        AnimExpirationListeners.Remove(this);
    }
    if (AnimationGotInvalid) {
        AnimExpirationListeners.Remove(this);
        AnimationGotInvalid = false;
    }
    coords.Z += 5;
    auto* memory = YRMemory::Allocate(sizeof(AnimClass));
    if (memory) {
        Animation = ::new (memory) AnimClass(RulesClass::Instance->ChronoPlacement, coords, 0, 1, 0x600, 0, false);
        if (Animation) AnimExpirationListeners.AddItem(this);
    }
}
