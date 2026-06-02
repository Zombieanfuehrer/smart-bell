# For tests, create empty library (MinimalMQTT has W5500 dependencies)
if(ENABLE_UNIT_TESTS)
    # Create empty library target to satisfy dependencies
    add_library("${LIB_MQTT}" INTERFACE)
    target_include_directories("${LIB_MQTT}" INTERFACE ${LIBRARY_INCLUDES})
else()
    set(LIB_MQTT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/MinimalMQTT.cpp")
    set(LIB_MQTT_HEADERS "${PROJECT_SOURCE_DIR}/public/MQTT/MinimalMQTT.h")

    add_library("${LIB_MQTT}" STATIC ${LIB_MQTT_SOURCES} ${LIB_MQTT_HEADERS})
    target_include_directories("${LIB_MQTT}" PUBLIC ${LIBRARY_INCLUDES})
    target_link_libraries("${LIB_MQTT}" PUBLIC ${LIB_W5500_ETHERNET} ${LIB_USART} ${LIB_TIMER_SERVICE})
endif()
