include_guard(GLOBAL)
include("${CMAKE_CURRENT_LIST_DIR}/YrppSourceOwnership.cmake")
get_filename_component(ra2_organization_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(ra2_owned_sources "")
foreach(record IN LISTS ra2_yrpp_source_ownership)
    string(REPLACE "|" ";" pair "${record}")
    list(GET pair 0 source)
    list(GET pair 1 header)
    if(NOT EXISTS "${ra2_organization_root}/${source}" OR NOT EXISTS "${ra2_organization_root}/${header}")
        message(FATAL_ERROR "Stale original-module correspondence: ${record}")
    endif()
    list(APPEND ra2_owned_sources "${source}")
endforeach()
file(GLOB_RECURSE ra2_original_sources RELATIVE "${ra2_organization_root}" CONFIGURE_DEPENDS
    "${ra2_organization_root}/core/src/yrpp/*.cpp"
    "${ra2_organization_root}/core/src/yrpp/*.hpp"
    "${ra2_organization_root}/core/src/yrpp/*.h")
foreach(source IN LISTS ra2_original_sources)
    if(NOT source IN_LIST ra2_owned_sources)
        message(FATAL_ERROR "YRpp source needs an existing original-module ownership entry: ${source}")
    endif()
endforeach()
file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/YrppPublicHeaders.txt" ra2_approved_headers REGEX "^core/")
file(GLOB_RECURSE ra2_current_headers RELATIVE "${ra2_organization_root}" CONFIGURE_DEPENDS
    "${ra2_organization_root}/core/include/yrpp/*.h"
    "${ra2_organization_root}/core/include/yrpp/*.hpp")
foreach(header IN LISTS ra2_current_headers)
    if(NOT header IN_LIST ra2_approved_headers)
        message(FATAL_ERROR "New public YRpp header requires approval: ${header}")
    endif()
endforeach()

# File shutdown must not regain knowledge of image consumers.
file(GLOB_RECURSE ra2_filesystem_sources CONFIGURE_DEPENDS
    "${ra2_organization_root}/core/src/filesystem/*.cpp"
    "${ra2_organization_root}/core/src/filesystem/*.hpp")
foreach(source IN LISTS ra2_filesystem_sources)
    file(READ "${source}" text)
    if(text MATCHES "#[ \t]*include[ \t]*[<\"](images/|yrpp/(Drawing|Surface|ConvertClass|FileFormats/SHP))")
        message(FATAL_ERROR "File environment depends on an image consumer: ${source}")
    endif()
    if(text MATCHES "(Unload_All_Shapes|unload_all_shapes)[ \t]*\\(")
        message(FATAL_ERROR "SHP teardown belongs to its host, not filesystem: ${source}")
    endif()
endforeach()

# A source-level tripwire, NOT an ABI proof. Whole-archive consumers and real
# integration tests additionally ensure production definitions are linkable.
file(GLOB_RECURSE ra2_adapter_sources CONFIGURE_DEPENDS
    "${ra2_organization_root}/bridge/*.cpp")
foreach(source IN LISTS ra2_adapter_sources)
    file(STRINGS "${source}" definitions REGEX
        "^[ \t]*(void[ \t]*\\*?|bool|size_t|std::size_t)[ \t]+(YRPP_CDECL[ \t]+)?YRMemory::[A-Za-z_]+[ \t]*\\(")
    file(STRINGS "${source}" rendering_definitions REGEX
        "^[ \t]*void[ \t]+(YRPP_FASTCALL[ \t]+)?CC_Draw_Shape[ \t]*\\(")
    if(rendering_definitions)
        message(FATAL_ERROR "Adapter must call, not redefine, the optional core software implementation: ${source}")
    endif()
    if(definitions)
        message(FATAL_ERROR "Adapter must not define core Memory methods: ${source}")
    endif()
endforeach()

function(ra2_verify_core_links)
    foreach(core_target ra2_core ra2_software_render)
        if(NOT TARGET ${core_target})
            continue()
        endif()
        foreach(property LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
            get_target_property(links ${core_target} ${property})
            foreach(link IN LISTS links)
                if(link MATCHES "(godot|bridge|compat|ra2_bootstrap|ra2_image_pal|ra2_files($|[>;]))")
                    message(FATAL_ERROR "Core target depends on its host: ${core_target} -> ${link}")
                endif()
            endforeach()
        endforeach()
    endforeach()
endfunction()
cmake_language(DEFER CALL ra2_verify_core_links)
