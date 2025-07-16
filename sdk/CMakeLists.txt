cmake_minimum_required(VERSION 3.15)

# Define the project name
project(ssdk)

set(SOURCE_FILES)
set(HEADER_FILES)

add_subdirectory("net")
add_subdirectory("util")
add_subdirectory("transports/transport-common")
add_subdirectory("transports/transport-amd")
add_subdirectory("video")
add_subdirectory("audio")
add_subdirectory("controllers")

# Add the library
add_library(ssdk STATIC ${SOURCE_FILES} ${HEADER_FILES})

# Include directories
set(SSDK_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/./"
    "${CMAKE_CURRENT_SOURCE_DIR}/../"
    "${CMAKE_CURRENT_SOURCE_DIR}/../amf"
    "${CMAKE_CURRENT_SOURCE_DIR}/../amf/amf"
    "${CMAKE_CURRENT_SOURCE_DIR}/../mbedtls/include"
)
if(UNIX)
    set(SSDK_INCLUDE_DIRS
        ${SSDK_INCLUDE_DIRS}
        /usr/include/libevdev-1.0/
    )
endif()
target_include_directories(ssdk PRIVATE ${SSDK_INCLUDE_DIRS})

# Compile definitions
target_compile_definitions(ssdk PRIVATE
    $<$<CONFIG:Debug>:_DEBUG;_LIB>
    $<$<CONFIG:Release>:NDEBUG;_LIB>
)

