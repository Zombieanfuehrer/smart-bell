set(LIB_USART_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/UART.cpp")
set(LIB_USART_HEADERS 
    "${PROJECT_SOURCE_DIR}/public/Serial/UART.h"
    "${PROJECT_SOURCE_DIR}/public/Serial/Interface.h")

add_library("${LIB_USART}" STATIC ${LIB_USART_SOURCES} ${LIB_USART_HEADERS})
target_include_directories("${LIB_USART}" PUBLIC ${LIBRARY_INCLUDES})

set(LIB_SPI_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/SPI.cpp")
set(LIB_SPI_HEADERS 
    "${PROJECT_SOURCE_DIR}/public/Serial/SPI.h"
    "${PROJECT_SOURCE_DIR}/public/Serial/Interface.h")


add_library("${LIB_SPI}" STATIC ${LIB_SPI_SOURCES} ${LIB_SPI_HEADERS})
target_include_directories("${LIB_SPI}" PUBLIC ${LIBRARY_INCLUDES})