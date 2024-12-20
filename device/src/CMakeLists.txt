target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    usb
)

set(usb_cpp_sources
    usb/app_base.cpp
    usb/command_app.cpp
    usb/controls_app.cpp
    usb/gamepad_app.cpp
    usb/hid_battery_app.cpp
    usb/keyboard_app.cpp
    usb/mouse_app.cpp
    usb/usb_compatibility.cpp
    usb/usb.cpp
)
file(GLOB_RECURSE keyboard_sources keyboard/*.c)
file(GLOB lvgl_dummy lvgl/*.c)

if(NOT CONFIG_BOARD_UHK_60_RIGHT)
    # C headers compiled with C++ give pointer conversion errors, turn them to warnings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
    target_link_libraries(${PROJECT_NAME} PRIVATE
        c2usb
    )

    target_sources(${PROJECT_NAME} PRIVATE
        ${usb_cpp_sources}
        bt_conn.c
        bt_manager.c
        bt_pair.c
        debug_eventloop_timing.c
        device_state.c
        flash.c
        logger.c
        messenger_queue.c
        messenger.c
        settings.c
        shell.c
        stack_trace.c
        state_sync.c
    )
    if(CONFIG_BT_PERIPHERAL)
        target_sources(${PROJECT_NAME} PRIVATE
            bt_advertise.c
            nus_server.c
        )
    endif()
    if(CONFIG_BT_CENTRAL)
        target_sources(${PROJECT_NAME} PRIVATE
            bt_scan.c
            nus_client.c
        )
    endif()
    if(CONFIG_BOARD_UHK_DONGLE)
        target_sources(${PROJECT_NAME} PRIVATE
            dongle_leds.c
        )
    else()
        target_sources(${PROJECT_NAME} PRIVATE
            ${keyboard_sources}
        )
        if(CONFIG_BOARD_UHK_80_RIGHT)
            target_sources(${PROJECT_NAME} PRIVATE
                ${lvgl_dummy}
            )
        endif()
    endif()
endif()
