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

#ifdef _WIN32

#include "RemoteDesktopServerWin.h"

#include "amf/public/include/components/DisplayCapture.h"
#include "amf/public/src/components/CursorCapture/CursorCaptureWin.h"
#include "controllers/server/win/MouseControllerWinSvr.h"
#include "controllers/server/win/KeyboardControllerWinSvr.h"
#include "controllers/server/win/GameControllerWinSvr.h"
#include "controllers/server/win/TouchScreenControllerWinSvr.h"

#include "amf/public/common/TraceAdapter.h"

#include <iostream>
#include <direct.h>

#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "dxgi.lib")

#define AMF_FACILITY L"RemoteDesktopServerWin"

static constexpr const wchar_t* PARAM_NAME_INTERACTIVE = L"interactive";
static constexpr const wchar_t* PARAM_NAME_VIDEO_API = L"videoapi";
static constexpr const wchar_t* PARAM_NAME_VIDEO_CAPTURE_TYPE = L"capture";
static constexpr const wchar_t* PARAM_NAME_CAPTURE_MODE = L"CaptureMode";
static constexpr const wchar_t* PARAM_NAME_CAPTURE_RATE = L"FrameRate";


//  Values for PARAM_NAME_CAPTURE_MODE
static constexpr const wchar_t* CAPTURE_MODE_FRAMERATE = L"FRAMERATE";
static constexpr const wchar_t* CAPTURE_MODE_PRESENT = L"PRESENT";
static constexpr const wchar_t* CAPTURE_MODE_ASAP = L"ASAP";


// Game controllers max count
static constexpr const int8_t  GAME_USER_MAX_COUNT = 4;

static bool RedirectIOToConsole()
{
    AllocConsole();

    FILE* fpResult = freopen("CONIN$", "r", stdin);
    if (fpResult != stdin)
    {
        errno_t err;
        _get_errno(&err);
        AMFTraceError(AMF_FACILITY, L"Failed to reopen stdin, errno = %d", err);
        return false;
    }

    fpResult = freopen("CONOUT$", "w", stdout);
    if (fpResult != stdout)
    {
        errno_t err;
        _get_errno(&err);
        AMFTraceError(AMF_FACILITY, L"Failed to reopen stdout, errno = %d", err);
        return false;
    }

    fpResult = freopen("CONOUT$", "w", stderr);
    if (fpResult != stderr)
    {
        errno_t err;
        _get_errno(&err);
        AMFTraceError(AMF_FACILITY, L"Failed to reopen stderr, errno = %d", err);
        return false;
    }
    return true;
}


RemoteDesktopServerWin::RemoteDesktopServerWin()
{
    SetParamDescription(PARAM_NAME_INTERACTIVE, ParamCommon, L"Enable interactive mode (true, false), default=true", ParamConverterBoolean);
    SetParamDescription(PARAM_NAME_VIDEO_API, ParamCommon, L"Select video API (DX11, DX12), default=DX12", nullptr);
    SetParamDescription(PARAM_NAME_VIDEO_CAPTURE_TYPE, ParamCommon, L"Select video capture type (AMD, DD), default=AMD", nullptr);
    SetParamDescription(PARAM_NAME_CAPTURE_MODE, ParamCommon, L"Specify capture mode: [framerate, present, asap], default = framerate", nullptr);
}

RemoteDesktopServerWin::~RemoteDesktopServerWin()
{
    Terminate();
}

bool RemoteDesktopServerWin::OnAppInit()
{
    RemoteDesktopServer::OnAppInit();

    bool result = false;
    bool interactive = true;
    GetParam(PARAM_NAME_INTERACTIVE, interactive);
    if (interactive == true)
    {
        RedirectIOToConsole();
        m_Interactive = true;
    }

    if (InitDX() == true)
    {
        result = true;
    }
    return result;
}

void RemoteDesktopServerWin::OnAppTerminate()
{
    TerminateVideoCapture();
    TerminateDX();
    if (m_Interactive == true)
    {
        FreeConsole();
    }
    RemoteDesktopServer::OnAppTerminate();
}

