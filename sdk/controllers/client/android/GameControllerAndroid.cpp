//
// Notice Regarding Standards.  AMD does not provide a license or sublicense to
// any Intellectual Property Rights relating to any standards, including but not
// limited to any audio and/or video codec technologies such as MPEG-2, MPEG-4;
// AVC/H.264; HEVC/H.265; AAC decode/FFMPEG; AAC encode/FFMPEG; VC-1; and MP3
// (collectively, the "Media Technologies"). For clarity, you will pay any
// royalties due for such third party technologies, which may include the Media
// Technologies that are owed as a result of AMD providing the Software to you.
//
// MIT license
//
//
// Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#if defined(__ANDROID__)

#include <android/input.h>
#include <android/native_activity.h>
#include <android/log.h>
#include "GameControllerAndroid.h"
#include "../ControllerManager.h"

// Button and axis constants
#define XBOX_BUTTON_A         AKEYCODE_BUTTON_A
#define XBOX_BUTTON_B         AKEYCODE_BUTTON_B
#define XBOX_BUTTON_X         AKEYCODE_BUTTON_X
#define XBOX_BUTTON_Y         AKEYCODE_BUTTON_Y
#define XBOX_BUTTON_LB        AKEYCODE_BUTTON_L1
#define XBOX_BUTTON_RB        AKEYCODE_BUTTON_R1
#define XBOX_BUTTON_BACK      AKEYCODE_BUTTON_SELECT
#define XBOX_BUTTON_START     AKEYCODE_BUTTON_START
#define XBOX_BUTTON_LS        AKEYCODE_BUTTON_THUMBL
#define XBOX_BUTTON_RS        AKEYCODE_BUTTON_THUMBR
#define XBOX_DPAD_UP          AKEYCODE_DPAD_UP
#define XBOX_DPAD_DOWN        AKEYCODE_DPAD_DOWN
#define XBOX_DPAD_LEFT        AKEYCODE_DPAD_LEFT
#define XBOX_DPAD_RIGHT       AKEYCODE_DPAD_RIGHT

#define XBOX_AXIS_LS_X        AMOTION_EVENT_AXIS_X
#define XBOX_AXIS_LS_Y        AMOTION_EVENT_AXIS_Y
#define XBOX_AXIS_RS_X        AMOTION_EVENT_AXIS_Z
#define XBOX_AXIS_RS_Y        AMOTION_EVENT_AXIS_RZ
#define XBOX_AXIS_LT          AMOTION_EVENT_AXIS_LTRIGGER
#define XBOX_AXIS_RT          AMOTION_EVENT_AXIS_RTRIGGER

namespace ssdk::ctls
{
    static constexpr const wchar_t* const AMF_FACILITY = L"GameController";

    //-------------------------------------------------------------------------------------------------
    GameController::GameController(ControllerManagerPtr pControllerManager, ssdk::util::JavaObjectHolder& activity, int iIndex) :
        ControllerBase(pControllerManager),
        m_Activity(activity),
        m_iIndex(iIndex)
    {
        m_ID = std::string(DEVICE_GAME_CONTROLLER) + std::string(1, (char)('0' + m_iIndex));
        m_Description = "XBox controller";


        m_eventMap[AKEYCODE_BUTTON_THUMBL] = DEVICE_CTRL_JOYSTICK_LEFT_CLICK;
        m_eventMap[AKEYCODE_BUTTON_THUMBR] = DEVICE_CTRL_JOYSTICK_RIGHT_CLICK;
        m_eventMap[AKEYCODE_BUTTON_MODE] = DEVICE_CTRL_MODE_CLICK;
        m_eventMap[AKEYCODE_BUTTON_START] = DEVICE_CTRL_SYSTEM_CLICK;
        m_eventMap[AKEYCODE_BUTTON_SELECT] = DEVICE_CTRL_APPMENU_CLICK;
        m_eventMap[AKEYCODE_BUTTON_L1] = DEVICE_CTRL_GRIP_LEFT_CLICK;
        m_eventMap[AKEYCODE_BUTTON_R1] = DEVICE_CTRL_GRIP_RIGHT_CLICK;
        m_eventMap[AKEYCODE_BUTTON_A] = DEVICE_CTRL_A_CLICK;
        m_eventMap[AKEYCODE_BUTTON_B] = DEVICE_CTRL_B_CLICK;
        m_eventMap[AKEYCODE_BUTTON_X] = DEVICE_CTRL_X_CLICK;
        m_eventMap[AKEYCODE_BUTTON_Y] = DEVICE_CTRL_Y_CLICK;
/*
        switch (m_connectionType)
        {
        case CONNECTION_TYPE::USB:
            m_eventMap[AMOTION_EVENT_AXIS_RY] = AMOTION_EVENT_AXIS_RZ;
            m_eventMap[AMOTION_EVENT_AXIS_RX] = AMOTION_EVENT_AXIS_Z;
            m_eventMap[AMOTION_EVENT_AXIS_Z] = AMOTION_EVENT_AXIS_RTRIGGER;
            m_eventMap[AMOTION_EVENT_AXIS_RZ] = AMOTION_EVENT_AXIS_LTRIGGER;
            break;
        case CONNECTION_TYPE::BLUETOOTH:
            break;
        default:
            break;
        }
*/
    }

