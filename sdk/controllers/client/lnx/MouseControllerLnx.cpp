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

#include "../MouseController.h"
#include "../ControllerManager.h"

namespace ssdk::ctls
{
    static constexpr const wchar_t* const AMF_FACILITY = L"MouseControllerLnx";
    //-------------------------------------------------------------------------------------------------
    void MouseController::Init()
    {
        // Check for WAYLAND_DISPLAY environment variable
        const char* wayland_display = std::getenv("WAYLAND_DISPLAY");
        if (wayland_display)
        {
            AMFTraceDebug(AMF_FACILITY, L"The current protocol is Wayland. WAYLAND_DISPLAY: %S", wayland_display);
            m_bWayland = true;
        }
        else
        {
            AMFTraceDebug(AMF_FACILITY, L"The current protocol is not Wayland (likely X11 or another protocol).");
            m_bWayland = false;
        }
    }

    //-------------------------------------------------------------------------------------------------
    void MouseController::UpdateFromServer()
    {
        Window focusedWindow;
        int revert_to;
        XDisplayPtr dpy(m_dpy);
        XGetInputFocus(dpy, &focusedWindow, &revert_to);
        if (amf_handle(focusedWindow) == m_hWnd)
        {
            amf::AMFLock lock(&m_Sect);

            AMFFloatPoint2D fMousePos = GetMouseConfig()->MousePosition();
            WindowPoint pointScreen = { int(fMousePos.x), int(fMousePos.y) };

            // Hide Cursor for warping on XWayland
            if (m_bWayland == true)
            {
                // Hide Cursor
                XFixesHideCursor((Display*)dpy, (Window)m_hWnd);
                XFlush((Display*)dpy);
            }
            
            // Set Cursor pos
            XWarpPointer((Display*)dpy, None, (Window)m_hWnd, 0, 0, 0, 0, pointScreen.x, pointScreen.y);
            XFlush((Display*)dpy);
            
            // Show Cursor
            if (m_bWayland == true)
            {
                // Show Cursor
                XFixesShowCursor((Display*)dpy, (Window)m_hWnd);
                XFlush((Display*)dpy);
            }
            
            //AMFTraceInfo(AMF_FACILITY, L"MouseController::MousePos(%d,%d)", pointScreen.x, pointScreen.y);
            m_LastMousePoint.x = pointScreen.x;
            m_LastMousePoint.y = pointScreen.y;
        }
    }

    //-------------------------------------------------------------------------------------------------
    AMF_RESULT MouseController::UpdateFromInternalInput(const WindowMessage& msg)
    {
        amf::AMFLock lock(&m_Sect);
        const XIRawEvent* rawEvent = (const XIRawEvent*)msg.xcookie.data;
        const double* rawInput = (const double*)rawEvent->raw_values;
        amf_int32 relativeRawPos[2] = { 0 };

        for (amf_int32 i = 0; i < 2; i++)
        {
            if (XIMaskIsSet(rawEvent->valuators.mask, i))
            {
                relativeRawPos[i] = (amf_int32) int(*rawInput++);
            }
        }

        XWindowAttributes attrib;
        XDisplayPtr dpy(m_dpy);
        XGetWindowAttributes(dpy, (Window)m_hWnd, &attrib);

        int diffXaxis = relativeRawPos[0];
        int diffYaxis = relativeRawPos[1];

        if (diffXaxis == 0 && diffYaxis == 0)
        {
            return AMF_OK;
        }

        CtlEvent ev = {};
        ev.value.type = amf::AMF_VARIANT_FLOAT_POINT2D;
        ev.flags = DEVICE_MOUSE_POS_ABSOLUTE;

        WindowPoint pointScreen = { -1, -1 };

        // Normalize mouse position
        ev.value.floatPoint2DValue.x = float(diffXaxis) / float(attrib.width);
        ev.value.floatPoint2DValue.y = float(diffYaxis) / float(attrib.height);

        pointScreen.x = m_LastMousePoint.x + diffXaxis;
        pointScreen.y = m_LastMousePoint.y + diffYaxis;
        //AMFTraceInfo(AMF_FACILITY, L"MouseController::UpdateFromInternalInput diffXY(%d,%d)", diffXaxis, diffYaxis);

        if (pointScreen.x != m_LastMousePoint.x || pointScreen.y != m_LastMousePoint.y)
        {
            if (m_pControllerManager != nullptr)
            {
                m_pControllerManager->SendControllerEvent(m_EventIDs[DEVICE_MOUSE_POS_INDEX].c_str(), &ev);
                //AMFTraceInfo(AMF_FACILITY, L"MouseController: SendControllerEvent ev.value.XY(%5.4f,%5.4f)", ev.value.floatPoint2DValue.x, ev.value.floatPoint2DValue.y);
            }

            m_LastMousePoint.x = pointScreen.x;
            m_LastMousePoint.y = pointScreen.y;
        }

        return AMF_OK;
    }

    //-------------------------------------------------------------------------------------------------
    AMF_RESULT MouseController::ProcessMessage(const WindowMessage& msg)
    {
        if (msg.type == ButtonPress || msg.type == ButtonRelease)
        {
            XButtonEvent* buttonEvent = (XButtonEvent*)&msg;

            CtlEvent ev = {};
            ev.value.type = amf::AMF_VARIANT_BOOL;
            ev.value.boolValue = (msg.type == ButtonPress);
            std::string id;
            switch (buttonEvent->button)
            {
            case Button1:
                id = m_EventIDs[DEVICE_MOUSE_L_CLICK_INDEX].c_str();
                break;
            case Button2:
                id = m_EventIDs[DEVICE_MOUSE_M_CLICK_INDEX].c_str();
                break;
            case Button3:
                id = m_EventIDs[DEVICE_MOUSE_R_CLICK_INDEX].c_str();
                break;
            case Button4:
            case Button5:
                ev.value.type = amf::AMF_VARIANT_INT64;
                id = m_EventIDs[DEVICE_MOUSE_M_TRACK_INDEX].c_str();
                amf_int64 delta = (buttonEvent->button == Button5 ? -1 : 1) * 100;
                ev.value.int64Value = delta;
                break;
            }
            if (id.length() != 0)
            {
                if (m_pControllerManager != nullptr)
                {
                    m_pControllerManager->SendControllerEvent(id.c_str(), &ev);
                }
            }
        }

        if (m_bRelativePos == true)
        {
            if (msg.type == GenericEvent)
            {
                UpdateFromInternalInput(msg);
            }
            return AMF_OK;
        }
        else if (msg.type == MotionNotify)
        {
            //TODO: change this to polling. i.e. save it somewhere for the polling thread to read it.

            XMotionEvent* motionEvent = (XMotionEvent*)&msg;

            XWindowAttributes attrib;
            XGetWindowAttributes(motionEvent->display, motionEvent->window, &attrib);

            CtlEvent ev = {};
            ev.value.type = amf::AMF_VARIANT_FLOAT_POINT2D;

            ev.flags = DEVICE_MOUSE_POS_ABSOLUTE;
            ev.value.floatPoint2DValue.x = float(motionEvent->x) / attrib.width;
            ev.value.floatPoint2DValue.y = float(motionEvent->y) / attrib.height;

            std::string id = m_EventIDs[DEVICE_MOUSE_POS_INDEX];
            if (m_pControllerManager != nullptr)
            {
                m_pControllerManager->SendControllerEvent(id.c_str(), &ev);
            }
        }
        return AMF_OK;
    }
}// namespace ssdk::ctls
#endif
