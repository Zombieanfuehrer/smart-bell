set(LIB_CONFIG_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/ConfigManager.cpp")
set(LIB_CONFIG_HEADERS "${PROJECT_SOURCE_DIR}/public/Config/ConfigManager.h")

add_library("${LIB_CONFIG}" STATIC ${LIB_CONFIG_SOURCES} ${LIB_CONFIG_HEADERS})
target_include_directories("${LIB_CONFIG}" PUBLIC ${LIBRARY_INCLUDES})
target_link_libraries("${LIB_CONFIG}" PUBLIC ${LIB_MQTT} ${LIB_USART})