    //-------------------------------------------------------------------------------------------------
    void AMF_STD_CALL GameController::FireEvent(const char* id, CtlEvent& ev)
    {
        if (m_pControllerManager != nullptr)
        {
            m_pControllerManager->SendControllerEvent(id, ev);
        }
    }

    AMF_RESULT  AMF_STD_CALL GameController::ProcessMessage(const WindowMessage& msg)
    {
        AMF_RESULT result = AMF_FAIL;
        int eventSource = AInputEvent_getSource(msg);
        int eventType = AInputEvent_getType(msg);
        if (eventSource & AINPUT_SOURCE_GAMEPAD)
        {
            if (eventType == AINPUT_EVENT_TYPE_KEY)
            {
                int32_t keyCode = AKeyEvent_getKeyCode(msg);
                int32_t action = AKeyEvent_getAction(msg);
                if (action == AKEY_EVENT_ACTION_DOWN && AKeyEvent_getRepeatCount(msg) == 0)
                {
                    ProcessButton(keyCode, true);
                    result = AMF_OK;
                }
                else if (action == AKEY_EVENT_ACTION_UP)
                {
                    ProcessButton(keyCode, false);
                    result = AMF_OK;
                }
            }
        }
        else if ((eventSource & AINPUT_SOURCE_JOYSTICK) && eventType == AINPUT_EVENT_TYPE_MOTION)
        {
            ProcessAnalogControls1D(0, ANALOG_1D::LEFT_TRIGGER, msg);
            ProcessAnalogControls1D(0, ANALOG_1D::RIGHT_TRIGGER, msg);
            ProcessAnalogControls2D(0, ANALOG_2D::LEFT_JOYSTICK, msg);
            ProcessAnalogControls2D(0, ANALOG_2D::RIGHT_JOYSTICK, msg);
            ProcessDpad(0, msg);
        }
        return result;
    }

    void GameController::ProcessButton(int keyCode, bool isPressed)
    {
        amf::AMFLock lock(&m_Guard);
        CtlEvent ev = {};
        std::string id;

        if (m_eventMap.count(keyCode))
        {
            id = m_ID + m_eventMap[keyCode];
            ev.value.type = amf::AMF_VARIANT_BOOL;
            ev.value.boolValue = isPressed;
            FireEvent(id.c_str(), ev);
        }
    }

    void GameController::ProcessAnalogControls1D(size_t index, GameController::ANALOG_1D control,
                                                 WindowMessage const &msg)
    {
        bool fireEvent = false;
        CtlEvent ev = {};
        std::string id;
        {
            amf::AMFLock lock(&m_Guard);
            float value = 0;
            switch (control)
            {
            case ANALOG_1D::LEFT_TRIGGER:
                value = AMotionEvent_getAxisValue(msg, AMOTION_EVENT_AXIS_LTRIGGER, index);
                if (value == 0)
                {
                    value = AMotionEvent_getAxisValue(msg, AMOTION_EVENT_AXIS_BRAKE, index);
                }
                if (value != m_State.absLeftTrigger)
                {
                    id = m_ID + DEVICE_CTRL_TRIGGER_LEFT_VALUE;
                    ev.value.type = amf::AMF_VARIANT_FLOAT;
                    ev.value.floatValue = value;
                    fireEvent = true;
                    m_State.absLeftTrigger = value;
                }
                break;
            case ANALOG_1D::RIGHT_TRIGGER:
                value = AMotionEvent_getAxisValue(msg, AMOTION_EVENT_AXIS_RTRIGGER, index);
                if (value == 0)
                {
                    value = AMotionEvent_getAxisValue(msg, AMOTION_EVENT_AXIS_GAS, index);
                }
                if (value != m_State.absRightTrigger)
                {
                    id = m_ID + DEVICE_CTRL_TRIGGER_RIGHT_VALUE;
                    ev.value.type = amf::AMF_VARIANT_FLOAT;
                    ev.value.floatValue = value;
                    fireEvent = true;
                    m_State.absRightTrigger = value;
                }
                break;
            default:
                break;
            }
        }
        if (fireEvent == true)
        {
            FireEvent(id.c_str(), ev);
        }
    }

