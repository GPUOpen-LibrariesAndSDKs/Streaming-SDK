cmake_minimum_required(VERSION 3.15)

# Define the project name
project(audio)

# Define source files
set(SOURCE_FILES
    ${SOURCE_FILES}
    ${SUBFOLDER_AUDIO}/encoders/AudioEncodeEngine.cpp
    ${SUBFOLDER_AUDIO}/encoders/AMFAudioEncoder.cpp
    ${SUBFOLDER_AUDIO}/encoders/AudioEncoderAAC.cpp
    ${SUBFOLDER_AUDIO}/encoders/AudioEncoderOPUS.cpp
    ${SUBFOLDER_AUDIO}/AudioOutput.cpp
    ${SUBFOLDER_AUDIO}/decoders/AudioDecodeEngine.cpp
    ${SUBFOLDER_AUDIO}/decoders/AMFAudioDecoder.cpp
    ${SUBFOLDER_AUDIO}/decoders/AudioDecoderAAC.cpp
    ${SUBFOLDER_AUDIO}/decoders/AudioDecoderOPUS.cpp
    ${SUBFOLDER_AUDIO}/AudioCodecs.cpp
    ${SUBFOLDER_AUDIO}/AudioInput.cpp
    ${SUBFOLDER_AUDIO}/AudioReceiverPipeline.cpp
    ${SUBFOLDER_AUDIO}/AudioDispatcher.cpp
    ${SUBFOLDER_AUDIO}/AudioTransmitterAdapter.cpp
    PARENT_SCOPE
)

# Define header files
set(HEADER_FILES
    ${HEADER_FILES}
    ${SUBFOLDER_AUDIO}/encoders/AudioEncodeEngine.h
    ${SUBFOLDER_AUDIO}/encoders/AMFAudioEncoder.h
    ${SUBFOLDER_AUDIO}/encoders/AudioEncoderAAC.h
    ${SUBFOLDER_AUDIO}/encoders/AudioEncoderOPUS.h
    ${SUBFOLDER_AUDIO}/AudioOutput.h
    ${SUBFOLDER_AUDIO}/decoders/AudioDecodeEngine.h
    ${SUBFOLDER_AUDIO}/decoders/AMFAudioDecoder.h
    ${SUBFOLDER_AUDIO}/decoders/AudioDecoderAAC.h
    ${SUBFOLDER_AUDIO}/decoders/AudioDecoderOPUS.h
    ${SUBFOLDER_AUDIO}/AudioCodecs.h
    ${SUBFOLDER_AUDIO}/AudioInput.h
    ${SUBFOLDER_AUDIO}/AudioReceiverPipeline.h
    ${SUBFOLDER_AUDIO}/AudioDispatcher.h
    ${SUBFOLDER_AUDIO}/AudioTransmitterAdapter.h
    PARENT_SCOPE
)
