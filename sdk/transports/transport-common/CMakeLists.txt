cmake_minimum_required(VERSION 3.15)

# Define the project name
project(transport-common)

# Define source files
set(SOURCE_FILES
    ${SOURCE_FILES}
    ${SUBFOLDER_TRANSPORT_COMMON}/Controller.cpp
    ${SUBFOLDER_TRANSPORT_COMMON}/Video.cpp
	${SUBFOLDER_TRANSPORT_COMMON}/Audio.cpp
	${SUBFOLDER_TRANSPORT_COMMON}/ClientTransport.cpp
    PARENT_SCOPE
)

# Define header files
set(HEADER_FILES
    ${HEADER_FILES}
    ${SUBFOLDER_TRANSPORT_COMMON}/Controller.h
    ${SUBFOLDER_TRANSPORT_COMMON}/Video.h
    ${SUBFOLDER_TRANSPORT_COMMON}/Audio.h
    ${SUBFOLDER_TRANSPORT_COMMON}/ClientTransport.h
    ${SUBFOLDER_TRANSPORT_COMMON}/CodecDescriptor.h
    ${SUBFOLDER_TRANSPORT_COMMON}/ServerTransport.h
    ${SUBFOLDER_TRANSPORT_COMMON}/Transport.h
    PARENT_SCOPE
)
