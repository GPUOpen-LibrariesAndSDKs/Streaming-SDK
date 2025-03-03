cmake_minimum_required(VERSION 3.15)

# Define the project name
project(SimpleStreamingClient)

if(WIN32)

# On Windows we want the client to be a Windows (non-console) application
if(MSVC)
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /SUBSYSTEM:WINDOWS")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /SUBSYSTEM:WINDOWS")
endif()

set(AMFLITE_LIB "amfrtlt64.dll")
set(AMFLITE_LIB_LOCATION "${CMAKE_SOURCE_DIR}/prebuilt/Windows/AMD64")

# Define source files
set(SOURCE_FILES
    "main.cpp"
    "SimpleStreamingClient.cpp"
    "SimpleStreamingClientWin.cpp"
)

# Define header files
set(HEADER_FILES
    "SimpleStreamingClient.h"
    "SimpleStreamingClientWin.h"
)

# Add the executable
add_executable(SimpleStreamingClient ${SOURCE_FILES} ${HEADER_FILES})

# Define the path for the manifest file
set(MANIFEST_FILE "app.manifest")
get_filename_component(MANIFEST_FILE_ABS ${MANIFEST_FILE} ABSOLUTE)
# Add a custom command to embed the manifest file into the executable
add_custom_command(
    TARGET SimpleStreamingClient
    POST_BUILD
    COMMAND ${WINDOWS_SDK_BIN}/mt.exe -manifest ${MANIFEST_FILE_ABS} -outputresource:$<TARGET_FILE:SimpleStreamingClient>\\;1
    COMMENT "Embedding manifest file into SimpleStreamingClient"
    VERBATIM
)

# Include directories
set(SSDK_INCLUDE_DIRS
    "../../amf"
    "../../amf/amf"
    "../../sdk"
    "../../"
)

target_include_directories(SimpleStreamingClient PUBLIC ${SSDK_INCLUDE_DIRS})

# Compile definitions
target_compile_definitions(SimpleStreamingClient PRIVATE
    $<$<CONFIG:Debug>:_DEBUG;_CONSOLE>
    $<$<CONFIG:Release>:NDEBUG;_CONSOLE>
)

# Link libraries
set(LIBRARIES
    ssdk
    amf-public
)

set(PLATFORM_LIBRARIES Xinput.lib)

add_dependencies(SimpleStreamingClient ssdk amf-public amf-component-ffmpeg64 mbedtls-custom)

target_link_libraries(SimpleStreamingClient PRIVATE
    ${LIBRARIES}
    ${PLATFORM_LIBRARIES}
)

# Additional link directories
target_link_directories(SimpleStreamingClient PRIVATE
    $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/bin/Debug>
    $<$<CONFIG:Release>:${CMAKE_SOURCE_DIR}/bin/Release>
)

# Link mbedTLS library
target_link_libraries(SimpleStreamingClient PRIVATE
    mbedtls-custom
)

set(OUTPUT_DIRECTORY "$<IF:$<CONFIG:Debug>,${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG},$<IF:$<CONFIG:Release>,${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE},$<IF:$<CONFIG:RelWithDebInfo>,${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO},${CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL}>>>")

add_custom_command(
    TARGET SimpleStreamingClient
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${AMFLITE_LIB_LOCATION}/${AMFLITE_LIB}
        ${OUTPUT_DIRECTORY}/${AMFLITE_LIB}
    COMMENT "Copying ${AMFLITE_LIB_LOCATION}/${AMFLITE_LIB} to ${OUTPUT_DIRECTORY}/${AMFLITE_LIB}"
)

endif()
