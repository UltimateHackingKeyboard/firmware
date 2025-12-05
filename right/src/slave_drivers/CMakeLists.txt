target_sources(${PROJECT_NAME} PRIVATE
    is31fl3xxx_driver.c
    $<$<BOOL:${IS_MCUX_SDK}>:${CMAKE_CURRENT_SOURCE_DIR}/kboot_driver.c>
    touchpad_driver.c
    uhk_module_driver.c
)
