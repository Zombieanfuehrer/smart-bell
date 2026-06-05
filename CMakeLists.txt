cmake_minimum_required(VERSION 3.20)

project(
    ATmega328__T_
    VERSION 1.0.0
    LANGUAGES C CXX)

# --- CONAN SETUP ---
if(NOT DEFINED CONAN_PROFILE_NAME)
    set(CONAN_PROFILE_NAME "default" CACHE STRING "Name of the conan profile to use")
endif()
message(STATUS "CONAN_PROFILE_NAME: ${CONAN_PROFILE_NAME}")

# --- GLOBAL VARIABLES & STANDARDS ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- HARDWARE CONFIGURATION ---
if(NOT DEFINED F_CPU)
    set(F_CPU "16000000UL")
endif()
add_compile_definitions(F_CPU=${F_CPU} __AVR__)

option(USE_SPI_BUFFERED "Nutze gepuffertes und Interrupt-basiertes SPI" OFF)
if(USE_SPI_BUFFERED)
    add_compile_definitions(USE_SPI_BUFFERED)
endif()

# --- OPTIONS ---
option(ENABLE_WARNINGS "Enable to add warnings to a target." ON)
option(ENABLE_WARNINGS_AS_ERRORS "Enable to treat warnings as errors." OFF)
option(ENABLE_CPPLINT "Enable to add cpplint, static code analysis target" ON)
option(ENABLE_CMAKE_FORMAT "Enable to add cmake-format target." ON)
option(ENABLE_DOXYGEN_DOCU "Enable to add doxygen docu target." ON)
option(ENABLE_UNIT_TESTS "Enable to run unit tests for logical part of this project sources" OFF)
option(ENABLE_EXAMPLES "Enable to build example targets" OFF)

# --- PROJECT / LIBRARY NAMES ---
set(LIB_WTD "ATmega328_WTD")
set(LIB_PININT "ATmega328_PIN_INT")
set(LIB_TIMERINT "ATmega328_TIMER_INT")
set(LIB_USART "ATmega328_USART")
set(LIB_SPI "ATmega328_SPI")
set(LIB_UTILS "ATmega328_UTILS")
set(LIB_W5500_ETHERNET "ATmega328_W5500_ETHERNET")
set(LIB_WIZNET_IOLIBRARY "ATmega328_W5500_ETHERNET_WIZNET_IOLIBRARY")
set(LIB_MQTT "ATmega328_MQTT")
set(LIB_CONFIG "ATmega328_CONFIG")
set(LIB_TIMER_SERVICE "ATmega328_TIMER_SERVICE")
set(LIB_IOLIBRARY_INTERNET "ATmega328_IOLIBRARY_INTERNET")
set(LIB_NETWORK "ATmega328_NETWORK")
set(LIB_APP "ATmega328_APP")

set(EXECUTABLE_UART_EXAMPLE "ATmega328_UART_EXAMPLE_FW")
set(EXECUTABLE_SMART_BELL "ATmega328_SMART_BELL_FW")

set(IOLIBRARY_INTERNET_DIR "${PROJECT_SOURCE_DIR}/extern/ioLibrary_Driver/ioLibrary_Driver-3.2.0/Internet")

# --- CMAKE MODULES ---
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/")

include(PythonVEnv)
set_python_virtual_enviroment()

if(ENABLE_CPPLINT)
    include(CppLinter)
    add_cpplint_custom_target()
endif()

if(ENABLE_CMAKE_FORMAT)
    include(CMakeFormat)
    add_cmake_format_target()
endif()

if(ENABLE_DOXYGEN_DOCU)
    include(DoxygenDocu)
endif()

include(ConanInstall)
set(CMAKE_USER_PRESET "${PROJECT_SOURCE_DIR}/CMakeUserPresets.json")

prepare_build_presets_by_conan(
    PACKAGE_CONANFILE_PY_PATH "${PROJECT_SOURCE_DIR}/conanfile.py"
    CONAN_BUILD_PROFILE ${CONAN_PROFILE_NAME}
    CMAKE_USER_PRESET_PATH ${CMAKE_USER_PRESET}
    CONAN_BUILD_PRESET CONAN_GENERATED_CMAKE_PRESET)

if(CONAN_GENERATED_CMAKE_PRESET)
    create_target_load_cmake_preset(CMAKE_BUILD_PRESET_NAME ${CONAN_GENERATED_CMAKE_PRESET})
else()
    message(FATAL_ERROR "No valid CMake preset found!")
endif()

# --- AVR SPECIFIC TOOLCHAIN SETTINGS ---
if(CONAN_PROFILE_NAME STREQUAL "avr-mega328p")
    message(STATUS "AVR target detected: Disabling LTO (Interprocedural Optimization)")
    
    # Der saubere Weg: CMake sagt dem Compiler explizit, kein LTO zu nutzen.
    # Dies ist besser, als die Strings in CMAKE_C_FLAGS zu manipulieren.
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE OFF)
endif()

# --- SUBDIRECTORIES ---
add_subdirectory(src)
add_subdirectory(app)

# --- TESTS ---
if(ENABLE_UNIT_TESTS)
    find_package(GTest REQUIRED)
    include_directories(${GTEST_INCLUDE_DIRS})
    set(EXECUTABLE_UNIT_TEST "run_gTest")
    enable_testing()
    add_subdirectory("${CMAKE_SOURCE_DIR}/tests")
endif()

# --- INSTALLATION & POST-BUILD ---
include(Install)
install_target(${LIB_WTD})
install_target(${LIB_PININT})
install_target(${LIB_TIMERINT})
install_target(${LIB_UTILS})

if(NOT ENABLE_UNIT_TESTS)
    install_target(${LIB_USART})
    install_target(${LIB_SPI})
    install_target(${LIB_W5500_ETHERNET})
    install_target(${LIB_MQTT})
    install_target(${LIB_CONFIG})
    install_target(${LIB_IOLIBRARY_INTERNET})
    install_target(${EXECUTABLE_SMART_BELL})
    
    include(Objcopy)

    run_bin2hex(
        TARGET ${EXECUTABLE_SMART_BELL}
        TARGET_ARTIFACT ${CMAKE_BINARY_DIR}/app/${EXECUTABLE_SMART_BELL}
        HEX_FILE ${CMAKE_BINARY_DIR}/${EXECUTABLE_SMART_BELL}.hex)

    if(ENABLE_EXAMPLES)
        message(STATUS "[CMake] Enabling Hex generation for UART Example Target")
        run_bin2hex(
            TARGET ${EXECUTABLE_UART_EXAMPLE}
            TARGET_ARTIFACT ${CMAKE_BINARY_DIR}/app/examples/${EXECUTABLE_UART_EXAMPLE}
            HEX_FILE ${CMAKE_BINARY_DIR}/${EXECUTABLE_UART_EXAMPLE}.hex)
    endif()

    find_program(BLOATY_CMD bloaty)
    if(NOT BLOATY_CMD AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/bloaty/build/bloaty")
        set(BLOATY_CMD "${CMAKE_CURRENT_SOURCE_DIR}/extern/bloaty/build/bloaty")
    endif()
    
    if(BLOATY_CMD)
        message(STATUS "[CMake] Bloaty gefunden: ${BLOATY_CMD}")
        add_custom_command(
            TARGET ${EXECUTABLE_SMART_BELL}_hex
            POST_BUILD
            COMMAND ${BLOATY_CMD} -d symbols ${CMAKE_BINARY_DIR}/app/${EXECUTABLE_SMART_BELL}
            COMMENT "Analysiere ATmega328P RAM-Belegung..."
        )
    endif()
endif()