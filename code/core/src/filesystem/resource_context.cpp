#include "resource_context.hpp"
#include "resource_environment.hpp"
#include <stdexcept>
namespace game {
namespace { thread_local const ResourceContext* active = nullptr; }
const ResourceContext* try_current_context() noexcept { return active; }
const ResourceContext& current_context() {
    if (!active) throw std::logic_error("resource operation has no active host context");
    return *active;
}
ResourceScope::ResourceScope(ResourceContext context) : platform_scope_(context.files ? &context.files->platform() : nullptr), context_(context), previous_(active) { active = &context_; }
ResourceScope::ResourceScope(FileLocation& source)
    : platform_scope_(&current_file_system()), context_{nullptr, &source, {}}, previous_(active) {
    active = &context_;
}
ResourceScope::~ResourceScope() { active = previous_; }
}
