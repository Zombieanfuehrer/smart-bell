# Network library sources
set(LIB_NETWORK_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/DHCPClient.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/DNSClient.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/MQTTClient.cpp")

set(LIB_NETWORK_HEADERS 
    "${PROJECT_SOURCE_DIR}/public/Network/DHCPClient.h"
    "${PROJECT_SOURCE_DIR}/public/Network/DNSClient.h"
    "${PROJECT_SOURCE_DIR}/public/Network/MQTTClient.h")

add_library("${LIB_NETWORK}" STATIC 
    ${LIB_NETWORK_SOURCES} 
    ${LIB_NETWORK_HEADERS})

target_include_directories("${LIB_NETWORK}" PUBLIC 
    ${LIBRARY_INCLUDES}
    "${IOLIBRARY_INTERNET_DIR}/DHCP"
    "${IOLIBRARY_INTERNET_DIR}/DNS"
    "${IOLIBRARY_INTERNET_DIR}/MQTT"
    "${IOLIBRARY_INTERNET_DIR}/MQTT/MQTTPacket/src")

target_link_libraries("${LIB_NETWORK}" PUBLIC 
    "${LIB_IOLIBRARY_INTERNET}"
    "${LIB_TIMER_SERVICE}"
    "${LIB_W5500_ETHERNET}"
    "${LIB_WTD}"
    "${LIB_USART}")
