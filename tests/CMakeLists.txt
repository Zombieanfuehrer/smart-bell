set(TEST_SOURCES_UTILS ${CMAKE_CURRENT_SOURCE_DIR}/Utils/CircularBuffer_test.cpp)

set(TEST_SOURCES_CONFIG 
    # ConfigManager_test.cpp disabled - uses AVR-specific code
    ${CMAKE_CURRENT_SOURCE_DIR}/Config/LightweightConfig_test.cpp
)

# MQTT tests disabled - require W5500 API not available for Linux builds
# set(TEST_SOURCES_MQTT
#     ${CMAKE_CURRENT_SOURCE_DIR}/MQTT/MinimalMQTT_test.cpp
# )

# Ethernet Tests vorläufig deaktiviert wegen Header-Kollisionen mit WIZnet-Library
# Benötigt Wrapper-Layer um POSIX/Makro-Konflikte zu vermeiden
# TODO: Erstelle einen C++ Wrapper um die WIZnet C-API für bessere Testbarkeit
# set(TEST_SOURCES_ETHERNET 
#     ${CMAKE_CURRENT_SOURCE_DIR}/Ethernet/MockWiznetAPI.cpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/Ethernet/EmbeddedSocketInterface_test.cpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/Ethernet/W5500Interface_test.cpp
# )

# All active tests
set(TEST_SOURCES_ALL 
    ${TEST_SOURCES_UTILS} 
    ${TEST_SOURCES_CONFIG}
    # ${TEST_SOURCES_MQTT}
)

add_executable(${EXECUTABLE_UNIT_TEST} ${TEST_SOURCES_ALL})

# Include-Verzeichnisse für Tests
target_include_directories(${EXECUTABLE_UNIT_TEST} PRIVATE
    ${CMAKE_SOURCE_DIR}/public
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/Mocks  # Hardware mocks
)

# Define NO_AVR for test builds to enable conditional compilation
target_compile_definitions(${EXECUTABLE_UNIT_TEST} PRIVATE NO_AVR)

target_link_libraries(
    ${EXECUTABLE_UNIT_TEST}
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    ${LIB_UTILS}
    ${LIB_CONFIG}
    ${LIB_TIMER_SERVICE})

add_test(NAME ${EXECUTABLE_UNIT_TEST} COMMAND ${EXECUTABLE_UNIT_TEST} --gtest_output=xml:report.xml --gtest_color=yes
                                              --gtest_verbose)
