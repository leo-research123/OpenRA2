#include "yrpp/MixFileClass.h"
#include "filesystem/bootstrap_services.hpp"

// Original 0x5301A0 control flow. Services own the objects and globals; this
// entry never resets lists or mounts languages. Preserve partial failure state.
bool YRPP_CDECL MixFileClass::Bootstrap() {
    auto session = game::make_bootstrap_session();
    const auto previous_disk = session.disk();
    session.set_disk(-2);
    unsigned completed = 0;
    auto before = [&](const char* name) {
        if (session.observer.before) session.observer.before(session.observer.context, name, completed);
    };
    auto after = [&](const char* name) {
        ++completed;
        if (session.observer.after) session.observer.after(session.observer.context, name, completed);
    };
    for (int i = 99; i >= 0; --i) {
        char name[] = "EXPANDMD00.MIX";
        name[8] = char('0' + i / 10);
        name[9] = char('0' + i % 10);
        before(name);
        if (session.raw_exists(name)) {
            if (auto* mix = session.create_mix(name))
                // Original ignores an expansion-array append failure; the
                // constructed MIX stays registered in the global chain.
                session.append_expansion(mix);
        }
        // Allocation failure for an expansion is non-fatal in 5301A0.
        after(name);
    }
    struct GenericStep { const char* name; game::GenericMixSlot slot; bool cache; };
    constexpr GenericStep steps[] = {
        {"RA2MD.MIX", game::GenericMixSlot::ra2md, false},
        {"RA2.MIX", game::GenericMixSlot::ra2, false},
        {"CACHEMD.MIX", game::GenericMixSlot::cachemd, true},
        {"CACHE.MIX", game::GenericMixSlot::cache, true},
        {"LOCALMD.MIX", game::GenericMixSlot::localmd, false},
        {"LOCAL.MIX", game::GenericMixSlot::local, false}
    };
    for (const auto& step : steps) {
        before(step.name);
        auto* mix = session.create_mix(step.name);
        session.set_generic(step.slot, mix); // also write null on allocation failure
        if (!mix || (step.cache && !session.cache(step.name)))
            // Original returns early, retains prior mounts and does NOT restore
            // the disk on these failure exits. Do not "fix" that with RAII.
            return false;
        after(step.name);
    }
    session.set_disk(previous_disk);
    return true;
}
