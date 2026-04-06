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

#if defined(_WIN32)

#include "TouchControllerWin.h"

#include "../ControllerManager.h"

#include "amf/public/common/TraceAdapter.h"

#include <windows.h>
#include <windowsx.h>
#include <vector>
#include <cmath>

namespace ssdk::ctls
{
    static constexpr const wchar_t* const AMF_FACILITY = L"TouchControllerWin";

    TouchControllerWin::TouchControllerWin(ControllerManagerPtr pControllerManager,
                                           VideoPresenterPtr pVideoPresenter,
                                           ssdk::video::VideoReceiverPipelinePC::Ptr pVideoPipeline,
                                           amf_handle hWnd)
        : ControllerBase(pControllerManager),
          m_pVideoPresenter(pVideoPresenter),
          m_pVideoPipeline(pVideoPipeline),
          m_hWnd(hWnd)
    {
        m_ID          = DEVICE_TOUCHSCREEN;
        m_Description = "Touch Screen";
        m_EventIDs.push_back(m_ID + DEVICE_TOUCHSCREEN_TOUCH);

        // Ask Windows to send pointer messages for touch/pen to this hwnd.
        if (m_hWnd != nullptr)
        {
            (void)::RegisterPointerInputTarget((HWND)m_hWnd, PT_TOUCH);
            (void)::RegisterPointerInputTarget((HWND)m_hWnd, PT_PEN);

            // Avoid duplicate mouse input being generated from touch.
            (void)::EnableMouseInPointer(FALSE);
        }
    }

    // Normalize a client-space point to [0,1]x[0,1] based on presenter source rect.
    // Points that hit letterbox bars are rejected (x/y set to -1).
    TouchControllerWin::TouchPoint TouchControllerWin::NormalizeTouchPoint(const TouchPoint& point) const
    {
        TouchPoint out(point);

        if (m_hWnd == nullptr || m_pVideoPresenter == nullptr)
        {
            out.x = -1.f;
            out.y = -1.f;
            return out;
        }

        // Map client coords to source/presenter space
        AMFPoint pointSource = m_pVideoPresenter->MapClientToSource(AMFConstructPoint((amf_int32)point.x, (amf_int32)point.y));
        AMFRect  displayRect  = m_pVideoPresenter->GetSourceRect();

        // Get stream viewport (the source image area / resolution)
        AMFRect streamViewport = {0,0,0,0};
        if (m_pVideoPipeline != nullptr)
        {
            streamViewport = m_pVideoPipeline->GetStreamViewport();
        }

        // Validate source point is within the display rectangle
        if (pointSource.x < displayRect.left || pointSource.x >= displayRect.right || pointSource.y < displayRect.top || pointSource.y >= displayRect.bottom)
        {
            out.x = -1.f;
            out.y = -1.f;
            return out;
        }

        // If stream viewport is invalid fall back to simple normalization against display rect
        if (streamViewport.Width() <= 0 || streamViewport.Height() <= 0)
        {
            const float denomX = float(displayRect.right - displayRect.left - 1);
            const float denomY = float(displayRect.bottom - displayRect.top - 1);
            if (denomX <= 0.f || denomY <= 0.f)
            {
                out.x = -1.f;
                out.y = -1.f;
                return out;
            }
            out.x = float(pointSource.x - displayRect.left) / denomX;
            out.y = float(pointSource.y - displayRect.top) / denomY;
            return out;
        }

        const float displayW = float(displayRect.right - displayRect.left);
        const float displayH = float(displayRect.bottom - displayRect.top);
        const float streamW = float(streamViewport.Width());
        const float streamH = float(streamViewport.Height());

        if (displayW <= 0.f || displayH <= 0.f || streamW <= 0.f || streamH <= 0.f)
        {
            out.x = -1.f;
            out.y = -1.f;
            return out;
        }

        const float displayAspect = displayW / displayH;
        const float streamAspect  = streamW / streamH;

        // Compute the active viewport inside the display where the stream image is rendered (centered)
        AMFRect active = displayRect;

        if (fabs(displayAspect - streamAspect) > 1e-6f)
        {
            if (streamAspect < displayAspect)
            {
                // Stream is narrower than display -> pillarbox (black bars on sides)
                float viewportW = displayH * streamAspect;
                int32_t left = displayRect.left + int((displayW - viewportW) * 0.5f + 0.5f);
                int32_t right = left + int(viewportW + 0.5f);
                active.left = left;
                active.right = right;
                active.top = displayRect.top;
                active.bottom = displayRect.bottom;
            }
            else
            {
                // Stream is wider than display -> letterbox (black bars above and below)
                float viewportH = displayW / streamAspect;
                int32_t top = displayRect.top + int((displayH - viewportH) * 0.5f + 0.5f);
                int32_t bottom = top + int(viewportH + 0.5f);
                active.top = top;
                active.bottom = bottom;
                active.left = displayRect.left;
                active.right = displayRect.right;
            }
        }

        // If the point falls into the black bars, reject it
        if (pointSource.x < active.left || pointSource.x >= active.right || pointSource.y < active.top || pointSource.y >= active.bottom)
        {
            out.x = -1.f;
            out.y = -1.f;
            return out;
        }

        const float denomX = float(active.right - active.left - 1);
        const float denomY = float(active.bottom - active.top - 1);
        if (denomX <= 0.f || denomY <= 0.f)
        {
            out.x = -1.f;
            out.y = -1.f;
            return out;
        }

        out.x = float(pointSource.x - active.left) / denomX;
        out.y = float(pointSource.y - active.top) / denomY;
        return out;
    }

