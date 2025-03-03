cmake_minimum_required(VERSION 3.15)

# Define the project name
project(util)

# Define source files
set(SOURCE_FILES
    ${SOURCE_FILES}
    ${CMAKE_CURRENT_SOURCE_DIR}/encryption/AESPSKCipher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/SynchronousSlot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/AsynchronousSlot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/PipelineSlot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/AVPipeline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/AVSynchronizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/AVDispatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/AVTransmitterAdapter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/TimestampCalibrator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stats/ComponentStats.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stats/ClientStatsManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/QoS/QoS.cpp
    PARENT_SCOPE
 )

# Define header files
set(HEADER_FILES
    ${HEADER_FILES}
    ${CMAKE_CURRENT_SOURCE_DIR}/encryption/AESPSKCipher.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/PipelineSlot.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/SynchronousSlot.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/AsynchronousSlot.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/AVPipeline.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/AVSynchronizer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/AVDispatcher.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/AVTransmitterAdapter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/TimestampCalibrator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stats/ComponentStats.h
    ${CMAKE_CURRENT_SOURCE_DIR}/stats/ClientStatsManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QoS/QoS.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QoS/ValueHistory.h
    PARENT_SCOPE
 )

