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
#include "controllers/server/lnx/MouseControllerLnxSvr.h"
#include "controllers/server/lnx/KeyboardControllerLnxSvr.h"
#include "controllers/server/lnx/GameControllerLnxSvr.h"
#include "controllers/server/lnx/TouchScreenControllerLnxSvr.h"

#include "amf/public/common/TraceAdapter.h"

#include <iostream>
#include <sys/stat.h>
#include <filesystem>
#include <fcntl.h>

static constexpr const wchar_t* const AMF_FACILITY = L"RemoteDesktopServerLinux";

static constexpr const wchar_t* PARAM_NAME_INTERACTIVE = L"interactive";

// Game controllers max count
static constexpr const int8_t  GAME_USER_MAX_COUNT = 1;

static bool RedirectIOToConsole()
{
    bool result = true;
    // Check if the standard input/output streams are already connected to a terminal. If not, then open a new terminal or pseudo-terminal
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO))
    {
        // Open a new or pseudo-terminal. Opens the terminal for both reading and writing
        // /dev/tty: opens the controlling terminal of the current process, assumes that /dev/tty is accessible
        int fd = open("/dev/tty", O_RDWR);
        if (fd == -1)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to open /dev/tty");
            result = false;
        }
        // Redirect stdin, stdout, and stderr to the pseudo-terminal
        else if (dup2(fd, STDIN_FILENO) == -1)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to redirect stdin");
            result = false;
        }
        else if (dup2(fd, STDOUT_FILENO) == -1)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to redirect stdout");
            result = false;
        }
        else if (dup2(fd, STDERR_FILENO) == -1)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to redirect stderr");
            result = false;
        }

        close(fd);
    }
    return result;

    /*// Command to open a new terminal emulator and run the application
    const char* terminalCommand = "xterm -e bash -c './RemoteDesktopServer; exec bash' &"; // Replace xterm with your preferred terminal emulator if needed

    // Execute the command to open a new terminal window
    int result = std::system(terminalCommand);
    if (result == -1)
    {
        AMFTraceError(AMF_FACILITY, L"Failed to open a new terminal window");
        return false;
    }

    return true;*/
}


RemoteDesktopServerLinux::RemoteDesktopServerLinux()
{
    SetParamDescription(PARAM_NAME_INTERACTIVE, ParamCommon, L"Enable interactive mode (true, false), default=true", ParamConverterBoolean);

    XInitThreads();
    m_pDisplay = XDisplay::Ptr(new XDisplay);

    InitCrtcInfo();
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
    AMF_RESULT amfResult = AMF_FAIL;
    if ((amfResult = m_DeviceVulkan.Init(0, m_Context)) != AMF_OK)
    {
        AMFTraceError(AMF_FACILITY, L"Failed to initialize a Vulkan device, result=%s - make sure an AMD GPU is present and the AMD driver is installed", amf::AMFGetResultText(amfResult));
    }
    else if (m_Context != nullptr)
    {
        if ((amfResult = amf::AMFContext1Ptr(m_Context)->InitVulkan(m_DeviceVulkan.GetDevice())) != AMF_OK)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to initialize AMFContext for Vulkan, result=%s", amf::AMFGetResultText(amfResult));
        }
        else
        {
            m_MemoryType = amf::AMF_MEMORY_TYPE::AMF_MEMORY_VULKAN;
        }
    }

    return (amfResult == AMF_OK);
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
        m_DeviceVulkan.Terminate();
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
    const char* tmpDir = std::filesystem::temp_directory_path().c_str();
    if (tmpDir == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"Couldn't find tmp directory");
    }
    else
    {
        m_ShutdownDir = tmpDir;
        m_ShutdownDir += "/RemoteDesktopServer";
        result = mkdir(m_ShutdownDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0 || errno == EEXIST;
        m_ShutdownDir += '/';  //  Prepare the final OS-specific path delimiter so that the base class could just append a file name to it.
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
    m_pControllerManager->AddController(ssdk::ctls::svr::ControllerBase::Ptr(new ssdk::ctls::svr::MouseControllerLnx(m_pControllerManager, m_pDisplay, m_pCrtcInfo)));
    m_pControllerManager->AddController(ssdk::ctls::svr::ControllerBase::Ptr(new ssdk::ctls::svr::KeyboardControllerLnx(m_pControllerManager, m_pDisplay)));

    for (int8_t i = 0; i < GAME_USER_MAX_COUNT; i++)
    {
        m_pControllerManager->AddController(ssdk::ctls::svr::ControllerBase::Ptr(new ssdk::ctls::svr::GameControllerLnx(m_pControllerManager, i)));
    }
    
    m_pControllerManager->AddController(ssdk::ctls::svr::ControllerBase::Ptr(new ssdk::ctls::svr::TouchScreenControllerLnx(m_pControllerManager, m_pDisplay, m_pCrtcInfo)));

    AMFTraceDebug(AMF_FACILITY, L"Controller Manager initialized");

    return result;
}

AMF_RESULT RemoteDesktopServerLinux::InitCrtcInfo()
{
    AMF_RESULT res = AMF_OK;

    amf::AMFLock lock(&m_sync);
    if (nullptr == m_pCrtcInfo)
    {
        int64_t monitorID = 0;
        GetParam(PARAM_NAME_MONITOR_ID, monitorID);

        XDisplayPtr display(m_pDisplay);
        Window root = DefaultRootWindow((Display*)display);

        // Get proper screen dimension by Xrandr api. the indexing in XRRScreenResources->crtcs
        // is the same as monintorID in screen capture.
        XRRScreenResourcesPtr screenResource(XRRGetScreenResourcesCurrent(display, root), &XRRFreeScreenResources);
        AMF_RETURN_IF_FALSE(nullptr != screenResource, AMF_FAIL, L"Cannot get screenResource for current screen.")
        AMF_RETURN_IF_FALSE(screenResource->ncrtc > monitorID, AMF_FAIL, L"monitorID=%d is out of range. There are only %d crtcs.", monitorID, screenResource->ncrtc);

        // Get CRTC info.
        XRRCrtcInfoPtr crtcInfo(XRRGetCrtcInfo(display,screenResource.get(), screenResource->crtcs[monitorID]), &XRRFreeCrtcInfo);
        AMF_RETURN_IF_FALSE(nullptr != crtcInfo, AMF_FAIL, L"Cannot get XRRCrtcInfo from current XRRScreenResource.")
        m_pCrtcInfo = crtcInfo;
    }

    return res;
}

#endif
