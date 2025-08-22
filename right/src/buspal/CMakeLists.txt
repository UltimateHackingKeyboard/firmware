target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/bm_usb
)
target_sources(${PROJECT_NAME} PRIVATE
    bus_pal_hardware.c
    command.c
    fsl_rtos_abstraction.c
    bm_usb/composite.c
    bm_usb/hid_bootloader.c
    bm_usb/usb_descriptor.c
    microseconds/microseconds_pit.c
    packet/serial_packet.c
)
