set(WIZNET_IOLIBRARY_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/W5500/w5500.c" 
    "${CMAKE_CURRENT_SOURCE_DIR}/socket.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/wizchip_conf.c")

set(WIZNET_IOLIBRARY_HEADERS
    "${PROJECT_SOURCE_DIR}/public/Ethernet/W5500/w5500.h" 
    "${PROJECT_SOURCE_DIR}/public/Ethernet/socket.h"
    "${PROJECT_SOURCE_DIR}/public/Ethernet/wizchip_conf.h")

add_library(${LIB_WIZNET_IOLIBRARY} STATIC 
    ${WIZNET_IOLIBRARY_SOURCES} 
    ${WIZNET_IOLIBRARY_HEADERS})

target_include_directories(${LIB_WIZNET_IOLIBRARY} PUBLIC 
    "${PROJECT_SOURCE_DIR}/public/Ethernet/"
    "${PROJECT_SOURCE_DIR}/public/Ethernet/W5500/")

set_target_properties(${LIB_WIZNET_IOLIBRARY} PROPERTIES LINKER_LANGUAGE C)

set(LIB_W5500_ETHERNET_SOURCE 
    "${CMAKE_CURRENT_SOURCE_DIR}/W5500/W5500Interface.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/EmbeddedSocketInterface.cpp")

set(LIB_W5500_ETHERNET_HEADERS 
    "${PROJECT_SOURCE_DIR}/public/Ethernet/W5500/W5500Interface.h")

add_library("${LIB_W5500_ETHERNET}" STATIC 
    ${LIB_W5500_ETHERNET_SOURCE} 
    ${LIB_W5500_ETHERNET_HEADERS})

target_include_directories("${LIB_W5500_ETHERNET}" PUBLIC 
    "${PROJECT_SOURCE_DIR}/public/Ethernet/"
    "${PROJECT_SOURCE_DIR}/public/Ethernet/W5500/")

target_link_libraries(${LIB_W5500_ETHERNET} PUBLIC 
    "${LIB_WIZNET_IOLIBRARY}"
    "${LIB_SPI}"
    "${LIB_UTILS}"
    "${LIB_USART}")
