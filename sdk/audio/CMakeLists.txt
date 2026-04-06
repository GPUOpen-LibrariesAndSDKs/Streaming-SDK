# cmake_minimum_required(VERSION 3.15)

# Define the project name
# project(audio)

if (ANDROID)
    set(ANDROID_HEADER_FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/android/AudioPresenterAndroid.h
    )
    set(ANDROID_SOURCE_FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/android/AudioPresenterAndroid.cpp
    )
else()
    set(ANDROID_HEADER_FILES)
    set(ANDROID_SOURCE_FILES)

endif()



# Define source files
set(SOURCE_FILES
    ${SOURCE_FILES}
    ${ANDROID_SOURCE_FILES}
    ${CMAKE_CURRENT_SOURCE_DIR}/encoders/AudioEncodeEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/encoders/AMFAudioEncoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/encoders/AudioEncoderAAC.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/encoders/AudioEncoderOPUS.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/encoders/AudioEncoderPCM.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AudioOutput.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/decoders/AudioDecodeEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/decoders/AMFAudioDecoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/decoders/AudioDecoderAAC.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/decoders/AudioDecoderOPUS.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/decoders/AudioDecoderPCM.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AudioCodecs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AudioInput.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AudioReceiverPipeline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AudioDispatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AudioTransmitterAdapter.cpp
    PARENT_SCOPE
)

# Define header files
set(HEADER_FILES
    ${HEADER_FILES}
    ${ANDROID_HEADER_FILES}
    ${CMAKE_CURRENT_SOURCE_DIR}/encoders/AudioEncodeEngine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/encoders/AMFAudioEncoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/encoders/AudioEncoderAAC.h
    ${CMAKE_CURRENT_SOURCE_DIR}/encoders/AudioEncoderOPUS.h
    ${CMAKE_CURRENT_SOURCE_DIR}/encoders/AudioEncoderPCM.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AudioOutput.h
    ${CMAKE_CURRENT_SOURCE_DIR}/decoders/AudioDecodeEngine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/decoders/AMFAudioDecoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/decoders/AudioDecoderAAC.h
    ${CMAKE_CURRENT_SOURCE_DIR}/decoders/AudioDecoderOPUS.h
    ${CMAKE_CURRENT_SOURCE_DIR}/decoders/AudioDecoderPCM.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AudioCodecs.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AudioInput.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AudioReceiverPipeline.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AudioDispatcher.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AudioTransmitterAdapter.h
    PARENT_SCOPE
)
