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

#include "X11TouchController.h"
#include "controllers/TouchEvent.h"
#include "../ControllerManager.h"

namespace ssdk::ctls
{

#if !defined(_WIN32)
    X11TouchController::X11TouchController(ControllerManagerPtr pControllerManager, Display* dpy, Window hWnd,
        ssdk::video::VideoReceiverPipeline::Ptr pVideoPipeline) :
        ControllerBase(pControllerManager),
        m_pDpy(dpy),
        m_hWnd(hWnd),
        m_pVideoPipeline(pVideoPipeline)
    {
        m_ID = DEVICE_TOUCHSCREEN;
        m_Description = "Touch Screen";
        m_EventIDs.push_back(m_ID + DEVICE_TOUCHSCREEN_TOUCH);
    }

    //-------------------------------------------------------------------------------------------------
    void X11TouchController::OnConnectionEstablished()
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

    void X11TouchController::OnConnectionTerminated()
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
    AMF_RESULT AMF_STD_CALL X11TouchController::ProcessMessageExt(const ExtWindowMessage& msg)
    {
        TouchEvent touchEvent{};
        int32_t action{};
        switch (msg.evtype)
        {
        case XI_TouchBegin:
            action = TOUCH_EVENT_ACTION_POINTER_DOWN;
            break;
        case XI_TouchUpdate:
            action = TOUCH_EVENT_ACTION_MOVE;
            break;
        case XI_TouchEnd:
            action = TOUCH_EVENT_ACTION_POINTER_UP;
            break;
        case XI_RawTouchBegin:
        case XI_RawTouchUpdate:
        case XI_RawTouchEnd:
            return AMF_OK;
        default:
            return AMF_OK;
        }

        int pointerId = msg.detail % 0xFF; // [0, 255] TOUCH_EVENT_ACTION_POINTER_INDEX_MASK(0x0000FF00)
        int32_t packedAction = action | (pointerId << TOUCH_EVENT_ACTION_POINTER_INDEX_SHIFT);
        touchEvent = TouchEvent(packedAction, msg.time);

        TouchPoint normalized = NormalizeTouchPoint({ pointerId, msg.event_x, msg.event_y });
        touchEvent.AddPointer(normalized.touchId, normalized.x, normalized.y);

        CtlEvent ev{};
        ev.value.type = amf::AMF_VARIANT_INTERFACE;
        TouchEvent::Ptr pTouchEvent = new TouchEvent{};
        *pTouchEvent = std::move(touchEvent);
        ev.value.pInterface = pTouchEvent.Detach();

        if (m_pControllerManager != nullptr)
        {
            m_pControllerManager->SendControllerEvent(m_EventIDs[0].c_str(), &ev);
        }

        return AMF_OK;
    }

    //-------------------------------------------------------------------------------------------------
    void X11TouchController::ProcessInputEvent(const char* eventID, const amf::AMFVariant& /*event*/)
    {
        if (strcmp(eventID, DEVICE_TOUCHSCREEN_TOUCH) == 0)
        {
        }
    }

    //-------------------------------------------------------------------------------------------------
    X11TouchController::TouchPoint X11TouchController::NormalizeTouchPoint(TouchPoint touchPoint) const
    {
        Window root_return{};
        int x_return{}, y_return{};
        unsigned int width_return{}, height_return{};
        unsigned int border_width_return{};
        unsigned int depth_return{};
        XGetGeometry(m_pDpy, m_hWnd, &root_return, &x_return, &y_return, &width_return, &height_return, &border_width_return, &depth_return);
        AMFSize frameSize = AMFConstructSize(width_return - 2 * border_width_return, height_return - 2 * border_width_return);

        AMFRect rcViewport{ 0, 0, frameSize.width, frameSize.height };
        if (m_pVideoPipeline != nullptr)
        {
            rcViewport = m_pVideoPipeline->GetViewport();
        }
        
        double viewportAspectRatio = double(rcViewport.Width()) / double(rcViewport.Height());
        double frameAspectRatio = double(frameSize.width) / double(frameSize.height);
        
        if (viewportAspectRatio == frameAspectRatio)
        {
            touchPoint.x /= float(frameSize.width);
            touchPoint.y /= float(frameSize.height);
        }
        else
        {
            AMFRect viewport = {};
            if (viewportAspectRatio < frameAspectRatio)         //  viewport is narrower than frame, black bars on the sides
            {
                int32_t viewportWidth = static_cast<int32_t>(frameSize.height * viewportAspectRatio);
                viewport.left = (frameSize.width - viewportWidth) / 2;
                viewport.right = frameSize.width - viewport.left;
                viewport.top = 0;
                viewport.bottom = frameSize.height;
            }
            else // if (viewportAspectRatio > frameAspectRatio) //  viewport is wider than frame, black bars above and below
            {
                int32_t viewportHeight = static_cast<int32_t>(frameSize.width / viewportAspectRatio);
                viewport.top = (frameSize.height - viewportHeight) / 2;
                viewport.bottom = frameSize.height - viewport.top;
                viewport.left = 0;
                viewport.right = frameSize.width;
            }

            touchPoint.x = (touchPoint.x - viewport.left) / float(viewport.Width());
            touchPoint.y = (touchPoint.y - viewport.top) / float(viewport.Height());
        }

        return touchPoint;
    }
#endif
}
