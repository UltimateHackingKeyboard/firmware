# due to the unconventional project structure, this cmake file is include()-d
# instead of add_subdirectory()-d, so every path has to be absolute
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

set(UHK_VERSIONS_FILE ${CMAKE_CURRENT_LIST_DIR}/versions.c)
if (NOT EXISTS ${UHK_VERSIONS_FILE})
    message(WARNING "Generating missing ${UHK_VERSIONS_FILE} for UHK")
    find_package(Python 3 REQUIRED)
    if(Python_VERSION_MAJOR LESS 3)
        message(FATAL_ERROR "Python 3 is required versions generation.")
    endif()
    execute_process(
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_LIST_DIR}/../scripts/generate_versions.py"
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
    )
endif()
set_source_files_properties(${UHK_VERSIONS_FILE} PROPERTIES GENERATED TRUE)

target_sources(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/bool_array_converter.c
    ${CMAKE_CURRENT_LIST_DIR}/buffer.c
    ${CMAKE_CURRENT_LIST_DIR}/crc16.c
    ${CMAKE_CURRENT_LIST_DIR}/key_matrix.c
    $<$<BOOL:${IS_MCUX_SDK}>:${CMAKE_CURRENT_LIST_DIR}/key_vector.c>
    ${CMAKE_CURRENT_LIST_DIR}/slave_protocol.c
    ${CMAKE_CURRENT_LIST_DIR}/versions.c
)

target_compile_options(${PROJECT_NAME} PUBLIC
    $<$<BOOL:${IS_MCUX_SDK}>:-O2>
    $<$<BOOL:${IS_MCUX_SDK} AND CONFIG:Debug>:-g3>
    -Wall
    -Wextra
    -Woverflow
    -Wno-unused-parameter
    -Wno-type-limits
    -Wlogical-op
    -Wrestrict
    -Wnull-dereference
    -Wshadow
    $<$<COMPILE_LANGUAGE:CXX>:
        -felide-constructors
        -fno-exceptions
        -fno-rtti
    >
)
