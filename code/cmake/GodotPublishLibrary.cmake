# Do not expose the partially written linker output to a running Godot project.
get_filename_component(destination_dir "${RA2_DESTINATION}" DIRECTORY)
file(MAKE_DIRECTORY "${destination_dir}")
file(LOCK "${RA2_DESTINATION}.lock" GUARD PROCESS TIMEOUT 120)
file(COPY_FILE "${RA2_LIBRARY}" "${RA2_DESTINATION}.pending")
file(RENAME "${RA2_DESTINATION}.pending" "${RA2_DESTINATION}")
