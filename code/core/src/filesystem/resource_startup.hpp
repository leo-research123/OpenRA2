#pragma once
namespace game { class ResourceEnvironment; }
namespace game {
// Separate WinMain calls at 6BD7EF/6BD81F, before 5301A0. Deliberately no
// cancellation/UI/session object in these synchronous original boundaries.
void mount_language_md(ResourceEnvironment& files);
void mount_language(ResourceEnvironment& files);
using ShutdownObserver = void (*)(void*, const char*) noexcept;
// Active first-stage subsequence of 6BE1C0. Frees slot/array-owned objects;
// orphan allocations remain until the host file service is fully cleared.
void shutdown_resource_files(ResourceEnvironment& files,
    ShutdownObserver observer = nullptr, void* context = nullptr) noexcept;
}
