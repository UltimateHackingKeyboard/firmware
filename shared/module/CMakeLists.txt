target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_sources(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/startup/startup_MKL03Z4.S
    ${CMAKE_CURRENT_LIST_DIR}/i2c_watchdog.c
    ${CMAKE_CURRENT_LIST_DIR}/init_clock.c
    ${CMAKE_CURRENT_LIST_DIR}/init_peripherals.c
    ${CMAKE_CURRENT_LIST_DIR}/key_scanner.c
    ${CMAKE_CURRENT_LIST_DIR}/led_pwm.c
    ${CMAKE_CURRENT_LIST_DIR}/main.c
    ${CMAKE_CURRENT_LIST_DIR}/slave_protocol_handler.c
    ${CMAKE_CURRENT_LIST_DIR}/test_led.c
)

mcux_add_armgcc_linker_script(LINKER link/MKL03Z32xxx4_flash.ld)
