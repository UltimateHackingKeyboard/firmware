target_include_directories(app PUBLIC
    .
    legacy
    legacy/config_parser
    shared
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
file(GLOB_RECURSE legacy_sources legacy/*.c)
file(GLOB_RECURSE shared_sources shared/*.c)

if(CONFIG_BOARD_UHK_60_RIGHT)
    target_sources(app PRIVATE
        uhk60/main.c
    )
else()
    target_sources(app PRIVATE
        ${usb_cpp_sources}
        ${legacy_sources}
        ${shared_sources}
        bt_conn.c
        bt_manager.c
        bt_pair.c
        debug_eventloop_timing.c
        device_state.c
        flash.c
        logger.c
        main.c
        messenger_queue.c
        messenger.c
        settings.c
        shell.c
        stack_trace.c
        state_sync.c
    )
    if(CONFIG_BT_PERIPHERAL)
        target_sources(app PRIVATE
            bt_advertise.c
            nus_server.c
        )
    endif()
    if(CONFIG_BT_CENTRAL)
        target_sources(app PRIVATE
            bt_scan.c
            nus_client.c
        )
    endif()
    if(CONFIG_BOARD_UHK_DONGLE)
        target_sources(app PRIVATE
            dongle_leds.c
        )
    else()
        target_sources(app PRIVATE
            ${keyboard_sources}
        )
        if(CONFIG_BOARD_UHK_80_RIGHT)
            target_sources(app PRIVATE
                ${lvgl_dummy}
            )
        endif()
    endif()
endif()
