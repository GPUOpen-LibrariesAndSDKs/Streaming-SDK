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

#include "MouseControllerSvr.h"
#include "ControllerManagerSvr.h"
#include "transports/transport-amd/messages/video/Cursor.h"
//#include "amf/amf/public/common/TraceAdapter.h"

namespace ssdk::ctls::svr
{
    static constexpr const wchar_t* AMF_FACILITY = L"MouseControllerSvr";
    static constexpr const uint8_t CURSOR_UPDATE_COUNTER_MAX = 3;

    //-------------------------------------------------------------------------------------------------
    // MouseController
    //-------------------------------------------------------------------------------------------------
    MouseController::MouseController(ControllerManagerPtr pControllerManager) :
        ControllerBase(pControllerManager),
        m_LastMousePos(AMFConstructPoint(-1, -1)),
        m_LastMouseSent(0),
        m_pLastCursor(nullptr),
        m_bSendLastCursorToNewUser(false),
        m_bSendLastCursorPosToNewUser(false),
        m_bLastMouseVisible(false),
        m_CursorUpdateCounter(0),
        m_CursorPosUpdateCounter(0)
    {
        m_ID = DEVICE_MOUSE;
    }

    void MouseController::Update()
    {
        UpdateMousePos();
        UpdateCursor();
    }

    //-------------------------------------------------------------------------------------------------
    void MouseController::UpdateMousePos()
    {
        if (GetState() == CTRL_DEVICE_STATE::CONNECTED)
        {
            amf::AMFLock lock(&m_MouseGuard);

            AMFPoint mousePosition = {};
            bool bPosAcquire = GetMousePosition(&mousePosition);
            amf_pts now = amf_high_precision_clock();

            // Send mouse position if it changed or pos is not changed but we got new user connected or update counter still isn't full
            if (bPosAcquire && (mousePosition != m_LastMousePos || m_bSendLastCursorPosToNewUser || m_CursorPosUpdateCounter < CURSOR_UPDATE_COUNTER_MAX))
            {
                m_LastMouseSent = now;
                //AMFTraceInfo(AMF_FACILITY, L"UpdateMouse m_LastMousePos %d %d mousePosition %d %d NewUser %d PosUpdateCounter %d",
                //    m_LastMousePos.x, m_LastMousePos.y, mousePosition.x, mousePosition.y, m_bSendLastCursorPosToNewUser, m_CursorPosUpdateCounter);

                if (m_pControllerManager != nullptr)
                {
                    transport_common::ServerTransport::Ptr pServerTransport = m_pControllerManager->GetServerTransport();
                    if (pServerTransport != nullptr)
                    {
                        AMFPoint pos = mousePosition;
                        AMFRect desktopRect = {};
                        GetDesktopRect(desktopRect);
                        pos.x -= desktopRect.left;
                        pos.y -= desktopRect.top;

                        // cusror physical position should match capture desktop resolution
                        AMFSize screenSize = GetScreenSize();

                        // Move mouse cursor
                        pServerTransport->MoveCursor(pos.x / float(screenSize.width - 1), pos.y / float(screenSize.height - 1));

                        m_bSendLastCursorPosToNewUser = false;
                        if (m_CursorPosUpdateCounter < CURSOR_UPDATE_COUNTER_MAX)
                        {
                            m_CursorPosUpdateCounter++;
                        }
                    }
                }

                // AMFTraceInfo(AMF_FACILITY, L"out(%6.5f,%6.5f) get(%d,%d), screenSize(%d,%d)", ev.value.floatPoint2DValue.x, ev.value.floatPoint2DValue.y, (int)pt.x, (int)pt.y, screenSize.width, screenSize.height);
                m_LastMousePos = mousePosition;
            }
        }
    }

