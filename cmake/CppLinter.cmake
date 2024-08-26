# -*- mode: cmake -*-
#######################################################################################################################
# @file CppLinter.cmake
#
# @author David A. Haufe (david.haufe90@gmail.com)
# @version 1.0.0
# @date 2024-07-29
#
# @brief This file contains functions to parse and process the cpplint configuration.
#
# @copyright MIT License
#######################################################################################################################

# List of cpplint configuration options
set(CPPLINT_CONFIG
    "verbose" # Set the verbosity level of the output
    "output" # Specify the output format
    "filter" # Filters to apply during linting
    "counting" # Type of counting output (e.g., detailed)
    "repository" # Repository root for source files
    "root" # Root directory for linting
    "linelength" # Maximum line length
    "recursive" # Whether to lint recursively
    "exclude" # Files or directories to exclude
    "extensions" # File extensions to consider
    "headers" # Header file extensions
)

# List of cpplint filter types
set(CPPLINT_FILTER_TYPES
    "build" # Filters related to build issues
    "legal" # Filters related to legal issues (e.g., copyright)
    "readability" # Filters related to code readability
    "runtime" # Filters related to runtime issues
    "whitespace" # Filters related to whitespace formatting
)

#######################################################################################################################
# @brief Function extracts the contents of the passed array (ARRAY_TYPENAME) from the given JSON stream
#        (CPPLINT_CFG_JSON_CONTENT) and appends them to FILTER_LIST.
#
# @param [IN] CPPLINT_CFG_JSON_CONTENT The content of the cpplint configuration JSON.
# @param [IN] ARRAY_TYPENAME The type of the filter array (e.g., "build", "readability").
# @param [IN / OUT] FILTER_LIST The list to store / append the extracted filter entries.
#
#######################################################################################################################
function(get_linter_filter_array)
    set(oneValueArgs CPPLINT_CFG_JSON_CONTENT ARRAY_TYPENAME FILTER_LIST)
    cmake_parse_arguments(
        GET_LINTER_FILTER_ARRAY
        ""
        "${oneValueArgs}"
        ""
        ${ARGN})

    string(
        JSON
        VALUE
        GET
        ${GET_LINTER_FILTER_ARRAY_CPPLINT_CFG_JSON_CONTENT}
        "filter")

    set(CURRENT_FILTER_LIST)
    if(NOT
       "${VALUE}"
       STREQUAL
       "")
        set(SEARCH_FOR_FILTER_ARRAY_ENTRYS_REGEX "[+-]${GET_LINTER_FILTER_ARRAY_ARRAY_TYPENAME}\/[a-zA-Z0-9_+]+")
        string(
            REGEX MATCHALL
                  "${SEARCH_FOR_FILTER_ARRAY_ENTRYS_REGEX}"
                  CPPLINT_CONFIG_FILTER_ENTRIES
                  ${VALUE})
        list(APPEND CURRENT_FILTER_LIST ${CPPLINT_CONFIG_FILTER_ENTRIES})
        set(${GET_LINTER_FILTER_ARRAY_FILTER_LIST}
            "${CURRENT_FILTER_LIST}"
            PARENT_SCOPE)
    else()
        message(STATUS "[get_linter_filter_array] no valid 'filter' found in cpplint-config.json file!")
    endif()
endfunction(get_linter_filter_array)

