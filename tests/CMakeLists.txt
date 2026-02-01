set(TEST_SOURCES_UTILS ${CMAKE_CURRENT_SOURCE_DIR}/Utils/CircularBuffer_test.cpp)

# Ethernet Tests vorläufig deaktiviert wegen Header-Kollisionen mit WIZnet-Library
# Benötigt Wrapper-Layer um POSIX/Makro-Konflikte zu vermeiden
# TODO: Erstelle einen C++ Wrapper um die WIZnet C-API für bessere Testbarkeit
# set(TEST_SOURCES_ETHERNET 
#     ${CMAKE_CURRENT_SOURCE_DIR}/Ethernet/MockWiznetAPI.cpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/Ethernet/EmbeddedSocketInterface_test.cpp
#     ${CMAKE_CURRENT_SOURCE_DIR}/Ethernet/W5500Interface_test.cpp
# )

# Nur Utils-Tests aktiviert
set(TEST_SOURCES_ALL ${TEST_SOURCES_UTILS})

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
