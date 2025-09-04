target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_sources(${PROJECT_NAME} PRIVATE
    usb_descriptor_configuration.c
    usb_descriptor_device.c
    usb_descriptor_hid.c
    usb_descriptor_strings.c
    usb_descriptors_microsoft.c
)
