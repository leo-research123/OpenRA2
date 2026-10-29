#pragma once
#include "api/ini_runtime.hpp"

namespace game {
const IniRuntimeServices& ini_runtime();
// Target composition supplies original EXE services or no default on a host.
const IniRuntimeServices* default_ini_runtime() noexcept;
}
