set(TEST_SOURCES_UTILS ${CMAKE_CURRENT_SOURCE_DIR}/Utils/CircularBuffer_test.cpp)

add_executable(${EXECUTABLE_UNIT_TEST} ${TEST_SOURCES_UTILS})
target_link_libraries(
    ${EXECUTABLE_UNIT_TEST}
    GTest::gtest
    GTest::gtest_main
    ${LIB_UTILS})

add_test(NAME ${EXECUTABLE_UNIT_TEST} COMMAND ${EXECUTABLE_UNIT_TEST} --gtest_output=xml:report.xml --gtest_color=yes
                                              --gtest_verbose)
