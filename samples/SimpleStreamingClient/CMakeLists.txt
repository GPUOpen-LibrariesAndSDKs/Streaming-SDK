cmake_minimum_required(VERSION 3.15)

# Define the project name
project(SimpleStreamingClient)

# On Windows we want the client to be a Windows (non-console) application
if(MSVC)
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /SUBSYSTEM:WINDOWS")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /SUBSYSTEM:WINDOWS")
endif()

if(WIN32)
    set(AMFLITE_LIB "amfrtlt64.dll")
    set(AMFLITE_LIB_LOCATION "${CMAKE_SOURCE_DIR}/prebuilt/Windows/AMD64")
elseif(UNIX)
    set(AMFLITE_LIB "libamfrtlt64.so.1.4.36")
    set(AMFLITE_SYMLINK "libamfrtlt64.so.1")
    set(AMFLITE_LIB_LOCATION "${CMAKE_SOURCE_DIR}/prebuilt/Linux/amd64")
endif()

# Define source files
set(SOURCE_FILES
    "main.cpp"
    "SimpleStreamingClient.cpp"
)

# Define header files
set(HEADER_FILES
    "SimpleStreamingClient.h"
)

# Add platform-specific files:
if(WIN32)
    set(SOURCE_FILES ${SOURCE_FILES}
        "SimpleStreamingClientWin.cpp"
    )

    set(HEADER_FILES ${HEADER_FILES}
        "SimpleStreamingClientWin.h"
    )
elseif(UNIX)
    set(SOURCE_FILES ${SOURCE_FILES}
        "SimpleStreamingClientLinux.cpp"
    )

    set(HEADER_FILES ${HEADER_FILES}
        "SimpleStreamingClientLinux.h"
    )
endif()


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
set(SSDK_INCLUDE_DIRS
    "../../amf"
    "../../amf/amf"
    "../../sdk"
    "../../"
)
if(UNIX)
    set(SSDK_INCLUDE_DIRS
        ${SSDK_INCLUDE_DIRS}
    )
endif()
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

if(WIN32)
    set(PLATFORM_LIBRARIES Xinput.lib)
elseif(UNIX)
    set(PLATFORM_LIBRARIES Xi X11 evdev asound Xcursor Xfixes)
endif()

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

if(DEFINED AMFLITE_LIB AND NOT "${AMFLITE_LIB}" STREQUAL "" AND EXISTS ${AMFLITE_LIB_LOCATION}/${AMFLITE_LIB})
    if(WIN32)
        add_custom_command(
            TARGET SimpleStreamingClient
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${AMFLITE_LIB_LOCATION}/${AMFLITE_LIB}
                ${OUTPUT_DIRECTORY}/${AMFLITE_LIB}
            COMMENT "Copying ${AMFLITE_LIB_LOCATION}/${AMFLITE_LIB} to ${OUTPUT_DIRECTORY}/${AMFLITE_LIB}"
        )
    elseif(UNIX)
        add_custom_command(
            TARGET SimpleStreamingClient
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                ${AMFLITE_LIB_LOCATION}/${AMFLITE_LIB}
                ${OUTPUT_DIRECTORY}/${AMFLITE_SYMLINK}
            COMMENT "Linking ${OUTPUT_DIRECTORY}/${AMFLITE_SYMLINK} to ${AMFLITE_LIB_LOCATION}/${AMFLITE_LIB}"
        )
    endif()
endif()

