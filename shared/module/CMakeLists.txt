target_include_directories(${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}
)
target_compile_definitions(${PROJECT_NAME} PUBLIC
    __STARTUP_CLEAR_BSS
)
target_sources(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/src/module.c
    ${CMAKE_CURRENT_LIST_DIR}/startup/startup_MKL03Z4.S
    ${CMAKE_CURRENT_LIST_DIR}/i2c_watchdog.c
    ${CMAKE_CURRENT_LIST_DIR}/init_clock.c
    ${CMAKE_CURRENT_LIST_DIR}/init_peripherals.c
    ${CMAKE_CURRENT_LIST_DIR}/key_scanner.c
    ${CMAKE_CURRENT_LIST_DIR}/led_pwm.c
    ${CMAKE_CURRENT_LIST_DIR}/main.c
    ${CMAKE_CURRENT_LIST_DIR}/slave_protocol_handler.c
    ${CMAKE_CURRENT_LIST_DIR}/test_led.c
    ${CMAKE_CURRENT_LIST_DIR}/uart.c
)

mcux_add_linker_symbol(SYMBOLS
    __heap_size__=0
    __stack_size__=0x0100
)
mcux_add_armgcc_linker_script(LINKER link/MKL03Z32xxx4_flash.ld)

mcux_convert_binary(
    BINARY "${APPLICATION_BINARY_DIR}/${PROJECT_NAME}.bin"
)
