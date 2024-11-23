cmake_minimum_required(VERSION 3.15)

# Define the project name
project(video)

# Define source files
set(SOURCE_FILES
    ${SOURCE_FILES}
    ${SUBFOLDER_VIDEO}/encoders/VideoEncodeEngine.cpp
    ${SUBFOLDER_VIDEO}/encoders/HDREncoder.cpp
    ${SUBFOLDER_VIDEO}/encoders/GPUEncoderH264.cpp
    ${SUBFOLDER_VIDEO}/encoders/GPUEncoderHEVC.cpp
    ${SUBFOLDER_VIDEO}/encoders/GPUEncoderAV1.cpp
    ${SUBFOLDER_VIDEO}/MonoscopicVideoOutput.cpp
    ${SUBFOLDER_VIDEO}/decoders/VideoDecodeEngine.cpp
    ${SUBFOLDER_VIDEO}/decoders/UVDDecoder.cpp
    ${SUBFOLDER_VIDEO}/decoders/UVDDecoderH264.cpp
    ${SUBFOLDER_VIDEO}/decoders/UVDDecoderHEVC.cpp
    ${SUBFOLDER_VIDEO}/decoders/UVDDecoderAV1.cpp
    ${SUBFOLDER_VIDEO}/MonoscopicVideoInput.cpp
    ${SUBFOLDER_VIDEO}/Defines.cpp
    ${SUBFOLDER_VIDEO}/VideoReceiverPipeline.cpp
    ${SUBFOLDER_VIDEO}/VideoDispatcher.cpp
    ${SUBFOLDER_VIDEO}/VideoTransmitterAdapter.cpp
    PARENT_SCOPE
)

# Define header files
set(HEADER_FILES
    ${HEADER_FILES}
    ${SUBFOLDER_VIDEO}/encoders/VideoEncodeEngine.h
    ${SUBFOLDER_VIDEO}/encoders/HDREncoder.h
    ${SUBFOLDER_VIDEO}/encoders/GPUEncoderH264.h
    ${SUBFOLDER_VIDEO}/encoders/GPUEncoderHEVC.h
    ${SUBFOLDER_VIDEO}/encoders/GPUEncoderAV1.h
    ${SUBFOLDER_VIDEO}/MonoscopicVideoOutput.h
    ${SUBFOLDER_VIDEO}/decoders/VideoDecodeEngine.h
    ${SUBFOLDER_VIDEO}/decoders/UVDDecoder.h
    ${SUBFOLDER_VIDEO}/decoders/UVDDecoderH264.h
    ${SUBFOLDER_VIDEO}/decoders/UVDDecoderHEVC.h
    ${SUBFOLDER_VIDEO}/decoders/UVDDecoderAV1.h
    ${SUBFOLDER_VIDEO}/MonoscopicVideoInput.h
    ${SUBFOLDER_VIDEO}/Defines.h
    ${SUBFOLDER_VIDEO}/VideoReceiverPipeline.h
    ${SUBFOLDER_VIDEO}/VideoDispatcher.h
    ${SUBFOLDER_VIDEO}/VideoTransmitterAdapter.h
    PARENT_SCOPE
)
