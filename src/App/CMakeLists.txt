set(LIB_APP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/SmartBellApp.cpp")
set(LIB_APP_HEADERS "${PROJECT_SOURCE_DIR}/public/App/SmartBellApp.h")

add_library("${LIB_APP}" STATIC 
    ${LIB_APP_SOURCES} 
    ${LIB_APP_HEADERS})

target_include_directories("${LIB_APP}" PUBLIC ${LIBRARY_INCLUDES})

target_link_libraries("${LIB_APP}" PUBLIC 
    "${LIB_CONFIG}"
    "${LIB_NETWORK}"
    "${LIB_TIMER_SERVICE}"
    "${LIB_W5500_ETHERNET}"
    "${LIB_WTD}"
    "${LIB_USART}")
