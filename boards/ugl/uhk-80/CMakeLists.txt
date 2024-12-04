zephyr_library()
if(CONFIG_DEVICE_ID) # don't add it to bootloader image as well
    zephyr_library_sources(board.c)
endif()
