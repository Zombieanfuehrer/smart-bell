# For tests, only build LightweightConfig (ConfigManager has AVR dependencies)
if(ENABLE_UNIT_TESTS)
    set(LIB_CONFIG_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/LightweightConfig.cpp")
    set(LIB_CONFIG_HEADERS "${PROJECT_SOURCE_DIR}/public/Config/LightweightConfig.h")
else()
    set(LIB_CONFIG_SOURCES 
        "${CMAKE_CURRENT_SOURCE_DIR}/ConfigManager.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/LightweightConfig.cpp")
    # UARTCommandParser.cpp temporarily disabled - needs API updates for new ConfigManager
    set(LIB_CONFIG_HEADERS 
        "${PROJECT_SOURCE_DIR}/public/Config/ConfigManager.h"
        "${PROJECT_SOURCE_DIR}/public/Config/LightweightConfig.h")
endif()

add_library("${LIB_CONFIG}" STATIC ${LIB_CONFIG_SOURCES} ${LIB_CONFIG_HEADERS})
target_include_directories("${LIB_CONFIG}" PUBLIC ${LIBRARY_INCLUDES})
# Removed LIB_MQTT dependency - ConfigManager no longer uses old MQTT::MQTTConfig
# For unit tests, don't link against UART (uses mocks instead)
if(NOT ENABLE_UNIT_TESTS)
    target_link_libraries("${LIB_CONFIG}" PUBLIC ${LIB_USART})
endif()
