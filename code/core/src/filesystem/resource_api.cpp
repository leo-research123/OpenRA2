#include "api/filesystem.hpp"
#include "filesystem/resource_context.hpp"
#include "filesystem/resource_environment.hpp"
#include "filesystem/resource_startup.hpp"
#include "yrpp/StringTable.h"
#include "yrpp/CCFileClass.h"
#include <exception>

namespace game {
class ResourceHandle {
public:
    explicit ResourceHandle(std::string_view directory) : files(path_from_utf8(directory)) {}
    ~ResourceHandle() {
        if(owned_labels && StringTable::Labels==owned_labels)StringTable::Unload();
    }
    ResourceEnvironment files;
    CSFLabel* owned_labels=nullptr;
};

bool create_resources(std::string_view directory, ResourceHandle*& output, std::string& error) {
    if (output) { error = "resource output must be null"; return false; }
    try {
        auto* resources = new ResourceHandle(directory);
        error.clear();
        output = resources;
        return true;
    } catch (const std::exception& e) { error = e.what(); return false; }
}
void destroy_resources(ResourceHandle* resources) noexcept { delete resources; }

ResourceLoadResult load_resources(ResourceHandle& resources, ResourceCallbacks callbacks, std::string& error) {
    error.clear();
    const auto cancelled = [&] { return callbacks.cancelled && callbacks.cancelled(callbacks.context); };
    const auto report = [&](const char* name, unsigned completed) {
        if (callbacks.progress) callbacks.progress(callbacks.context, name, completed, bootstrap_steps + 2);
    };
    try {
        if (cancelled()) return ResourceLoadResult::cancelled;
        report("LANGMD.MIX", 0);
        mount_language_md(resources.files);
        report("LANGMD.MIX", 1);
        if (cancelled()) return ResourceLoadResult::cancelled;
        report("LANGUAGE.MIX", 1);
        mount_language(resources.files);
        report("LANGUAGE.MIX", 2);
        if (cancelled()) return ResourceLoadResult::cancelled;
        const auto observe = [](void* context, const char* name, unsigned count) noexcept {
            const auto& callback = *static_cast<ResourceCallbacks*>(context);
            if (callback.progress) callback.progress(callback.context, name, count + 2, bootstrap_steps + 2);
        };
        ResourceScope scope({&resources.files, nullptr, {&callbacks, observe, observe}});
        if (!MixFileClass::Bootstrap()) {
            error = "bootstrap allocation or cache failure";
            return ResourceLoadResult::failed;
        }
        // Language archives own the original CSF input. Keep its decoded
        // StringTable alive across map/UI reloads, until this resource owner
        // closes. Detached format-test archives may contain no CSF at all.
        if(!StringTable::IsLoaded){
            const char* name="RA2MD.CSF";
            CCFileClass csf(name);
            if(!csf.Exists()){name="RA2.CSF";csf.SetFileName(name);}
            if(csf.Exists()){
                if(!StringTable::LoadFile(name)){error="Could not load original CSF strings";return ResourceLoadResult::failed;}
                resources.owned_labels=StringTable::Labels;
            }
        }
        return cancelled() ? ResourceLoadResult::cancelled : ResourceLoadResult::complete;
    } catch (const std::exception& e) { error = e.what(); return ResourceLoadResult::failed; }
}

bool with_resources(ResourceHandle& resources, void (*operation)(void*), void* context, std::string& error) {
    if (!operation) { error = "resource operation must not be null"; return false; }
    try {
        ResourceScope scope({&resources.files, nullptr, {}});
        operation(context);
        error.clear();
        return true;
    } catch (const std::exception& e) { error = e.what(); return false; }
}
}
