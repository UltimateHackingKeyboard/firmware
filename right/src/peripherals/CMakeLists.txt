target_sources(${PROJECT_NAME} PRIVATE
    $<$<BOOL:${IS_MCUX_SDK}>:${CMAKE_CURRENT_SOURCE_DIR}/adc.c>
    $<$<BOOL:${IS_MCUX_SDK}>:${CMAKE_CURRENT_SOURCE_DIR}/led_driver.c>
    $<$<BOOL:${IS_MCUX_SDK}>:${CMAKE_CURRENT_SOURCE_DIR}/leakage_test.c>
    merge_sensor.c
    reset_button.c
    $<$<BOOL:${IS_MCUX_SDK}>:${CMAKE_CURRENT_SOURCE_DIR}/test_led.c>
)
