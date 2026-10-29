#pragma once
namespace game {
// Optional process-wide diagnostic output; disabled until a host supplies a
// UTF-8 filesystem path. Null disables it. No exceptions cross this boundary.
// Creates missing parent directories. Keeps at most 8 MiB plus one previous file. Call before simulation starts.
bool configure_projectile_log(const char* path) noexcept;
}
