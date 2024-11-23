cmake_minimum_required(VERSION 3.15)

# Define the project name
project(SimpleStreamingClient)

# On Windows we want the client to be a Windows (non-console) application
if(MSVC)
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /SUBSYSTEM:WINDOWS")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /SUBSYSTEM:WINDOWS")
endif()

# Define source files
set(SOURCE_FILES
    "main.cpp"
    "SimpleStreamingClient.cpp"
    "SimpleStreamingClientWin.cpp"
    "SimpleStreamingClientLinux.cpp"
    )

# Define header files
set(HEADER_FILES
    "SimpleStreamingClient.h"
    "SimpleStreamingClientWin.h"
    "SimpleStreamingClientLinux.h"
    )


# Add the executable
add_executable(SimpleStreamingClient ${SOURCE_FILES} ${HEADER_FILES})

if(WIN32)
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
endif()

# Include directories
target_include_directories(SimpleStreamingClient PUBLIC
    ../../amf
    ../../amf/amf
    ../../sdk
    ../../
)

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

if(WIN32)
set(PLATFORM_LIBRARIES Xinput.lib)
else()
set(PLATFORM_LIBRARIES X11 evdev Xcursor Xfixes)
endif()

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
