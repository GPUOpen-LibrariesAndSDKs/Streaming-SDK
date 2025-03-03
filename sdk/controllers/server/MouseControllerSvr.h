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
#pragma once

#include "ControllerBaseSvr.h"

namespace ssdk::ctls::svr
{
    //-------------------------------------------------------------------------------------------------
    // MouseController
    //-------------------------------------------------------------------------------------------------
    class MouseController : public ControllerBase
    {
    public:
        MouseController(ControllerManagerPtr pControllerManager);
        virtual ~MouseController() {}

        typedef std::shared_ptr<MouseController> Ptr;

        enum class MouseButton {
            LeftButton = 0,
            MiddleButton,
            RightButton
        };

        virtual CTRL_DEVICE_TYPE GetType() const override { return CTRL_DEVICE_TYPE::MOUSE; };
        virtual void Update() override;
        virtual void ProcessInputEvent(const char* eventID, const ssdk::ctls::CtlEvent& event) override;
        virtual void OnConnectionEstablished() override;
        virtual void OnConnectionTerminated() override;

    protected:
        virtual AMFSize GetScreenSize() const = 0;
        virtual void GetDesktopRect(AMFRect& rect) const = 0;
        virtual bool GetMousePosition(AMFPoint* pos) const = 0;
        virtual void SendMouseInput(AMFFloatPoint2D position, bool isAbsolute) = 0;
        virtual void SendMouseButtonInput(MouseButton button, bool isDown) = 0;
        virtual void SendMouseWheelInput(amf_int64 distance) = 0;

        void UpdateMousePos();
        void UpdateCursor();

    protected:
        amf::AMFCriticalSection m_MouseGuard;
        AMFPoint                m_LastMousePos;
        amf_pts                 m_LastMouseSent;
        amf::AMFSurfacePtr      m_pLastCursor;
        bool                    m_bSendLastCursorToNewUser;
        bool                    m_bSendLastCursorPosToNewUser;
        bool                    m_bLastMouseVisible;
        uint8_t                 m_CursorUpdateCounter;
        uint8_t                 m_CursorPosUpdateCounter;
    };

} // namespace ssdk::ctls::svr
