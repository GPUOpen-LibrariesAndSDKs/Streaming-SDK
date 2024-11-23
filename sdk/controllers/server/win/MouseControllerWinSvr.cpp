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

#ifdef _WIN32

#include "MouseControllerWinSvr.h"

namespace ssdk::ctls::svr
{
    //-------------------------------------------------------------------------------------------------
    // MouseControllerWin
    //-------------------------------------------------------------------------------------------------
    MouseControllerWin::MouseControllerWin(ControllerManagerPtr pControllerManager) :
        MouseController(pControllerManager)
    {
        
    }

    AMFSize MouseControllerWin::GetScreenSize() const
    {
        if (::GetSystemMetrics(SM_CMONITORS) > 1)
        {
            AMFPoint mousePosition = {};
            GetMousePosition(&mousePosition);
            const POINT pt = { mousePosition.x, mousePosition.y };
            HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
            MONITORINFO mi = {};
            mi.cbSize = sizeof(mi);
            GetMonitorInfo(hMonitor, &mi);
            return { mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top };
        }

        return { ::GetSystemMetrics(SM_CXSCREEN) , ::GetSystemMetrics(SM_CYSCREEN) };
    }

    void MouseControllerWin::GetDesktopRect(AMFRect& rect) const
    {
        if (::GetSystemMetrics(SM_CMONITORS) > 1)
        {
            AMFPoint mousePosition = {};
            GetMousePosition(&mousePosition);
            const POINT pt = { mousePosition.x, mousePosition.y };
            HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
            MONITORINFO mi = {};
            mi.cbSize = sizeof(mi);
            GetMonitorInfo(hMonitor, &mi);
            rect = AMFConstructRect(mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right, mi.rcMonitor.bottom);
        }
        else
        {
            rect = AMFConstructRect(0, 0, ::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN));
        }
    }

    bool MouseControllerWin::GetMousePosition(AMFPoint* pos) const
    {
        POINT ptPhys;
        GetPhysicalCursorPos(&ptPhys);
        *pos = AMFConstructPoint(ptPhys.x, ptPhys.y);
        return true;
    }
    void MouseControllerWin::SendMouseInput(AMFFloatPoint2D position, bool isAbsolute)
    {
        static const float normaizationValue = 65535.f;

        INPUT inp = {};
        inp.type = INPUT_MOUSE;

        inp.mi.dwFlags = MOUSEEVENTF_MOVE;

        // The position is a float between 0-1, a fraction of the screen height/width. Depending on the mouse movement type, we need to do different normalization.
        if (isAbsolute)
        {
            inp.mi.dwFlags |= MOUSEEVENTF_ABSOLUTE;
            position.x *= normaizationValue;
            position.y *= normaizationValue;
        }
        else
        {
            AMFSize desktopSize = GetScreenSize();
            position.x *= desktopSize.width;
            position.y *= desktopSize.height;

            position.x += m_virtualMouseReminder.x;
            position.y += m_virtualMouseReminder.y;

            m_virtualMouseReminder.x = position.x - amf_float(LONG(position.x));
            m_virtualMouseReminder.y = position.y - amf_float(LONG(position.y));
        }

        inp.mi.dx = LONG(position.x);
        inp.mi.dy = LONG(position.y);

        ::SendInput(1, &inp, sizeof(inp));
    }
    void MouseControllerWin::SendMouseButtonInput(MouseButton button, bool isDown)
    {
        INPUT inp = {};
        inp.type = INPUT_MOUSE;
        inp.mi.mouseData = 0;

        switch (button)
        {
        case MouseButton::LeftButton:
            inp.mi.dwFlags |= isDown ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
            break;
        case MouseButton::MiddleButton:
            inp.mi.dwFlags |= isDown ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
            break;
        case MouseButton::RightButton:
            inp.mi.dwFlags |= isDown ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
            break;
        default:
            break;
        }

        ::SendInput(1, &inp, sizeof(inp));
    }
    void MouseControllerWin::SendMouseWheelInput(amf_int64 distance)
    {
        INPUT inp = {};
        inp.type = INPUT_MOUSE;
        inp.mi.mouseData = DWORD(distance);

        inp.mi.dwFlags = MOUSEEVENTF_WHEEL;
        ::SendInput(1, &inp, sizeof(inp));
    }

}// namespace ssdk::ctls::svr

#endif // _WIN32
