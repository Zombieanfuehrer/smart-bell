set(TEST_SOURCES_UTILS ${CMAKE_CURRENT_SOURCE_DIR}/Utils/CircularBuffer_test.cpp)

# Config-Tests kompilieren die Quellen direkt ohne AVR-abhängige Libraries
set(TEST_SOURCES_CONFIG
    ${CMAKE_CURRENT_SOURCE_DIR}/Config/ConfigManager_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Config/UARTCommandParser_test.cpp
)

# Quellen für Config-Tests (ohne Hardware-Abhängigkeiten)
set(CONFIG_SOURCES_FOR_TEST
    ${CMAKE_SOURCE_DIR}/src/Config/ConfigManager.cpp
    ${CMAKE_SOURCE_DIR}/src/Config/UARTCommandParser.cpp
)

# Ethernet Tests vorläufig deaktiviert wegen Header-Kollisionen mit WIZnet-Library
# Benötigt Wrapper-Layer um POSIX/Makro-Konflikte zu vermeiden
# TODO: Erstelle einen C++ Wrapper um die WIZnet C-API für bessere Testbarkeit
# set(TEST_SOURCES_ETHERNET 
#     ${CMAKE_CURRENT_SOURCE_DIR}/Ethernet/MockWiznetAPI.cpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/Ethernet/EmbeddedSocketInterface_test.cpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/Ethernet/W5500Interface_test.cpp
# )

# Utils- und Config-Tests aktiviert (Config-Quellen direkt inkludiert)
set(TEST_SOURCES_ALL 
    ${TEST_SOURCES_UTILS} 
    ${TEST_SOURCES_CONFIG}
    ${CONFIG_SOURCES_FOR_TEST}
)

add_executable(${EXECUTABLE_UNIT_TEST} ${TEST_SOURCES_ALL})

# Include-Verzeichnisse für Tests
target_include_directories(${EXECUTABLE_UNIT_TEST} PRIVATE
    ${CMAKE_SOURCE_DIR}/public
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(
    ${EXECUTABLE_UNIT_TEST}
    GTest::gtest
    GTest::gtest_main
    ${LIB_UTILS})

add_test(NAME ${EXECUTABLE_UNIT_TEST} COMMAND ${EXECUTABLE_UNIT_TEST} --gtest_output=xml:report.xml --gtest_color=yes
                                              --gtest_verbose)
