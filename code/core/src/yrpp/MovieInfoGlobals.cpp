#include "yrpp/Unsorted.h"

namespace {
DynamicVectorClass<const char*> movies;
struct MovieNamesCleanup {
    ~MovieNamesCleanup() { MovieInfo::ClearArray(); }
} cleanup;
}
DynamicVectorClass<const char*>& MovieInfo::Array = movies;
