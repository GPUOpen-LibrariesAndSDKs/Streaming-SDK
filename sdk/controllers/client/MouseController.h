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
#pragma once

#include "ControllerBase.h"
#include "amf/public/common/ByteArray.h"
#include "amf/public/samples/CPPSamples/common/VideoPresenter.h"
#include "sdk/video/VideoReceiverPipeline.h"

#if defined(__linux)
#include "public/common/Linux/XDisplay.h"
#endif

namespace ssdk::ctls
{
    //----------------------------------------------------------------------------------------------
    class MouseControllerConfig : public ControllerConfig
    {
    public:
        virtual ~MouseControllerConfig() {};
        
        // Set Data
        void AMF_STD_CALL SetBlackBarsSize(const AMFSize& size);
        void AMF_STD_CALL SetMousePosition(const AMFFloatPoint2D& mousePos);
        void AMF_STD_CALL SetFullScreen(bool bFullScreen);

        // Get Data
        const AMFSize&         AMF_STD_CALL BlackBarsSize() const;
        const AMFFloatPoint2D& AMF_STD_CALL MousePosition() const;
        bool                   AMF_STD_CALL IsFullScreen() const;

    protected:
        AMFSize m_BlackBarsSize = { 0, 0 };
        AMFFloatPoint2D m_MousePosition = { 0, 0 };
        bool m_bFullScreen = false;
    };
    //----------------------------------------------------------------------------------------------
    typedef std::shared_ptr<MouseControllerConfig> MouseControllerConfigPtr;
    //----------------------------------------------------------------------------------------------

    //----------------------------------------------------------------------------------------------
    class MouseController : public ControllerBase
    {
    public:
#if defined(_WIN32)
        MouseController(ControllerManagerPtr pControllerManager, VideoPresenterPtr pVideoPresenter, ssdk::video::VideoReceiverPipeline::Ptr pVideoPipeline, amf_handle hWnd, bool useRelativeInput);
#else
        MouseController(ControllerManagerPtr pControllerManager, VideoPresenterPtr pVideoPresenter, ssdk::video::VideoReceiverPipeline::Ptr pVideoPipeline, Display* dpy, amf_handle hWnd, bool useRelativeInput);
#endif
        virtual ~MouseController();

        // Input interface;
        virtual CTRL_DEVICE_TYPE AMF_STD_CALL GetType() const override { return CTRL_DEVICE_TYPE::MOUSE; }
        virtual AMF_RESULT  AMF_STD_CALL Update() override;

        // ControllerBase interface
        virtual AMF_RESULT  AMF_STD_CALL ProcessMessage(const WindowMessage& msg) override;
        virtual void        AMF_STD_CALL ProcessInputEvent(const char* eventID, const amf::AMFVariant& event) override;
        virtual void        AMF_STD_CALL OnConnectionEstablished() override;
        virtual void        AMF_STD_CALL OnConnectionTerminated() override;

    protected:
        void UpdateFromServer();
#if defined(_WIN32)
        void InitRawInput();
        AMF_RESULT  UpdateFromInternalInput(LPARAM lParam);
#else
        void Init();
        AMF_RESULT  UpdateFromInternalInput(const WindowMessage& msg);
#endif

        void CalculateMousePos(AMFFloatPoint2D& mousePos, AMFRect rcViewport, AMFSize frameSize) const noexcept;

        inline MouseControllerConfigPtr AMF_STD_CALL GetMouseConfig() const { return std::dynamic_pointer_cast<MouseControllerConfig> (m_pConfig); };

        enum CONTROLLER_ID_INDEX_ENUM
        {
            DEVICE_MOUSE_POS_INDEX = 0,
            DEVICE_MOUSE_L_CLICK_INDEX = 1,
            DEVICE_MOUSE_R_CLICK_INDEX = 2,
            DEVICE_MOUSE_M_CLICK_INDEX = 3,
            DEVICE_MOUSE_M_TRACK_INDEX = 4,
            DEVICE_KEYBOARD_INDEX = 5,
        };

        amf_handle                              m_hWnd;
        WindowPoint                             m_LastMousePoint{0,0};
        amf::AMFCriticalSection                 m_Sect;
        AMFByteArray                            m_Data;
        bool                                    m_bRelativePos;
        bool                                    m_bRawInputInitialized;
        VideoPresenterPtr                       m_pVideoPresenter;
        ssdk::video::VideoReceiverPipeline::Ptr m_pVideoPipeline;
#if defined(__linux)
        XDisplay::Ptr                           m_dpy;
        bool                                    m_bWayland{false};
#endif
    };
}// namespace ssdk::ctls