bool RemoteDesktopServerWin::InitDX()
{
    //bool result = false;
    AMF_RESULT amfResult = AMF_FAIL;

    std::wstring dx = L"DX12";
    if (GetParamWString(PARAM_NAME_VIDEO_API, dx) == AMF_OK)
    {
        dx = ::toUpper(dx);
    }
    if (dx == L"DX12")
    {
        if ((amfResult = m_DeviceDX12.Init(0)) != AMF_OK)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to initialize a DX12 device, result=%s - make sure an AMD GPU is present and the AMD driver is installed", amf::AMFGetResultText(amfResult));
        }
        else if (m_Context != nullptr)
        {
            amf::AMFContext2Ptr context(m_Context);
            if (context == nullptr)
            {
                AMFTraceError(AMF_FACILITY, L"Failed to query the AMFContext2 interface, seems that you have an outdated AMD driver");
            }
            else
            {
                if ((amfResult = context->InitDX12(m_DeviceDX12.GetDevice())) != AMF_OK)
                {
                    AMFTraceError(AMF_FACILITY, L"Failed to initialize AMFContext for DX12, result=%s", amf::AMFGetResultText(amfResult));
                }
                else
                {
                    m_MemoryType = amf::AMF_MEMORY_TYPE::AMF_MEMORY_DX12;
                    AMFTraceInfo(AMF_FACILITY, L"Using DX12 video pipeline");
                }
            }
        }
    }
    else if (dx == L"DX11")
    {
        if ((amfResult = m_DeviceDX11.Init(0)) != AMF_OK)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to initialize a DX12 device, result=%s - make sure an AMD GPU is present and the AMD driver is installed", amf::AMFGetResultText(amfResult));
        }
        else if (m_Context != nullptr)
        {
            if ((amfResult = m_Context->InitDX11(m_DeviceDX11.GetDevice())) != AMF_OK)
            {
                AMFTraceError(AMF_FACILITY, L"Failed to initialize AMFContext for DX12, result=%s", amf::AMFGetResultText(amfResult));
            }
            else
            {
                m_MemoryType = amf::AMF_MEMORY_TYPE::AMF_MEMORY_DX11;
                AMFTraceInfo(AMF_FACILITY, L"Using DX11 video pipeline");
            }
        }
    }
    else
    {
        AMFTraceError(AMF_FACILITY, L"Unsupported graphics API \"%S\"", dx.c_str());
    }
    return (amfResult == AMF_OK);// result;
}

void RemoteDesktopServerWin::TerminateDX()
{
    if (m_Context != nullptr)
    {
        m_Context->Terminate();
        m_Context = nullptr;
    }
    if (m_MemoryType == amf::AMF_MEMORY_TYPE::AMF_MEMORY_DX11)
    {
        m_DeviceDX11.Terminate();
    }
    else if (m_MemoryType == amf::AMF_MEMORY_TYPE::AMF_MEMORY_DX12)
    {
        m_DeviceDX12.Terminate();
    }

}

bool RemoteDesktopServerWin::InitVideoCapture()
{
    bool result = false;
    AMF_RESULT amfResult = AMF_OK;
    static constexpr const int64_t DEFAULT_FRAMERATE = 60;

    std::wstring captureType = L"AMD";
    if (GetParamWString(PARAM_NAME_VIDEO_CAPTURE_TYPE, captureType) == AMF_OK)
    {
        captureType = ::toUpper(captureType);
    }

    if (captureType == L"DD")    //  Desktop Duplication is a Microsoft-specific API and is only supported on Windows
    {
        if ((amfResult = AMFCreateComponentDisplayCapture(m_Context, nullptr, &m_VideoCapture)) != AMF_OK)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to create Microsoft Desktop Duplication video capture component, result=%s", amf::AMFGetResultText(amfResult));
        }
        else
        {
            AMFTraceInfo(AMF_FACILITY, L"Using Microsoft Desktop Duplication API");
        }
    }
    else
    {
        if ((amfResult = g_AMFFactory.GetFactory()->CreateComponent(m_Context, AMFDisplayCapture, &m_VideoCapture)) != AMF_OK)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to create AMD video capture component, result=%s", amf::AMFGetResultText(amfResult));
        }
        else
        {
            AMFTraceInfo(AMF_FACILITY, L"Using AMD Direct Capture API");
        }
    }
    if (amfResult == AMF_OK)
    {
        std::wstring captureMode = CAPTURE_MODE_FRAMERATE;
        if (GetParamWString(PARAM_NAME_CAPTURE_MODE, captureMode) == AMF_OK)
        {
            captureMode = ::toUpper(captureMode);
        }
        if (captureMode == CAPTURE_MODE_FRAMERATE)
        {
            amfResult = m_VideoCapture->SetProperty(AMF_DISPLAYCAPTURE_MODE, amf_int64(AMF_DISPLAYCAPTURE_MODE_ENUM::AMF_DISPLAYCAPTURE_MODE_KEEP_FRAMERATE));
            switch (amfResult)
            {
            case AMF_OK:
            {
                int64_t frameRate = DEFAULT_FRAMERATE;
                GetParam(PARAM_NAME_CAPTURE_RATE, frameRate);
                amfResult = m_VideoCapture->SetProperty(AMF_DISPLAYCAPTURE_FRAMERATE, AMFConstructRate(int32_t(frameRate), 1));
                switch (amfResult)
                {
                case AMF_OK:
                    AMFTraceInfo(AMF_FACILITY, L"Capture mode is set to \"%s\" at %d fps", CAPTURE_MODE_FRAMERATE, int32_t(frameRate));
                    m_VideoStreamDescriptor.SetFramerate(static_cast<float>(frameRate));
                    result = true;
                    break;
                case AMF_NOT_FOUND:
                case AMF_ACCESS_DENIED:
                    AMFTraceWarning(AMF_FACILITY, L"Failed to set video capture frame rate to %d fps", int32_t(frameRate));
                    result = true;
                    break;
                default:
                    break;
                }
            }
            break;
            case AMF_NOT_FOUND:
            case AMF_ACCESS_DENIED:
                AMFTraceWarning(AMF_FACILITY, L"Selected video capture method does not support capture at a fixed frame rate, -%s %s ignored", PARAM_NAME_CAPTURE_MODE, CAPTURE_MODE_FRAMERATE);
                result = true;
                break;
            default:
                AMFTraceError(AMF_FACILITY, L"Error setting the AMF_DISPLAYCAPTURE_MODE property on the display capture component, result=%s", amf::AMFGetResultText(amfResult));
                break;
            }
            m_CaptureMode = CaptureMode::FRAMERATE;
        }
        else if (captureMode == CAPTURE_MODE_PRESENT)
        {
            amfResult = m_VideoCapture->SetProperty(AMF_DISPLAYCAPTURE_MODE, amf_int64(AMF_DISPLAYCAPTURE_MODE_ENUM::AMF_DISPLAYCAPTURE_MODE_WAIT_FOR_PRESENT));
            m_VideoStreamDescriptor.SetFramerate(float(DEFAULT_FRAMERATE)); //  Set the initial frame rate to 60fps, it will be adjusted automatically in a few seconds to the actual frame rate determined by capture
            m_CaptureMode = CaptureMode::PRESENT;
            result = true;
        }
        else if (captureMode == CAPTURE_MODE_ASAP)
        {
            amfResult = m_VideoCapture->SetProperty(AMF_DISPLAYCAPTURE_MODE, amf_int64(AMF_DISPLAYCAPTURE_MODE_ENUM::AMF_DISPLAYCAPTURE_MODE_GET_CURRENT_SURFACE));
            m_VideoStreamDescriptor.SetFramerate(float(DEFAULT_FRAMERATE)); //  Set the initial frame rate to 60fps, it will be adjusted automatically in a few seconds to the actual frame rate determined by capture
            m_CaptureMode = CaptureMode::ASAP;
            result = true;
        }
        if (result != true)
        {
            if (amfResult == AMF_OK)
            {
                AMFTraceInfo(AMF_FACILITY, L"Capture mode is set to %s", captureMode.c_str());
            }
            else
            {
                AMFTraceError(AMF_FACILITY, L"Failed to set video capture mode to %s, result=%s", captureMode.c_str(), amf::AMFGetResultText(amfResult));
            }
        }
        else
        {
            result = RemoteDesktopServer::InitVideoCapture();
        }
    }
    return result;
}