    bool TouchControllerWin::AddTouchPoint(TouchEvent::Ptr touchEvent, const TouchPoint& point)
    {
        if (touchEvent == nullptr)
        {
            return false;
        }

        TouchPoint normalized = NormalizeTouchPoint(point);
        if (normalized.x < 0.f || normalized.y < 0.f)
        {
            return false;
        }

        touchEvent->AddPointer(normalized.touchId - m_PointerIDBase, normalized.x, normalized.y);
        AMFTraceDebug(AMF_FACILITY, L"Added touch point: id=%d, pos=(%f,%f)", normalized.touchId - m_PointerIDBase, normalized.x, normalized.y);
        return true;
    }

    AMF_RESULT TouchControllerWin::ProcessMessage(const WindowMessage& msg)
    {
        if (msg.message != WM_POINTERDOWN && msg.message != WM_POINTERUP && msg.message != WM_POINTERUPDATE)
        {
            return AMF_FAIL;
        }

        const UINT32 pointerId = GET_POINTERID_WPARAM(msg.wParam);

        POINTER_INPUT_TYPE pointerType = PT_POINTER;
        if (!::GetPointerType(pointerId, &pointerType) || pointerType != PT_TOUCH)
        {
            // Not a touchscreen message.
            return AMF_FAIL;
        }

        POINTER_INFO pi{};
        if (!::GetPointerInfo(pointerId, &pi))
        {
            return AMF_FAIL;
        }

        if (m_ActivePointers.size() == 0)
        {
            m_PointerIDBase = pointerId;
        }

        POINT ptClient = pi.ptPixelLocation;
        ::ScreenToClient((HWND)m_hWnd, &ptClient);


        int32_t action = 0;
        switch (msg.message)
        {
        case WM_POINTERDOWN:
            action = TOUCH_EVENT_ACTION_POINTER_DOWN;
            m_ActivePointers[(int32_t)pointerId] = true;
//            AMFTraceDebug(AMF_FACILITY, L"Received TOUCH_EVENT_ACTION_POINTER_DOWN message: id=%d, type=%d, action=%d, pos=(%d,%d)", pointerId, pointerType, msg.message, ptClient.x, ptClient.y);
            break;
        case WM_POINTERUP:
            action = TOUCH_EVENT_ACTION_POINTER_UP;
            m_ActivePointers.erase((int32_t)pointerId);
//            AMFTraceDebug(AMF_FACILITY, L"Received TOUCH_EVENT_ACTION_POINTER_UP message: id=%d, type=%d, action=%d, pos=(%d,%d)", pointerId, pointerType, msg.message, ptClient.x, ptClient.y);
            break;
        case WM_POINTERUPDATE:
            action = TOUCH_EVENT_ACTION_MOVE;
//            AMFTraceDebug(AMF_FACILITY, L"Received TOUCH_EVENT_ACTION_MOVE message: id=%d, type=%d, action=%d, pos=(%d,%d)", pointerId, pointerType, msg.message, ptClient.x, ptClient.y);
            break;
        default:
            return AMF_FAIL;
        }

        const int64_t eventTime = (int64_t)::GetMessageTime();
        TouchEvent::Ptr touchEvent = new TouchEvent(action, eventTime);

        bool handled = false;

        if (msg.message == WM_POINTERUPDATE)
        {
            // Try to send all touch pointers in the same frame.
            UINT32 frameCount = 0;
            if (::GetPointerFrameInfo(pointerId, &frameCount, nullptr) && frameCount > 0)
            {
                std::vector<POINTER_INFO> frame(frameCount);
                if (::GetPointerFrameInfo(pointerId, &frameCount, frame.data()))
                {
                    for (UINT32 i = 0; i < frameCount; ++i)
                    {
                        POINTER_INPUT_TYPE t = PT_POINTER;
                        if (!::GetPointerType(frame[i].pointerId, &t) || t != PT_TOUCH)
                        {
                            continue;
                        }
                        if (m_ActivePointers.count((int32_t)frame[i].pointerId) == 0)
                        {
                            continue;
                        }

                        POINT pt = frame[i].ptPixelLocation;
                        ::ScreenToClient((HWND)m_hWnd, &pt);
                        handled = handled || AddTouchPoint(touchEvent, TouchPoint((int32_t)frame[i].pointerId, (float)pt.x, (float)pt.y));
                    }
                }
            }

            // Fallback to just the current pointer.
            if (!handled && m_ActivePointers.count((int32_t)pointerId) != 0)
            {
                handled = AddTouchPoint(touchEvent, TouchPoint((int32_t)pointerId, (float)ptClient.x, (float)ptClient.y));
            }
        }
        else
        {
            handled = AddTouchPoint(touchEvent, TouchPoint((int32_t)pointerId, (float)ptClient.x, (float)ptClient.y));
        }

        if (!handled)
        {
            return AMF_FAIL;
        }

        CtlEvent ev{};
        ev.value.type       = amf::AMF_VARIANT_INTERFACE;
        ev.value.pInterface = touchEvent.Detach();

        if (m_pControllerManager != nullptr)
        {
            m_pControllerManager->SendControllerEvent(m_EventIDs[0].c_str(), ev);
        }

        return AMF_OK;
    }

} // namespace ssdk::ctls

#endif // _WIN32
