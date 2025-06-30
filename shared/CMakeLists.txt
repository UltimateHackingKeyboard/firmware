# due to the unconventional project structure, this cmake file is include()-d
# instead of add_subdirectory()-d, so every path has to be absolute
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

set(UHK_VERSIONS_MODE "clean" CACHE STRING "UHK versions file generation mode: none, null, clean, hash")
set(UHK_VERSIONS_FILE ${CMAKE_CURRENT_LIST_DIR}/versions.c)
# only set to generated if the file is generated at build time (execute_process() creates it at configure time)
#set_source_files_properties(${UHK_VERSIONS_FILE} PROPERTIES GENERATED TRUE)

if(NOT UHK_VERSIONS_MODE STREQUAL "none")
    message(STATUS "UHK: generating ${UHK_VERSIONS_FILE}")
    find_package(Python 3 REQUIRED)
    if(Python_VERSION_MAJOR LESS 3)
        message(FATAL_ERROR "Python 3 is required versions generation.")
    endif()

    set(GENERATE_VERSIONS_ARG "")
    if(UHK_VERSIONS_MODE STREQUAL "hash")
        set(GENERATE_VERSIONS_ARG "--withMd5Sums")
    elseif(UHK_VERSIONS_MODE STREQUAL "null")
        set(GENERATE_VERSIONS_ARG "--withZeroVersions")
    endif()
    execute_process(
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_LIST_DIR}/../scripts/generate_versions.py" ${GENERATE_VERSIONS_ARG}
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
        RESULT_VARIABLE GENERATE_VERSIONS_RESULT
        OUTPUT_VARIABLE GENERATE_VERSIONS_OUTPUT
        ERROR_VARIABLE GENERATE_VERSIONS_ERROR
    )
    if(NOT GENERATE_VERSIONS_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to generate versions.c: ${GENERATE_VERSIONS_ERROR}${GENERATE_VERSIONS_OUTPUT}")
    endif()
endif()

target_sources(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/bool_array_converter.c
    ${CMAKE_CURRENT_LIST_DIR}/buffer.c
    ${CMAKE_CURRENT_LIST_DIR}/crc16.c
    $<$<BOOL:${IS_MCUX_SDK}>:${CMAKE_CURRENT_LIST_DIR}/key_matrix.c>
    $<$<BOOL:${IS_MCUX_SDK}>:${CMAKE_CURRENT_LIST_DIR}/key_vector.c>
    ${CMAKE_CURRENT_LIST_DIR}/slave_protocol.c
    ${CMAKE_CURRENT_LIST_DIR}/versions.c
)

if (IS_MCUX_SDK)
    # mysteriously used by the mcuxsdk toolchain
    set(to_be_ignored_variables "${APP_DIR}" "${PrjRootDirPath}" "${CMAKE_TOOLCHAIN_FILE}")

    # TODO: port to zephyr correctly, then remove the if
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
            -fno-use-cxa-atexit
            -fno-threadsafe-statics
        >
    )
endif()
