if(NOT DEFINED SdkRootDirPath)
    execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR
    " Wrong SDK is selected, please run\n"
    " west config manifest.file west_mcuxsdk.yml\n"
    " and try again.")
endif()
