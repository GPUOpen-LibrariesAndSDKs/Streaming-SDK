cmake_minimum_required(VERSION 3.15)

# Define the project name
project(util)

# Define source files
set(SOURCE_FILES
    ${SOURCE_FILES}
    mbedtls/library/sha256.c
    ${SUBFOLDER_UTIL}/encryption/AESPSKCipher.cpp
    ${SUBFOLDER_UTIL}/pipeline/SynchronousSlot.cpp
    ${SUBFOLDER_UTIL}/pipeline/AsynchronousSlot.cpp
    ${SUBFOLDER_UTIL}/pipeline/PipelineSlot.cpp
    ${SUBFOLDER_UTIL}/pipeline/AVPipeline.cpp
    ${SUBFOLDER_UTIL}/pipeline/AVSynchronizer.cpp
    ${SUBFOLDER_UTIL}/pipeline/AVDispatcher.cpp
    ${SUBFOLDER_UTIL}/pipeline/AVTransmitterAdapter.cpp
    ${SUBFOLDER_UTIL}/pipeline/TimestampCalibrator.cpp
    ${SUBFOLDER_UTIL}/stats/ComponentStats.cpp
    ${SUBFOLDER_UTIL}/stats/ClientStatsManager.cpp
    ${SUBFOLDER_UTIL}/QoS/QoS.cpp
    PARENT_SCOPE
 )

# Define header files
set(HEADER_FILES
    ${HEADER_FILES}
    mbedtls/include/mbedtls/sha256.h
    ${SUBFOLDER_UTIL}/encryption/AESPSKCipher.h
    ${SUBFOLDER_UTIL}/pipeline/PipelineSlot.h
    ${SUBFOLDER_UTIL}/pipeline/SynchronousSlot.h
    ${SUBFOLDER_UTIL}/pipeline/AsynchronousSlot.h
    ${SUBFOLDER_UTIL}/pipeline/AVPipeline.h
    ${SUBFOLDER_UTIL}/pipeline/AVSynchronizer.h
    ${SUBFOLDER_UTIL}/pipeline/AVDispatcher.h
    ${SUBFOLDER_UTIL}/pipeline/AVTransmitterAdapter.h
    ${SUBFOLDER_UTIL}/pipeline/TimestampCalibrator.h
    ${SUBFOLDER_UTIL}/stats/ComponentStats.h
    ${SUBFOLDER_UTIL}/stats/ClientStatsManager.h
    ${SUBFOLDER_UTIL}/QoS/QoS.h
    ${SUBFOLDER_UTIL}/QoS/ValueHistory.h
    PARENT_SCOPE
 )

