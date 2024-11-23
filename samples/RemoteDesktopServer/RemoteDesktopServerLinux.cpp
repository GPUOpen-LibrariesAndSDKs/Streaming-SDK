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

#ifndef _WIN32

#include "RemoteDesktopServerLinux.h"

#include "amf/public/include/components/DisplayCapture.h"
#include "amf/public/src/components/CursorCapture/CursorCaptureLinux.h"
//#include "controllers/svr/lnx/MouseControllerLnxSvr.h"
//#include "controllers/svr/lnx/KeyboardControllerLnxSvr.h"
//#include "controllers/svr/lnx/GameControllerLnxSvr.h"

#include "amf/public/common/TraceAdapter.h"

#include <iostream>
#include <sys/stat.h>

static constexpr const wchar_t* const AMF_FACILITY = L"RemoteDesktopServerLinux";

static constexpr const wchar_t* PARAM_NAME_INTERACTIVE = L"interactive";

// Game controllers max count
static constexpr const int8_t  GAME_USER_MAX_COUNT = 4;

static bool RedirectIOToConsole()
{
    FILE* fpResult = freopen("CONIN$", "r", stdin);
    if (fpResult != stdin)
    {
        AMFTraceError(AMF_FACILITY, L"Failed to reopen stdin, errno = %d", errno);
        return false;
    }

    fpResult = freopen("CONOUT$", "w", stdout);
    if (fpResult != stdout)
    {
        AMFTraceError(AMF_FACILITY, L"Failed to reopen stdout, errno = %d", errno);
        return false;
    }

    fpResult = freopen("CONOUT$", "w", stderr);
    if (fpResult != stderr)
    {
        AMFTraceError(AMF_FACILITY, L"Failed to reopen stderr, errno = %d", errno);
        return false;
    }
    return true;
}


RemoteDesktopServerLinux::RemoteDesktopServerLinux()
{
    SetParamDescription(PARAM_NAME_INTERACTIVE, ParamCommon, L"Enable interactive mode (true, false), default=true", ParamConverterBoolean);
}

RemoteDesktopServerLinux::~RemoteDesktopServerLinux()
{
    Terminate();
}

bool RemoteDesktopServerLinux::OnAppInit()
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

    if (InitVulkan() == true)
    {
        result = true;
    }
    return result;
}

void RemoteDesktopServerLinux::OnAppTerminate()
{
    TerminateVideoCapture();
    TerminateVulkan();
    if (m_Interactive == true)
    {
        //FreeConsole();
    }
    RemoteDesktopServer::OnAppTerminate();
}

bool RemoteDesktopServerLinux::InitVulkan()
{

    return false;
}

void RemoteDesktopServerLinux::TerminateVulkan()
{
    if (m_Context != nullptr)
    {
        m_Context->Terminate();
        m_Context = nullptr;
    }
    if (m_MemoryType == amf::AMF_MEMORY_TYPE::AMF_MEMORY_VULKAN)
    {
        //m_DeviceVulkan.Terminate();
    }
}

bool RemoteDesktopServerLinux::InitVideoCapture()
{
    return RemoteDesktopServer::InitVideoCapture();
}

bool RemoteDesktopServerLinux::InitCursorCapture()
{
    m_CursorCapture = new amf::AMFCursorCaptureLinux(m_Context);
    return RemoteDesktopServer::InitCursorCapture();
}

bool RemoteDesktopServerLinux::RunServer()
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

bool RemoteDesktopServerLinux::InitDirectories()
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
        result = mkdir(m_ShutdownDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0 || errno == EEXIST;
        m_ShutdownDir += '\\';  //  Prepare the final OS-specific path delimiter so that the base class could just append a file name to it.
        if (result == false)
        {
            AMFTraceError(AMF_FACILITY, L"Ensure that RemoteDesktopServer has write permissions to %S", tmpDir);
        }
    }
    return result;
}

bool RemoteDesktopServerLinux::InitControllers()
{
    bool result = RemoteDesktopServer::InitControllers();
    if (result == false || m_pControllerManager == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"Failed to initialize controller manager.");
        return false;
    }

    // Create Controllers
    /*m_pControllerManager->AddController(ssdk::ctls::svr::ControllerBase::Ptr(new ssdk::ctls::svr::MouseControllerLnx(m_pControllerManager)));
    m_pControllerManager->AddController(ssdk::ctls::svr::ControllerBase::Ptr(new ssdk::ctls::svr::KeyboardControllerLnx(m_pControllerManager)));

    for (int8_t i = 0; i < GAME_USER_MAX_COUNT; i++)
    {
        m_pControllerManager->AddController(ssdk::ctls::svr::ControllerBase::Ptr(new ssdk::ctls::svr::GameControllerLnx(m_pControllerManager, i)));
    }*/

    AMFTraceDebug(AMF_FACILITY, L"Controller Manager initialized");

    return result;
}

#endif
