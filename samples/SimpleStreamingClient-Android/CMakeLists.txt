
# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

# Declares the project name. The project name can be accessed via ${ PROJECT_NAME},
# Since this is the top level CMakeLists.txt, the project name is also accessible
# with ${CMAKE_PROJECT_NAME} (both CMake variables are in-sync within the top level
# build script scope).
project("SimpleStreamingClient")

message(STATUS, "Buiding the sample ${CMAKE_PROJECT_NAME} ${PROJECT_NAME} ")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
#
# In this top level CMakeLists.txt, ${CMAKE_PROJECT_NAME} is used to define
# the target library name; in the sub-module's CMakeLists.txt, ${PROJECT_NAME}
# is preferred for the same purpose.
#
# In order to load a library into your app from Java/Kotlin, you must call
# System.loadLibrary() and pass the name of the library defined here;
# for GameActivity/NativeActivity derived applications, the same library name must be
# used in the AndroidManifest.xml file.

set(SSDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(SSDK_BIN_PATH ${SSDK_DIR}/bin/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}/${CMAKE_BUILD_TYPE})

# add_dependencies(${CMAKE_PROJECT_NAME} amf-public)
# Specifies libraries CMake should link to your target library. You
# can link libraries from various origins, such as libraries defined in this
# build script, prebuilt third-party libraries, or Android system libraries.
set(SSDK_LIBRARIES
    "${SSDK_BIN_PATH}/libamf-public.a"
    "${SSDK_BIN_PATH}/libmbedtls-custom.a"
    "${SSDK_BIN_PATH}/libssdk.a"
)

set(SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/app/src/main/cpp/SimpleStreamingClient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/app/src/main/cpp/StreamingActivity.cpp
)

set(HEADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/app/src/main/cpp/SimpleStreamingClient.h
)

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${HEADER_FILES})

add_dependencies(${PROJECT_NAME} amf-public mbedtls-custom ssdk)
# Include directories
set(SSDK_INCLUDE_DIRS
    "${SSDK_DIR}/amf"
    "${SSDK_DIR}/amf/amf"
    "${SSDK_DIR}/sdk"
    "${SSDK_DIR}"
)
target_include_directories(${PROJECT_NAME} PUBLIC ${SSDK_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PRIVATE
    # List libraries link to the target library
    android
    log
    aaudio
    jnigraphics
    GLESv2
    EGL
    ${SSDK_LIBRARIES}
)
