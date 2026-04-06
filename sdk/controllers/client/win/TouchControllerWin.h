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
#pragma once

#if defined(_WIN32)

#include "../ControllerBase.h"
#include "controllers/TouchEvent.h"
#include "sdk/video/VideoReceiverPipelinePC.h"
#include "amf/public/samples/CPPSamples/common/VideoPresenter.h"

#include <map>

namespace ssdk::ctls
{
    class TouchControllerWin : public ControllerBase
    {
    public:
        TouchControllerWin(ControllerManagerPtr pControllerManager,
                           VideoPresenterPtr pVideoPresenter,
                           ssdk::video::VideoReceiverPipelinePC::Ptr pVideoPipeline,
                           amf_handle hWnd);
        ~TouchControllerWin() = default;

        virtual CTRL_DEVICE_TYPE AMF_STD_CALL GetType() const override { return CTRL_DEVICE_TYPE::TOUCHSCREEN; }
        virtual AMF_RESULT  AMF_STD_CALL Update() override { return AMF_OK; }
        virtual AMF_RESULT  AMF_STD_CALL ProcessMessage(const WindowMessage& msg) override;

    private:
        struct TouchPoint
        {
            TouchPoint() = default;
            TouchPoint(int32_t touchId, float x, float y) : touchId(touchId), x(x), y(y) {}

            int32_t touchId = 0;
            float   x       = 0.f;
            float   y       = 0.f;
        };

        // Normalize a raw client-coordinate touch point into [0,1] x [0,1],
        // accounting for letterbox bars imposed by aspect-ratio preservation.
        // Returns a point with x == -1 when the input falls outside the active viewport.
        TouchPoint NormalizeTouchPoint(const TouchPoint& point) const;

        // Append a normalized pointer to touchEvent.
        // Returns true when the point was inside the active viewport.
        bool AddTouchPoint(TouchEvent::Ptr touchEvent, const TouchPoint& point);

        VideoPresenterPtr                         m_pVideoPresenter;
        ssdk::video::VideoReceiverPipelinePC::Ptr m_pVideoPipeline;
        amf_handle                                m_hWnd = nullptr;

        std::map<int32_t, bool> m_ActivePointers;
        int32_t                 m_PointerIDBase = 0;
    };

}// namespace ssdk::ctls

#endif // _WIN32
