target_sources(${PROJECT_NAME} PRIVATE
    controls_report.c
    keyboard_report.c
    mouse_report.c
    command_app.cpp
    controls_app.cpp
    keyboard_app.cpp
    mouse_app.cpp
    transport.cpp
    transport_usb.cpp
)

if(CONFIG_C2USB_HOGP)
    target_sources(${PROJECT_NAME} PRIVATE
        ble_app.cpp
        transport_ble.cpp
    )
endif()
