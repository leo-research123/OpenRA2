#pragma once
#include <string>
#include <string_view>

namespace game {
class ResourceHandle;
enum class ResourceLoadResult { complete, failed, cancelled };
struct ResourceCallbacks {
    void* context = nullptr;
    void (*progress)(void*, const char* name, unsigned completed, unsigned total) noexcept = nullptr;
    bool (*cancelled)(void*) noexcept = nullptr;
};

// One resource environment per process. output must be null; failure leaves it
// unchanged. The owner serializes operations and joins workers before destroy.
// This API owns files only: before destroy, the host must end all image use and
// call the existing SHP module's Unload_All_Shapes(). That operation keeps
// caller-owned SHP references alive; destroy_resources neither traverses nor
// deletes them. Optional FileSystem/PCX/Convert caches have their own ownership
// and must be released/invalidated by their users before file teardown.
bool create_resources(std::string_view directory, ResourceHandle*& output, std::string& error);
void destroy_resources(ResourceHandle* resources) noexcept;
// Synchronous startup, preserving the original package globals and ownership.
// Callbacks are borrowed for this call, must not throw or mutate resources.
// Cancellation is observed between language packages and after Bootstrap.
ResourceLoadResult load_resources(ResourceHandle& resources, ResourceCallbacks callbacks, std::string& error);
// Run original YRpp operations with this environment active on the calling
// thread. Nested calls and exceptions restore the preceding context. Borrowed
// file/cache views must be released before their resource handle is destroyed.
bool with_resources(ResourceHandle& resources, void (*operation)(void*), void* context, std::string& error);
}
