# Loaded by CMake after its compiler platform module, once per enabled language.
# This override belongs only to clang-msvc-windows-x86.cmake, not native MSVC.
foreach(language C CXX)
    set(CMAKE_${language}_CREATE_SHARED_LIBRARY
        "<CMAKE_LINKER> /nologo <OBJECTS> /out:<TARGET> /implib:<TARGET_IMPLIB> /pdb:<TARGET_PDB> /dll /version:<TARGET_VERSION_MAJOR>.<TARGET_VERSION_MINOR> /manifest:embed <LINK_FLAGS> <LINK_LIBRARIES> <MANIFESTS>")
    set(CMAKE_${language}_CREATE_SHARED_MODULE "${CMAKE_${language}_CREATE_SHARED_LIBRARY}")
    set(CMAKE_${language}_LINK_EXECUTABLE
        "<CMAKE_LINKER> /nologo <OBJECTS> /out:<TARGET> /implib:<TARGET_IMPLIB> /pdb:<TARGET_PDB> /version:<TARGET_VERSION_MAJOR>.<TARGET_VERSION_MINOR> /manifest:embed <CMAKE_${language}_LINK_FLAGS> <LINK_FLAGS> <LINK_LIBRARIES> <MANIFESTS>")
    set(CMAKE_${language}_LINKER_MANIFEST_FLAG "/manifestinput:")
endforeach()
