cmake_minimum_required(VERSION 3.15)

# Define the project name
project(video)

# Define source files
set(SOURCE_FILES
    ${SOURCE_FILES}
    ${CMAKE_CURRENT_SOURCE_DIR}/encoders/VideoEncodeEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/encoders/HDREncoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/encoders/GPUEncoderH264.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/encoders/GPUEncoderHEVC.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/encoders/GPUEncoderAV1.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MonoscopicVideoOutput.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/decoders/VideoDecodeEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/decoders/UVDDecoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/decoders/UVDDecoderH264.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/decoders/UVDDecoderHEVC.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/decoders/UVDDecoderAV1.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MonoscopicVideoInput.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Defines.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoReceiverPipeline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoDispatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoTransmitterAdapter.cpp
    PARENT_SCOPE
)

# Define header files
set(HEADER_FILES
    ${HEADER_FILES}
    ${CMAKE_CURRENT_SOURCE_DIR}/encoders/VideoEncodeEngine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/encoders/HDREncoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/encoders/GPUEncoderH264.h
    ${CMAKE_CURRENT_SOURCE_DIR}/encoders/GPUEncoderHEVC.h
    ${CMAKE_CURRENT_SOURCE_DIR}/encoders/GPUEncoderAV1.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MonoscopicVideoOutput.h
    ${CMAKE_CURRENT_SOURCE_DIR}/decoders/VideoDecodeEngine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/decoders/UVDDecoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/decoders/UVDDecoderH264.h
    ${CMAKE_CURRENT_SOURCE_DIR}/decoders/UVDDecoderHEVC.h
    ${CMAKE_CURRENT_SOURCE_DIR}/decoders/UVDDecoderAV1.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MonoscopicVideoInput.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Defines.h
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoReceiverPipeline.h
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoDispatcher.h
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoTransmitterAdapter.h
    PARENT_SCOPE
)
