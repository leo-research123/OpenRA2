# Source lists only: each consumer compiles these files with its own ABI and
# definitions. Host storage and target-specific service providers stay with
# their targets. A variable-scoped guard also supports standalone subprojects.
include_guard()
get_filename_component(ra2_sources_native "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

set(ra2_audio_sources
    core/src/audio_backend.cpp
    core/src/yrpp/Audio.cpp
    core/src/yrpp/AudioClock.cpp
    core/src/yrpp/AudioIDXData.cpp
    core/src/yrpp/AudioStream.cpp
    core/src/yrpp/AudioController.cpp)

set(ra2_video_sources
    core/src/video_backend.cpp
    core/src/yrpp/GameMovie.cpp)

set(ra2_clock_sources
    core/src/clock.cpp
    core/src/clock_host.cpp
    core/src/yrpp/Timer.cpp)

set(ra2_scenario_sources
    core/src/map_runtime.cpp
    core/src/scenario_runtime.cpp
    core/src/scenario_type_order.cpp
    core/src/yrpp/WinModemClass.cpp
    core/src/yrpp/Randomizer.cpp
    core/src/yrpp/CellClassLighting.cpp
    core/src/yrpp/ScenarioClass.cpp
    core/src/yrpp/ScenarioClassVariables.cpp
    core/src/yrpp/ScenarioClassINI.cpp
    core/src/yrpp/TacticalClassProjection.cpp
    core/src/yrpp/TacticalClassCamera.cpp
    core/src/yrpp/TacticalClassLifecycle.cpp
    core/src/yrpp/ScenarioClassLighting.cpp
    core/src/yrpp/ScenarioClassStream.cpp
    core/src/yrpp/ScenarioClassPause.cpp
    core/src/yrpp/ScenarioClassStart.cpp
    core/src/yrpp/ScenarioClassWorld.cpp
    core/src/yrpp/ScenarioClassInitialize.cpp
    core/src/yrpp/ScenarioClassClearWorld.cpp
    core/src/yrpp/ScenarioClassDestroyWorldObjects.cpp
    core/src/yrpp/ScenarioClassHouses.cpp
    core/src/yrpp/HouseClassInitialization.cpp
    core/src/yrpp/SessionClassPlayers.cpp
    core/src/yrpp/VocClassVolume.cpp
    core/src/yrpp/ScenarioClassLifecycle.cpp)

set(ra2_map_root_sources
    core/src/yrpp/GScreenClass.cpp
    core/src/yrpp/MapClassLifecycle.cpp
    core/src/yrpp/RadarClass.cpp
    core/src/yrpp/RadarClassStream.cpp
    core/src/yrpp/RadarClassGeometry.cpp
    core/src/yrpp/RadarClassLayout.cpp
    core/src/yrpp/DisplayClass.cpp
    core/src/yrpp/DisplayClassStream.cpp
    core/src/yrpp/DisplayClassLayout.cpp
    core/src/yrpp/SidebarClassLayout.cpp
    core/src/yrpp/TabClassLayout.cpp
    core/src/yrpp/PowerClass.cpp
    core/src/yrpp/SidebarClass.cpp
    core/src/yrpp/MouseClass.cpp
    core/src/yrpp/MouseClassCursor.cpp)

set(ra2_rules_sources
    core/src/yrpp/RulesClassRadar.cpp
    core/src/yrpp/RulesClassLoading.cpp
    core/src/yrpp/RulesClassCombat.cpp
    core/src/yrpp/RulesClassAudioVisual.cpp
    core/src/yrpp/RulesClassColors.cpp
    core/src/yrpp/RulesClassCommandBar.cpp
    core/src/yrpp/RulesClassSides.cpp
    core/src/yrpp/RulesClassTypeContents.cpp
    core/src/yrpp/MissionControlClass.cpp
    core/src/rules_runtime.cpp
    core/src/yrpp/RulesClassCrates.cpp
    core/src/yrpp/RulesClassRadiation.cpp
    core/src/yrpp/RulesClassSpecialWeapons.cpp
    core/src/yrpp/RulesClassTypeLists.cpp
    core/src/yrpp/RulesClassLifecycle.cpp
    core/src/yrpp/RulesClassGeneral.cpp
    core/src/yrpp/RulesClassAI.cpp
    core/src/yrpp/RulesClassPointers.cpp
    core/src/yrpp/RulesClassTables.cpp
    core/src/yrpp/AnimTypeClassLookup.cpp
    core/src/yrpp/MovieInfo.cpp)

set(ra2_file_chain_sources
    core/src/yrpp/FileClass.cpp
    core/src/filesystem/file_system.cpp
    core/src/yrpp/RawFileClass.cpp
    core/src/yrpp/BufferIOFileClass.cpp
    core/src/yrpp/CDFileClass.cpp
    core/src/yrpp/CCFileClass.cpp
    core/src/yrpp/YRAllocator.cpp)

# INI algorithms shared by standalone core and original-game composition.
# Globals and the runtime provider are supplied by each target.
set(ra2_ini_sources
    core/src/yrpp/INIClass.cpp
    core/src/yrpp/INIClassValues.cpp
    core/src/yrpp/INIClassBlocks.cpp
    core/src/yrpp/INIClassTypes.cpp
    core/src/yrpp/CCINIClass.cpp
    core/src/yrpp/CCINIClassDigest.cpp
    core/src/ini_runtime.cpp)

# All targets compile the same Memory.h definitions for their ABI/CRT.
set(ra2_core_memory_sources core/src/yrpp/Memory.cpp)
set(ra2_voxel_format_sources
    core/src/yrpp/VoxLib.cpp
    core/src/yrpp/MotLib.cpp
    core/src/images/voxel_palette.cpp)
set(ra2_filesystem_name_sources core/src/filesystem/file_names.cpp)

# Original Convert/Blitter algorithms, used by the optional software component
# and original-image replacement. No target supplies substitute implementations.
set(ra2_palette_sources
    core/src/yrpp/ConvertClassPalette.cpp
    core/src/yrpp/LightConvertClassPalette.cpp)
set(ra2_convert_blitter_sources
    core/src/yrpp/ConvertClassLifecycle.cpp
    core/src/yrpp/ConvertClassBlitters.cpp
    core/src/yrpp/LightConvertClass.cpp
    core/src/yrpp/Blitter.cpp
    core/src/yrpp/BlitterVariants.cpp)

set(ra2_software_render_sources
    core/src/yrpp/ColorSchemeGraphics.cpp
    core/src/type_drawing_software.cpp
    core/src/yrpp/IsometricTileTypeClassDraw.cpp
    core/src/yrpp/Drawing.cpp
    core/src/yrpp/AlphaLightingRemapClass.cpp
    core/src/yrpp/SurfaceSHP.cpp
    core/src/yrpp/PCXRead.cpp
    core/src/yrpp/PCXCache.cpp
    core/src/yrpp/FileSystemPalette.cpp
    core/src/yrpp/ConvertClass.cpp)

foreach(source_set ra2_palette_sources ra2_map_root_sources ra2_voxel_format_sources ra2_audio_sources ra2_video_sources ra2_clock_sources ra2_scenario_sources ra2_rules_sources ra2_ini_sources ra2_file_chain_sources ra2_core_memory_sources ra2_filesystem_name_sources ra2_convert_blitter_sources ra2_software_render_sources)
    list(TRANSFORM ${source_set} PREPEND "${ra2_sources_native}/")
endforeach()
