#pragma once
namespace game {
class ResourceEnvironment;
// Host adaptation for numbered CnCNet expansion carriers. Throws locally;
// the resource entry catches failures before returning to its caller.
void mount_installation_expansions(ResourceEnvironment&);
}
