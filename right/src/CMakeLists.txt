target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/arduino_hid
)

add_subdirectory(config_parser)
add_subdirectory(layouts)
add_subdirectory(macros)
add_subdirectory(peripherals)
add_subdirectory(slave_drivers)
add_subdirectory(usb_commands)
add_subdirectory(usb_interfaces)
add_subdirectory(test_suite)
if(IS_MCUX_SDK)
    add_subdirectory(buspal)
    add_subdirectory(ksdk_usb)
    add_subdirectory(usb_descriptors)
    target_sources(${PROJECT_NAME} PRIVATE
        startup/startup_MK22F51212.S
    )
    # need to add header explicitly to avoid pulling in wrong SDK header
    # https://github.com/nxp-mcuxpresso/mcuxsdk-middleware-usb/issues/12
    mcux_add_source(SOURCES usb_device_config.h)
endif()

target_sources(${PROJECT_NAME} PRIVATE
    caret_config.c
    config_manager.c
    debug.c
    eeprom.c
    event_scheduler.c
    host_connection.c
    i2c.c
    i2c_error_logger.c
    $<$<BOOL:${IS_MCUX_SDK}>:${CMAKE_CURRENT_SOURCE_DIR}/i2c_watchdog.c>
    $<$<BOOL:${IS_MCUX_SDK}>:${CMAKE_CURRENT_SOURCE_DIR}/init_clock.c>
    $<$<BOOL:${IS_MCUX_SDK}>:${CMAKE_CURRENT_SOURCE_DIR}/init_peripherals.c>
    key_states.c
    keymap.c
    layer.c
    layer_stack.c
    layer_switcher.c
    led_display.c
    led_manager.c
    $<$<BOOL:${IS_MCUX_SDK}>:${CMAKE_CURRENT_SOURCE_DIR}/led_pwm.c>
    ledmap.c
    logger.c
    macro_events.c
    macro_recorder.c
    $<$<BOOL:${IS_MCUX_SDK}>:${CMAKE_CURRENT_SOURCE_DIR}/main.c>
    $<$<BOOL:${IS_MCUX_SDK}>:${CMAKE_CURRENT_SOURCE_DIR}/module.c>
    mouse_controller.c
    mouse_keys.c
    module.c
    oneshot.c
    postponer.c
    power_mode.c
    $<$<BOOL:${IS_MCUX_SDK}>:${CMAKE_CURRENT_SOURCE_DIR}/right_key_matrix.c>
    secondary_role_driver.c
    segment_display.c
    slave_scheduler.c
    storage.c
    str_utils.c
    stubs.c
    test_switches.c
    timer.c
    trace.c
    $<$<BOOL:${IS_MCUX_SDK}>:${CMAKE_CURRENT_SOURCE_DIR}/trace_reasons.c>
    $<$<BOOL:${IS_MCUX_SDK}>:${CMAKE_CURRENT_SOURCE_DIR}/usb_composite_device.c>
    usb_protocol_handler.c
    usb_report_updater.c
    user_logic.c
    utils.c
    wormhole.c
)
