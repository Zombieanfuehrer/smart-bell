cmake_minimum_required(VERSION 3.20)

project(
    ATmega328__T_
    VERSION 1.0.0
    LANGUAGES CXX)

if(DEFINED CONAN_PROFILE_NAME)
    set(CONAN_PROFILE_NAME ${CONAN_PROFILE_NAME} CACHE STRING "Name of the conan profile to use")
else()
    set(CONAN_PROFILE_NAME "default" CACHE STRING "Name of the conan profile to use")
endif()
message(STATUS "CONAN_PROFILE_NAME ${CONAN_PROFILE_NAME}")

if(NOT DEFINED F_CPU)
    set(F_CPU "16000000UL")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CONAN_C_FLAGS} -DF_CPU=${F_CPU}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CONAN_CXX_FLAGS} -DF_CPU=${F_CPU}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CONAN_EXE_LINKER_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${CONAN_SHARED_LINKER_FLAGS}")

# GLOBAL VARIABLES
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# OPTIONS
option(ENABLE_WARNINGS "Enable to add warnings to a target." ON)
option(ENABLE_WARNINGS_AS_ERRORS "Enable to treat warnings as errors." OFF)
option(ENABLE_CPPLINT "Enable to add cpplint, static code analysis target" ON)
option(ENABLE_CMAKE_FORMAT "Enable to add cmake-format target." ON)
option(ENABLE_DOXYGEN_DOCU "Enable to add doxygen docu target." ON)

# PROJECT / LIBRARY NAMES
set(LIBRARY_NAME "ATmega328__T_LIB")
set(EXECUTABLE_NAME "ATmega328__T_")

# CMAKE MODULES
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/")

add_subdirectory(src)
add_subdirectory(app)

include(PythonVEnv)
set_python_virtual_enviroment()

if(${ENABLE_CPPLINT})
    include(CppLinter)
    add_cpplint_custom_target()
endif()

if(${ENABLE_CMAKE_FORMAT})
    include(CMakeFormat)
    add_cmake_format_target()
endif()

if(${ENABLE_DOXYGEN_DOCU})
    include(DoxygenDocu)
endif()

include(ConanInstall)
set(CMAKE_USER_PRESET "${PROJECT_SOURCE_DIR}/CMakeUserPresets.json")

prepare_build_presets_by_conan(
    PACKAGE_CONANFILE_PY_PATH
    "${PROJECT_SOURCE_DIR}/conanfile.py"
    CONAN_BUILD_PROFILE
    ${CONAN_PROFILE_NAME}
    CMAKE_USER_PRESET_PATH
    ${CMAKE_USER_PRESET}
    CONAN_BUILD_PRESET
    CONAN_GENERATED_CMAKE_PRESET)

if(CONAN_GENERATED_CMAKE_PRESET)
    create_target_load_cmake_preset(CMAKE_BUILD_PRESET_NAME ${CONAN_GENERATED_CMAKE_PRESET})
else()
    message(FATAL_ERROR "No valid CMake preset found!")
endif()

# Include the install commands
include(Install)
install_target(${LIBRARY_NAME})
install_target(${EXECUTABLE_NAME})
