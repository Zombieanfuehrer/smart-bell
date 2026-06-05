add_subdirectory(examples)

set(APP_SMART_BELL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/smart_bell.cpp")
add_executable(${EXECUTABLE_SMART_BELL} ${APP_SMART_BELL_SOURCES})

target_link_libraries(${EXECUTABLE_SMART_BELL} PUBLIC "${LIB_WTD}")
target_link_libraries(${EXECUTABLE_SMART_BELL} PUBLIC "${LIB_PININT}")
target_link_libraries(${EXECUTABLE_SMART_BELL} PUBLIC "${LIB_TIMERINT}")
target_link_libraries(${EXECUTABLE_SMART_BELL} PUBLIC "${LIB_USART}")
target_link_libraries(${EXECUTABLE_SMART_BELL} PUBLIC "${LIB_SPI}")
target_link_libraries(${EXECUTABLE_SMART_BELL} PUBLIC "${LIB_UTILS}")
target_link_libraries(${EXECUTABLE_SMART_BELL} PUBLIC "${LIB_W5500_ETHERNET}")
target_link_libraries(${EXECUTABLE_SMART_BELL} PUBLIC "${LIB_MQTT}")
target_link_libraries(${EXECUTABLE_SMART_BELL} PUBLIC "${LIB_CONFIG}")
target_link_options(${EXECUTABLE_SMART_BELL} PRIVATE "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${EXECUTABLE_SMART_BELL}.map,--cref,--noinhibit-exec")