    void GameController::ProcessAnalogControls2D(size_t index, GameController::ANALOG_2D control, WindowMessage const &msg)
    {
        float x = 0;
        float y = 0;
        CtlEvent ev = {};
        std::string id;
        bool fireEvent = false;
        {
            amf::AMFLock lock(&m_Guard);
            switch (control)
            {
            case ANALOG_2D::LEFT_JOYSTICK:
                x = AMotionEvent_getAxisValue(msg, AMOTION_EVENT_AXIS_X, index);
                y = -AMotionEvent_getAxisValue(msg, AMOTION_EVENT_AXIS_Y, index);
                if (x != m_State.absX || y != m_State.absY)
                {
                    id = m_ID + DEVICE_CTRL_JOYSTICK_LEFT_VALUE;
                    ev.value.type = amf::AMF_VARIANT_FLOAT_POINT2D;
                    m_State.absX = x;
                    m_State.absY = y;
                    fireEvent = true;
                }
                break;
            case ANALOG_2D::RIGHT_JOYSTICK:
                x = AMotionEvent_getAxisValue(msg, AMOTION_EVENT_AXIS_Z, index);
                y = -AMotionEvent_getAxisValue(msg, AMOTION_EVENT_AXIS_RZ, index);
                if (x != m_State.absZ || y != m_State.absRZ)
                {
                    id = m_ID + DEVICE_CTRL_JOYSTICK_RIGHT_VALUE;
                    ev.value.type = amf::AMF_VARIANT_FLOAT_POINT2D;
                    m_State.absZ = x;
                    m_State.absRZ = y;
                    fireEvent = true;
                }
                break;
            default:
                break;
            }
        }
        if (fireEvent == true)
        {
            constexpr const float flat = 0.1;
            if (std::abs(x) < flat && std::abs(y) < flat)
            {
                x = 0;
                y = 0;
            }
            ev.value.floatPoint2DValue.x = x;
            ev.value.floatPoint2DValue.y = y;
            FireEvent(id.c_str(), ev);
        }
    }

    void GameController::ProcessDpad(size_t index, WindowMessage const &msg)
    {
        float val = 0;
        std::string idX, idY;
        bool fireEventX = false;
        bool fireEventY = false;
        CtlEvent evX = {};
        CtlEvent evY = {};
        {
            amf::AMFLock lock(&m_Guard);
            val = AMotionEvent_getAxisValue(msg, AMOTION_EVENT_AXIS_HAT_X, index);
            if (val != m_State.hat0x)
            {
                idX = m_ID;
                if (val < 0)
                {
                    idX += DEVICE_CTRL_DPAD_LEFT_CLICK;
                }
                else if (val > 0)
                {
                    idX += DEVICE_CTRL_DPAD_RIGHT_CLICK;
                }
                else
                {
                    idX += (m_State.hat0x < 0) ? DEVICE_CTRL_DPAD_LEFT_CLICK : DEVICE_CTRL_DPAD_RIGHT_CLICK;
                }
                evX.value.type = amf::AMF_VARIANT_BOOL;
                evX.value.boolValue = (val != 0);
                m_State.hat0x = val;
                fireEventX = true;
            }

            val = AMotionEvent_getAxisValue(msg, AMOTION_EVENT_AXIS_HAT_Y, index);
            if (val != m_State.hat0y)
            {
                idY = m_ID;
                if (val < 0)
                {
                    idY += DEVICE_CTRL_DPAD_UP_CLICK;
                }
                else if (val > 0)
                {
                    idY += DEVICE_CTRL_DPAD_DOWN_CLICK;
                }
                else
                {
                    idY += (m_State.hat0y < 0) ? DEVICE_CTRL_DPAD_UP_CLICK : DEVICE_CTRL_DPAD_DOWN_CLICK;
                }
                evY.value.type = amf::AMF_VARIANT_BOOL;
                evY.value.boolValue = (val != 0);
                m_State.hat0y = val;
                fireEventY = true;
            }
        }
        if (fireEventX == true)
        {
            FireEvent(idX.c_str(), evX);
        }
        if (fireEventY == true)
        {
            FireEvent(idY.c_str(), evY);
        }
    }

    //-------------------------------------------------------------------------------------------------
    void GameController::ProcessInputEvent(const char* eventID, const amf::AMFVariant& event)
    {
        if (std::strcmp(eventID, DEVICE_CTRL_HAPTIC_STATE) == 0)
        {
            if (event.type == amf::AMF_VARIANT_FLOAT_POINT2D)
            {
                AMFFloatPoint2D motorsSpeed = event.floatPoint2DValue;
                float vibrationStrength = std::max(motorsSpeed.x, motorsSpeed.y);

                if (vibrationStrength > 0.01f && m_Activity != nullptr)
                {
                    try
                    {
                        m_Activity.CallVoidMethod("performHapticFeedback", "(F)V", vibrationStrength);
                        AMFTraceDebug(AMF_FACILITY, L"Haptic sent, strength: %f", vibrationStrength);
                    }
                    catch (std::bad_function_call& e)
                    {
                        AMFTraceError(AMF_FACILITY, L"Haptic feedback not sent - not supported by the client device");
                    }
                    catch (std::invalid_argument& e)
                    {
                        AMFTraceError(AMF_FACILITY, L"Haptic feedback not sent - activity is NULL");
                    }
                }
            }
        }
    }


}// namespace ssdk::ctls
#endif
