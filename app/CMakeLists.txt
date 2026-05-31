set(APP_UART_EXAMPLE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/uart_example.cpp")
add_executable(${EXECUTABLE_UART_EXAMPLE} ${APP_UART_EXAMPLE_SOURCES})

target_link_libraries(${EXECUTABLE_UART_EXAMPLE} PUBLIC "${LIB_WTD}")
target_link_libraries(${EXECUTABLE_UART_EXAMPLE} PUBLIC "${LIB_USART}")
target_link_libraries(${EXECUTABLE_UART_EXAMPLE} PUBLIC "${LIB_UTILS}")

set(APP_TCP_SERVER_EXAMPLE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tcp_server_example.cpp")
add_executable(${EXECUTABLE_TCP_SERVER_EXAMPLE} ${APP_TCP_SERVER_EXAMPLE_SOURCES})

target_link_libraries(${EXECUTABLE_TCP_SERVER_EXAMPLE} PUBLIC "${LIB_W5500_ETHERNET}")
target_link_libraries(${EXECUTABLE_TCP_SERVER_EXAMPLE} PUBLIC "${LIB_SPI}")
target_link_libraries(${EXECUTABLE_TCP_SERVER_EXAMPLE} PUBLIC "${LIB_USART}")
target_link_libraries(${EXECUTABLE_TCP_SERVER_EXAMPLE} PUBLIC "${LIB_UTILS}")

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
# SmartBell main application
set(APP_SMART_BELL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/smart_bell_main.cpp")
add_executable(${EXECUTABLE_SMART_BELL} ${APP_SMART_BELL_SOURCES})

target_link_libraries(${EXECUTABLE_SMART_BELL} PUBLIC "${LIB_APP}")
target_link_libraries(${EXECUTABLE_SMART_BELL} PUBLIC "${LIB_NETWORK}")
target_link_libraries(${EXECUTABLE_SMART_BELL} PUBLIC "${LIB_CONFIG}")
target_link_libraries(${EXECUTABLE_SMART_BELL} PUBLIC "${LIB_TIMER_SERVICE}")
target_link_libraries(${EXECUTABLE_SMART_BELL} PUBLIC "${LIB_W5500_ETHERNET}")
target_link_libraries(${EXECUTABLE_SMART_BELL} PUBLIC "${LIB_SPI}")
target_link_libraries(${EXECUTABLE_SMART_BELL} PUBLIC "${LIB_USART}")
target_link_libraries(${EXECUTABLE_SMART_BELL} PUBLIC "${LIB_PININT}")
target_link_libraries(${EXECUTABLE_SMART_BELL} PUBLIC "${LIB_WTD}")
target_link_libraries(${EXECUTABLE_SMART_BELL} PUBLIC "${LIB_UTILS}")

