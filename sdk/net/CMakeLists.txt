cmake_minimum_required(VERSION 3.15)

# Define the project name
project(net)

# Define source files
set(SOURCE_FILES
    ${SOURCE_FILES}
    ${SUBFOLDER_NET}/Client.cpp
    ${SUBFOLDER_NET}/ClientSession.cpp
    ${SUBFOLDER_NET}/DatagramClient.cpp
    ${SUBFOLDER_NET}/DatagramClientSession.cpp
    ${SUBFOLDER_NET}/DatagramServer.cpp
    ${SUBFOLDER_NET}/DatagramServerSession.cpp
    ${SUBFOLDER_NET}/DatagramSocket.cpp
    ${SUBFOLDER_NET}/Initializer.cpp
    ${SUBFOLDER_NET}/Selector.cpp
    ${SUBFOLDER_NET}/Server.cpp
    ${SUBFOLDER_NET}/Session.cpp
    ${SUBFOLDER_NET}/SessionManager.cpp
    ${SUBFOLDER_NET}/Socket.cpp
    ${SUBFOLDER_NET}/SocketAddress.cpp
    ${SUBFOLDER_NET}/StreamClient.cpp
    ${SUBFOLDER_NET}/StreamClientSession.cpp
    ${SUBFOLDER_NET}/StreamServer.cpp
    ${SUBFOLDER_NET}/StreamServerSession.cpp
    ${SUBFOLDER_NET}/StreamSocket.cpp
    ${SUBFOLDER_NET}/UnixStreamSocket.cpp
    ${SUBFOLDER_NET}/Url.cpp
    PARENT_SCOPE
)

# Define header files
set(HEADER_FILES
    ${HEADER_FILES}
    ${SUBFOLDER_NET}/Client.h
    ${SUBFOLDER_NET}/ClientSession.h
    ${SUBFOLDER_NET}/DatagramClient.h
    ${SUBFOLDER_NET}/DatagramClientSession.h
    ${SUBFOLDER_NET}/DatagramServer.h
    ${SUBFOLDER_NET}/DatagramServerSession.h
    ${SUBFOLDER_NET}/DatagramSocket.h
    ${SUBFOLDER_NET}/Initializer.h
    ${SUBFOLDER_NET}/Selector.h
    ${SUBFOLDER_NET}/Server.h
    ${SUBFOLDER_NET}/Session.h
    ${SUBFOLDER_NET}/SessionManager.h
    ${SUBFOLDER_NET}/Socket.h
    ${SUBFOLDER_NET}/StreamClient.h
    ${SUBFOLDER_NET}/StreamClientSession.h
    ${SUBFOLDER_NET}/StreamServer.h
    ${SUBFOLDER_NET}/StreamServerSession.h
    ${SUBFOLDER_NET}/StreamSocket.h
    ${SUBFOLDER_NET}/UnixStreamSocket.h
    ${SUBFOLDER_NET}/Url.h
    PARENT_SCOPE
)
