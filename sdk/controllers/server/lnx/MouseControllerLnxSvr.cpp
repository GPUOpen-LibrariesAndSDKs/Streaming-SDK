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

#if defined(__linux)

#include "MouseControllerLnxSvr.h"
#include <X11/extensions/XTest.h>

namespace ssdk::ctls::svr
{
    //-------------------------------------------------------------------------------------------------
    // MouseControllerLnx
    //-------------------------------------------------------------------------------------------------
    MouseControllerLnx::MouseControllerLnx(ControllerManagerPtr pControllerManager, XDisplay::Ptr pDisplay, XRRCrtcInfoPtr pCrtcInfo) :
        MouseController(pControllerManager),
        m_pDisplay(pDisplay),
        m_pCrtcInfo(pCrtcInfo)
    {
        
    }

    AMFSize MouseControllerLnx::GetScreenSize() const
    {
        if (m_pCrtcInfo == nullptr)
        {
            return AMFConstructSize(0, 0);
        }

        return AMFConstructSize(m_pCrtcInfo->width, m_pCrtcInfo->height);
    }

    void MouseControllerLnx::GetDesktopRect(AMFRect& rect) const
    {
        AMFSize screenSize = GetScreenSize();
        rect = AMFConstructRect(0, 0, screenSize.width, screenSize.height);
    }

    bool MouseControllerLnx::GetMousePosition(AMFPoint* pos) const
    {
        XDisplayPtr pDisplay(m_pDisplay);
        Window root = DefaultRootWindow((Display*)pDisplay);

        Window rootOut, childOut = 0;
        int rootX, rootY = 0;
        int winX, winY = 0;
        unsigned int mask = 0;
        int res = XQueryPointer(pDisplay, root, &rootOut, &childOut, &rootX, &rootY, &winX, &winY, &mask);

        if (res == 1 && m_pCrtcInfo != nullptr)
        {
            *pos = AMFConstructPoint(rootX - m_pCrtcInfo->x, rootY - m_pCrtcInfo->y);
            return true;
        }

        return false;
    }
    void MouseControllerLnx::SendMouseInput(AMFFloatPoint2D position, bool isAbsolute)
    {
        AMFSize size = GetScreenSize();

        XDisplayPtr display(m_pDisplay);
        if (isAbsolute)
        {
            XTestFakeMotionEvent(display, -1, position.x * size.width, position.y * size.height, 0);
        }
        else
        {
            position.x *= size.width;
            position.y *= size.height;

            position.x += m_virtualMouseReminder.x;
            position.y += m_virtualMouseReminder.y;

            m_virtualMouseReminder.x = position.x - amf_float(long(position.x));
            m_virtualMouseReminder.y = position.y - amf_float(long(position.y));

            XTestFakeRelativeMotionEvent(display, position.x, position.y, 0);
        }
        XSync(display, false);
    }
    void MouseControllerLnx::SendMouseButtonInput(MouseButton button, bool isDown)
    {
        int xButton = 0;
        switch (button)
        {
            case MouseButton::LeftButton: xButton = 1; break;
            case MouseButton::MiddleButton: xButton = 2; break;
            case MouseButton::RightButton: xButton = 3; break;
            default: return;
        }

        XDisplayPtr display(m_pDisplay);
        XTestFakeButtonEvent(display, xButton, isDown, 0);
        XSync(display, false);
    }
    void MouseControllerLnx::SendMouseWheelInput(amf_int64 distance)
    {
        // The scroll wheel is just another mouse button according to x11
        int xButton = (distance > 0) ? 4 : 5;

        XDisplayPtr display(m_pDisplay);
        XTestFakeButtonEvent(display, xButton, true, 0);
        XTestFakeButtonEvent(display, xButton, false, 0);
        XSync(display, false);
    }

}// namespace ssdk::ctls::svr

#endif // __linux
