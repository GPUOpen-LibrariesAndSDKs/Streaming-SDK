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

#if defined(_WIN32)

#include "../MouseController.h"
#include "../ControllerManager.h"

namespace ssdk::ctls
{
    static constexpr const wchar_t* const AMF_FACILITY = L"MouseController";

    //-------------------------------------------------------------------------------------------------
    void MouseController::InitRawInput()
    {
        if (m_bRawInputInitialized == false)
        {
            UINT               uiNumDevices = 0;
            GetRawInputDeviceList(nullptr, &uiNumDevices, sizeof(RAWINPUTDEVICELIST));
            if (uiNumDevices > 0)
            {
                m_Data.SetSize(sizeof(RAWINPUTDEVICELIST) * uiNumDevices);
                PRAWINPUTDEVICELIST pRawInputDeviceList = (PRAWINPUTDEVICELIST)m_Data.GetData();
                GetRawInputDeviceList(pRawInputDeviceList, &uiNumDevices, sizeof(RAWINPUTDEVICELIST));
                for (UINT i = 0; i < uiNumDevices; i++)
                {
                    if (pRawInputDeviceList[i].dwType == RIM_TYPEMOUSE)
                    {
                        // AMFTraceError(AMF_FACILITY, L"MouseController::  GetRawInputDeviceList() - mouse found");
                        break;
                    }
                }

            }
            else
            {
                AMFTraceError(AMF_FACILITY, L"MouseController::No devices()");
            }
            RAWINPUTDEVICE Rid[1];

            Rid[0].usUsagePage = 0x01;
            Rid[0].usUsage = 0x02;
            Rid[0].dwFlags = RIDEV_INPUTSINK; // RIDEV_NOLEGACY; // adds HID mouse and also ignores legacy mouse messages
            Rid[0].hwndTarget = (HWND)m_hWnd;
            if (RegisterRawInputDevices(Rid, 1, sizeof(Rid[0])) == FALSE)
            {
                AMFTraceError(AMF_FACILITY, L"MouseController::RegisterRawInputDevices() failed ERR=%d", (int)GetLastError());
            }
            m_bRawInputInitialized = true;
        }
    }

    //-------------------------------------------------------------------------------------------------
    void MouseController::UpdateFromServer()
    {
        if (GetForegroundWindow() != m_hWnd)
        {
            return;
        }
        amf::AMFLock lock(&m_Sect);

        CtlEvent ev = {};
        ev.value.type = amf::AMF_VARIANT_FLOAT_POINT2D;
        ev.flags = DEVICE_MOUSE_POS_ABSOLUTE;
        ev.value.floatPoint2DValue = GetMouseConfig()->MousePosition();

        //AMFTraceInfo(AMF_FACILITY, L"MouseController::SetCursorPos(%6.5f,%6.5f)", ev.value.floatPoint2DValue.x, ev.value.floatPoint2DValue.y);
        SetCursorPos(int(ev.value.floatPoint2DValue.x), int(ev.value.floatPoint2DValue.y));
        
        WindowPoint pointScreen = { -1, -1 };
        GetCursorPos(&pointScreen);
        //AMFTraceInfo(AMF_FACILITY, L"MouseController::GetCursorPos out(%d,%d)", pointScreen.x, pointScreen.y);

        m_LastMousePoint.x = pointScreen.x;
        m_LastMousePoint.y = pointScreen.y;
    }

