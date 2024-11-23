/*
Notice Regarding Standards.  AMD does not provide a license or sublicense to
any Intellectual Property Rights relating to any standards, including but not
limited to any audio and/or video codec technologies such as MPEG-2, MPEG-4;
AVC/H.264; HEVC/H.265; AAC decode/FFMPEG; AAC encode/FFMPEG; VC-1; and MP3
(collectively, the "Media Technologies"). For clarity, you will pay any
royalties due for such third party technologies, which may include the Media
Technologies that are owed as a result of AMD providing the Software to you.

This software uses libraries from the FFmpeg project under the LGPLv2.1.

MIT license

Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include "GameControllerSvr.h"
#include "amf/public/include/core/Version.h"
#include "amf/public/common/TraceAdapter.h"
#include "ControllerManagerSvr.h"

namespace ssdk::ctls::svr
{
    static constexpr const wchar_t* AMF_FACILITY = L"GameControllerSvr";

    //-------------------------------------------------------------------------------------------------
    // GameController
    //-------------------------------------------------------------------------------------------------
    GameController::GameController(ControllerManagerPtr pControllerManager, int8_t iIndex) :
        ControllerBase(pControllerManager),
        m_iIndex(iIndex)
    {
        m_ID = std::string(DEVICE_GAME_CONTROLLER) + std::string(1, (char)('0' + m_iIndex));

        CreateXInputController();
    }

    GameController::~GameController()
    {
        m_pXInputController = nullptr;
    }

    //-------------------------------------------------------------------------------------------------
    void GameController::CreateXInputController()
    {
        if (m_pXInputController == nullptr && m_pControllerManager != nullptr)
        {
            AMFXInputCreateController_Fn AMFXInputCreateController = m_pControllerManager->GetAMFXInputCreateController();
            if (AMFXInputCreateController != nullptr)
            {
                amf::AMFXInputCreationDesc params = {};
                params.eType = amf::AMF_CONTROLLER_XBOX360;

                AMF_RESULT res = AMFXInputCreateController(AMF_FULL_VERSION, &params, &m_pXInputController);
                if (res == AMF_OK)
                {
                    m_pXInputController->SetCallback(this);
                    AMFTraceInfo(AMF_FACILITY, L"Game controller connected");
                }
                else
                {
                    AMFTraceInfo(AMF_FACILITY, L"AMFXInputCreateController returned error %s", amf::AMFFormatResult(res).c_str());
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------------
    void GameController::ProcessInputEvent(const char* eventID, const ssdk::ctls::CtlEvent& event)
    {
        // If Input Event is Game Controller actions (from Client)
        if (m_pXInputController != nullptr)
        {
            amf::AMFXInputState state = {};
            m_pXInputController->GetState(&state);

            if (strcmp(eventID, DEVICE_CTRL_TRIGGER_LEFT_VALUE) == 0)
            {
                state.fLeftTrigger = event.value.floatValue;
            }
            else if (strcmp(eventID, DEVICE_CTRL_TRIGGER_RIGHT_VALUE) == 0)
            {
                state.fRightTrigger = event.value.floatValue;
            }
            else if (strcmp(eventID, DEVICE_CTRL_JOYSTICK_LEFT_VALUE) == 0)
            {
                state.fThumbLX = event.value.floatPoint2DValue.x;
                state.fThumbLY = event.value.floatPoint2DValue.y;
            }
            else if (strcmp(eventID, DEVICE_CTRL_JOYSTICK_RIGHT_VALUE) == 0)
            {
                state.fThumbRX = event.value.floatPoint2DValue.x;
                state.fThumbRY = event.value.floatPoint2DValue.y;
            }
            else if (strcmp(eventID, DEVICE_CTRL_DPAD_UP_CLICK) == 0)
            {
                SetButtonState(state.uiButtonStates, event.value.boolValue, AMF_XINPUT_GAMEPAD_DPAD_UP);
            }
            else if (strcmp(eventID, DEVICE_CTRL_DPAD_DOWN_CLICK) == 0)
            {
                SetButtonState(state.uiButtonStates, event.value.boolValue, AMF_XINPUT_GAMEPAD_DPAD_DOWN);
            }
            else if (strcmp(eventID, DEVICE_CTRL_DPAD_LEFT_CLICK) == 0)
            {
                SetButtonState(state.uiButtonStates, event.value.boolValue, AMF_XINPUT_GAMEPAD_DPAD_LEFT);
            }
            else if (strcmp(eventID, DEVICE_CTRL_DPAD_RIGHT_CLICK) == 0)
            {
                SetButtonState(state.uiButtonStates, event.value.boolValue, AMF_XINPUT_GAMEPAD_DPAD_RIGHT);
            }
            else if (strcmp(eventID, DEVICE_CTRL_JOYSTICK_LEFT_CLICK) == 0)
            {
                SetButtonState(state.uiButtonStates, event.value.boolValue, AMF_XINPUT_GAMEPAD_LEFT_THUMB);
            }
            else if (strcmp(eventID, DEVICE_CTRL_JOYSTICK_RIGHT_CLICK) == 0)
            {
                SetButtonState(state.uiButtonStates, event.value.boolValue, AMF_XINPUT_GAMEPAD_RIGHT_THUMB);
            }
            else if (strcmp(eventID, DEVICE_CTRL_SYSTEM_CLICK) == 0)
            {
                SetButtonState(state.uiButtonStates, event.value.boolValue, AMF_XINPUT_GAMEPAD_START);
            }
            else if (strcmp(eventID, DEVICE_CTRL_APPMENU_CLICK) == 0)
            {
                SetButtonState(state.uiButtonStates, event.value.boolValue, AMF_XINPUT_GAMEPAD_BACK);
            }
            else if (strcmp(eventID, DEVICE_CTRL_GRIP_LEFT_CLICK) == 0)
            {
                SetButtonState(state.uiButtonStates, event.value.boolValue, AMF_XINPUT_GAMEPAD_LEFT_SHOULDER);
            }
            else if (strcmp(eventID, DEVICE_CTRL_GRIP_RIGHT_CLICK) == 0)
            {
                SetButtonState(state.uiButtonStates, event.value.boolValue, AMF_XINPUT_GAMEPAD_RIGHT_SHOULDER);
            }
            else if (strcmp(eventID, DEVICE_CTRL_A_CLICK) == 0)
            {
                SetButtonState(state.uiButtonStates, event.value.boolValue, AMF_XINPUT_GAMEPAD_A);
            }
            else if (strcmp(eventID, DEVICE_CTRL_B_CLICK) == 0)
            {
                SetButtonState(state.uiButtonStates, event.value.boolValue, AMF_XINPUT_GAMEPAD_B);
            }
            else if (strcmp(eventID, DEVICE_CTRL_X_CLICK) == 0)
            {
                SetButtonState(state.uiButtonStates, event.value.boolValue, AMF_XINPUT_GAMEPAD_X);
            }
            else if (strcmp(eventID, DEVICE_CTRL_Y_CLICK) == 0)
            {
                SetButtonState(state.uiButtonStates, event.value.boolValue, AMF_XINPUT_GAMEPAD_Y);
            }

            AMF_RESULT res = m_pXInputController->SetState(&state);
            if (res != AMF_OK)
            {
                AMFTraceWarning(AMF_FACILITY, L"SetState() failed with %s", amf::AMFFormatResult(res).c_str());
            }
        }
    }

    void GameController::SetButtonState(amf_uint32& state, bool bValue, amf_uint32 flag) const
    {
        if (bValue)
        {
            state |= flag;
        }
        else
        {
            state &= (~flag);
        }
    }

    //-------------------------------------------------------------------------------------------------
    void AMF_STD_CALL GameController::OnHaptic(amf_int32 id, const amf::AMFXInputHaptic* pHaptic)
    {
        amf::AMFLock lock(&m_Guard);

        if (m_pXInputController != nullptr && m_pControllerManager != nullptr)
        {
            if (m_pXInputController->GetControllerID() == id)
            {
                ssdk::ctls::CtlEvent ev = {};
                std::string eventID = std::string(GetID()) + std::string(DEVICE_CTRL_HAPTIC_STATE);
                ev.value.type = amf::AMF_VARIANT_FLOAT_POINT2D;
                ev.value.floatPoint2DValue.x = pHaptic->fLeftMotor;
                ev.value.floatPoint2DValue.y = pHaptic->fRightMotor;

                m_pControllerManager->SendControllerEvent(eventID.c_str(), &ev);
            }
        }
    }

}// namespace ssdk::ctls::svr
