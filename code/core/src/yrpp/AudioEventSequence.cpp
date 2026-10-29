#include "yrpp/VocClass.h"
#include <algorithm>

void AudioEvent::PrepareSamples(int (&indices)[32], RandomRange random, void* context) noexcept {
    if (SampleCount && type->MinDelay < 33) return;
    std::fill_n(indices, 32, 0);
    SampleCount = 0;
    if (type->MinDelay < 33) {
        if (type->Attack) indices[SampleCount++] = random(context, 0, type->Attack - 1);
        if (unsigned(type->Control) & 4) {
            for (int i = type->Attack; i < type->NumSamples - type->Decay; ++i)
                indices[SampleCount++] = i;
        } else
            indices[SampleCount++] = unsigned(type->Control) & 2
                                         ? random(context, type->Attack, type->NumSamples - type->Decay - 1)
                                         : type->Attack;
        if (type->Decay) indices[SampleCount++] = random(context, type->NumSamples - type->Decay, type->NumSamples - 1);
    } else {
        const int tail = type->Decay && (unsigned(type->Control) & 0x40)
                             ? random(context, type->NumSamples - type->Decay, type->NumSamples - 1)
                             : -1;
        int selected = -1;
        NextDelayedSample(random, context, selected);
        if (selected >= 0) indices[SampleCount++] = selected;
        if (tail >= 0 && (selected >= 0 || !(Flags & 0x10))) {
            indices[SampleCount++] = tail;
            if (selected < 0) Flags |= 0x10;
        }
    }
}
bool AudioEvent::FirstSample(RandomRange random, void* context, int& output) noexcept {
    Remaining = SampleCount;
    if (!SampleCount || (Flags & 0x20)) return false;
    if (type->MinDelay >= 33) {
        output = 0;
        return true;
    }
    int start = 0;
    if (unsigned(type->Control) & 0x20) {
        start = 1;
        --Remaining;
        CurrentSample = 0;
    } else
        CurrentSample = -1;
    if (unsigned(type->Control) & 0x40) --Remaining;
    if (Remaining <= 0) {
        type->SamplesOK = false;
        return false;
    }
    for (int i = 0; i < Remaining; ++i)
        Order[i] = start + i;
    if (!(Flags & 8) && (unsigned(type->Control) & 0x20)) {
        output = 0;
        return true;
    }
    return NextSample(random, context, output);
}
bool AudioEvent::NextSample(RandomRange random, void* context, int& output) noexcept {
    if (type->MinDelay >= 33 && !(Flags & 0x20)) return false;
    if (Remaining && !(Flags & 0x20)) {
        if (unsigned(type->Control) & 2) {
            const int pick = random(context, 0, Remaining - 1);
            CurrentSample = Order[pick];
            for (int i = pick; i < Remaining - 1; ++i)
                Order[i] = Order[i + 1];
        } else
            ++CurrentSample;
        --Remaining;
        output = CurrentSample;
        return true;
    }
    if ((unsigned(type->Control) & 1) && !(Flags & 0x20) && (!type->Loop || Cycles < type->Loop - 1)) {
        ++Cycles;
        return FirstSample(random, context, output);
    }
    if (!(unsigned(type->Control) & 0x40) || (Flags & 0x10) || SampleCount < 1) return false;
    Flags |= 0x10;
    output = SampleCount - 1;
    return true;
}
bool AudioEvent::NextDelayedSample(RandomRange random, void* context, int& output) noexcept {
    if (DelayedCount == DelayedCursor) {
        if (!(unsigned(type->Control) & 1) && DelayedPasses == 1) return false;
        if ((unsigned(type->Control) & 1) && type->Loop && DelayedPasses >= type->Loop) return false;
        int size = 0;
        if (type->Attack && !(Flags & 8)) DelayedOrder[size++] = random(context, 0, type->Attack - 1);
        if (unsigned(type->Control) & 4) {
            for (int i = type->Attack; i < type->NumSamples - type->Decay; ++i)
                DelayedOrder[size++] = i;
        } else {
            int selected = type->Attack;
            if (unsigned(type->Control) & 2) {
                if (!DelayedPasses) DelayedChoice = random(context, type->Attack, type->NumSamples - type->Decay - 1);
                selected = DelayedChoice;
            }
            DelayedOrder[size++] = selected;
        }
        DelayedCount = size;
        DelayedCursor = 0;
        ++DelayedPasses;
    }
    if (!(unsigned(type->Control) & 2) || (type->Attack && !(Flags & 8) && DelayedCursor <= 0)) {
        output = DelayedOrder[DelayedCursor++];
        return true;
    }
    const int low = type->Attack && !(Flags & 8) ? 1 : 0;
    const int pick = random(context, low, DelayedCount - 1);
    const int result = DelayedOrder[pick];
    for (int i = pick; i < DelayedCount - 1; ++i)
        DelayedOrder[i] = DelayedOrder[i + 1];
    --DelayedCount;
    output = result;
    return true;
}
