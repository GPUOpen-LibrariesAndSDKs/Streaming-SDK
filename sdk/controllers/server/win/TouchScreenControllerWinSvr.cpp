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

#include "TouchScreenControllerWinSvr.h"
#include <algorithm>

namespace ssdk::ctls::svr
{
    //-------------------------------------------------------------------------------------------------
    // TouchScreenControllerWin
    //-------------------------------------------------------------------------------------------------
    TouchScreenControllerWin::TouchScreenControllerWin(ControllerManagerPtr pControllerManager) :
        TouchScreenController(pControllerManager)
    {
        if (InitializeTouchInjection(MAX_TOUCH_COUNT, TOUCH_FEEDBACK_NONE) == TRUE)
        {
            SetScreenSize(GetScreenSize());
        }
    }

    BOOL TouchSorter(const POINTER_TOUCH_INFO& ti1, const POINTER_TOUCH_INFO& ti2)
    {
        return ti1.pointerInfo.pointerId > ti2.pointerInfo.pointerId;
    }

    void TouchScreenControllerWin::ProcessInputEvent(const char* /*eventID*/, const ssdk::ctls::CtlEvent& event)
    {
        TouchEvent::Ptr pTouchEvent{ event.value.pInterface };

        int32_t action = pTouchEvent->GetAction();
        int32_t pointerId = (action & TOUCH_EVENT_ACTION_POINTER_INDEX_MASK) >> TOUCH_EVENT_ACTION_POINTER_INDEX_SHIFT;
        int32_t maskedAction = (action & TOUCH_EVENT_ACTION_MASK);

        POINTER_TOUCH_INFO touchInfo{ DefaultPointerTouchInfo() };

        for (std::vector<TouchEvent::Pointer>::const_iterator i = pTouchEvent->PointersBegin(); i != pTouchEvent->PointersEnd(); i++)
        {
            if (i->m_id == pointerId)
            {
                POINTER_INFO pointerInfo{ DefaultPointerInfo() };
                pointerInfo.pointerId = i->m_id;
                pointerInfo.ptPixelLocation = { static_cast<LONG>(i->m_x * static_cast<double>(m_ScreenSize.width)), static_cast<LONG>(i->m_y * static_cast<double>(m_ScreenSize.height)) };
                LONG ptContactSize = 2;
                touchInfo.rcContact = { pointerInfo.ptPixelLocation.x - ptContactSize, pointerInfo.ptPixelLocation.y - ptContactSize, pointerInfo.ptPixelLocation.x + ptContactSize, pointerInfo.ptPixelLocation.y + ptContactSize };
                switch (maskedAction) {
                case TOUCH_EVENT_ACTION_DOWN:
                case TOUCH_EVENT_ACTION_POINTER_DOWN:
                    pointerInfo.pointerFlags = TOUCH_BEGIN;
                    break;
                case TOUCH_EVENT_ACTION_MOVE:
                    pointerInfo.pointerFlags = TOUCH_MOVE;
                    break;
                case TOUCH_EVENT_ACTION_UP:
                case TOUCH_EVENT_ACTION_POINTER_UP:
                    pointerInfo.pointerFlags = TOUCH_END;
                break;
                default:
                    return;
                }
                touchInfo.pointerInfo = std::move(pointerInfo);
                UpdateTouch(touchInfo);
            }
        }

        std::sort(m_Touches.begin(), m_Touches.end(), &TouchSorter);

        BOOL inject = InjectTouchInput((UINT32) m_Touches.size(), m_Touches.data());
        if (!inject)
        {
            m_Touches.clear();
            return;
        }

        if (maskedAction == TOUCH_EVENT_ACTION_POINTER_UP || maskedAction == TOUCH_EVENT_ACTION_UP)
        {
            RemoveTouch(pointerId);
        }
    }

    void TouchScreenControllerWin::SetScreenSize(AMFSize screenSize)
    {
        m_ScreenSize = screenSize;
    }

    POINTER_TOUCH_INFO TouchScreenControllerWin::DefaultPointerTouchInfo() const
    {
        POINTER_TOUCH_INFO touchInfo{};
        ZeroMemory(&touchInfo, sizeof(POINTER_TOUCH_INFO));
        touchInfo.pointerInfo = DefaultPointerInfo();
        touchInfo.touchMask = TOUCH_MASK_CONTACTAREA | TOUCH_MASK_ORIENTATION | TOUCH_MASK_PRESSURE;
        touchInfo.orientation = 90;
        touchInfo.pressure = 512;
        return touchInfo;
    }

    POINTER_INFO TouchScreenControllerWin::DefaultPointerInfo() const
    {
        POINTER_INFO pointerInfo{};
        ZeroMemory(&pointerInfo, sizeof(POINTER_INFO));
        pointerInfo.pointerType = PT_TOUCH;
        return pointerInfo;
    }

    void TouchScreenControllerWin::UpdateTouch(const POINTER_TOUCH_INFO& touchInfo)
    {
        for (POINTER_TOUCH_INFO& touchInfo_ : m_Touches)
        {
            if (touchInfo_.pointerInfo.pointerId == touchInfo.pointerInfo.pointerId)
            {
                touchInfo_ = touchInfo;
                return;
            }
            else
            {
                touchInfo_.pointerInfo.pointerFlags = TOUCH_MOVE;
            }
        }
        // If there is no touch pointer with such Id, Add it
        if ((touchInfo.pointerInfo.pointerFlags & TOUCH_BEGIN) == TOUCH_BEGIN)
        {
            m_Touches.emplace_back(touchInfo);
        }
    }

    void TouchScreenControllerWin::RemoveTouch(UINT32 pointerId)
    {
        for (Touches::iterator it = m_Touches.begin(); it != m_Touches.end(); ++it)
        {
            if (it->pointerInfo.pointerId == pointerId)
            {
                m_Touches.erase(it);
                break;
            }
        }
    }

    AMFSize TouchScreenControllerWin::GetScreenSize()
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

    bool TouchScreenControllerWin::GetMousePosition(AMFPoint* pos) const
    {
        POINT ptPhys;
        GetPhysicalCursorPos(&ptPhys);
        *pos = AMFConstructPoint(ptPhys.x, ptPhys.y);
        return true;
    }

}// namespace ssdk::ctls::svr

#endif // _WIN32
