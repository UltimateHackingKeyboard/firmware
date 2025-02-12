# the library target is created here, so version generation will work without extra steps
add_library(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    # this really needs to be changed when time and restructuring permits
    ${PROJECT_SOURCE_DIR}
)

if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/versions.c")
    message(WARNING "Generating missing versions.c for UHK")
    find_package(Python 3 REQUIRED)
    if(Python_VERSION_MAJOR LESS 3)
        message(FATAL_ERROR "Python 3 is required versions generation.")
    endif()
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/versions.c"
        COMMAND ${Python3_EXECUTABLE} "${PROJECT_SOURCE_DIR}/scripts/generate_versions.py"
        DEPENDS "${PROJECT_SOURCE_DIR}/scripts/generate_versions.py"
    )
endif()
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/versions.c PROPERTIES GENERATED TRUE)

target_sources(${PROJECT_NAME} PRIVATE
    bool_array_converter.c
    buffer.c
    crc16.c
    key_matrix.c
    slave_protocol.c
    versions.c
)
