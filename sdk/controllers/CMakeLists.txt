# cmake_minimum_required(VERSION 3.15)

# Define the project name
# project(Controllers)
if(WIN32)

# Define source files
set(SOURCE_FILES
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

# Define header files
set(HEADER_FILES
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
    ${CMAKE_CURRENT_SOURCE_DIR}/client/win/MouseCursorWin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/client/win/GameControllerWin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/server/win/MouseControllerWinSvr.h
    ${CMAKE_CURRENT_SOURCE_DIR}/server/win/KeyboardControllerWinSvr.h
    ${CMAKE_CURRENT_SOURCE_DIR}/server/win/GameControllerWinSvr.h
    ${CMAKE_CURRENT_SOURCE_DIR}/server/win/TouchScreenControllerWinSvr.h
    PARENT_SCOPE
)

endif()
