// Original charge state: 0x6CB560/0x6CBCA0/0x6CBEE0/0x6CC260/0x6CE0B0.
#include "yrpp/SuperClass.h"
#include "yrpp/HouseClass.h"
#include "yrpp/AnimClass.h"
#include "yrpp/VoxClass.h"
#include "yrpp/VocClass.h"
#include "yrpp/StringTable.h"
#include "RulesClassReaders.hpp"
#include "x87_integer.hpp"

namespace {
void clear_animation(SuperClass& super) {
    if (super.Animation) {
        super.Animation->RemainingIterations = 0;
        super.Animation = nullptr;
        SuperClass::AnimExpirationListeners.Remove(&super);
    }
    if (super.AnimationGotInvalid) {
        SuperClass::AnimExpirationListeners.Remove(&super);
        super.AnimationGotInvalid = false;
    }
}
void announce_ready(const SuperClass& super) {
    constexpr const char* names[] = {"EVA_NuclearMissileReady",
                                     "EVA_IronCurtainReady",
                                     "EVA_LightningStormReady",
                                     "EVA_ChronosphereReady",
                                     nullptr,
                                     "EVA_ReinforcementsReady",
                                     "EVA_ReinforcementsReady",
                                     "EVA_PsychicDominatorReady",
                                     "EVA_SpyPlaneReady",
                                     "EVA_GeneticMutatorReady",
                                     "EVA_ForceShieldReady",
                                     "EVA_PsychicRevealReady"};
    const int type = int(super.Type->Type);
    if (type >= 0 && type < 12 && names[type]) VoxClass::Play(names[type]);
}
}
int SuperClass::GetRechargeTime() const { return CustomChargeTime == -1 ? Type->RechargeTime : CustomChargeTime; }
const wchar_t* SuperClass::NameReadiness() const {
    const char* key = nullptr;
    int line = 0;
    if (IsSuspended) {
        key = "TXT_HOLD";
        line = 950;
    } else if (Type->UseChargeDrain) {
        switch (ChargeDrainState) {
            case ::ChargeDrainState::Charging:
                key = "TXT_CHARGING";
                line = 919;
                break;
            case ::ChargeDrainState::Ready:
                key = "TXT_READY";
                line = 922;
                break;
            case ::ChargeDrainState::Draining:
                key = "TXT_FIRESTORM_ON";
                line = 925;
                break;
            default:
                break;
        }
    } else if (IsReady) {
        key = "TXT_READY";
        line = 944;
    }
    return key ? StringTable::LoadString(key, nullptr, "D:\\ra2mdpost\\Super.CPP", line) : nullptr;
}
bool SuperClass::ShouldDrawProgress() const { return Type->UseChargeDrain || !IsReady; }
bool SuperClass::ShouldFlashTab() const {
    if (IsSuspended || (Type->UseChargeDrain ? ChargeDrainState == ::ChargeDrainState::Charging : !IsReady))
        return false;
    const int duration = Type->FlashSidebarTabFrames;
    return duration == -1 ||
           (duration && std::bit_cast<int>(unsigned(duration) + unsigned(ReadyFrame)) > Unsorted::CurrentFrame);
}
void SuperClass::SetCharge(int percentage) {
    if (!IsPresent || unsigned(percentage) > 100) return;
    const int total = GetRechargeTime();
    const int remaining = std::bit_cast<int>(
        unsigned(total) - unsigned(game::x87_integer(double(percentage) * double(0.01f) * double(total))));
    if (!remaining) IsReady = true;
    RechargeTimer.Start(remaining);
}
void SuperClass::SetRechargeTime(int time) { CustomChargeTime = time; }
void SuperClass::Reset() {
    clear_animation(*this);
    IsReady = false;
    CameoChargeState = -1;
    const bool ticking = RechargeTimer.IsTicking();
    RechargeTimer.Start(GetRechargeTime());
    if (!ticking) RechargeTimer.Pause();
}
bool SuperClass::Grant(bool oneTime, bool announce, bool onHold) {
    if (IsPresent) return false;
    IsPresent = true;
    IsOneTime = oneTime;
    BlinkState = false;
    unused_3C = DWORD(reinterpret_cast<std::uintptr_t>(Type->UIName));
    if (!oneTime && IsSuspended && CanHold) {
        if (Type->ManualControl)
            RechargeTimer.Pause();
        else
            RechargeTimer.Resume();
        IsSuspended = false;
    }
    if (Type->ShowTimer && !Owner->Type->MultiplayPassive) ShowTimers.AddItem(this);
    bool changed = false;
    if (Type->ManualControl) {
        CameoChargeState = -1;
        RechargeTimer.Start(GetRechargeTime());
        RechargeTimer.Pause();
    } else {
        clear_animation(*this);
        if (!IsReady && (!IsSuspended || Type->PreClick)) {
            CameoChargeState = -1;
            RechargeTimer.Resume();
            RechargeTimer.Start(GetRechargeTime());
            if (Type->UseChargeDrain) ChargeDrainState = ::ChargeDrainState::Charging;
            changed = true;
        }
    }
    if (onHold) SetOnHold(true);
    if (IsOneTime) {
        IsReady = true;
        RechargeTimer.Start(0);
        if (announce) announce_ready(*this);
        ReadyFrame = Unsorted::CurrentFrame;
        if (Type->UseChargeDrain) ChargeDrainState = ::ChargeDrainState::Ready;
    }
    return changed;
}
int SuperClass::AnimStage() const {
    if (!IsPresent) return 0;
    if (!Type->UseChargeDrain && IsReady) return 54;
    const int duration = GetRechargeTime(), left = RechargeTimer.GetTimeLeft();
    double progress;
    if (Type->UseChargeDrain && ChargeDrainState == ::ChargeDrainState::Draining) {
        const double total = double(duration) * RulesClass::Instance->ChargeToDrainRatio;
        progress = 1.0 - (total - double(left)) / total;
    } else
        progress = double(std::bit_cast<int>(unsigned(duration) - unsigned(left))) / double(duration);
    const int stage = rule_integer(progress * 54.0);
    return stage >= 53 ? 53 : stage;
}
bool SuperClass::HasChargeProgressed(bool isPlayer) {
    if (SpecialSoundDuration > 0) --SpecialSoundDuration;
    if (!SpecialSoundDuration) {
        SpecialSoundDuration = -1;
        VocClass::PlayAt(Type->SpecialSound, SpecialSoundLocation);
    }
    if (Unsorted::CurrentSWType != 4 && Animation) {
        Animation->Invisible = true;
        Animation->StartSoundAudioController.Stop();
    }
    if (!IsPresent || (IsReady && !Type->UseChargeDrain) || IsSuspended) return false;
    if (!RechargeTimer.IsTicking()) {
        if (CameoChargeState == -1) return false;
        CameoChargeState = -1;
        return true;
    }
    if (RechargeTimer.GetTimeLeft()) {
        const int stage = AnimStage();
        if (stage == CameoChargeState) return false;
        CameoChargeState = stage;
        return true;
    }
    if (Type->UseChargeDrain) {
        if (ChargeDrainState == ::ChargeDrainState::Draining) {
            ChargeDrainState = ::ChargeDrainState::Charging;
            RechargeTimer.Start(GetRechargeTime());
        } else {
            ChargeDrainState = ::ChargeDrainState::Ready;
            IsReady = true;
        }
    } else {
        IsReady = true;
        if (isPlayer) announce_ready(*this);
        ReadyFrame = Unsorted::CurrentFrame;
    }
    return true;
}
