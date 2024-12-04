target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    # this really needs to be changed when time and restructuring permits
    ${PROJECT_SOURCE_DIR}
)

target_sources(${PROJECT_NAME}-shared PRIVATE
    bool_array_converter.c
    buffer.c
    crc16.c
    fallback_versions.c
    key_matrix.c
    slave_protocol.c
)

# add the versions.c source file conditionally
if(NOT NOVERSIONS)
    target_sources(${PROJECT_NAME} PRIVATE versions.c)
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/versions.c PROPERTIES GENERATED TRUE)
endif()
