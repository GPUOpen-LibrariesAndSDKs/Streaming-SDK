cmake_minimum_required(VERSION 3.15)

# Define the project name
project(Controllers)

# Define source files
set(SOURCE_FILES
    ${SOURCE_FILES}
    ${SUBFOLDER_CONTROLLERS}/client/ControllerBase.cpp
    ${SUBFOLDER_CONTROLLERS}/client/KeyboardController.cpp
    ${SUBFOLDER_CONTROLLERS}/client/lnx/GameControllerLnx.cpp
    ${SUBFOLDER_CONTROLLERS}/client/lnx/KeyboardControllerLnx.cpp
    ${SUBFOLDER_CONTROLLERS}/client/lnx/MouseControllerLnx.cpp
    ${SUBFOLDER_CONTROLLERS}/client/lnx/X11TouchController.cpp
    ${SUBFOLDER_CONTROLLERS}/client/lnx/tbls/WindowsToX11KeyTable.cpp
    ${SUBFOLDER_CONTROLLERS}/client/lnx/tbls/X11ToWindowsScanCodeTable.cpp
    ${SUBFOLDER_CONTROLLERS}/client/MouseController.cpp
    ${SUBFOLDER_CONTROLLERS}/client/MouseCursor.cpp
    ${SUBFOLDER_CONTROLLERS}/client/win/MouseCursorWin.cpp
    ${SUBFOLDER_CONTROLLERS}/client/lnx/MouseCursorLnx.cpp
    ${SUBFOLDER_CONTROLLERS}/client/TouchEvent.cpp
    ${SUBFOLDER_CONTROLLERS}/client/TrackedHeadset.cpp
    ${SUBFOLDER_CONTROLLERS}/UserInput.cpp
    ${SUBFOLDER_CONTROLLERS}/client/win/GameControllerWin.cpp
    ${SUBFOLDER_CONTROLLERS}/client/win/KeyboardControllerWin.cpp
    ${SUBFOLDER_CONTROLLERS}/client/win/MouseControllerWin.cpp
    ${SUBFOLDER_CONTROLLERS}/client/ControllerManager.cpp
    ${SUBFOLDER_CONTROLLERS}/server/MouseControllerSvr.cpp
    ${SUBFOLDER_CONTROLLERS}/server/KeyboardControllerSvr.cpp
    ${SUBFOLDER_CONTROLLERS}/server/GameControllerSvr.cpp
    ${SUBFOLDER_CONTROLLERS}/server/TouchScreenControllerSvr.cpp
    ${SUBFOLDER_CONTROLLERS}/server/win/MouseControllerWinSvr.cpp
    ${SUBFOLDER_CONTROLLERS}/server/win/KeyboardControllerWinSvr.cpp
    ${SUBFOLDER_CONTROLLERS}/server/win/GameControllerWinSvr.cpp
    ${SUBFOLDER_CONTROLLERS}/server/win/TouchScreenControllerWinSvr.cpp
    ${SUBFOLDER_CONTROLLERS}/server/ControllerManagerSvr.cpp
    PARENT_SCOPE
)

# Define header files
set(HEADER_FILES
    ${HEADER_FILES}
    ${SUBFOLDER_CONTROLLERS}/ControllerTypes.h
    ${SUBFOLDER_CONTROLLERS}/client/ControllerBase.h
    ${SUBFOLDER_CONTROLLERS}/client/GameController.h
    ${SUBFOLDER_CONTROLLERS}/client/KeyboardController.h
    ${SUBFOLDER_CONTROLLERS}/client/MouseController.h
    ${SUBFOLDER_CONTROLLERS}/client/MouseCursor.h
    ${SUBFOLDER_CONTROLLERS}/client/win/MouseCursorWin.h
    ${SUBFOLDER_CONTROLLERS}/client/lnx/MouseCursorLnx.h
    ${SUBFOLDER_CONTROLLERS}/client/TouchEvent.h
    ${SUBFOLDER_CONTROLLERS}/client/TrackedHeadset.h
    ${SUBFOLDER_CONTROLLERS}/UserInput.h
    ${SUBFOLDER_CONTROLLERS}/client/win/GameControllerWin.h
    ${SUBFOLDER_CONTROLLERS}/client/lnx/GameControllerLnx.h
    ${SUBFOLDER_CONTROLLERS}/client/lnx/X11TouchController.h
    ${SUBFOLDER_CONTROLLERS}/client/lnx/tbls/WindowsToX11KeyTable.h
    ${SUBFOLDER_CONTROLLERS}/client/lnx/tbls/X11ToWindowsScanCodeTable.h
    ${SUBFOLDER_CONTROLLERS}/client/ControllerManager.h
    ${SUBFOLDER_CONTROLLERS}/server/ControllerBaseSvr.h
    ${SUBFOLDER_CONTROLLERS}/server/MouseControllerSvr.h
    ${SUBFOLDER_CONTROLLERS}/server/KeyboardControllerSvr.h
    ${SUBFOLDER_CONTROLLERS}/server/GameControllerSvr.h
    ${SUBFOLDER_CONTROLLERS}/server/TouchScreenControllerSvr.h
    ${SUBFOLDER_CONTROLLERS}/server/win/MouseControllerWinSvr.h
    ${SUBFOLDER_CONTROLLERS}/server/win/KeyboardControllerWinSvr.h
    ${SUBFOLDER_CONTROLLERS}/server/win/GameControllerWinSvr.h
    ${SUBFOLDER_CONTROLLERS}/server/win/TouchScreenControllerWinSvr.h
    ${SUBFOLDER_CONTROLLERS}/server/ControllerManagerSvr.h
    PARENT_SCOPE
)
