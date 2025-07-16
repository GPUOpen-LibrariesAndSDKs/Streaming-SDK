# cmake_minimum_required(VERSION 3.15)

# Define the project name
# project(Controllers)

# Define source files
set(SOURCE_FILES_CTRLS
    ${SOURCE_FILES}
    ${CMAKE_CURRENT_SOURCE_DIR}/TouchEvent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UserInput.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/client/ControllerBase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/client/MouseController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/client/MouseCursor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/client/KeyboardController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/client/TrackedHeadset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/client/ControllerManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/server/MouseControllerSvr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/server/KeyboardControllerSvr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/server/GameControllerSvr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/server/TouchScreenControllerSvr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/server/ControllerManagerSvr.cpp
)

# Define header files
set(HEADER_FILES_CTRLS
    ${HEADER_FILES}
    ${CMAKE_CURRENT_SOURCE_DIR}/TouchEvent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UserInput.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ControllerTypes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/client/ControllerBase.h
    ${CMAKE_CURRENT_SOURCE_DIR}/client/MouseController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/client/MouseCursor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/client/KeyboardController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/client/GameController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/client/TrackedHeadset.h
    ${CMAKE_CURRENT_SOURCE_DIR}/client/ControllerManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/server/ControllerBaseSvr.h
    ${CMAKE_CURRENT_SOURCE_DIR}/server/MouseControllerSvr.h
    ${CMAKE_CURRENT_SOURCE_DIR}/server/KeyboardControllerSvr.h
    ${CMAKE_CURRENT_SOURCE_DIR}/server/GameControllerSvr.h
    ${CMAKE_CURRENT_SOURCE_DIR}/server/TouchScreenControllerSvr.h
    ${CMAKE_CURRENT_SOURCE_DIR}/server/ControllerManagerSvr.h
)

# Add platform-specific files:
if(WIN32)
    set(SOURCE_FILES ${SOURCE_FILES_CTRLS}
        ${CMAKE_CURRENT_SOURCE_DIR}/client/win/MouseControllerWin.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/client/win/MouseCursorWin.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/client/win/KeyboardControllerWin.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/client/win/GameControllerWin.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/server/win/MouseControllerWinSvr.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/server/win/KeyboardControllerWinSvr.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/server/win/GameControllerWinSvr.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/server/win/TouchScreenControllerWinSvr.cpp
        PARENT_SCOPE
    )

    set(HEADER_FILES ${HEADER_FILES_CTRLS}
        ${CMAKE_CURRENT_SOURCE_DIR}/client/win/MouseCursorWin.h
        ${CMAKE_CURRENT_SOURCE_DIR}/client/win/GameControllerWin.h
        ${CMAKE_CURRENT_SOURCE_DIR}/server/win/MouseControllerWinSvr.h
        ${CMAKE_CURRENT_SOURCE_DIR}/server/win/KeyboardControllerWinSvr.h
        ${CMAKE_CURRENT_SOURCE_DIR}/server/win/GameControllerWinSvr.h
        ${CMAKE_CURRENT_SOURCE_DIR}/server/win/TouchScreenControllerWinSvr.h
        PARENT_SCOPE
    )
elseif(UNIX)
    set(SOURCE_FILES ${SOURCE_FILES_CTRLS}
        ${CMAKE_CURRENT_SOURCE_DIR}/client/lnx/MouseControllerLnx.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/client/lnx/MouseCursorLnx.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/client/lnx/KeyboardControllerLnx.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/client/lnx/GameControllerLnx.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/client/lnx/X11TouchController.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/server/lnx/MouseControllerLnxSvr.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/server/lnx/KeyboardControllerLnxSvr.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/server/lnx/GameControllerLnxSvr.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/server/lnx/TouchScreenControllerLnxSvr.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tbls/WindowsToX11KeyTable.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tbls/X11ToWindowsScanCodeTable.cpp
        #${CMAKE_CURRENT_SOURCE_DIR}/xdg/SDBusImportTable.cpp
        #${CMAKE_CURRENT_SOURCE_DIR}/xdg/SDBusMessagePtr.cpp
        #${CMAKE_CURRENT_SOURCE_DIR}/xdg/SDBusSlotPtr.cpp
        #${CMAKE_CURRENT_SOURCE_DIR}/xdg/DBusConnection.cpp
        #${CMAKE_CURRENT_SOURCE_DIR}/xdg/XDGRDPortalConnection.cpp
        #${CMAKE_CURRENT_SOURCE_DIR}/xdg/XDGDesktopPortal.cpp
        PARENT_SCOPE
    )

    set(HEADER_FILES ${HEADER_FILES_CTRLS}
        ${CMAKE_CURRENT_SOURCE_DIR}/client/lnx/MouseCursorLnx.h
        ${CMAKE_CURRENT_SOURCE_DIR}/client/lnx/GameControllerLnx.h
        ${CMAKE_CURRENT_SOURCE_DIR}/client/lnx/X11TouchController.h
        ${CMAKE_CURRENT_SOURCE_DIR}/server/lnx/MouseControllerLnxSvr.h
        ${CMAKE_CURRENT_SOURCE_DIR}/server/lnx/KeyboardControllerLnxSvr.h
        ${CMAKE_CURRENT_SOURCE_DIR}/server/lnx/GameControllerLnxSvr.h
        ${CMAKE_CURRENT_SOURCE_DIR}/server/lnx/TouchScreenControllerLnxSvr.h
        ${CMAKE_CURRENT_SOURCE_DIR}/tbls/WindowsToX11KeyTable.h
        ${CMAKE_CURRENT_SOURCE_DIR}/tbls/X11ToWindowsScanCodeTable.h
        #${CMAKE_CURRENT_SOURCE_DIR}/xdg/AbortController.h
        #${CMAKE_CURRENT_SOURCE_DIR}/xdg/SDBusImportTable.h
        #${CMAKE_CURRENT_SOURCE_DIR}/xdg/SDBusMessagePtr.h
        #${CMAKE_CURRENT_SOURCE_DIR}/xdg/SDBusSlotPtr.h
        #${CMAKE_CURRENT_SOURCE_DIR}/xdg/DBusConnection.h
        #${CMAKE_CURRENT_SOURCE_DIR}/xdg/XDGRDPortalConnection.h
        #${CMAKE_CURRENT_SOURCE_DIR}/xdg/XDGDesktopPortal.h
        PARENT_SCOPE
    )
endif()
