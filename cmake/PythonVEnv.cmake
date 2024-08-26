# -*- mode: cmake -*-
#######################################################################################################################
# @file PythonVEnv.cmake
#
# @author David A. Haufe (david.haufe90@gmail.com)
# @version 1.0.0
# @date 2024-07-29
#
# @brief This file contains functions to use Python in a virtual enviroment.
#
# @copyright MIT License
#######################################################################################################################

#######################################################################################################################
# @brief Function to set up a Python virtual environment for the project.
#        This function checks if Python 3 is installed and if the project has a requirements.txt file.
#        If the requirements.txt file exists and the .venv directory does not exist, it sets up a virtual environment
#        using the venv module. If both the requirements.txt file and the .venv directory exist, it activates the
#        virtual environment and installs the required packages.
#        The Python interpreter is changed to use the virtual environment.
#        Finally, it installs the packages specified in the requirements.txt file using pip.
#
#######################################################################################################################

function(set_python_virtual_enviroment)
    # Check if Python 3 is installed
    find_package(
        Python3
        COMPONENTS Interpreter
        REQUIRED)
    if(Python3_FOUND)
        message(STATUS "[set_python_virtual_enviroment] Found Python 3 Version: ${Python3_VERSION}")
    else()
        message(FATAL_ERROR "[set_python_virtual_enviroment] Python 3 missing, but is a requirement for this project!")
    endif()

    # Check if requirements.txt file exists and .venv directory does not exist
    if(EXISTS "${CMAKE_SOURCE_DIR}/requirements.txt" AND NOT EXISTS "${CMAKE_SOURCE_DIR}/.venv/")
        message(STATUS "[set_python_virtual_enviroment] setup virtual python enviroment for this project")

        # Set up virtual environment using venv module
        execute_process(COMMAND ${Python3_EXECUTABLE} -m venv .venv WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    endif()

    # Check if requirements.txt file and .venv directory exist
    if(EXISTS "${CMAKE_SOURCE_DIR}/requirements.txt" AND EXISTS "${CMAKE_SOURCE_DIR}/.venv/")
        message(STATUS "[set_python_virtual_enviroment] load virtual python enviroment for this project")
        execute_process(COMMAND source activate WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/.venv/bin)

        # Change Python interpreter to use the virtual environment
        set(ENV{VIRTUAL_ENV} "${CMAKE_SOURCE_DIR}/.venv")
        set(Python3_FIND_VIRTUALENV FIRST)
        unset(Python3_EXECUTABLE)
        find_package(
            Python3
            COMPONENTS Interpreter
            REQUIRED)

        if(Python3_FOUND)
            message(STATUS "[set_python_virtual_enviroment] Python 3 from .venv -> Version: ${Python3_VERSION}")
        else()
            message(FATAL_ERROR "[set_python_virtual_enviroment] Python 3 from .venv not found!")
        endif()

        # Install packages specified in requirements.txt using pip
        execute_process(
            COMMAND pip install -r ./requirements.txt
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE PIP_INSTALL_STDOUT
            ERROR_VARIABLE PIP_INSTALL_STDERR
            RESULT_VARIABLE PIP_INSTALL_RESULT
            OUTPUT_STRIP_TRAILING_WHITESPACE)

        if(PIP_INSTALL_RESULT)
            message(STATUS "[set_python_virtual_enviroment] pip install error is '${PIP_INSTALL_STDOUT}'")
            message(STATUS "[set_python_virtual_enviroment] pip install error1 is '${PIP_INSTALL_STDERR}'")
        else()
            message(STATUS "[set_python_virtual_enviroment] pip install succeeded: '${PIP_INSTALL_STDOUT}'")
        endif()

    endif()

endfunction(set_python_virtual_enviroment)
