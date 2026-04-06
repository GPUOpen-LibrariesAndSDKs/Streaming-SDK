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

#if defined(__ANDROID__)

#include "TouchControllerAndroid.h"
#include "../ControllerManager.h"

#include "amf/public/common/TraceAdapter.h"

namespace ssdk::ctls
{
    static constexpr const wchar_t* const AMF_FACILITY = L"TouchController";

    TouchController::TouchController(ControllerManagerPtr pControllerManager,
        ssdk::video::VideoReceiver::Ptr pVideoPipeline) :
        ControllerBase(pControllerManager),
        m_pVideoPipeline(pVideoPipeline)
    {
        m_ID = DEVICE_TOUCHSCREEN;
        m_Description = "Touch Screen";
        m_EventIDs.push_back(m_ID + DEVICE_TOUCHSCREEN_TOUCH);
    }

    //-------------------------------------------------------------------------------------------------
    TouchController::TouchPoint TouchController::NormalizeTouchPoint(const TouchPoint& point, bool preserveAspect) const
    {
        TouchPoint touchPoint(point);

        AMFSize displayResolution = m_pVideoPipeline->GetDisplayResolution();
        AMFRect rcViewport = m_pVideoPipeline->GetDisplayViewport();
        AMFSize streamResolution = m_pVideoPipeline->GetStreamResolution();
        AMFRect streamViewport = m_pVideoPipeline->GetStreamViewport();

        if (point.x >= rcViewport.left && point.x <= rcViewport.right && point.y >= rcViewport.top && point.y <= rcViewport.bottom)
        {
            float frameOrgX = preserveAspect ? streamViewport.left / streamResolution.width : 0;
            float frameOrgY = preserveAspect ? streamViewport.top / streamResolution.height : 0;
            float frameScaleX = preserveAspect ? streamViewport.Width() / streamResolution.width : 1;
            float frameScaleY = preserveAspect ? streamViewport.Height() / streamResolution.height : 1;

            float adjustedX = ((point.x - rcViewport.left) / float(rcViewport.Width()) - frameOrgX) * frameScaleX;
            float adjustedY = ((point.y - rcViewport.top) / float(rcViewport.Height()) - frameOrgY) * frameScaleY;

            if (adjustedX <= 1.0 || adjustedY <= 1.0)
            {
                touchPoint.x = adjustedX;
                touchPoint.y = adjustedY;
            }
            else
            {   //  Point outside of viewport - ignore
                touchPoint.x = -1;
                touchPoint.y = -1;
            }
        }
        else
        {   //  Point outside viewport - ignore
            touchPoint.x = -1;
            touchPoint.y = -1;
        }
        return touchPoint;
    }

