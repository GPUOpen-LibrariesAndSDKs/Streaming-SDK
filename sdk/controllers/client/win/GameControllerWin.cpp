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
// Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
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

#if defined(_WIN32)

#include "GameControllerWin.h"
#include "../ControllerManager.h"

namespace ssdk::ctls
{
    //static constexpr const wchar_t* const AMF_FACILITY = L"GameController";

    //-------------------------------------------------------------------------------------------------
    GameController::GameController(ControllerManagerPtr pControllerManager, DWORD iIndex) :
        ControllerBase(pControllerManager),
        m_iIndex(iIndex),
        m_State{}
    {
        m_ID = std::string(DEVICE_GAME_CONTROLLER) + std::string(1, (char)('0' + m_iIndex));
        m_Description = "Game controller";
    }

    //-------------------------------------------------------------------------------------------------
    GameController::~GameController()
    {
    }

    //-------------------------------------------------------------------------------------------------
    void GameController::OnConnectionEstablished()
    {
        if (m_pControllerManager != nullptr)
        {
            ssdk::transport_common::ClientTransport::Ptr pClientTransport = m_pControllerManager->GetClientTransport();
            if (pClientTransport != nullptr)
            {
                pClientTransport->SendDeviceConnected(GetID(), GetType(), m_Description.c_str());
            }
        }
    }

    void GameController::OnConnectionTerminated()
    {
        if (m_pControllerManager != nullptr)
        {
            ssdk::transport_common::ClientTransport::Ptr pClientTransport = m_pControllerManager->GetClientTransport();
            if (pClientTransport != nullptr)
            {
                pClientTransport->SendDeviceDisconnected(GetID());
            }
        }
    }

    //-------------------------------------------------------------------------------------------------
    void AMF_STD_CALL GameController::FireEvent(const char* id, CtlEvent* ev)
    {
        if (m_pControllerManager != nullptr)
        {
            //AMFTraceInfo(AMF_FACILITY, L"GameController::FireEvent");
            m_pControllerManager->SendControllerEvent(id, ev);
        }
    }

    //-------------------------------------------------------------------------------------------------
    void AMF_STD_CALL GameController::UpdateSensors()
    {
        XINPUT_CAPABILITIES caps = {};
        if (XInputGetCapabilities(m_iIndex, XINPUT_FLAG_GAMEPAD, &caps) == ERROR_SUCCESS)
        {
            Update();
        }
    }

