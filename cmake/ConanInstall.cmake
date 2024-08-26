# -*- mode: cmake -*-
#######################################################################################################################
# @file ConanInstall.cmake
#
# @author David A. Haufe (david.haufe90@gmail.com)
# @version 1.0.0
# @date 2024-07-29
#
# @brief This file contains functions use conan as build / packaging tool.
#
# @copyright MIT License
#######################################################################################################################

#######################################################################################################################
# @brief This function runs the conan install command with the specified arguments.
#        It sets the CONAN_INSTALL_STDOUT, CONAN_INSTALL_STDERR, and CONAN_INSTALL_RESULT variables.
#        If the CONAN_INSTALL_RESULT is non-zero, it displays an error message and exits.
#        Otherwise, it displays a success message.
#        It also extracts the generators folder path from the CONAN_INSTALL_STDERR and sets the CONAN_GENERATORS_FOLDER_PATH variable.
#        If the generators folder path is not found, it displays a warning message.
#
# @param [IN] CONANFILE_PY_PATH The path to the conanfile.py file.
# @param [IN] CONAN_PROFILE The conan profile to use.
# @param [IN] CONAN_GENERATORS_PATH The path to the conan generators folder.
#
#######################################################################################################################
function(run_conan_install)
    cmake_parse_arguments(
        RUN_CONAN_INSTALL
        ""
        "CONANFILE_PY_PATH;CONAN_PROFILE;CONAN_GENERATORS_PATH"
        ""
        ${ARGN})

    # Run the conan install command
    execute_process(
        COMMAND conan install ${RUN_CONAN_INSTALL_CONANFILE_PY_PATH} --build=missing
                -pr:h=${RUN_CONAN_INSTALL_CONAN_PROFILE} -pr:b=${RUN_CONAN_INSTALL_CONAN_PROFILE}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE CONAN_INSTALL_STDOUT
        ERROR_VARIABLE CONAN_INSTALL_STDERR
        RESULT_VARIABLE CONAN_INSTALL_RESULT
        OUTPUT_STRIP_TRAILING_WHITESPACE)

    # Check if the conan install command failed
    if(CONAN_INSTALL_RESULT)
        message(STATUS "[run_conan_install] Conan error is '${CONAN_INSTALL_STDERR}'")
        message(FATAL_ERROR "[run_conan_install] Conan install failed with profile ${RUN_CONAN_INSTALL_CONAN_PROFILE}!")
        return()
    else()
        message(STATUS "[run_conan_install] Conan install succeeded with profile ${RUN_CONAN_INSTALL_CONAN_PROFILE}.")
    endif()

    # Extract the path to the conan generators folder from the output
    string(
        REGEX MATCH
              "Generators folder: .*$"
              CONAN_GENERATORS_FOLDER_LINE
              "${CONAN_INSTALL_STDERR}")
    if(CONAN_GENERATORS_FOLDER_LINE)
        string(
            REGEX
            REPLACE "Generators folder: ([^\r\n]*).*$"
                    "\\1"
                    CONAN_GENERATORS_FOLDER_PATH
                    "${CONAN_GENERATORS_FOLDER_LINE}")
        string(
            REGEX
            REPLACE "[^A-Za-z0-9_./\:-]+$"
                    ""
                    CONAN_GENERATORS_FOLDER_PATH
                    "${CONAN_GENERATORS_FOLDER_PATH}")
        message(STATUS "[run_conan_install] Extracted generators folder path: ${CONAN_GENERATORS_FOLDER_PATH}")
        set(${RUN_CONAN_INSTALL_CONAN_GENERATORS_PATH}
            "${CONAN_GENERATORS_FOLDER_PATH}"
            PARENT_SCOPE)
    else()
        message(WARNING "[run_conan_install] Could not find 'Generators folder:' line in Conan output.")
    endif()
endfunction(run_conan_install)

