cmake_minimum_required(VERSION 3.15)

if(POLICY CMP0111)
    cmake_policy(SET CMP0111 OLD)
endif()

# Define the project name
project(amf-component-ffmpeg64)

# Define source files
set(SOURCE_FILES
    "../../amf/amf/public/common/AMFFactory.cpp"
    "../../amf/amf/public/common/AMFSTL.cpp"
    "../../amf/amf/public/common/DataStreamFactory.cpp"
    "../../amf/amf/public/common/DataStreamFile.cpp"
    "../../amf/amf/public/common/DataStreamMemory.cpp"
    "../../amf/amf/public/common/IOCapsImpl.cpp"
    "../../amf/amf/public/common/PropertyStorageExImpl.cpp"
    "../../amf/amf/public/common/Thread.cpp"
    "../../amf/amf/public/common/TraceAdapter.cpp"
    "../../amf/amf/public/src/components/ComponentsFFMPEG/AudioConverterFFMPEGImpl.cpp"
    "../../amf/amf/public/src/components/ComponentsFFMPEG/AudioDecoderFFMPEGImpl.cpp"
    "../../amf/amf/public/src/components/ComponentsFFMPEG/AudioEncoderFFMPEGImpl.cpp"
    "../../amf/amf/public/src/components/ComponentsFFMPEG/AV1EncoderFFMPEGImpl.cpp"
    "../../amf/amf/public/src/components/ComponentsFFMPEG/BaseEncoderFFMPEGImpl.cpp"
    "../../amf/amf/public/src/components/ComponentsFFMPEG/ComponentFactory.cpp"
    "../../amf/amf/public/src/components/ComponentsFFMPEG/FileDemuxerFFMPEGImpl.cpp"
    "../../amf/amf/public/src/components/ComponentsFFMPEG/FileMuxerFFMPEGImpl.cpp"
    "../../amf/amf/public/src/components/ComponentsFFMPEG/H264EncoderFFMPEGImpl.cpp"
    "../../amf/amf/public/src/components/ComponentsFFMPEG/H264Mp4ToAnnexB.cpp"
    "../../amf/amf/public/src/components/ComponentsFFMPEG/HEVCEncoderFFMPEGImpl.cpp"
    "../../amf/amf/public/src/components/ComponentsFFMPEG/UtilsFFMPEG.cpp"
    "../../amf/amf/public/src/components/ComponentsFFMPEG/VideoDecoderFFMPEGImpl.cpp"
)

# Add platform-specific files:
if(WIN32)
set(SOURCE_FILES ${SOURCE_FILES}
    "../../amf/amf/public/common/Windows/ThreadWindows.cpp"
)
elseif(UNIX)
    set(SOURCE_FILES ${SOURCE_FILES}
    "../../amf/amf/public/common/Linux/ThreadLinux.cpp"
)
endif()

# Define header files
set(HEADER_FILES
    "../../amf/amf/public/common/AMFFactory.h"
    "../../amf/amf/public/common/AMFSTL.h"
    "../../amf/amf/public/common/DataStreamFile.h"
    "../../amf/amf/public/common/DataStreamMemory.h"
    "../../amf/amf/public/common/IOCapsImpl.h"
    "../../amf/amf/public/common/PropertyStorageImpl.h"
    "../../amf/amf/public/common/PropertyStorageExImpl.h"
    "../../amf/amf/public/common/ObservableImpl.h"
    "../../amf/amf/public/common/Thread.h"
    "../../amf/amf/public/common/TraceAdapter.h"
    "../../amf/amf/public/include/components/Component.h"
    "../../amf/amf/public/include/components/ComponentCaps.h"
    "../../amf/amf/public/include/components/FFMPEGAudioConverter.h"
    "../../amf/amf/public/include/components/FFMPEGAudioDecoder.h"
    "../../amf/amf/public/include/components/FFMPEGAudioEncoder.h"
    "../../amf/amf/public/include/components/FFMPEGEncoderAV1.h"
    "../../amf/amf/public/include/components/FFMPEGEncoderH264.h"
    "../../amf/amf/public/include/components/FFMPEGEncoderHEVC.h"
    "../../amf/amf/public/include/components/FFMPEGFileDemuxer.h"
    "../../amf/amf/public/include/components/FFMPEGFileMuxer.h"
    "../../amf/amf/public/include/components/FFMPEGVideoDecoder.h"
	"../../amf/amf/public/include/core/AudioBuffer.h"
	"../../amf/amf/public/include/core/Buffer.h"
	"../../amf/amf/public/include/core/Compute.h"
	"../../amf/amf/public/include/core/Context.h"
	"../../amf/amf/public/include/core/Data.h"
	"../../amf/amf/public/include/core/Debug.h"
	"../../amf/amf/public/include/core/Factory.h"
	"../../amf/amf/public/include/core/Interface.h"
	"../../amf/amf/public/include/core/Plane.h"
	"../../amf/amf/public/include/core/Platform.h"
	"../../amf/amf/public/include/core/PropertyStorage.h"
	"../../amf/amf/public/include/core/PropertyStorageEx.h"
	"../../amf/amf/public/include/core/Result.h"
	"../../amf/amf/public/include/core/Surface.h"
	"../../amf/amf/public/include/core/Trace.h"
	"../../amf/amf/public/include/core/Variant.h"
	"../../amf/amf/public/include/core/Version.h"
)