bool RemoteDesktopServerWin::InitCursorCapture()
{
    m_CursorCapture = new amf::AMFCursorCaptureWin(m_Context);
    return RemoteDesktopServer::InitCursorCapture();
}

bool RemoteDesktopServerWin::RunServer()
{
    bool result = false;
    if (m_Interactive == true)
    {
        std::cout << "Type \'exit\' or \'quit\' to terminate\n";
        bool keepRunning = true;
        do
        {
            std::string command;
            std::getline(std::cin, command);
            if (command == "exit" || command == "quit")
            {
                keepRunning = false;
            }
        } while (keepRunning == true);
        result = true;
    }
    else
    {
        result = RemoteDesktopServer::RunServer();
    }
    return result;
}

bool RemoteDesktopServerWin::InitDirectories()
{
    bool result = false;
    const char* tmpDir = getenv("TEMP");
    if (tmpDir == nullptr)
    {
        tmpDir = getenv("TMP");
    }
    if (tmpDir == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"Ensure that either the TEMP or TMP environment is set");
    }
    else
    {
        m_ShutdownDir = tmpDir;
        m_ShutdownDir += "\\RemoteDesktopServer";
        result = mkdir(m_ShutdownDir.c_str()) == 0 || errno == EEXIST;
        m_ShutdownDir += '\\';  //  Prepare the final OS-specific path delimiter so that the base class could just append a file name to it.
        if (result == false)
        {
            AMFTraceError(AMF_FACILITY, L"Ensure that RemoteDesktopServer has write permissions to %S", tmpDir);
        }
    }
    return result;
}

bool RemoteDesktopServerWin::InitControllers()
{
    bool result = RemoteDesktopServer::InitControllers();
    if (result == false || m_pControllerManager == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"Failed to initialize controller manager.");
        return false;
    }

    // Create Controllers
    m_pControllerManager->AddController(ssdk::ctls::svr::ControllerBase::Ptr(new ssdk::ctls::svr::MouseControllerWin(m_pControllerManager)));
    m_pControllerManager->AddController(ssdk::ctls::svr::ControllerBase::Ptr(new ssdk::ctls::svr::KeyboardControllerWin(m_pControllerManager)));

    for (int8_t i = 0; i < GAME_USER_MAX_COUNT; i++)
    {
        m_pControllerManager->AddController(ssdk::ctls::svr::ControllerBase::Ptr(new ssdk::ctls::svr::GameControllerWin(m_pControllerManager, i)));
    }

    m_pControllerManager->AddController(ssdk::ctls::svr::ControllerBase::Ptr(new ssdk::ctls::svr::TouchScreenControllerWin(m_pControllerManager)));

    AMFTraceDebug(AMF_FACILITY, L"Controller Manager initialized");

    return result;
}

#endif
