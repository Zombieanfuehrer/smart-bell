set(LIBRARY_INCLUDES "${PROJECT_SOURCE_DIR}/public" "${CMAKE_BINARY_DIR}/configured/include")

set(LIB_WTD_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/SetupWDT.cpp")
set(LIB_WTD_HEADERS "${PROJECT_SOURCE_DIR}/public/SetupWDT.h")

add_library("${LIB_WTD}" STATIC ${LIB_WTD_SOURCES} ${LIB_WTD_HEADERS})
target_include_directories("${LIB_WTD}" PUBLIC ${LIBRARY_INCLUDES})

set(LIB_PININT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/SetupEXT_IN_Interrupt.cpp")
set(LIB_PININT_HEADERS "${PROJECT_SOURCE_DIR}/public/SetupEXT_IN_Interrupt.h")

add_library("${LIB_PININT}" STATIC ${LIB_PININT_SOURCES} ${LIB_PININT_HEADERS})
target_include_directories("${LIB_PININT}" PUBLIC ${LIBRARY_INCLUDES})

set(LIB_LIB_TIMERINT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/SetupTimer.cpp")
set(LIB_LIB_TIMERINT_HEADERS "${PROJECT_SOURCE_DIR}/public/SetupTimer.h")

add_library("${LIB_TIMERINT}" STATIC ${LIB_LIB_TIMERINT_SOURCES} ${LIB_LIB_TIMERINT_HEADERS})
target_include_directories("${LIB_TIMERINT}" PUBLIC ${LIBRARY_INCLUDES})

# ioLibrary Internet protocols (MQTT only - Static IP mode)
set(IOLIBRARY_INTERNET_SOURCES
    "${IOLIBRARY_INTERNET_DIR}/MQTT/mqtt_interface.c"
    "${IOLIBRARY_INTERNET_DIR}/MQTT/MQTTClient.c"
    "${IOLIBRARY_INTERNET_DIR}/MQTT/MQTTPacket/src/MQTTConnectClient.c"
    "${IOLIBRARY_INTERNET_DIR}/MQTT/MQTTPacket/src/MQTTConnectServer.c"
    "${IOLIBRARY_INTERNET_DIR}/MQTT/MQTTPacket/src/MQTTDeserializePublish.c"
    "${IOLIBRARY_INTERNET_DIR}/MQTT/MQTTPacket/src/MQTTFormat.c"
    "${IOLIBRARY_INTERNET_DIR}/MQTT/MQTTPacket/src/MQTTPacket.c"
    "${IOLIBRARY_INTERNET_DIR}/MQTT/MQTTPacket/src/MQTTSerializePublish.c"
    "${IOLIBRARY_INTERNET_DIR}/MQTT/MQTTPacket/src/MQTTSubscribeClient.c"
    "${IOLIBRARY_INTERNET_DIR}/MQTT/MQTTPacket/src/MQTTSubscribeServer.c"
    "${IOLIBRARY_INTERNET_DIR}/MQTT/MQTTPacket/src/MQTTUnsubscribeClient.c"
    "${IOLIBRARY_INTERNET_DIR}/MQTT/MQTTPacket/src/MQTTUnsubscribeServer.c")

add_library("${LIB_IOLIBRARY_INTERNET}" STATIC ${IOLIBRARY_INTERNET_SOURCES})

target_include_directories("${LIB_IOLIBRARY_INTERNET}" PUBLIC
    "${PROJECT_SOURCE_DIR}/public/Ethernet"
    "${PROJECT_SOURCE_DIR}/public/Ethernet/W5500"
    "${IOLIBRARY_INTERNET_DIR}/DHCP"
    "${IOLIBRARY_INTERNET_DIR}/DNS"
    "${IOLIBRARY_INTERNET_DIR}/MQTT"
    "${IOLIBRARY_INTERNET_DIR}/MQTT/MQTTPacket/src")

set_target_properties("${LIB_IOLIBRARY_INTERNET}" PROPERTIES LINKER_LANGUAGE C)

target_link_libraries("${LIB_IOLIBRARY_INTERNET}" PUBLIC "${LIB_WIZNET_IOLIBRARY}")

add_subdirectory(Serial)
add_subdirectory(Utils)
add_subdirectory(Ethernet)
add_subdirectory(MQTT)
add_subdirectory(Config)