    //-------------------------------------------------------------------------------------------------
    AMF_RESULT AMF_STD_CALL GameController::Update()
    {
        amf::AMFLock lock(&m_Guard);

        // get controller state
        XINPUT_STATE state = {};
        if (XInputGetState(m_iIndex, &state) == ERROR_SUCCESS)
        {
            // send events if state changed
            CtlEvent ev = {};
            std::string id;
            if (m_State.Gamepad.bLeftTrigger != state.Gamepad.bLeftTrigger)
            {
                id = m_ID;
                id += DEVICE_CTRL_TRIGGER_LEFT_VALUE;
                ev.value.type = amf::AMF_VARIANT_FLOAT;
                ev.value.floatValue = state.Gamepad.bLeftTrigger / 255.f;
                FireEvent(id.c_str(), &ev);
            }
            if (m_State.Gamepad.bRightTrigger != state.Gamepad.bRightTrigger)
            {
                id = m_ID;
                id += DEVICE_CTRL_TRIGGER_RIGHT_VALUE;
                ev.value.type = amf::AMF_VARIANT_FLOAT;
                ev.value.floatValue = state.Gamepad.bRightTrigger / 255.f;
                FireEvent(id.c_str(), &ev);
            }
            if (m_State.Gamepad.sThumbLX != state.Gamepad.sThumbLX || m_State.Gamepad.sThumbLY != state.Gamepad.sThumbLY)
            {
                id = m_ID;
                id += DEVICE_CTRL_JOYSTICK_LEFT_VALUE;
                ev.value.type = amf::AMF_VARIANT_FLOAT_POINT2D;
                ev.value.floatPoint2DValue.x = state.Gamepad.sThumbLX / 32767.f;
                ev.value.floatPoint2DValue.y = state.Gamepad.sThumbLY / 32767.f;
                FireEvent(id.c_str(), &ev);
            }
            if (m_State.Gamepad.sThumbRX != state.Gamepad.sThumbRX || m_State.Gamepad.sThumbRY != state.Gamepad.sThumbRY)
            {
                id = m_ID;
                id += DEVICE_CTRL_JOYSTICK_RIGHT_VALUE;
                ev.value.type = amf::AMF_VARIANT_FLOAT_POINT2D;
                ev.value.floatPoint2DValue.x = state.Gamepad.sThumbRX / 32767.f;
                ev.value.floatPoint2DValue.y = state.Gamepad.sThumbRY / 32767.f;
                FireEvent(id.c_str(), &ev);
            }
            if ((m_State.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP))
            {
                id = m_ID;
                id += DEVICE_CTRL_DPAD_UP_CLICK;
                ev.value.type = amf::AMF_VARIANT_BOOL;
                ev.value.boolValue = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0;
                FireEvent(id.c_str(), &ev);
            }
            if ((m_State.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN))
            {
                id = m_ID;
                id += DEVICE_CTRL_DPAD_DOWN_CLICK;
                ev.value.type = amf::AMF_VARIANT_BOOL;
                ev.value.boolValue = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
                FireEvent(id.c_str(), &ev);
            }
            if ((m_State.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT))
            {
                id = m_ID;
                id += DEVICE_CTRL_DPAD_LEFT_CLICK;
                ev.value.type = amf::AMF_VARIANT_BOOL;
                ev.value.boolValue = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
                FireEvent(id.c_str(), &ev);
            }
            if ((m_State.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT))
            {
                id = m_ID;
                id += DEVICE_CTRL_DPAD_RIGHT_CLICK;
                ev.value.type = amf::AMF_VARIANT_BOOL;
                ev.value.boolValue = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;
                FireEvent(id.c_str(), &ev);
            }
            if ((m_State.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB))
            {
                id = m_ID;
                id += DEVICE_CTRL_JOYSTICK_LEFT_CLICK;
                ev.value.type = amf::AMF_VARIANT_BOOL;
                ev.value.boolValue = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0;
                FireEvent(id.c_str(), &ev);
            }
            if ((m_State.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB))
            {
                id = m_ID;
                id += DEVICE_CTRL_JOYSTICK_RIGHT_CLICK;
                ev.value.type = amf::AMF_VARIANT_BOOL;
                ev.value.boolValue = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;
                FireEvent(id.c_str(), &ev);
            }
            if ((m_State.Gamepad.wButtons & XINPUT_GAMEPAD_START) != (state.Gamepad.wButtons & XINPUT_GAMEPAD_START))
            {
                id = m_ID;
                id += DEVICE_CTRL_SYSTEM_CLICK;
                ev.value.type = amf::AMF_VARIANT_BOOL;
                ev.value.boolValue = (state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0;
                FireEvent(id.c_str(), &ev);
            }
            if ((m_State.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != (state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK))
            {
                id = m_ID;
                id += DEVICE_CTRL_APPMENU_CLICK;
                ev.value.type = amf::AMF_VARIANT_BOOL;
                ev.value.boolValue = (state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0;
                FireEvent(id.c_str(), &ev);
            }
            if ((m_State.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER))
            {
                id = m_ID;
                id += DEVICE_CTRL_GRIP_LEFT_CLICK;
                ev.value.type = amf::AMF_VARIANT_BOOL;
                ev.value.boolValue = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
                FireEvent(id.c_str(), &ev);
            }
            if ((m_State.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER))
            {
                id = m_ID;
                id += DEVICE_CTRL_GRIP_RIGHT_CLICK;
                ev.value.type = amf::AMF_VARIANT_BOOL;
                ev.value.boolValue = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
                FireEvent(id.c_str(), &ev);
            }
            if ((m_State.Gamepad.wButtons & XINPUT_GAMEPAD_A) != (state.Gamepad.wButtons & XINPUT_GAMEPAD_A))
            {
                id = m_ID;
                id += DEVICE_CTRL_A_CLICK;
                ev.value.type = amf::AMF_VARIANT_BOOL;
                ev.value.boolValue = (state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
                FireEvent(id.c_str(), &ev);
            }
            if ((m_State.Gamepad.wButtons & XINPUT_GAMEPAD_B) != (state.Gamepad.wButtons & XINPUT_GAMEPAD_B))
            {
                id = m_ID;
                id += DEVICE_CTRL_B_CLICK;
                ev.value.type = amf::AMF_VARIANT_BOOL;
                ev.value.boolValue = (state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0;
                FireEvent(id.c_str(), &ev);
            }
            if ((m_State.Gamepad.wButtons & XINPUT_GAMEPAD_X) != (state.Gamepad.wButtons & XINPUT_GAMEPAD_X))
            {
                id = m_ID;
                id += DEVICE_CTRL_X_CLICK;
                ev.value.type = amf::AMF_VARIANT_BOOL;
                ev.value.boolValue = (state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0;
                FireEvent(id.c_str(), &ev);
            }
            if ((m_State.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != (state.Gamepad.wButtons & XINPUT_GAMEPAD_Y))
            {
                id = m_ID;
                id += DEVICE_CTRL_Y_CLICK;
                ev.value.type = amf::AMF_VARIANT_BOOL;
                ev.value.boolValue = (state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0;
                FireEvent(id.c_str(), &ev);
            }
            m_State = state;
        }
        return AMF_OK;
    }

    //-------------------------------------------------------------------------------------------------
    void GameController::ProcessInputEvent(const char* eventID, const amf::AMFVariant& event)
    {
        // If Input Event is haptic vibration (from Server)
        if (std::strcmp(eventID, DEVICE_CTRL_HAPTIC_STATE) == 0)
        {
            // Get motors speed and set to game controller
            if (event.type == amf::AMF_VARIANT_FLOAT_POINT2D)
            {
                // Get motors speed
                AMFFloatPoint2D motorsSpeed = event.floatPoint2DValue;

                // Vibrate
                XINPUT_VIBRATION vibration = {};
                vibration.wLeftMotorSpeed = WORD(motorsSpeed.x * 65535);
                vibration.wRightMotorSpeed = WORD(motorsSpeed.y * 65535);
                XInputSetState(m_iIndex, &vibration);
            }
        }
    }

}// namespace ssdk::ctls
#endif
