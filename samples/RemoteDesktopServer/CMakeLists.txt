cmake_minimum_required(VERSION 3.15)

# Define the project name
project(RemoteDesktopServer)

if(WIN32)

# On Windows we want the client to be a Windows (non-console) application
if(MSVC)
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /SUBSYSTEM:WINDOWS")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /SUBSYSTEM:WINDOWS")
endif()


# Define source files
set(SOURCE_FILES
    "main.cpp"
    "RemoteDesktopServer.cpp"
    "AVStreamer.cpp"
    "RemoteDesktopServerWin.cpp"
 )

# Define header files
set(HEADER_FILES
    "RemoteDesktopServer.h"
    "AVStreamer.h"
    "RemoteDesktopServerWin.h"
 )

# Add the executable
add_executable(RemoteDesktopServer ${SOURCE_FILES} ${HEADER_FILES})

# Define the path for the manifest file
set(MANIFEST_FILE "app.manifest")
get_filename_component(MANIFEST_FILE_ABS ${MANIFEST_FILE} ABSOLUTE)
# Add a custom command to embed the manifest file into the executable
add_custom_command(
    TARGET RemoteDesktopServer
    POST_BUILD
    COMMAND ${WINDOWS_SDK_BIN}/mt.exe -manifest ${MANIFEST_FILE_ABS} -outputresource:$<TARGET_FILE:RemoteDesktopServer>\\;1
    COMMENT "Embedding manifest file into RemoteDesktopServer"
    VERBATIM
)

# Include directories
target_include_directories(RemoteDesktopServer PUBLIC
    ../../amf
    ../../amf/amf
    ../../sdk
    ../../
)

# Compile definitions
target_compile_definitions(RemoteDesktopServer PRIVATE
    $<$<CONFIG:Debug>:_DEBUG;_CONSOLE>
    $<$<CONFIG:Release>:NDEBUG;_CONSOLE>
)

# Link libraries
set(LIBRARIES
    ssdk
    amf-public
)

set(PLATFORM_LIBRARIES Xinput.lib)

add_dependencies(RemoteDesktopServer ssdk amf-public amf-component-ffmpeg64 mbedtls-custom)

target_link_libraries(RemoteDesktopServer PRIVATE
    ${LIBRARIES}
    ${PLATFORM_LIBRARIES}
)

# Additional link directories
target_link_directories(RemoteDesktopServer PRIVATE
    $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/bin/Debug>
    $<$<CONFIG:Release>:${CMAKE_SOURCE_DIR}/bin/Release>
)

# Link mbedTLS library
target_link_libraries(RemoteDesktopServer PRIVATE
    mbedtls-custom
)
endif()
