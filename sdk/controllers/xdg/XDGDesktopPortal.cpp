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

#if defined(__linux)

#include "XDGDesktopPortal.h"
#include "amf/public/common/TraceAdapter.h"

namespace ssdk::ctls
{
    //-------------------------------------------------------------------------------------------------
    // the definitions for these methods can be found here:
    // https://flatpak.github.io/xdg-desktop-portal/docs/index.html
    //-------------------------------------------------------------------------------------------------
    static constexpr const wchar_t* const AMF_FACILITY = L"XDGDesktopPortal";
    static const char* SSDK_XDG_HANDLE_TOKEN = "amd_ssdk_xdg_capture";

    XDGDesktopPortal::XDGDesktopPortal() :
        m_dbusRD(DBusConnection::DBUS_BUS_TYPE::USER)
    {

    }

    XDGDesktopPortal::~XDGDesktopPortal()
    {
        Terminate();
    }

    AMF_RESULT XDGDesktopPortal::Initialize()
    {
        amf::AMFLock lock(&m_Sect);

        // Reset the controller that controls if the m_xdgRD.Start call should continue (see InitializeXDGRDDBus)
        m_rdAbortController.Reset();

        // We need dynamically link/initialize the sbus
        AMF_RETURN_IF_FAILED(m_dbusRD.Initialize());
        AMF_RETURN_IF_FAILED(m_xdgRD.Initialize(&m_dbusRD, SSDK_XDG_HANDLE_TOKEN));

        // Initialize Remode Desktop portal sd-bus
        AMF_RETURN_IF_FAILED(InitializeXDGDRDBus());

        return AMF_OK;
    }

    AMF_RESULT XDGDesktopPortal::InitializeXDGDRDBus()
    {
        // We start by querying the source types and cursor modes. these are not used yet.
        XDGRDPortalConnection::SOURCE_TYPE sourceTypes = XDGRDPortalConnection::SOURCE_TYPE_NONE;
        AMF_RETURN_IF_FAILED(m_xdgRD.AvailableDeviceTypes(sourceTypes));
        AMFTraceInfo(AMF_FACILITY, L"m_xdgRD.AvailableDeviceTypes = %d", sourceTypes);

        // Create Remote Desktop portal session
        AMF_RETURN_IF_FAILED(m_xdgRD.CreateSession(SSDK_XDG_HANDLE_TOKEN));

        // now we select a particular source to capture from
        AMF_RETURN_IF_FAILED(m_xdgRD.SelectDevices(XDGRDPortalConnection::SOURCE_TYPE_POINTER, ""));

        // Next we ask the user for permission to start
        // Because we could wait indefinitely for the user to approve the recording, we pass in
        // the "m_rdAbortController", which will allow us to cancel the request should Terminate be
        // called while we are in the middle of initializing
        AMF_RETURN_IF_FAILED(m_xdgRD.Start("", m_rdAbortController));

        return AMF_OK;
    }

    AMF_RESULT XDGDesktopPortal::NotifyPointerMotion(int dx, int dy)
    {
        // The dx and dy values indicate the change in position since the last pointer event
        //AMFTraceInfo(AMF_FACILITY, L"XDGDesktopPortal::NotifyPointerMotion dx(%d), fdx(%5.4f); dy(%d), fdy(%5.4f)", dx, fdx, dy, fdy);
        AMF_RETURN_IF_FAILED(m_xdgRD.NotifyPointerMotion(dx, dy));

        return AMF_OK;
    }

    AMF_RESULT XDGDesktopPortal::NotifyPointerMotionAbsolute(int ptScreenX, int ptScreenY)
    {
        // Normalize screen coordinates
        double x = float(ptScreenX) / float(m_SrceenResolution.width);
        double y = float(ptScreenY) / float(m_SrceenResolution.height);

        // Append normalized coordinates [0, 1] (e.g., 0.5, 0.5 for the center of the screen)
        AMF_RETURN_IF_FAILED(m_xdgRD.NotifyPointerMotionAbsolute(x, y));

        return AMF_OK;
    }

    void XDGDesktopPortal::SetScreenResolution(const AMFSize& srceenResolution)
    {
        m_SrceenResolution = srceenResolution;
    }

    void XDGDesktopPortal::GetScreenResolutionFromXDG(AMFSize& srceenResolution)
    {
        // Get resolution
        // To Do...

        AMFTraceDebug(AMF_FACILITY, L"Screen resolution: %dx%d", srceenResolution.width, srceenResolution.height);
    }

    AMF_RESULT XDGDesktopPortal::Terminate()
    {
        // Clean up
        // Will cancel the m_xdgRD.Start call if it's still waiting for user input
        m_rdAbortController.Cancel();

        amf::AMFLock lock(&m_Sect);

        AMFTraceInfo(AMF_FACILITY, L"Terminate()");
        
        m_xdgRD.Terminate();
        m_dbusRD.Terminate();

        return AMF_OK;
    }
    //-----------------------------------------------------------------------------------------

} // namespace ssdk::ctls

#endif // __linux
