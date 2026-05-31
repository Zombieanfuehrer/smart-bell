# Network library sources (Static IP mode - MQTT only)
set(LIB_NETWORK_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/MQTTClient.cpp")

set(LIB_NETWORK_HEADERS 
    "${PROJECT_SOURCE_DIR}/public/Network/MQTTClient.h")

add_library("${LIB_NETWORK}" STATIC 
    ${LIB_NETWORK_SOURCES} 
    ${LIB_NETWORK_HEADERS})

target_include_directories("${LIB_NETWORK}" PUBLIC 
    ${LIBRARY_INCLUDES}
    "${IOLIBRARY_INTERNET_DIR}/MQTT"
    "${IOLIBRARY_INTERNET_DIR}/MQTT/MQTTPacket/src")

target_link_libraries("${LIB_NETWORK}" PUBLIC 
    "${LIB_IOLIBRARY_INTERNET}"
    "${LIB_TIMER_SERVICE}"
    "${LIB_W5500_ETHERNET}"
    "${LIB_WTD}"
    "${LIB_USART}")