    //-------------------------------------------------------------------------------------------------
    AMF_RESULT MouseController::UpdateFromInternalInput(LPARAM lParam)
    {
        if (GetForegroundWindow() != m_hWnd)
        {
            return AMF_OK;
        }

        amf::AMFLock lock(&m_Sect);

        CtlEvent ev = {};
        ev.value.type = amf::AMF_VARIANT_FLOAT_POINT2D;

        WindowPoint pointScreen = { -1, -1 };
        //AMFTraceInfo(AMF_FACILITY, L"MouseController::Update From Internal Input - lParam: %d", lParam);

        if (true == m_bRelativePos)
        {
            UINT cbSizeT = 0;
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &cbSizeT, sizeof(RAWINPUTHEADER));

            if (cbSizeT > 0)
            {
                if (cbSizeT > m_Data.GetSize())
                {
                    m_Data.SetSize((amf_size)cbSizeT);
                }
                PRAWINPUT pri = (PRAWINPUT)m_Data.GetData();
                GetRawInputData((HRAWINPUT)lParam, RID_INPUT, pri, &cbSizeT, sizeof(RAWINPUTHEADER));

                if (pri->header.dwType == RIM_TYPEMOUSE)
                {
                    AMFSize screenSize = { ::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN) };

                    pointScreen.x = pri->data.mouse.lLastX;
                    pointScreen.y = pri->data.mouse.lLastY;
                    if (pri->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
                    {
                        ev.flags = DEVICE_MOUSE_POS_ABSOLUTE;

                        static const float normaizationValue = 65535.f;

                        ev.value.floatPoint2DValue.x = float(pointScreen.x) / normaizationValue;
                        ev.value.floatPoint2DValue.y = float(pointScreen.y) / normaizationValue;

                        pointScreen.x = LONG(float(pointScreen.x) / normaizationValue * float(screenSize.width));
                        pointScreen.y = LONG(float(pointScreen.y) / normaizationValue * float(screenSize.height));
                    }
                    else
                    {

                        ev.value.floatPoint2DValue.x = float(pointScreen.x) / float(screenSize.width);
                        ev.value.floatPoint2DValue.y = float(pointScreen.y) / float(screenSize.height);

                        pointScreen.x += m_LastMousePoint.x;
                        pointScreen.y += m_LastMousePoint.y;
                    }

                    if (pointScreen.x != m_LastMousePoint.x || pointScreen.y != m_LastMousePoint.y)
                    {
                        if ((pri->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == 0)
                        {
                            if (m_pControllerManager != nullptr)
                            {
                                m_pControllerManager->SendControllerEvent(m_EventIDs[DEVICE_MOUSE_POS_INDEX].c_str(), &ev);
                            }
                        }
                        m_LastMousePoint.x = pointScreen.x;
                        m_LastMousePoint.y = pointScreen.y;

                    }
                }
            }
        }
        else
        {
            if (GetMouseConfig()->IsFullScreen() == false)
            {
                GetCursorPos(&pointScreen);
            }
            else
            {
                GetPhysicalCursorPos(&pointScreen);
            }

            WindowPoint pointClient = pointScreen;

            if ((m_LastMousePoint.x != pointScreen.x || m_LastMousePoint.y != pointScreen.y) && m_pVideoPresenter != nullptr)
            {
                RECT client;
                GetClientRect((HWND)m_hWnd, &client);
                if (m_pVideoPresenter->GetFullScreen() == false)
                {
                    ScreenToClient((HWND)m_hWnd, &pointClient);
                }
                else
                {
                    int clientWidth = (client.right - client.left);
                    int clientHeight = (client.bottom - client.top);
                    pointClient = { (pointScreen.x * clientWidth) / ::GetSystemMetrics(SM_CXSCREEN), (pointScreen.y * clientHeight) / ::GetSystemMetrics(SM_CYSCREEN) };
                }

                AMFPoint pointSource = m_pVideoPresenter->MapClientToSource(AMFConstructPoint(pointClient.x, pointClient.y));

                AMFRect sourceRect = m_pVideoPresenter->GetSourceRect();
                pointSource.x = AMF_CLAMP(pointSource.x, sourceRect.left, sourceRect.right - 1);
                pointSource.y = AMF_CLAMP(pointSource.y, sourceRect.top, sourceRect.bottom - 1);

                ev.flags = DEVICE_MOUSE_POS_ABSOLUTE;
                ev.value.floatPoint2DValue.x = float(pointSource.x - sourceRect.left) / float(sourceRect.right - sourceRect.left - 1);
                ev.value.floatPoint2DValue.y = float(pointSource.y - sourceRect.top) / float(sourceRect.bottom - sourceRect.top - 1);
                // AMFTraceInfo(AMF_FACILITY, L"MouseController:: out(%6.5f,%6.5f)", ev.value.floatPoint2DValue.x, ev.value.floatPoint2DValue.y);
                
                if (m_pControllerManager != nullptr)
                {
                    m_pControllerManager->SendControllerEvent(m_EventIDs[DEVICE_MOUSE_POS_INDEX].c_str(), &ev);
                }

                m_LastMousePoint.x = pointScreen.x;
                m_LastMousePoint.y = pointScreen.y;
            }
        }

        return AMF_OK;
    }

    //-------------------------------------------------------------------------------------------------
    AMF_RESULT MouseController::ProcessMessage(const WindowMessage& msg)
    {
        AMF_RESULT result = AMF_FAIL;
        //AMFTraceInfo(AMF_FACILITY, L"MouseController::Process Message - msg.message: %d", msg.message);
        if (true == m_bRelativePos)
        {
            InitRawInput();

            if (msg.message == WM_INPUT)
            {
                UpdateFromInternalInput(msg.lParam);
            }
        }

        if (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST && msg.message != WM_MOUSEMOVE)
        {

            CtlEvent ev = {};
            ev.value.type = amf::AMF_VARIANT_BOOL;
            std::string id;
            switch (msg.message)
            {
            case WM_LBUTTONDOWN:
                id = m_EventIDs[DEVICE_MOUSE_L_CLICK_INDEX].c_str();
                ev.value.boolValue = true;
                break;
            case WM_LBUTTONUP:
                id = m_EventIDs[DEVICE_MOUSE_L_CLICK_INDEX].c_str();
                ev.value.boolValue = false;
                break;
            case WM_RBUTTONDOWN:
                id = m_EventIDs[DEVICE_MOUSE_R_CLICK_INDEX].c_str();
                ev.value.boolValue = true;
                break;
            case WM_RBUTTONUP:
                id = m_EventIDs[DEVICE_MOUSE_R_CLICK_INDEX].c_str();
                ev.value.boolValue = false;
                break;
            case WM_MBUTTONDOWN:
                id = m_EventIDs[DEVICE_MOUSE_M_CLICK_INDEX].c_str();
                ev.value.boolValue = true;
                break;
            case WM_MBUTTONUP:
                id = m_EventIDs[DEVICE_MOUSE_M_CLICK_INDEX].c_str();
                ev.value.boolValue = false;
                break;
            case WM_MOUSEWHEEL:
                ev.value.type = amf::AMF_VARIANT_INT64;
                id = m_EventIDs[DEVICE_MOUSE_M_TRACK_INDEX].c_str();
                ev.value.int64Value = amf_int64(GET_WHEEL_DELTA_WPARAM(msg.wParam));
                break;
            }
            if (id.length() != 0)
            {
                if (m_pControllerManager != nullptr)
                {
                    m_pControllerManager->SendControllerEvent(id.c_str(), &ev);
                }

                result = AMF_OK;
            }
        }

        return result;
    }

}// namespace ssdk::ctls
#endif
