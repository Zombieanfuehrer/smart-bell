set(APP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

add_executable(${EXECUTABLE_NAME} ${APP_SOURCES})

target_link_libraries(${EXECUTABLE_NAME} PUBLIC "${LIB_WTD}")
target_link_libraries(${EXECUTABLE_NAME} PUBLIC "${LIB_PININT}")
target_link_libraries(${EXECUTABLE_NAME} PUBLIC "${LIB_TIMERINT}")
target_link_libraries(${EXECUTABLE_NAME} PUBLIC "${LIB_SPI}")
target_link_libraries(${EXECUTABLE_NAME} PUBLIC "${LIB_USART}")

