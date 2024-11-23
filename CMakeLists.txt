cmake_minimum_required(VERSION 3.15)

# Define the project name
project(StreamingSDK)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(MSVC)
    # Set common compile flags for MSVC
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive- /W4 /std:c++latest /WX /FC /DUNICODE /D_UNICODE /nologo")

    # Ensure runtime library settings and flags are consistent across the entire project when using MSVC
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd /ZI /JMC /sdl")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /Zi /Oi /GL /Gy")

    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    # Set global link and lib options for Debug and Release configurations
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG /NOLOGO")
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /DEBUG /NOLOGO")
    set(CMAKE_STATIC_LINKER_FLAGS_DEBUG "${CMAKE_STATIC_LINKER_FLAGS_DEBUG} /NOLOGO")

    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /INCREMENTAL:NO /OPT:REF /OPT:ICF /NOLOGO")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG /INCREMENTAL:NO /OPT:REF /OPT:ICF /NOLOGO")
    set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE} /LTCG /NOLOGO")

#elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # # compiler error checking flags for linux
    #-Wno-register   \
    #-Werror \
    #-Wall \
    #-Wextra \
    #-Wno-unknown-pragmas \
    #-Wno-reorder \
    #-Wno-unused \
    #-Wno-switch \
    #-Wno-sign-compare \
    #-Wno-missing-field-initializers \
    #-Wno-nonnull \
    #-Wno-overloaded-virtual \
endif()

if(WIN32)
    set(PROGRAM_FILES_X86 "$ENV{ProgramFiles\(x86\)}")
    set(WINDOWS_SDK_VERSION "10.0.26100.0")
    set(WINDOWS_SDK_INCLUDE_BASE "${PROGRAM_FILES_X86}/Windows Kits/10/Include/${WINDOWS_SDK_VERSION}")
    set(WINDOWS_SDK_INCLUDE
        "${WINDOWS_SDK_INCLUDE_BASE}/ucrt"
        "${WINDOWS_SDK_INCLUDE_BASE}/um"
        "${WINDOWS_SDK_INCLUDE_BASE}/shared"
        "${WINDOWS_SDK_INCLUDE_BASE}/winrt"
        "${WINDOWS_SDK_INCLUDE_BASE}/cppwinrt"
        "${WINDOWS_SDK_INCLUDE_BASE}/um"
        )
    set(WINDOWS_SDK_BIN "${PROGRAM_FILES_X86}/Windows Kits/10/bin/${WINDOWS_SDK_VERSION}/x64")
    set(OCL_ROOT "$ENV{OCL_ROOT}")
    set(VK_SDK_PATH "$ENV{VK_SDK_PATH}")
    include_directories(${WINDOWS_SDK_INCLUDE} $ENV{VK_SDK_PATH}/include)
    include_directories(${OCL_ROOT}/include)
endif()


# Set output directories for all build configurations
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/Debug")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/Debug")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/Debug")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/Release")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/Release")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/Release")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/bin/RelWithDebInfo")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/bin/RelWithDebInfo")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/bin/RelWithDebInfo")

# Solution configurations
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "" FORCE)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(SOURCE_FILES "")
set(HEADER_FILES "")

# Add subdirectories for each project
set(SUBFOLDER_NET sdk/net)
set(SUBFOLDER_UTIL sdk/util)
set(SUBFOLDER_TRANSPORT_COMMON sdk/transports/transport-common)
set(SUBFOLDER_TRANSPORT_AMD sdk/transports/transport-amd)
set(SUBFOLDER_VIDEO sdk/video)
set(SUBFOLDER_AUDIO sdk/audio)
set(SUBFOLDER_CONTROLLERS sdk/controllers)

add_subdirectory(amf-helper-libs/amf-public)
add_subdirectory(amf-helper-libs/amf-component-ffmpeg64)
add_subdirectory(${SUBFOLDER_NET})
add_subdirectory(${SUBFOLDER_UTIL})
add_subdirectory(${SUBFOLDER_TRANSPORT_COMMON})
add_subdirectory(${SUBFOLDER_TRANSPORT_AMD})
add_subdirectory(${SUBFOLDER_VIDEO})
add_subdirectory(${SUBFOLDER_AUDIO})
add_subdirectory(${SUBFOLDER_CONTROLLERS})
add_subdirectory(mbedtls-custom)
add_subdirectory(samples/SimpleStreamingClient)
add_subdirectory(samples/RemoteDesktopServer)

# Add the library
add_library(ssdk STATIC ${SOURCE_FILES} ${HEADER_FILES})

# Include directories
target_include_directories(ssdk PRIVATE
    ./
    sdk
    amf
    amf/amf
    mbedtls/include
)

# Compile definitions
target_compile_definitions(ssdk PRIVATE
    $<$<CONFIG:Debug>:_DEBUG;_LIB>
    $<$<CONFIG:Release>:NDEBUG;_LIB>
)

# Project dependencies
add_dependencies(SimpleStreamingClient ssdk amf-public amf-component-ffmpeg64 mbedtls-custom)
add_dependencies(RemoteDesktopServer ssdk amf-public amf-component-ffmpeg64 mbedtls-custom)

# Optionally, group targets into folders in the solution
set_target_properties(ssdk PROPERTIES FOLDER "libs")
set_target_properties(mbedtls-custom PROPERTIES FOLDER "libs")
set_target_properties(SimpleStreamingClient PROPERTIES FOLDER "samples")
set_target_properties(RemoteDesktopServer PROPERTIES FOLDER "samples")
