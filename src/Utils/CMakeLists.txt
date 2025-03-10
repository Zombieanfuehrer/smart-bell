set(LIB_UTILS_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/CircularBuffer.cpp")
set(LIB_UTILS_HEADERS "${PROJECT_SOURCE_DIR}/public/Utils/CircularBuffer.h")

add_library("${LIB_UTILS}" STATIC ${LIB_UTILS_SOURCE} ${LIB_UTILS_HEADERS})
target_include_directories("${LIB_UTILS}" PUBLIC ${LIBRARY_INCLUDES})
