#include "bootstrap_services.hpp"
#include "bootstrap_stage.hpp"
#include "filesystem/resource_globals.hpp"
#include "filesystem/resource_environment.hpp"
#include "filesystem/resource_context.hpp"
#include "yrpp/RawFileClass.h"
#include <new>
#include <stdexcept>

namespace game {
namespace {
// Stateless service adapter for the shared original control flow.
struct BootstrapHost {
    ResourceEnvironment& files;
    int disk() const { return game::disk_selection; }
    void set_disk(int value) { game::disk_selection = value; }
    bool raw_exists(const char* name) {
        ResourceScope scope({&files, nullptr, {}});
        RawFileClass file(name);
        return file.Exists();
    }
    MixFileClass* create_mix(const char* name) {
        try { return &files.mount(name); }
        catch (const std::bad_alloc&) { return nullptr; }
    }
    void append_expansion(MixFileClass* mix) { MixFileClass::Array.AddItem(mix); }
    void set_generic(GenericMixSlot slot, MixFileClass* mix) {
        game::generic_mix(slot) = mix;
    }
    bool cache(const char* name) { return MixFileClass::Cache(name, nullptr); }
};
BootstrapHost host(void* context) {
    return {*static_cast<decltype(ResourceContext::files)>(context)};
}
int disk(void* context) { return host(context).disk(); }
void set_disk(void* context, int value) { host(context).set_disk(value); }
bool raw_exists(void* context, const char* name) { return host(context).raw_exists(name); }
MixFileClass* create_mix(void* context, const char* name) { return host(context).create_mix(name); }
void append_expansion(void* context, MixFileClass* mix) { host(context).append_expansion(mix); }
void set_generic(void* context, GenericMixSlot slot, MixFileClass* mix) {
    host(context).set_generic(slot, mix);
}
bool cache(void* context, const char* name) { return host(context).cache(name); }
// Named function addresses remain constant expressions on MSVC as well.
constinit const BootstrapServices services{
    disk, set_disk, raw_exists, create_mix, append_expansion, set_generic, cache
};
}

BootstrapSession make_bootstrap_session() {
    const auto& context = current_context();
    if (!context.files) throw std::logic_error("MIX bootstrap requires host file services");
    return {services, context.files, context.observer};
}
}