# Add the library
add_library(amf-component-ffmpeg64 SHARED ${SOURCE_FILES} ${HEADER_FILES})

set(FFMPEG_VERSION 7.0)

if(WIN32)
set(FFMPEG_BASE_PATH "../../amf/Thirdparty/ffmpeg/ffmpeg/ffmpeg-${FFMPEG_VERSION}/x64/release")

get_filename_component(FFMPEG_BASE_PATH_ABS ${FFMPEG_BASE_PATH} ABSOLUTE)
message(STATUS "FFMPEG absolute path is: ${FFMPEG_BASE_PATH_ABS}")

set(FFMPEG_DLLS
	"${FFMPEG_BASE_PATH_ABS}/bin/avcodec-61.dll"
	"${FFMPEG_BASE_PATH_ABS}/bin/avdevice-61.dll"
	"${FFMPEG_BASE_PATH_ABS}/bin/avfilter-10.dll"
	"${FFMPEG_BASE_PATH_ABS}/bin/avformat-61.dll"
	"${FFMPEG_BASE_PATH_ABS}/bin/avutil-59.dll"
	"${FFMPEG_BASE_PATH_ABS}/bin/swresample-5.dll"
	"${FFMPEG_BASE_PATH_ABS}/bin/swscale-8.dll"
)

target_link_libraries(amf-component-ffmpeg64 PRIVATE
    "${FFMPEG_BASE_PATH_ABS}/lib/avcodec.lib"
    "${FFMPEG_BASE_PATH_ABS}/lib/avdevice.lib"
    "${FFMPEG_BASE_PATH_ABS}/lib/avfilter.lib"
    "${FFMPEG_BASE_PATH_ABS}/lib/avformat.lib"
    "${FFMPEG_BASE_PATH_ABS}/lib/avutil.lib"
    "${FFMPEG_BASE_PATH_ABS}/lib/swresample.lib"
    "${FFMPEG_BASE_PATH_ABS}/lib/swscale.lib" )

elseif(UNIX)
set(FFMPEG_BASE_PATH "${CMAKE_SOURCE_DIR}/amf/Thirdparty/ffmpeg/ffmpeg/ffmpeg-${FFMPEG_VERSION}/lnx64/release")

set(FFMPEG_DLLS
	"${FFMPEG_BASE_PATH}/bin/libavcodec.so.61"
	"${FFMPEG_BASE_PATH}/bin/libavdevice.so.61"
	"${FFMPEG_BASE_PATH}/bin/libavfilter.so.10"
	"${FFMPEG_BASE_PATH}/bin/libavformat.so.61"
	"${FFMPEG_BASE_PATH}/bin/libavutil.so.59"
	"${FFMPEG_BASE_PATH}/bin/libswresample.so.5"
	"${FFMPEG_BASE_PATH}/bin/libswscale.so.8"
)

target_link_libraries(amf-component-ffmpeg64 PRIVATE
    "${FFMPEG_BASE_PATH}/bin/libavcodec.so.61"
    "${FFMPEG_BASE_PATH}/bin/libavdevice.so.61"
    "${FFMPEG_BASE_PATH}/bin/libavfilter.so.10"
    "${FFMPEG_BASE_PATH}/bin/libavformat.so.61"
    "${FFMPEG_BASE_PATH}/bin/libavutil.so.59"
    "${FFMPEG_BASE_PATH}/bin/libswresample.so.5"
    "${FFMPEG_BASE_PATH}/bin/libswscale.so.8"
)

endif()


# Include directories
target_include_directories(amf-component-ffmpeg64 PRIVATE
    ../../amf/amf
    ${FFMPEG_BASE_PATH}/include
)

# Compile definitions
target_compile_definitions(amf-component-ffmpeg64 PRIVATE
    $<$<CONFIG:Debug>:_DEBUG;_LIB;AMF_COMPONENT_FFMPEG_EXPORTS;_CRT_SECURE_NO_WARNINGS>
    $<$<CONFIG:Release>:NDEBUG;_LIB;AMF_COMPONENT_FFMPEG_EXPORTS;_CRT_SECURE_NO_WARNINGS>
)

if(WIN32)
    # Link options
    target_link_options(amf-component-ffmpeg64 PRIVATE
        $<$<CONFIG:Debug>:/DEBUG /INCREMENTAL>
        $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF /INCREMENTAL:NO>
    )

    set(OUTPUT_DIRECTORY "$<IF:$<CONFIG:Debug>,${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG},$<IF:$<CONFIG:Release>,${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE},$<IF:$<CONFIG:RelWithDebInfo>,${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO},${CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL}>>>")

elseif(UNIX)
    set(OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()

foreach(DLL ${FFMPEG_DLLS})
    get_filename_component(DLL_NAME ${DLL} NAME)
    add_custom_command(
        TARGET amf-component-ffmpeg64
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${DLL}
            ${OUTPUT_DIRECTORY}/${DLL_NAME}
        COMMENT "Copying ${DLL} to ${OUTPUT_DIRECTORY}/${DLL_NAME}"
    )
endforeach()
