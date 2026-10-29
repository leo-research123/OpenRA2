#pragma once
#include "bootstrap_stage.hpp"

class MixFileClass;

namespace game {
// Object-external services for the shared original control flow. Exactly one
// provider is linked per target; no service pointer is added to a YRpp object.
struct BootstrapServices {
    int (*disk)(void*);
    void (*set_disk)(void*, int);
    bool (*raw_exists)(void*, const char*);
    MixFileClass* (*create_mix)(void*, const char*);
    void (*append_expansion)(void*, MixFileClass*);
    void (*set_generic)(void*, GenericMixSlot, MixFileClass*);
    bool (*cache)(void*, const char*);
};

// The context and observer belong to this invocation, including nested calls.
struct BootstrapSession {
    const BootstrapServices& services;
    void* context;
    BootstrapObserver observer;

    int disk() const { return services.disk(context); }
    void set_disk(int value) { services.set_disk(context, value); }
    bool raw_exists(const char* name) { return services.raw_exists(context, name); }
    MixFileClass* create_mix(const char* name) { return services.create_mix(context, name); }
    void append_expansion(MixFileClass* mix) { services.append_expansion(context, mix); }
    void set_generic(GenericMixSlot slot, MixFileClass* mix) { services.set_generic(context, slot, mix); }
    bool cache(const char* name) { return services.cache(context, name); }
};

// Link-selected provider. The standalone provider requires an active file
// context; the original provider needs neither that context nor an observer.
BootstrapSession make_bootstrap_session();
}
