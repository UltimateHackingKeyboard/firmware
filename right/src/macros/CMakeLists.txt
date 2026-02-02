# Generate command hash table from gperf input
find_program(GPERF_EXECUTABLE gperf)
if(GPERF_EXECUTABLE)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/command_hash.c
        COMMAND ${GPERF_EXECUTABLE}
                -t -L ANSI-C -N command_lookup -H command_hash
                ${CMAKE_CURRENT_SOURCE_DIR}/command_hash.gperf
                > ${CMAKE_CURRENT_SOURCE_DIR}/command_hash.c
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/command_hash.gperf
        COMMENT "Generating command_hash.c from command_hash.gperf"
    )
    add_custom_target(generate_command_hash DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/command_hash.c)
    add_dependencies(${PROJECT_NAME} generate_command_hash)
endif()

target_sources(${PROJECT_NAME} PRIVATE
    commands.c
    core.c
    debug_commands.c
    display.c
    key_timing.c
    keyid_parser.c
    scancode_commands.c
    set_command.c
    shortcut_parser.c
    status_buffer.c
    string_reader.c
    vars.c
)
