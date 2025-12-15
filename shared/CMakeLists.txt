# due to the unconventional project structure, this cmake file is include()-d
# instead of add_subdirectory()-d, so every path has to be absolute

find_package(Python 3 REQUIRED)
if(Python_VERSION_MAJOR LESS 3)
    message(FATAL_ERROR "Python 3 is required versions generation.")
endif()

set(UHK_VERSIONS_FILE ${CMAKE_CURRENT_LIST_DIR}/versions.c)
set(UHK_CHECKSUMS_FILE ${CMAKE_CURRENT_LIST_DIR}/fw_checksums.c)

add_library(uhk_versions_checksums OBJECT ${UHK_VERSIONS_FILE})
if(IS_NRF_SDK)
    target_link_libraries(app PUBLIC uhk_versions_checksums)
    target_link_libraries(uhk_versions_checksums PUBLIC zephyr_interface)
else()
    target_link_libraries(${PROJECT_NAME} PUBLIC uhk_versions_checksums)
    if(DEFINED DEVICE_ID)
        target_compile_definitions(uhk_versions_checksums PUBLIC
            DEVICE_ID=${DEVICE_ID}
        )
    endif()
endif()

set(UHK_USE_CHECKSUMS FALSE CACHE BOOL "Use calculated FW checksums in the built image")
if(SYSBUILD)
    # with sysbuild, the project isn't built directly, and the cmake arguments have to be fetched another way
    # https://github.com/zephyrproject-rtos/zephyr/issues/88303
    # https://github.com/zephyrproject-rtos/zephyr/issues/82678#issuecomment-2875599147
    zephyr_get(UHK_USE_CHECKSUMS SYSBUILD)
endif()
if(UHK_USE_CHECKSUMS)
    message("UHK: Using fw_checksums.c")
    target_sources(uhk_versions_checksums PRIVATE
        ${UHK_CHECKSUMS_FILE}
    )
endif()

# track changes in the package.json file
configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/../scripts/package.json
    ${CMAKE_CURRENT_BINARY_DIR}/package.json
    COPYONLY
)
execute_process(
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_LIST_DIR}/../scripts/generate_versions.py" ${CMAKE_CURRENT_BINARY_DIR}/package.json
    WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
    RESULT_VARIABLE GENERATE_VERSIONS_RESULT
    ERROR_VARIABLE GENERATE_VERSIONS_ERROR
)
if(NOT GENERATE_VERSIONS_RESULT EQUAL 0)
    message(FATAL_ERROR "UHK: Failed to generate versions.c: ${GENERATE_VERSIONS_ERROR}")
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_sources(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/bool_array_converter.c
    ${CMAKE_CURRENT_LIST_DIR}/buffer.c
    ${CMAKE_CURRENT_LIST_DIR}/crc16.c
    ${CMAKE_CURRENT_LIST_DIR}/uart_parser.c
    $<$<BOOL:${IS_MCUX_SDK}>:${CMAKE_CURRENT_LIST_DIR}/key_matrix.c>
    $<$<BOOL:${IS_MCUX_SDK}>:${CMAKE_CURRENT_LIST_DIR}/key_vector.c>
    ${CMAKE_CURRENT_LIST_DIR}/slave_protocol.c
)

if (IS_MCUX_SDK)
    # mysteriously used by the mcuxsdk toolchain, produces warnings
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
