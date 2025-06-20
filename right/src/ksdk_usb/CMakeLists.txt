target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_sources(${PROJECT_NAME} PRIVATE
    usb_device_ch9.c
    usb_device_class.c
    usb_device_hid.c
)