    AMF_RESULT TouchController::ProcessMessage(const WindowMessage& msg)
    {
        AMF_RESULT result = AMF_FAIL;

        int32_t type = AInputEvent_getType(msg);
        int32_t source = AInputEvent_getSource(msg);

        bool handled = false;

        //  This double check is necessary because Android duplicates physical mouse clicks with touch events for compatibility with
        //  applications that do not handle physical mouse explicitly. You get two subsequent events for a mouse click - a true mouse
        //  click event, followed by AMOTION_EVENT_ACTION_DOWN with both AINPUT_SOURCE_TOUCHSCREEN and AINPUT_SOURCE_MOUSE bits set. So
        //  we only want to process the true touch screen events here and let the mouse do its own thing.
        if ((source & AINPUT_SOURCE_TOUCHSCREEN) != 0 && (source & (~AINPUT_SOURCE_TOUCHSCREEN)) == 0)
        {
            bool preserveAspectRatio = ((source & AINPUT_SOURCE_TOUCHSCREEN) != 0) && (m_pVideoPipeline != nullptr && m_pVideoPipeline->IsAspectRatioPreserved()) ;

            if (type == AINPUT_EVENT_TYPE_MOTION)
            {
                int64_t eventTime = AMotionEvent_getEventTime(msg) / 100;
                int32_t action = AMotionEvent_getAction(msg);
                int32_t actionMasked = action & AMOTION_EVENT_ACTION_MASK;
                int32_t mappedAction = 0;

                TouchEvent::Ptr touchEvent = new TouchEvent(actionMasked, eventTime);

                switch (actionMasked)
                {
                case AMOTION_EVENT_ACTION_DOWN:
                {
                    // First pointer down
                    int32_t pointerId = AMotionEvent_getPointerId(msg, 0);
                    float x = AMotionEvent_getX(msg, 0);
                    float y = AMotionEvent_getY(msg, 0);
                    m_ActivePointers[pointerId] = true;
                    mappedAction = TOUCH_EVENT_ACTION_DOWN;
                    handled = AddTouchPoint(touchEvent, TouchPoint(pointerId, x, y), preserveAspectRatio);
                    break;
                }
                case AMOTION_EVENT_ACTION_POINTER_DOWN:
                {
                    // Additional pointer down
                    size_t pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
                    int32_t pointerId = AMotionEvent_getPointerId(msg, pointerIndex);
                    float x = AMotionEvent_getX(msg, pointerIndex);
                    float y = AMotionEvent_getY(msg, pointerIndex);
                    m_ActivePointers[pointerId] = true;
                    mappedAction = TOUCH_EVENT_ACTION_POINTER_DOWN;
                    handled = AddTouchPoint(touchEvent, TouchPoint(pointerId, x, y), preserveAspectRatio);
                    break;
                }
                case AMOTION_EVENT_ACTION_UP:
                {
                    // Last pointer up
                    int32_t pointerId = AMotionEvent_getPointerId(msg, 0);
                    float x = AMotionEvent_getX(msg, 0);
                    float y = AMotionEvent_getY(msg, 0);
                    m_ActivePointers.erase(pointerId);
                    mappedAction = TOUCH_EVENT_ACTION_UP;
                    handled = AddTouchPoint(touchEvent, TouchPoint(pointerId, x, y), preserveAspectRatio);
                    break;
                }
                case AMOTION_EVENT_ACTION_POINTER_UP:
                {
                    // One of multiple pointers up
                    size_t pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
                    int32_t pointerId = AMotionEvent_getPointerId(msg, pointerIndex);
                    float x = AMotionEvent_getX(msg, pointerIndex);
                    float y = AMotionEvent_getY(msg, pointerIndex);
                    m_ActivePointers.erase(pointerId);
                    mappedAction = TOUCH_EVENT_ACTION_POINTER_UP;
                    handled = AddTouchPoint(touchEvent, TouchPoint(pointerId, x, y), preserveAspectRatio);
                    break;
                }
                case AMOTION_EVENT_ACTION_MOVE:
                {
                    // Movement of one or more pointers
                    mappedAction = TOUCH_EVENT_ACTION_MOVE;
                    size_t pointerCount = AMotionEvent_getPointerCount(msg);
                    for (size_t i = 0; i < pointerCount; ++i)
                    {
                        int32_t pointerId = AMotionEvent_getPointerId(msg, i);
                        float x = AMotionEvent_getX(msg, i);
                        float y = AMotionEvent_getY(msg, i);
                        if (m_ActivePointers.count(pointerId) != 0)
                        {
                            handled = handled || AddTouchPoint(touchEvent, TouchPoint(pointerId, x, y), preserveAspectRatio);
                        }
                    }
                    break;
                }
                default:
                    break;
                }
                if (handled == true)
                {
                    CtlEvent ev{};
                    ev.value.type = amf::AMF_VARIANT_INTERFACE;
                    ev.value.pInterface = touchEvent.Detach();

                    if (m_pControllerManager != nullptr)
                    {
                        m_pControllerManager->SendControllerEvent(m_EventIDs[0].c_str(), ev);
                    }

                }
            }
        }
        return result;
    }

    bool TouchController::AddTouchPoint(TouchEvent::Ptr touchEvent, const TouchController::TouchPoint &touchPoint, bool preserveAspect)
    {
        bool result = false;
        TouchPoint normalizedTouchPoint(NormalizeTouchPoint(touchPoint, preserveAspect));

        if (normalizedTouchPoint.x != -1 && normalizedTouchPoint.y != -1)
        {
            touchEvent->AddPointer(normalizedTouchPoint.touchId, normalizedTouchPoint.x, normalizedTouchPoint.y);
            result = true;
        }
        return result;
    }
}

#endif // __ANDROID__
