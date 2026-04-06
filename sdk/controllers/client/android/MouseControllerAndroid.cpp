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

Copyright (c) Advanced Micro Devices, Inc. All rights reserved.

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

#include "MouseControllerAndroid.h"
#include "../ControllerManager.h"

#include "amf/public/common/TraceAdapter.h"

namespace ssdk::ctls
{
    static constexpr const wchar_t* AMF_FACILITY = L"ControllerManager";

    MouseControllerAndroid::MouseControllerAndroid(ControllerManagerPtr pControllerManager, ssdk::video::VideoReceiver::Ptr pVideoPipeline) :
        MouseController(std::move(pControllerManager)),
        m_pVideoPipeline(std::move(pVideoPipeline))
    {
    }

    AMF_RESULT  AMF_STD_CALL MouseControllerAndroid::ProcessMessage(const WindowMessage& msg)
    {
        AMF_RESULT result = AMF_FAIL;
        if ((AInputEvent_getSource(msg) & AINPUT_SOURCE_MOUSE) && (AInputEvent_getType(msg) == AINPUT_EVENT_TYPE_MOTION))
        {
            int32_t action = AMotionEvent_getAction(msg) & AMOTION_EVENT_ACTION_MASK;
            switch (action)
            {
            case AMOTION_EVENT_ACTION_BUTTON_PRESS:
            case AMOTION_EVENT_ACTION_BUTTON_RELEASE:
                result = HandleButtonPresses(AMotionEvent_getButtonState(msg));
                break;
            case AMOTION_EVENT_ACTION_SCROLL:
                result = HandleWheel(AMotionEvent_getAxisValue(msg, AMOTION_EVENT_AXIS_HSCROLL, 0), AMotionEvent_getAxisValue(msg, AMOTION_EVENT_AXIS_VSCROLL, 0));
                break;
            case AMOTION_EVENT_ACTION_MOVE:
            case AMOTION_EVENT_ACTION_HOVER_MOVE:
                result = HandleMove(AMotionEvent_getX(msg, 0), AMotionEvent_getY(msg, 0));
                break;
            case AMOTION_EVENT_ACTION_HOVER_ENTER:
                break;
            case AMOTION_EVENT_ACTION_HOVER_EXIT:
                break;
            default:
                break;
            }
        }
        return result;
    }

    AMF_RESULT MouseControllerAndroid::HandleButtonPresses(int32_t buttonState)
    {
        AMF_RESULT result = AMF_FAIL;
        if (buttonState != m_LastButtonState)
        {
            CtlEvent ev = {};
            ev.value.type = amf::AMF_VARIANT_BOOL;
            std::string id;

            int32_t changedBits = buttonState ^ m_LastButtonState;
            if (changedBits & AMOTION_EVENT_BUTTON_PRIMARY)
            {
                id = m_EventIDs[DEVICE_MOUSE_L_CLICK_INDEX].c_str();
                ev.value.boolValue = (buttonState & AMOTION_EVENT_BUTTON_PRIMARY) != 0;
            }
            if (changedBits & AMOTION_EVENT_BUTTON_SECONDARY)
            {
                id = m_EventIDs[DEVICE_MOUSE_R_CLICK_INDEX].c_str();
                ev.value.boolValue = (buttonState & AMOTION_EVENT_BUTTON_SECONDARY) != 0;
            }
            if (changedBits & AMOTION_EVENT_BUTTON_TERTIARY)
            {
                id = m_EventIDs[DEVICE_MOUSE_M_CLICK_INDEX].c_str();
                ev.value.boolValue = (buttonState & AMOTION_EVENT_BUTTON_TERTIARY) != 0;
            }
            m_LastButtonState = buttonState;
            //  Send mouse button event
            if (id.length() != 0)
            {
                if (m_pControllerManager != nullptr)
                {
                    m_pControllerManager->SendControllerEvent(id.c_str(), ev);
                    result = AMF_OK;
                }
            }
        }
        return result;
    }

    AMF_RESULT MouseControllerAndroid::HandleWheel(int32_t /*hStep*/, int32_t vStep)
    {
        AMF_RESULT result = AMF_FAIL;
        CtlEvent ev = {};
        ev.value.type = amf::AMF_VARIANT_BOOL;
        std::string id;

        ev.value.type = amf::AMF_VARIANT_INT64;
        id = m_EventIDs[DEVICE_MOUSE_M_TRACK_INDEX].c_str();
        ev.value.int64Value = vStep * 120;
        if (m_pControllerManager != nullptr)
        {
            m_pControllerManager->SendControllerEvent(id.c_str(), ev);
            result = AMF_OK;
        }
        return result;
    }

    AMF_RESULT MouseControllerAndroid::HandleMove(float x, float y)
    {
        AMF_RESULT result = AMF_FAIL;
        if (x != m_LastMousePoint.x || y != m_LastMousePoint.y)
        {
            m_LastMousePoint.x = x;
            m_LastMousePoint.y = y;
            AMFRect viewport = m_pVideoPipeline->GetDisplayViewport();
            if (x >= viewport.left && x <= viewport.right && y >= viewport.top && y <= viewport.bottom)
            {
                float normalizedX = float(x - viewport.left) / float(viewport.right - viewport.left);
                float normalizedY = float(y - viewport.top) / float(viewport.bottom - viewport.top);

                std::string id;

                CtlEvent ev = {};
                ev.value.type = amf::AMF_VARIANT_FLOAT_POINT2D;
                ev.flags = DEVICE_MOUSE_POS_ABSOLUTE;
                ev.value.floatPoint2DValue.x = normalizedX;
                ev.value.floatPoint2DValue.y = normalizedY;
                id = m_EventIDs[DEVICE_MOUSE_POS_INDEX].c_str();
                if (m_pControllerManager != nullptr)
                {
                    m_pControllerManager->SendControllerEvent(id.c_str(), ev);
                    result = AMF_OK;
                }
            }
        }
        return result;
    }

    void AMF_STD_CALL MouseControllerAndroid::ProcessInputEvent(const char* /*eventID*/, const amf::AMFVariant& /*event*/)
    {

    }

    void MouseControllerAndroid::UpdateFromServer()
    {

    }
}
