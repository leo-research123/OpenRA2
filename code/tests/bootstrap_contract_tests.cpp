#include "support/test_support.hpp"
#include "filesystem/bootstrap_services.hpp"
#include "support/yrpp_abi32.hpp"
#include <array>
#include <iostream>
#include <list>
#include <stdexcept>
#include <string>
#include <vector>

using namespace game;
namespace {

struct Object { std::string name; };
struct Host {
    std::list<Object> objects{{"LANGMD.MIX"}, {"LANGUAGE.MIX"}, {"PREEXISTING.MIX"}};
    std::array<Object*,32> slots{};
    std::vector<Object*> registered, expansions;
    std::vector<std::string> probes, constructed, cached;
    std::vector<int> disk_changes;
    std::string fail_allocate, fail_cache;
    bool fail_append = false;
    int selected_disk = 7;
    Host() {
        for (auto& object : objects) registered.push_back(&object);
        slots[3] = registered[0]; slots[2] = registered[1];
        slots[4] = registered[2];
    }
    int disk() const { return selected_disk; }
    void set_disk(int value) { selected_disk = value; disk_changes.push_back(value); }
    bool raw_exists(const char* name) {
        EXPECT_TRUE((selected_disk == -2)) << "disk selection must precede RawFile probe";
        probes.emplace_back(name);
        return probes.back() == "EXPANDMD99.MIX" || probes.back() == "EXPANDMD01.MIX";
    }
    Object* create_mix(const char* name) {
        constructed.emplace_back(name);
        if (fail_allocate == name) return nullptr;
        auto* result = &objects.emplace_back(Object{name});
        registered.push_back(result);
        return result;
    }
    void append_expansion(Object* object) { if (!fail_append) expansions.push_back(object); }
    void set_generic(GenericMixSlot slot, Object* object) { slots[unsigned(slot)] = object; }
    bool cache(const char* name) { cached.emplace_back(name); return fail_cache != name; }
};

// MIX pointers are opaque to Bootstrap: these test objects are only passed
// through the entry and converted back by this provider, never dereferenced as MIXes.
int disk(void* context) { return static_cast<Host*>(context)->disk(); }
void set_disk(void* context, int value) { static_cast<Host*>(context)->set_disk(value); }
bool raw_exists(void* context, const char* name) { return static_cast<Host*>(context)->raw_exists(name); }
MixFileClass* create_mix(void* context, const char* name) {
    return reinterpret_cast<MixFileClass*>(static_cast<Host*>(context)->create_mix(name));
}
void append_expansion(void* context, MixFileClass* mix) {
    static_cast<Host*>(context)->append_expansion(reinterpret_cast<Object*>(mix));
}
void set_generic(void* context, GenericMixSlot slot, MixFileClass* mix) {
    static_cast<Host*>(context)->set_generic(slot, reinterpret_cast<Object*>(mix));
}
bool cache(void* context, const char* name) { return static_cast<Host*>(context)->cache(name); }
constinit const BootstrapServices services{
    disk, set_disk, raw_exists, create_mix, append_expansion, set_generic, cache
};
const BootstrapSession* active_session = nullptr;
struct SessionScope {
    const BootstrapSession* previous = active_session;
    explicit SessionScope(const BootstrapSession& session) { active_session = &session; }
    ~SessionScope() { active_session = previous; }
};
BootstrapResult run_bootstrap(Host& host, BootstrapObserver observer = {}) {
    const BootstrapSession session{services, &host, observer};
    SessionScope scope(session);
    return MixFileClass::Bootstrap() ? BootstrapResult::complete : BootstrapResult::failed;
}
void preserved_state_and_order() {
    Host host;
    const auto before = host.registered;
    EXPECT_TRUE((run_bootstrap(host) == BootstrapResult::complete)) << "normal result";
    EXPECT_TRUE((host.probes.size() == 100 && host.probes.front() == "EXPANDMD99.MIX" &&
          host.probes.back() == "EXPANDMD00.MIX")) << "descending full probe interval";
    EXPECT_TRUE((host.constructed == std::vector<std::string>{"EXPANDMD99.MIX","EXPANDMD01.MIX",
        "RA2MD.MIX","RA2.MIX","CACHEMD.MIX","CACHE.MIX","LOCALMD.MIX","LOCAL.MIX"})) << "construction order";
    EXPECT_TRUE((host.disk_changes == std::vector<int>{-2,7})) << "successful disk restoration";
    for (size_t i=0;i<before.size();++i) EXPECT_TRUE((host.registered[i] == before[i])) << "pre-existing registration was changed";
    EXPECT_TRUE((host.slots[3] == before[0] && host.slots[2] == before[1] && host.slots[4] == before[2])) << "unrelated slots overwritten";
    EXPECT_TRUE((host.slots[0]->name == "RA2MD.MIX" && host.slots[23]->name == "LOCAL.MIX")) << "generic slot identity";
    EXPECT_TRUE((host.cached == std::vector<std::string>{"CACHEMD.MIX","CACHE.MIX"})) << "cache names/order";
    const auto first_ra2md = host.slots[0];
    EXPECT_TRUE((run_bootstrap(host) == BootstrapResult::complete)) << "second entry result";
    EXPECT_TRUE((host.slots[0] != first_ra2md && host.registered[0] == before[0] && host.expansions.size() == 4)) << "entry must append, not clear or remount languages, even on repeated calls";
}
void original_failure_exits() {
    Host expansion_failure;
    expansion_failure.fail_allocate = "EXPANDMD99.MIX";
    EXPECT_TRUE((run_bootstrap(expansion_failure) == BootstrapResult::complete && expansion_failure.expansions.size() == 1)) << "expansion allocation failure is non-fatal";
    Host array_failure;
    array_failure.fail_append = true;
    EXPECT_TRUE((run_bootstrap(array_failure) == BootstrapResult::complete && array_failure.expansions.empty() &&
        array_failure.registered.size() == 11)) << "array failure must not unmount successful expansion";
    constexpr const char* names[] = {"RA2MD.MIX","RA2.MIX","CACHEMD.MIX","CACHE.MIX","LOCALMD.MIX","LOCAL.MIX"};
    constexpr unsigned slots[] = {0,1,20,21,22,23};
    for (unsigned i=0;i<6;++i) {
        Host host;
        for (unsigned slot : slots) host.slots[slot] = host.registered.front();
        host.fail_allocate = names[i];
        EXPECT_TRUE((run_bootstrap(host) == BootstrapResult::failed)) << "generic allocation failure must propagate";
        EXPECT_TRUE((host.slots[slots[i]] == nullptr)) << "failed generic allocation must write null";
        EXPECT_TRUE((host.constructed.back() == names[i])) << "no calls after failed generic";
        EXPECT_TRUE((host.disk_changes == std::vector<int>{-2})) << "original failure must retain disk state -2";
        for (unsigned j=i+1;j<6;++j) EXPECT_TRUE((host.slots[slots[j]] == host.registered.front())) << "later slots changed on failure";
    }
    for (const char* name : {"CACHEMD.MIX", "CACHE.MIX"}) {
        Host host;
        host.fail_cache = name;
        EXPECT_TRUE((run_bootstrap(host) == BootstrapResult::failed && host.constructed.back() == name &&
            host.selected_disk == -2)) << "cache failure must retain allocated slot and prior mounts";
    }
}
void observer_boundary() {
    Host host;
    struct Context { unsigned before=0, after=0; } c;
    BootstrapObserver observer{&c,
        [](void* data,const char*,unsigned) noexcept { ++static_cast<Context*>(data)->before; },
        [](void* data,const char*,unsigned count) noexcept { static_cast<Context*>(data)->after=count; }};
    EXPECT_TRUE((run_bootstrap(host,observer) == BootstrapResult::complete && c.before == 106 && c.after == 106)) << "passive observer must see the complete original flow";
    EXPECT_TRUE((host.disk_changes == std::vector<int>{-2,7})) << "observer changed original successful disk restoration";
}
void nested_sessions() {
    Host outer, inner;
    inner.selected_disk = 11;
    struct Context { Host& inner; bool entered = false, completed = false; unsigned after = 0; } context{inner};
    BootstrapObserver observer{&context,
        [](void* data, const char*, unsigned) noexcept {
            auto& c = *static_cast<Context*>(data);
            if (!c.entered) {
                c.entered = true;
                try { c.completed = run_bootstrap(c.inner) == BootstrapResult::complete; }
                catch (...) { c.completed = false; }
            }
        },
        [](void* data, const char*, unsigned count) noexcept { static_cast<Context*>(data)->after = count; }};
    EXPECT_TRUE((run_bootstrap(outer, observer) == BootstrapResult::complete)) << "outer entry result";
    EXPECT_TRUE((context.completed)) << "nested entry result";
    EXPECT_TRUE((outer.probes.size() == 100 && inner.probes.size() == 100 && context.after == 106)) << "nested entry replaced the outer service context or observer";
    EXPECT_TRUE((outer.disk_changes == std::vector<int>{-2,7} && inner.disk_changes == std::vector<int>{-2,11})) << "nested entry mixed provider state";
    EXPECT_TRUE((active_session == nullptr)) << "test provider scope leaked";
}
}

namespace game {
BootstrapSession make_bootstrap_session() {
    EXPECT_TRUE((active_session != nullptr)) << "entry called without a test provider";
    return *active_session;
}
}


TEST(BootstrapContract, Contracts) {
    preserved_state_and_order(); original_failure_exits(); observer_boundary(); nested_sessions();
}
