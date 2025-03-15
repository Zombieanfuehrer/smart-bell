# -*- mode: cmake -*-
#######################################################################################################################
# @file CMakeFormat.cmake
#
# @author David A. Haufe (david.haufe90@gmail.com)
# @version 1.0.0
# @date 2024-07-29
#
# @brief This function adds a CMake custom Target target which executes CMake format and formats all CMake files automatically.
#
# @copyright MIT License
#######################################################################################################################
function(add_cmake_format_target)
    # Define the main CMake file and recursively search for CMake files
    set(ROOT_CMAKE_FILES "${CMAKE_SOURCE_DIR}/CMakeLists.txt")
    file(GLOB_RECURSE CMAKE_FILES_TXT "*/CMakeLists.txt")
    file(GLOB_RECURSE CMAKE_FILES_C "cmake/*.cmake")

    # Filter the files to exclude certain directories
    list(
        FILTER
        CMAKE_FILES_TXT
        EXCLUDE
        REGEX
        "${CMAKE_SOURCE_DIR}/(build|external|.venv)/.*")
    set(CMAKE_FILES ${ROOT_CMAKE_FILES} ${CMAKE_FILES_TXT} ${CMAKE_FILES_C})

    # Find the cpplint executable in the virtual environment or in the system PATH
    find_program(CMAKE_FORMAT NAMES cmake-format HINTS "${CMAKE_SOURCE_DIR}/.venv/bin" ENV PATH)
    if(NOT CMAKE_FORMAT)
        message(
            FATAL_ERROR
                "[add_cmake_format_target] cmake-format not found in virtual environment or system PATH! Please install to use this option"
        )
    else()
        set(CMAKE_CMAKE_FORMAT ${CMAKE_FORMAT})
    endif()

    if(EXISTS ${CMAKE_CMAKE_FORMAT})
        message(STATUS "[add_cmake_format_target] Added Cmake Format")

        # Create the list of formatting commands
        set(FORMATTING_COMMANDS)
        foreach(cmake_file ${CMAKE_FILES})
            list(
                APPEND
                FORMATTING_COMMANDS
                COMMAND
                "${CMAKE_CMAKE_FORMAT}"
                -c
                ${CMAKE_SOURCE_DIR}/style/.cmake-format.yaml
                -i
                ${cmake_file})
        endforeach()

        # Define the custom target for running the formatting
        add_custom_target(
            run_cmake_format
            COMMAND ${FORMATTING_COMMANDS}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Running cmake-format")
    else()
        message(WARNING "[add_cmake_format_target] CMAKE_FORMAT NOT FOUND")
    endif()
endfunction()
