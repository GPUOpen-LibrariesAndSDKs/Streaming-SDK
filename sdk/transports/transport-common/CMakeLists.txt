cmake_minimum_required(VERSION 3.15)

# Define the project name
project(transport-common)

# Define source files
set(SOURCE_FILES
    ${SOURCE_FILES}
    ${CMAKE_CURRENT_SOURCE_DIR}/Controller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Video.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Audio.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ClientTransport.cpp
    PARENT_SCOPE
)

# Define header files
set(HEADER_FILES
    ${HEADER_FILES}
    ${CMAKE_CURRENT_SOURCE_DIR}/Controller.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Video.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Audio.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ClientTransport.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CodecDescriptor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ServerTransport.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Transport.h
    PARENT_SCOPE
)
