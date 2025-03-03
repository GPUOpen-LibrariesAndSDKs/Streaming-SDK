# cmake_minimum_required(VERSION 3.15)

# Define the project name
# project(net)

# Define source files
set(SOURCE_FILES
    ${SOURCE_FILES}
    ${CMAKE_CURRENT_SOURCE_DIR}/Client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ClientSession.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DatagramClient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DatagramClientSession.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DatagramServer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DatagramServerSession.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DatagramSocket.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Initializer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Selector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Session.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SessionManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Socket.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SocketAddress.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/StreamClient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/StreamClientSession.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/StreamServer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/StreamServerSession.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/StreamSocket.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UnixStreamSocket.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Url.cpp
    PARENT_SCOPE
)

# Define header files
set(HEADER_FILES
    ${HEADER_FILES}
    ${CMAKE_CURRENT_SOURCE_DIR}/Client.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ClientSession.h
    ${CMAKE_CURRENT_SOURCE_DIR}/DatagramClient.h
    ${CMAKE_CURRENT_SOURCE_DIR}/DatagramClientSession.h
    ${CMAKE_CURRENT_SOURCE_DIR}/DatagramServer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/DatagramServerSession.h
    ${CMAKE_CURRENT_SOURCE_DIR}/DatagramSocket.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Initializer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Selector.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Server.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Session.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SessionManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Socket.h
    ${CMAKE_CURRENT_SOURCE_DIR}/StreamClient.h
    ${CMAKE_CURRENT_SOURCE_DIR}/StreamClientSession.h
    ${CMAKE_CURRENT_SOURCE_DIR}/StreamServer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/StreamServerSession.h
    ${CMAKE_CURRENT_SOURCE_DIR}/StreamSocket.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UnixStreamSocket.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Url.h
    PARENT_SCOPE
)
