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

#if defined(__linux)

#include "GameControllerLnx.h"
#include "../ControllerManager.h"

namespace ssdk::ctls
{
    static constexpr const wchar_t* const AMF_FACILITY = L"GameControllerLnx";
    //-------------------------------------------------------------------------------------------------
    GameController::GameController(ControllerManagerPtr pControllerManager, libevdev* dev, uint32_t index, CONNECTION_TYPE connection) :
        ControllerBase(pControllerManager),
        m_device{ dev },
        m_iIndex(index),
        m_connectionType(connection)
    {
        m_ID = std::string(DEVICE_GAME_CONTROLLER) + std::string(1, (char)('0' + m_iIndex));
        m_Description = "Game controller";

        mapEvents();
    }

    //-------------------------------------------------------------------------------------------------
    GameController::~GameController()
    {
        int fd = libevdev_get_fd(m_device);
        libevdev_free(m_device);
        close(fd);
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
    AMF_RESULT GameController::mapEvents()
    {
        m_eventMap[BTN_THUMBL] = BTN_THUMBL;
        m_eventMap[BTN_THUMBR] = BTN_THUMBR;
        m_eventMap[BTN_MODE] = BTN_MODE;
        m_eventMap[BTN_START] = BTN_START;
        m_eventMap[BTN_SELECT] = BTN_SELECT;
        m_eventMap[BTN_TL] = BTN_TL;
        m_eventMap[BTN_TR] = BTN_TR;
        m_eventMap[BTN_A] = BTN_A;
        m_eventMap[BTN_B] = BTN_B;
        m_eventMap[BTN_X] = BTN_X;
        m_eventMap[BTN_Y] = BTN_Y;
        m_eventMap[ABS_X] = ABS_X;
        m_eventMap[ABS_Y] = ABS_Y;
        m_eventMap[ABS_HAT0X] = ABS_HAT0X;
        m_eventMap[ABS_HAT0Y] = ABS_HAT0Y;
        m_eventMap[ABS_GAS] = ABS_GAS;
        m_eventMap[ABS_BRAKE] = ABS_BRAKE;
        m_eventMap[ABS_Z] = ABS_Z;
        m_eventMap[ABS_RZ] = ABS_RZ;
        m_eventMap[ABS_RY] = ABS_RY;
        m_eventMap[ABS_RX] = ABS_RX;

        switch (m_connectionType)
        {
        case CONNECTION_TYPE::USB:
            m_xAxisShift = 0;
            m_yAxisShift = 0;
            m_eventMap[ABS_RY] = ABS_RZ;
            m_eventMap[ABS_RX] = ABS_Z;
            m_eventMap[ABS_Z] = ABS_BRAKE;
            m_eventMap[ABS_RZ] = ABS_GAS;
            break;
        case CONNECTION_TYPE::BLUETOOTH:
            m_xAxisShift = 32768;
            m_yAxisShift = 32767;
            break;
        default:
            break;
        }
        return AMF_OK;
    }

    //-------------------------------------------------------------------------------------------------
    void AMF_STD_CALL GameController::FireEvent(const char* id, CtlEvent* ev)
    {
        if (m_pControllerManager != nullptr)
        {
            m_pControllerManager->SendControllerEvent(id, ev);
        }
    }

    //-------------------------------------------------------------------------------------------------
    void AMF_STD_CALL GameController::UpdateSensors()
    {
        Update();
    }

    //-------------------------------------------------------------------------------------------------
    AMF_RESULT AMF_STD_CALL GameController::Update()
    {
        amf::AMFLock lock(&m_Guard);

        struct input_event event;
        DEVICE_STATE state = m_State;
        amf_int eventResult = 0;

        while ((eventResult = libevdev_next_event(m_device, LIBEVDEV_READ_FLAG_NORMAL, &event)) == LIBEVDEV_READ_STATUS_SUCCESS)
        {
            CtlEvent ev = {};
            std::string id;

            if (event.type == EV_KEY && m_eventMap[event.code] >= BTN_GAMEPAD && m_eventMap[event.code] <= BTN_THUMBR)
            {
                switch (m_eventMap[event.code])
                {
                case BTN_THUMBL:
                {
                    id = m_ID;
                    id += DEVICE_CTRL_JOYSTICK_LEFT_CLICK;
                    ev.value.type = amf::AMF_VARIANT_BOOL;
                    ev.value.boolValue = event.value != 0;
                    FireEvent(id.c_str(), &ev);
                    break;
                }
                case BTN_THUMBR:
                {
                    id = m_ID;
                    id += DEVICE_CTRL_JOYSTICK_RIGHT_CLICK;
                    ev.value.type = amf::AMF_VARIANT_BOOL;
                    ev.value.boolValue = event.value != 0;
                    FireEvent(id.c_str(), &ev);
                    break;
                }
                case BTN_MODE:
                {
                    id = m_ID;
                    id += DEVICE_CTRL_MODE_CLICK;
                    ev.value.type = amf::AMF_VARIANT_BOOL;
                    ev.value.boolValue = event.value != 0;
                    FireEvent(id.c_str(), &ev);
                    break;
                }
                case BTN_START:
                {
                    id = m_ID;
                    id += DEVICE_CTRL_SYSTEM_CLICK;
                    ev.value.type = amf::AMF_VARIANT_BOOL;
                    ev.value.boolValue = event.value != 0;
                    FireEvent(id.c_str(), &ev);
                    break;
                }
                case BTN_SELECT:
                {
                    id = m_ID;
                    id += DEVICE_CTRL_APPMENU_CLICK;
                    ev.value.type = amf::AMF_VARIANT_BOOL;
                    ev.value.boolValue = event.value != 0;
                    FireEvent(id.c_str(), &ev);
                    break;
                }
                case BTN_TL:
                {
                    id = m_ID;
                    id += DEVICE_CTRL_GRIP_LEFT_CLICK;
                    ev.value.type = amf::AMF_VARIANT_BOOL;
                    ev.value.boolValue = event.value != 0;
                    FireEvent(id.c_str(), &ev);
                    break;
                }
                case BTN_TR:
                {
                    id = m_ID;
                    id += DEVICE_CTRL_GRIP_RIGHT_CLICK;
                    ev.value.type = amf::AMF_VARIANT_BOOL;
                    ev.value.boolValue = event.value != 0;
                    FireEvent(id.c_str(), &ev);
                    break;
                }
                case BTN_A:
                {
                    id = m_ID;
                    id += DEVICE_CTRL_A_CLICK;
                    ev.value.type = amf::AMF_VARIANT_BOOL;
                    ev.value.boolValue = event.value != 0;
                    FireEvent(id.c_str(), &ev);
                    break;
                }
                case BTN_B:
                {
                    id = m_ID;
                    id += DEVICE_CTRL_B_CLICK;
                    ev.value.type = amf::AMF_VARIANT_BOOL;
                    ev.value.boolValue = event.value != 0;
                    FireEvent(id.c_str(), &ev);
                    break;
                }
                case BTN_X:
                {
                    id = m_ID;
                    id += DEVICE_CTRL_X_CLICK;
                    ev.value.type = amf::AMF_VARIANT_BOOL;
                    ev.value.boolValue = event.value != 0;
                    FireEvent(id.c_str(), &ev);
                    break;
                }
                case BTN_Y:
                {
                    id = m_ID;
                    id += DEVICE_CTRL_Y_CLICK;
                    ev.value.type = amf::AMF_VARIANT_BOOL;
                    ev.value.boolValue = event.value != 0;
                    FireEvent(id.c_str(), &ev);
                    break;
                }
                default:
                    break;
                }
            }
            if (event.type == EV_ABS && m_eventMap[event.code] >= ABS_X && m_eventMap[event.code] <= ABS_HAT0Y)
            {
                switch (m_eventMap[event.code])
                {
                case ABS_GAS:
                {
                    id = m_ID;
                    id += DEVICE_CTRL_TRIGGER_RIGHT_VALUE;
                    ev.value.type = amf::AMF_VARIANT_FLOAT;
                    ev.value.floatValue = event.value / 255.f;
                    FireEvent(id.c_str(), &ev);
                    break;
                }
                case ABS_BRAKE:
                {
                    id = m_ID;
                    id += DEVICE_CTRL_TRIGGER_LEFT_VALUE;
                    ev.value.type = amf::AMF_VARIANT_FLOAT;
                    ev.value.floatValue = event.value / 255.f;
                    FireEvent(id.c_str(), &ev);
                    break;
                }
                case ABS_X:
                {
                    id = m_ID;
                    id += DEVICE_CTRL_JOYSTICK_LEFT_VALUE;
                    ev.value.type = amf::AMF_VARIANT_FLOAT_POINT2D;
                    ev.value.floatPoint2DValue.x = (event.value - m_xAxisShift) / 32767.f;
                    ev.value.floatPoint2DValue.y = (m_yAxisShift - m_State.absY) / 32767.f;
                    FireEvent(id.c_str(), &ev);
                    state.absX = event.value;
                    state.updated = true;
                    break;
                }
                case ABS_Y:
                {
                    id = m_ID;
                    id += DEVICE_CTRL_JOYSTICK_LEFT_VALUE;
                    ev.value.type = amf::AMF_VARIANT_FLOAT_POINT2D;
                    ev.value.floatPoint2DValue.x = (m_State.absX - m_xAxisShift) / 32767.f;
                    ev.value.floatPoint2DValue.y = (m_yAxisShift - event.value) / 32767.f;
                    FireEvent(id.c_str(), &ev);
                    state.absY = event.value;
                    state.updated = true;
                    break;
                }
                case ABS_Z:
                {
                    id = m_ID;
                    id += DEVICE_CTRL_JOYSTICK_RIGHT_VALUE;
                    ev.value.type = amf::AMF_VARIANT_FLOAT_POINT2D;
                    ev.value.floatPoint2DValue.x = (event.value - m_xAxisShift) / 32767.f;
                    ev.value.floatPoint2DValue.y = (m_yAxisShift - m_State.absRZ) / 32767.f;
                    FireEvent(id.c_str(), &ev);
                    state.absZ = event.value;
                    state.updated = true;
                    break;
                }
                case ABS_RZ:
                {
                    id = m_ID;
                    id += DEVICE_CTRL_JOYSTICK_RIGHT_VALUE;
                    ev.value.type = amf::AMF_VARIANT_FLOAT_POINT2D;
                    ev.value.floatPoint2DValue.x = (m_State.absZ - m_xAxisShift) / 32767.f;
                    ev.value.floatPoint2DValue.y = (m_yAxisShift - event.value) / 32767.f;
                    FireEvent(id.c_str(), &ev);
                    state.absRZ = event.value;
                    state.updated = true;
                    break;
                }
                case ABS_HAT0X:
                {
                    id = m_ID;
                    if (event.value < 0)
                    {
                        id += DEVICE_CTRL_DPAD_LEFT_CLICK;
                    }
                    else if (event.value > 0)
                    {
                        id += DEVICE_CTRL_DPAD_RIGHT_CLICK;
                    }
                    else
                    {
                        if (m_State.hat0x < 0)
                        {
                            id += DEVICE_CTRL_DPAD_LEFT_CLICK;
                        }
                        else
                        {
                            id += DEVICE_CTRL_DPAD_RIGHT_CLICK;
                        }
                    }

                    ev.value.type = amf::AMF_VARIANT_BOOL;
                    ev.value.boolValue = event.value != 0;
                    FireEvent(id.c_str(), &ev);
                    state.hat0x = event.value;
                    state.updated = true;
                    break;
                }
                case ABS_HAT0Y:
                {
                    id = m_ID;
                    if (event.value < 0)
                    {
                        id += DEVICE_CTRL_DPAD_UP_CLICK;
                    }
                    else if (event.value > 0)
                    {
                        id += DEVICE_CTRL_DPAD_DOWN_CLICK;
                    }
                    else
                    {
                        if (m_State.hat0y < 0)
                        {
                            id += DEVICE_CTRL_DPAD_UP_CLICK;
                        }
                        else
                        {
                            id += DEVICE_CTRL_DPAD_DOWN_CLICK;
                        }
                    }
                    ev.value.type = amf::AMF_VARIANT_BOOL;
                    ev.value.boolValue = event.value != 0;
                    FireEvent(id.c_str(), &ev);
                    state.hat0y = event.value;
                    state.updated = true;
                    break;
                }
                default:
                    break;
                }
            }
        }
        if (state.updated)
        {
            m_State = state;
        }

        if (eventResult == -ENODEV)
        {
            return AMF_NO_DEVICE;
        }

        return AMF_OK;
    }

    void AMF_STD_CALL GameController::ProcessInputEvent(const char* eventID, const amf::AMFVariant& event)
    {
        if (strcmp(eventID, DEVICE_CTRL_HAPTIC_STATE) == 0)
        {
            // Get motors speed and set to game controller
            if (event.type == amf::AMF_VARIANT_FLOAT_POINT2D)
            {
                // Get motors speed
                AMFFloatPoint2D motorsSpeed = event.floatPoint2DValue;

                // Vibrate
                int fd = libevdev_get_fd(m_device);
                if (m_rumble_effect != -1)
                {
                    ioctl(fd, EVIOCRMFF, m_rumble_effect);
                    m_rumble_effect = -1;
                }

                struct ff_effect effect = { 0 };
                effect.type = FF_RUMBLE;
                effect.id = m_rumble_effect;
                effect.replay.length = 1000;
                effect.u.rumble.strong_magnitude = motorsSpeed.x * 65535;
                effect.u.rumble.weak_magnitude = motorsSpeed.y * 65535;

                if (ioctl(fd, EVIOCSFF, &effect) == -1)
                {
                    return;
                }

                struct input_event rumble_effect;
                rumble_effect.type = EV_FF;
                rumble_effect.code = effect.id;
                rumble_effect.value = 1;

                if (write(fd, (const void*)&rumble_effect, sizeof(rumble_effect)) != -1)
                {
                    m_rumble_effect = rumble_effect.code;
                }
                else
                {
                    AMFTraceError(AMF_FACILITY, L"ProcessInputEvent: Failed to write event");
                }
            }
        }
    }

}// namespace ssdk::ctls
#endif
