# Define the output directories for libraries and executables
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_INCLUDE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/public)

function(install_target target_name)
    install(
        TARGETS ${target_name}
        RUNTIME DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        LIBRARY DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        ARCHIVE DESTINATION ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
endfunction()

# Install header files
install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/public/
    DESTINATION ${CMAKE_INCLUDE_OUTPUT_DIRECTORY}
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp")