#######################################################################################################################
# @brief This function sets the build environment through the conanbuild.sh script.
#        It searches for the conanbuild.sh script in the specified CONAN_GENERATORS_PATH.
#        If the script is found, it sets the build environment by executing the script.
#        Otherwise, it displays an error message and exits.
#
# @param [IN] CONAN_GENERATORS_PATH The path to the conan generators folder.
#
#######################################################################################################################
function(set_env_through_conanbuild_script)
    cmake_parse_arguments(
        SET_ENV_THROUGH_CONANBUILD_SCRIPT
        ""
        "CONAN_GENERATORS_PATH"
        ""
        ${ARGN})

    set(CONANBUILD_ENV_SCRIPT "${SET_ENV_THROUGH_CONANBUILD_SCRIPT_CONAN_GENERATORS_PATH}/conanbuild.sh")
    message(STATUS "[set_env_through_conanbuild_script] Search for: ${CONANBUILD_ENV_SCRIPT}")

    if(EXISTS "${CONANBUILD_ENV_SCRIPT}")
        message(STATUS "[set_env_through_conanbuild_script] Build environment will be set by ${CONANBUILD_ENV_SCRIPT}")
        execute_process(COMMAND source ${SET_ENV_THROUGH_CONANBUILD_SCRIPT_CONAN_GENERATORS_PATH}
                        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    else()
        message(FATAL_ERROR "[set_env_through_conanbuild_script] Could not find: ${CONANBUILD_ENV_SCRIPT}")
    endif()
endfunction(set_env_through_conanbuild_script)

#######################################################################################################################
# @brief This function finds the conan generator preset in the CMAKE_USER_PRESET_JSON_FILE_PATH.
#        It reads the CMAKE_USER_PRESET_JSON_FILE_PATH and extracts the path to the conan generated cmake build preset.
#        If the path is found, it reads the conan generated cmake build preset and extracts the preset name.
#        It displays the found preset name.
#
# @param [IN] CMAKE_USER_PRESET_JSON_FILE_PATH The path to the CMake user preset JSON file.
# @param [IN / OUT] CONAN_GENERATED_CMAKE_BUILD_PRESET_NAME The variable to store the conan generated cmake build preset name.
#
#######################################################################################################################
function(find_conan_generator_preset)
    cmake_parse_arguments(
        FIND_CONAN_GENERATOR_PRESET
        ""
        "CMAKE_USER_PRESET_JSON_FILE_PATH;CONAN_GENERATED_CMAKE_BUILD_PRESET_NAME"
        ""
        ${ARGN})

    if(EXISTS ${FIND_CONAN_GENERATOR_PRESET_CMAKE_USER_PRESET_JSON_FILE_PATH})
        file(READ ${FIND_CONAN_GENERATOR_PRESET_CMAKE_USER_PRESET_JSON_FILE_PATH} CMAKE_USER_PRESET_JSON)

        string(
            JSON
            CONAN_GENERATED_CMAKE_PRESET_PATH
            GET
            ${CMAKE_USER_PRESET_JSON}
            "include"
            0)
        if(${CONAN_GENERATED_CMAKE_PRESET_PATH} EQUAL "NOTFOUND")
            message(
                WARNING
                    "[find_conan_generator_preset] In ${CMAKE_USER_PRESET}, has non member 'include', so it is not clear which CMake preset should be loaded!"
            )
            return()
        else()
            file(READ ${CONAN_GENERATED_CMAKE_PRESET_PATH} CONAN_GENERATED_CMAKE_PRESET_JSON)

            string(
                JSON
                CONAN_GENERATED_CMAKE_BUILD_PRESETS_JSON_STR
                GET
                ${CONAN_GENERATED_CMAKE_PRESET_JSON}
                "buildPresets"
                0)
            string(
                JSON
                CONAN_GENERATED_CMAKE_BUILD_PRESET_NAME
                GET
                ${CONAN_GENERATED_CMAKE_BUILD_PRESETS_JSON_STR}
                "name")
            message(
                STATUS "[find_conan_generator_preset] Found CMake Preset : ${CONAN_GENERATED_CMAKE_BUILD_PRESET_NAME}")
        endif()

        set(${FIND_CONAN_GENERATOR_PRESET_CONAN_GENERATED_CMAKE_BUILD_PRESET_NAME}
            "${CONAN_GENERATED_CMAKE_BUILD_PRESET_NAME}"
            PARENT_SCOPE)
    endif()
endfunction(find_conan_generator_preset)

#######################################################################################################################
# @brief This function creates a custom target to load a CMake preset.
#        It creates a custom target with the specified preset name and adds a custom command to set the preset.
#
# @param [IN] CMAKE_BUILD_PRESET_NAME The name of the CMake build preset to load.
#
#######################################################################################################################
function(create_target_load_cmake_preset)
    cmake_parse_arguments(
        LOAD_CONAN_GENERATOR_PRESET
        ""
        "CMAKE_BUILD_PRESET_NAME"
        ""
        ${ARGN})

    message(
        STATUS "[create_target_load_cmake_preset] load preset : ${LOAD_CONAN_GENERATOR_PRESET_CMAKE_BUILD_PRESET_NAME}")

    add_custom_target(load_preset_${LOAD_CONAN_GENERATOR_PRESET_CMAKE_BUILD_PRESET_NAME}
                      COMMENT "Setting CMake Preset: ${LOAD_CONAN_GENERATOR_PRESET_CMAKE_BUILD_PRESET_NAME}")

    add_custom_command(
        TARGET load_preset_${LOAD_CONAN_GENERATOR_PRESET_CMAKE_BUILD_PRESET_NAME}
        COMMAND ${CMAKE_COMMAND} --preset ${LOAD_CONAN_GENERATOR_PRESET_CMAKE_BUILD_PRESET_NAME}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Set Preset: ${LOAD_CONAN_GENERATOR_PRESET_CMAKE_BUILD_PRESET_NAME}"
        VERBATIM)
endfunction(create_target_load_cmake_preset)

#######################################################################################################################
# @brief This function prepares the build presets by conan.
#        If the CMAKE_USER_PRESET_PATH does not exist, it runs the run_conan_install and set_env_through_conanbuild_script functions.
#        It then finds the conan generator preset in the CMAKE_USER_PRESET_PATH and sets the CONAN_BUILD_PRESET variable.
#
# @param [IN] PACKAGE_CONANFILE_PY_PATH The path to the package conanfile.py file.
# @param [IN] CONAN_BUILD_PROFILE The conan build profile to use.
# @param [IN] CMAKE_USER_PRESET_PATH The path to the CMake user preset.
# @param [IN / OUT] CONAN_BUILD_PRESET The variable to store the conan build preset.
#
#######################################################################################################################
function(prepare_build_presets_by_conan)
    cmake_parse_arguments(
        PREPARE_BUILD_BY_CONAN
        ""
        "PACKAGE_CONANFILE_PY_PATH;CONAN_BUILD_PROFILE;CMAKE_USER_PRESET_PATH;CONAN_BUILD_PRESET"
        ""
        ${ARGN})

    if(NOT EXISTS ${PREPARE_BUILD_BY_CONAN_CMAKE_USER_PRESET_PATH})
        set(CONAN_GENERATORS_FOLDER_PATH)
        run_conan_install(
            CONANFILE_PY_PATH
            "${PREPARE_BUILD_BY_CONAN_PACKAGE_CONANFILE_PY_PATH}"
            CONAN_PROFILE
            ${PREPARE_BUILD_BY_CONAN_CONAN_BUILD_PROFILE}
            CONAN_GENERATORS_PATH
            CONAN_GENERATORS_FOLDER_PATH)

        set_env_through_conanbuild_script(CONAN_GENERATORS_PATH ${CONAN_GENERATORS_FOLDER_PATH})
    endif()

    find_conan_generator_preset(
        CMAKE_USER_PRESET_JSON_FILE_PATH
        ${PREPARE_BUILD_BY_CONAN_CMAKE_USER_PRESET_PATH}
        CONAN_GENERATED_CMAKE_BUILD_PRESET_NAME
        CONAN_BUILDED_CMAKE_PRESET)

    set(${PREPARE_BUILD_BY_CONAN_CONAN_BUILD_PRESET}
        "${CONAN_BUILDED_CMAKE_PRESET}"
        PARENT_SCOPE)
endfunction(prepare_build_presets_by_conan)