    //-------------------------------------------------------------------------------------------------
    void MouseController::UpdateCursor()
    {
        if (GetState() == CTRL_DEVICE_STATE::CONNECTED)
        {
            // Set Cursor bitmap and send to all subscribers
            if (m_pControllerManager != nullptr)
            {
                amf::AMFCursorCapturePtr pCursorCapture = m_pControllerManager->GetCursorCapture();
                if (pCursorCapture != nullptr)
                {
                    amf::AMFSurfacePtr pCursorSurface;
                    AMF_RESULT res = pCursorCapture->AcquireCursor(&pCursorSurface);

                    // Use cached cursor if AMF_REPEAT, otherwise update cached cursor.
                    if (AMF_REPEAT == res)
                    {
                        pCursorSurface = m_pLastCursor;
                    }
                    else
                    {
                        m_pLastCursor = pCursorSurface;
                        m_CursorUpdateCounter = 0;
                    }

                    // Submit cursor if it got updated, or cursor is not changed but we got new user connected or update counter still isn't full
                    // Cursor Update Counter is used to send cursor to client after change for some count in case of network loss
                    if (AMF_OK == res || (AMF_REPEAT == res && (m_bSendLastCursorToNewUser || m_CursorUpdateCounter < CURSOR_UPDATE_COUNTER_MAX)))
                    {
                        bool bMouseVisible = (pCursorSurface != nullptr);
                        //AMFTraceInfo(AMF_FACILITY, L"UpdateMouse m_LastMousePos %d %d CursorUpdateCounter = %d", m_LastMousePos.x, m_LastMousePos.y, m_CursorUpdateCounter);

                        if (bMouseVisible || m_bLastMouseVisible)
                        {
                            transport_common::ServerTransport::Ptr pServerTransport = m_pControllerManager->GetServerTransport();
                            if (pServerTransport != nullptr)
                            {
                                bool visible = false;
                                bool monochrome = false;
                                int width = 0;
                                int height = 0;
                                int pitch = 0;
                                AMFPoint hs = { 0, 0 };
                                AMFSize screenSize = { 0, 0 };

                                // If null is passed in as bitmap, it means the cursor is invisible
                                if (nullptr != pCursorSurface)
                                {
                                    width = pCursorSurface->GetPlaneAt(0)->GetWidth();
                                    height = pCursorSurface->GetPlaneAt(0)->GetHeight();
                                    pCursorSurface->GetProperty(CURSOR_HOTSPOT, &hs);
                                    visible = true;

                                    amf::AMFPlane* p = pCursorSurface->GetPlaneAt(0);
                                    pitch = p->GetWidth() * p->GetPixelSizeInBytes();

                                    pCursorSurface->GetProperty(CURSOR_MONOCHROME, &monochrome);
                                    pCursorSurface->GetProperty(CURSOR_SCREEN_RESOLUTION, &screenSize);
                                }

                                amf::AMFLock lock(&m_MouseGuard);
                                ssdk::transport_amd::CursorData cursorData(width, height, pitch, hs.x, hs.y, screenSize.width, screenSize.height, visible, monochrome);

                                AMFPoint hotspot = { cursorData.GetHotspotX(), cursorData.GetHotspotY() };
                                AMFSize resolution = { cursorData.GetCaptureResolutionX(), cursorData.GetCaptureResolutionY() };
                                transport_common::Cursor::Type type = cursorData.GetMonochrome() ? transport_common::Cursor::Type::MONOCHROME : transport_common::Cursor::Type::COLOR;

                                transport_common::Cursor cursor(pCursorSurface, hotspot, resolution, type);
                                pServerTransport->SetCursor(cursor);
                            }
                            m_bSendLastCursorToNewUser = false;
                            if (m_CursorUpdateCounter < CURSOR_UPDATE_COUNTER_MAX)
                            {
                                m_CursorUpdateCounter++;
                            }
                        }

                        m_bLastMouseVisible = bMouseVisible;
                    }
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------------
    void MouseController::ProcessInputEvent(const char* eventID, const ssdk::ctls::CtlEvent& event)
    {
        // If Input Event is Mouse Move (from Client)
        if (strcmp(eventID, DEVICE_MOUSE_POS) == 0)
        {
            // Get mouse position, normalize and set to mouse controller
            if (event.value.type == amf::AMF_VARIANT_FLOAT_POINT2D)
            {
                // Get position
                AMFFloatPoint2D position = event.value.floatPoint2DValue;
                bool isAbsolute = event.flags& DEVICE_MOUSE_POS_ABSOLUTE;

                if (isAbsolute)
                {
                    AMFRect desktopRect = {};
                    GetDesktopRect(desktopRect);
                    AMFSize screenSize = GetScreenSize();

                    position.x += desktopRect.left / float(screenSize.width);
                    position.y += desktopRect.top / float(screenSize.height);
                }

                amf::AMFLock lock(&m_MouseGuard);
                SendMouseInput(position, isAbsolute);
            }
        }
        else if (strcmp(eventID, DEVICE_MOUSE_L_CLICK) == 0 && event.value.type == amf::AMF_VARIANT_BOOL)
        {
            SendMouseButtonInput(MouseButton::LeftButton, event.value.boolValue);
        }
        else if (strcmp(eventID, DEVICE_MOUSE_R_CLICK) == 0 && event.value.type == amf::AMF_VARIANT_BOOL)
        {
            SendMouseButtonInput(MouseButton::RightButton, event.value.boolValue);
        }
        else if (strcmp(eventID, DEVICE_MOUSE_M_CLICK) == 0 && event.value.type == amf::AMF_VARIANT_BOOL)
        {
            SendMouseButtonInput(MouseButton::MiddleButton, event.value.boolValue);
        }
        else if (strcmp(eventID, DEVICE_MOUSE_M_TRACK) == 0 && event.value.type == amf::AMF_VARIANT_INT64)
        {
            SendMouseWheelInput(event.value.int64Value);
        }
    }

    void MouseController::OnConnectionEstablished()
    {
        ControllerBase::OnConnectionEstablished();

        m_LastMousePos = AMFConstructPoint(-1, -1);
        m_LastMouseSent = 0;
        m_bSendLastCursorToNewUser = m_bSendLastCursorPosToNewUser = true;
        m_CursorUpdateCounter = m_CursorPosUpdateCounter = 0;
        if (m_pControllerManager != nullptr)
        {
            amf::AMFCursorCapturePtr pCursorCapture = m_pControllerManager->GetCursorCapture();
            if (pCursorCapture != nullptr)
            {
                pCursorCapture->Reset();
            }
        }
    }

    void MouseController::OnConnectionTerminated()
    {
        ControllerBase::OnConnectionTerminated();
    }

}// namespace ssdk::ctls::svr
