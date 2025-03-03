# cmake_minimum_required(VERSION 3.15)

# Define the project name
# project(transport-amd)

# Define source files
set(SOURCE_FILES
    ${SOURCE_FILES}
    ${CMAKE_CURRENT_SOURCE_DIR}/ClientImpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ClientSessionImpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ClientTransportImpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Codec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DgramClientSessionFlowCtrl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DgramFlowCtrlProtocol.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DiscoverySessionImpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/messages/audio/AudioData.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/messages/audio/AudioInit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/messages/Message.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/messages/sensors/DeviceEvent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/messages/sensors/TrackableDeviceCaps.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/messages/service/Connect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/messages/service/GenericMessage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/messages/service/StartStop.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/messages/service/Stats.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/messages/service/Update.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/messages/video/Cursor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/messages/video/QoS.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/messages/video/VideoData.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/messages/video/VideoInit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Misc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ServerDiscovery.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ServerSessionImpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/StreamFlowCtrlProtocol.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TCPServerImpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TCPServerSessionImpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TransportServerImpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UDPServerImpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UDPServerSessionImpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ServerTransportImpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Subscriber.cpp
    PARENT_SCOPE
)

# Define header files
set(HEADER_FILES
    ${HEADER_FILES}
    ${CMAKE_CURRENT_SOURCE_DIR}/ClientImpl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ClientSessionImpl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ClientTransportImpl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/DgramClientSessionFlowCtrl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/DiscoverySessionImpl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ServerDiscovery.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ServerSessionImpl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TCPServerSessionImpl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TransportClient.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TransportServer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Channels.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Codec.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FlowCtrlProtocol.h
    ${CMAKE_CURRENT_SOURCE_DIR}/messages/audio/AudioData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/messages/audio/AudioInit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/messages/Message.h
    ${CMAKE_CURRENT_SOURCE_DIR}/messages/sensors/DeviceEvent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/messages/sensors/TrackableDeviceCaps.h
    ${CMAKE_CURRENT_SOURCE_DIR}/messages/service/Connect.h
    ${CMAKE_CURRENT_SOURCE_DIR}/messages/service/GenericMessage.h
    ${CMAKE_CURRENT_SOURCE_DIR}/messages/service/StartStop.h
    ${CMAKE_CURRENT_SOURCE_DIR}/messages/service/Stats.h
    ${CMAKE_CURRENT_SOURCE_DIR}/messages/service/Update.h
    ${CMAKE_CURRENT_SOURCE_DIR}/messages/video/Cursor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/messages/video/QoS.h
    ${CMAKE_CURRENT_SOURCE_DIR}/messages/video/VideoData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/messages/video/VideoInit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TransportServerImpl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TransportSession.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UDPServerSessionImpl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ServerTransportImpl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Subscriber.h
    PARENT_SCOPE
)
