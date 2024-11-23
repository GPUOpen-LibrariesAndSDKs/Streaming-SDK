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

#include "MouseController.h"
#include "controllers/client/ControllerManager.h"

namespace ssdk::ctls
{
#if defined(_WIN32)
    MouseController::MouseController(ControllerManagerPtr pControllerManager, VideoPresenterPtr pVideoPresenter,
        ssdk::video::VideoReceiverPipeline::Ptr pVideoPipeline, amf_handle hWnd, bool useRelativeInput) :
#else
    MouseController::MouseController(ControllerManagerPtr pControllerManager, VideoPresenterPtr pVideoPresenter,
        ssdk::video::VideoReceiverPipeline::Ptr pVideoPipeline, Display* dpy, amf_handle hWnd, bool useRelativeInput) :
        m_dpy(dpy),
#endif
        ControllerBase(pControllerManager),
        m_pVideoPresenter(pVideoPresenter),
        m_pVideoPipeline(pVideoPipeline),
        m_hWnd(hWnd),
        m_LastMousePoint{},
        m_bRelativePos(useRelativeInput),
        m_bRawInputInitialized(false)
    {
        m_ID = DEVICE_MOUSE;
        m_Description = "PC mouse";

        m_EventIDs.push_back(m_ID + DEVICE_MOUSE_POS);
        m_EventIDs.push_back(m_ID + DEVICE_MOUSE_L_CLICK);
        m_EventIDs.push_back(m_ID + DEVICE_MOUSE_R_CLICK);
        m_EventIDs.push_back(m_ID + DEVICE_MOUSE_M_CLICK);
        m_EventIDs.push_back(m_ID + DEVICE_MOUSE_M_TRACK);

        m_pConfig = std::make_shared<MouseControllerConfig>();
        m_pConfig->Attach(this);
    }

    //-------------------------------------------------------------------------------------------------
    MouseController::~MouseController()
    {
    }

    //-------------------------------------------------------------------------------------------------
    AMF_RESULT MouseController::Update()
    {
        UpdateFromServer();

        return AMF_OK;
    }

    //-------------------------------------------------------------------------------------------------
    void MouseController::OnConnectionEstablished()
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

    void MouseController::OnConnectionTerminated()
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
    void MouseController::ProcessInputEvent(const char* eventID, const amf::AMFVariant& event)
    {
        // If Input Event is Mouse Move (from Server)
        if (strcmp(eventID, DEVICE_MOUSE_POS) == 0)
        {
            // Get mouse position, normalize and set to mouse controller
            if (event.type == amf::AMF_VARIANT_FLOAT_POINT2D)
            {
                // Get position in range [0, 1]
                AMFFloatPoint2D mousePos = event.floatPoint2DValue;

                // Denormalize mouse position according to client screen
                bool bFullScreen = true;
                if (m_pVideoPresenter != nullptr)
                {
                    AMFRect rcViewport{ 0, 0, m_pVideoPresenter->GetFrameWidth() - 1, m_pVideoPresenter->GetFrameHeight() - 1 };
                    if (m_pVideoPipeline != nullptr)
                    {
                        rcViewport = m_pVideoPipeline->GetViewport();
                    }

                    CalculateMousePos(mousePos, rcViewport, AMFConstructSize(m_pVideoPresenter->GetFrameWidth() - 1, m_pVideoPresenter->GetFrameHeight() - 1));
                    bFullScreen = m_pVideoPresenter->GetFullScreen();
                }

                // Set mouse position
                MouseControllerConfigPtr pMouseConfig = GetMouseConfig();
                if (pMouseConfig != nullptr)
                {
                    pMouseConfig->SetMousePosition(mousePos);
                    pMouseConfig->SetFullScreen(bFullScreen);
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------------
    void MouseController::CalculateMousePos(AMFFloatPoint2D& mousePos, AMFRect rcViewport, AMFSize frameSize) const noexcept
    {
        double viewportAspectRatio = double(rcViewport.Width()) / double(rcViewport.Height());
        double frameAspectRatio = double(frameSize.width) / double(frameSize.height);
        
        if (viewportAspectRatio == frameAspectRatio)
        {
            mousePos.x *= float(frameSize.width);
            mousePos.y *= float(frameSize.height);
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

            mousePos.x = viewport.left + mousePos.x * float(viewport.Width());
            mousePos.y = viewport.top + mousePos.y * float(viewport.Height());
        }
    }

    //-------------------------------------------------------------------------------------------------
    // MouseControllerConfig class implementation
    //------------------------------------------------------------------------------------------------
    void AMF_STD_CALL MouseControllerConfig::SetBlackBarsSize(const AMFSize& size)
    {
        amf::AMFLock lock(&m_Guard);
        m_BlackBarsSize = size;
        Notify();
    }
    void AMF_STD_CALL MouseControllerConfig::SetMousePosition(const AMFFloatPoint2D& mousePos)
    {
        amf::AMFLock lock(&m_Guard);
        m_MousePosition = mousePos;
        Notify();
    }
    void AMF_STD_CALL MouseControllerConfig::SetFullScreen(bool bFullScreen)
    {
        amf::AMFLock lock(&m_Guard);
        m_bFullScreen = bFullScreen;
        Notify();
    }

    const AMFSize& AMF_STD_CALL MouseControllerConfig::BlackBarsSize() const
    {
        amf::AMFLock lock(&m_Guard);
        return m_BlackBarsSize;
    }
    const AMFFloatPoint2D& AMF_STD_CALL MouseControllerConfig::MousePosition() const
    {
        amf::AMFLock lock(&m_Guard);
        return m_MousePosition;
    }
    bool AMF_STD_CALL MouseControllerConfig::IsFullScreen() const
    {
        amf::AMFLock lock(&m_Guard);
        return m_bFullScreen;
    }

}// namespace ssdk::ctls
