include_guard(GLOBAL)
get_filename_component(ra2_native "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

file(GLOB_RECURSE ra2_public_sources CONFIGURE_DEPENDS
    "${ra2_native}/core/include/*.cpp" "${ra2_native}/core/include/*.cc"
    "${ra2_native}/core/include/*.cxx")
if(ra2_public_sources)
    message(FATAL_ERROR "core/include must not contain implementation files: ${ra2_public_sources}")
endif()
if(EXISTS "${ra2_native}/yrpp")
    message(FATAL_ERROR "YRpp headers belong in core/include/yrpp, not native/yrpp")
endif()

# Resolve local includes using the actual implementation search roots. Unknown
# system/dependency headers are left to the compiler; project edges are checked
# even when a caller attempts to bypass the public target using a relative path.
foreach(area core/include core/src bridge experiments)
    file(GLOB_RECURSE sources CONFIGURE_DEPENDS
        "${ra2_native}/${area}/*.h" "${ra2_native}/${area}/*.hpp"
        "${ra2_native}/${area}/*.cpp" "${ra2_native}/${area}/*.inc")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${sources})
    foreach(source IN LISTS sources)
        file(RELATIVE_PATH caller "${ra2_native}" "${source}")
        get_filename_component(parent "${source}" DIRECTORY)
        file(STRINGS "${source}" includes REGEX "^[ \t]*#[ \t]*include[ \t]*[<\"]")
        foreach(line IN LISTS includes)
            string(REGEX REPLACE "^[ \t]*#[ \t]*include[ \t]*[<\"]([^>\"]+)[>\"].*$" "\\1" header "${line}")
            if(area STREQUAL "core/src" AND header MATCHES "^(godot_cpp|compat|bindings|bridge)/")
                message(FATAL_ERROR "Core implementation depends on its host: ${caller} -> ${header}")
            endif()
            set(dependency "")
            foreach(root "${parent}" "${ra2_native}/core/include" "${ra2_native}/core/src"
                    "${ra2_native}/bridge/include" "${ra2_native}")
                if(EXISTS "${root}/${header}" AND NOT IS_DIRECTORY "${root}/${header}")
                    file(REAL_PATH "${root}/${header}" resolved)
                    file(RELATIVE_PATH dependency "${ra2_native}" "${resolved}")
                    break()
                endif()
            endforeach()
            if(dependency STREQUAL "")
                continue()
            endif()
            if(area STREQUAL "core/include" AND NOT dependency MATCHES "^core/include/")
                message(FATAL_ERROR "Public core header includes implementation: ${caller} -> ${dependency}")
            elseif(area STREQUAL "core/src" AND dependency MATCHES "^(compat|bindings|bridge|experiments)/")
                message(FATAL_ERROR "Core implementation depends on its host: ${caller} -> ${dependency}")
            elseif((area STREQUAL "bridge" OR area STREQUAL "experiments") AND dependency MATCHES "^core/src/")
                message(FATAL_ERROR "Host must use the public core API: ${caller} -> ${dependency}")
            endif()
        endforeach()
    endforeach()
endforeach()

include("${CMAKE_CURRENT_LIST_DIR}/CheckCoreOrganization.cmake")
