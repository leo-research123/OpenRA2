// Existing YRpp Theater declaration, fixed 9402d7da.
// No same-class implementation exists in the vendored third-party snapshots.
// Lookup calibrated against gamemd 0x48DBE0; table copied from 0x7E1B78.
#include "yrpp/Theater.h"
#include <cstddef>
#include <cstring>
#ifdef _WIN32
#include <string.h>
#else
#include <strings.h>
#endif

static_assert(sizeof(Theater) == 0x70);
static_assert(offsetof(Theater, Extension) == 0x4e);

Theater const* Theater::Get(TheaterType theater) {
    return &Array[static_cast<int>(theater)];
}
Theater const& Theater::GetTheater(TheaterType theater) {
    return Array[static_cast<int>(theater)];
}
int YRPP_FASTCALL Theater::FindIndex(const char* name) {
    for (int i = 0; i < 6; ++i) {
#ifdef _WIN32
        if (_stricmp(name, Array[i].ID) == 0) return i;
#else
        if (strcasecmp(name, Array[i].ID) == 0) return i;
#endif
    }
    return -1;
}