#######################################################################################################################
# @brief Function reads the passed cpplint_config.json (CPPLINT_CONFIG_JSON_PATH) and writes the options found
#        into the passed list (CPPLINT_SET_FILTER_OPTIONS).
#
# @param [IN] CPPLINT_CONFIG_JSON_PATH The path to the cpplint configuration JSON file.
# @param [IN / OUT] FILTER_LIST The list to store the set options.
#
#######################################################################################################################
function(parse_cpplint_config)
    set(oneValueArgs CPPLINT_CONFIG_JSON_PATH CPPLINT_SET_FILTER_OPTIONS)
    cmake_parse_arguments(
        PARSE_CPPLINT_CONFIG
        ""
        "${oneValueArgs}"
        ""
        ${ARGN})

    message(
        STATUS
            "[parse_cpplint_config] search for cpplint configuration at : ${PARSE_CPPLINT_CONFIG_CPPLINT_CONFIG_JSON_PATH}"
    )
    file(READ "${PARSE_CPPLINT_CONFIG_CPPLINT_CONFIG_JSON_PATH}" CPPLINT_CONFIG_JSON_CONTENT)

    set(UPDATED_CPPLINT_CONFIG)
    foreach(CPP_CONFIGURATION_OPTION IN LISTS CPPLINT_CONFIG)
        # Handle filter arrays specially
        if(CPP_CONFIGURATION_OPTION MATCHES "^filter")
            list(APPEND UPDATED_CPPLINT_CONFIG "--filter=")
            foreach(CPP_LINTER_FILTER_ARRAY IN LISTS CPPLINT_FILTER_TYPES)
                set(UPDATED_CPPLINT_CONFIG_TMP "")
                get_linter_filter_array(
                    CPPLINT_CFG_JSON_CONTENT
                    ${CPPLINT_CONFIG_JSON_CONTENT}
                    ARRAY_TYPENAME
                    ${CPP_LINTER_FILTER_ARRAY}
                    FILTER_LIST
                    UPDATED_CPPLINT_CONFIG_TMP)
                list(APPEND UPDATED_CPPLINT_CONFIG "${UPDATED_CPPLINT_CONFIG_TMP}")
            endforeach()
        else()
            string(
                JSON
                VALUE
                GET
                ${CPPLINT_CONFIG_JSON_CONTENT}
                ${CPP_CONFIGURATION_OPTION})

            if(NOT
               "${VALUE}"
               STREQUAL
               "")
                list(APPEND UPDATED_CPPLINT_CONFIG "--${CPP_CONFIGURATION_OPTION}=${VALUE}")
            else()
                message(STATUS "[parse_cpplint_config] ${CPP_CONFIGURATION_OPTION}: No valid value found!")
            endif()
        endif()
    endforeach()
    set(${PARSE_CPPLINT_CONFIG_CPPLINT_SET_FILTER_OPTIONS}
        "${UPDATED_CPPLINT_CONFIG}"
        PARENT_SCOPE)
endfunction(parse_cpplint_config)

#######################################################################################################################
# @brief Function to add a custom target for running cpplint analysis, to do this, the style/cpplint_config.json
#        is evaluated and the corresponding options are transferred to cpplint.
#        addition, all relevant C++ files are searched for and passed to cpplint for checking.
#
#######################################################################################################################
function(add_cpplint_custom_target)
    # Find the cpplint executable
    find_program(CPPLINT "cpplint")
    if(NOT CPPLINT)
        message(FATAL_ERROR "[add_cpplint_custom_target] cpplint not found! Please install to use this option")
    else()
        set(CMAKE_CXX_CPPLINT ${CPPLINT})
    endif()

    # Set the path to the cpplint configuration file
    set(CPPLINT_CONFIG_JSON "${PROJECT_SOURCE_DIR}/style/cpplint_config.json")

    # Parse the cpplint configuration file and get the filter options
    set(USED_CPPLINT_OPTIONS)
    parse_cpplint_config(
        CPPLINT_CONFIG_JSON_PATH
        ${CPPLINT_CONFIG_JSON}
        CPPLINT_SET_FILTER_OPTIONS
        USED_CPPLINT_OPTIONS)

    # Prepare the list of cpplint options for the command line
    string(
        REPLACE ";"
                " "
                USED_CPPLINT_OPTIONS_STR
                "${USED_CPPLINT_OPTIONS}")
    string(
        REGEX
        REPLACE " "
                ";"
                USED_CPPLINT_OPTIONS_STR
                "${USED_CPPLINT_OPTIONS_STR}")

    # Get the list of C++ files in the source directory
    file(
        GLOB_RECURSE
        CXX_FILES
        "${CMAKE_SOURCE_DIR}/*.cpp"
        "${CMAKE_SOURCE_DIR}/*.cc"
        "${CMAKE_SOURCE_DIR}/*.cxx")

    # Exclude certain files from the list
    list(
        FILTER
        CXX_FILES
        EXCLUDE
        REGEX
        "${CMAKE_SOURCE_DIR}/(build|external|.venv|CMakeFiles)/.*")

    # Print the list of files being analyzed by cpplint
    message(STATUS "[add_cpplint_custom_target] add c++ files to cpplint analysis")
    foreach(CXX_FILE_CPPLINT_CMD ${CXX_FILES})
        message(STATUS "[add_cpplint_custom_target] add '${CXX_FILE_CPPLINT_CMD}' to cpplint static code analyse")
    endforeach()

    # Prepare the cpplint command line
    set(CPPLINT_CLI_CMD ${USED_CPPLINT_OPTIONS_STR})
    list(APPEND CPPLINT_CLI_CMD ${CXX_FILES})

    # Set up the custom target for running cpplint
    message(STATUS "[add_cpplint_custom_target] cpplint set up with following options: ${CPPLINT_CLI_CMD}")
    add_custom_target(
        run_cpplint ALL
        COMMAND ${CPPLINT} ${CPPLINT_CLI_CMD}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "run cpplint on cpp source install_files"
        VERBATIM)
endfunction(add_cpplint_custom_target)
