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

#ifndef _WIN32

#include "SimpleStreamingClient.h"
#include "SimpleStreamingClientLinux.h"
#include "sdk/controllers/client/ControllerManager.h"
#include "sdk/controllers/client/lnx/MouseCursorLnx.h"
#include "sdk/controllers/client/lnx/X11TouchController.h"

#include <X11/Xlib.h>

static constexpr const wchar_t* const AMF_FACILITY = L"SimpleStreamingClientLinux";
static constexpr const uint8_t GAME_USER_MAX_COUNT = 4;

SimpleStreamingClientLinux::SimpleStreamingClientLinux(const char** cmdLine)
{
    // Todo
    XInitThreads();
    Window hMainWindow; // needs to be created
    m_hMainWindow = (amf_handle)hMainWindow;
    m_hDisplayInst = amf_handle(XOpenDisplay(NULL));

    InitControllers();
    // m_ControllerManager->ProcessMessage(msg) needs to be called for Linux
}

bool SimpleStreamingClientLinux::Init()
{
    return false;
}

void SimpleStreamingClientLinux::RunMessageLoop()
{
}

void SimpleStreamingClientLinux::OnFrameUpdated()
{
}

bool SimpleStreamingClientLinux::InitControllers()
{
    bool result = SimpleStreamingClient::InitControllers();

    // Create Controllers
    bool useRawDataInput = true;
    GetParam(PARAM_NAME_RELATIVE_MOUSE_CAPTURE, useRawDataInput);

    m_ControllerManager->AddController(ssdk::ctls::ControllerBase::Ptr(new ssdk::ctls::MouseController(m_ControllerManager, m_VideoPresenter, m_VideoPipeline, (Display*)m_hDisplayInst, m_hMainWindow, useRawDataInput)));
    m_ControllerManager->AddController(ssdk::ctls::ControllerBase::Ptr(new ssdk::ctls::KeyboardController(m_ControllerManager)));
    CreateGameControllers();
    m_ControllerManager->AddController(ssdk::ctls::ControllerBase::Ptr(new ssdk::ctls::X11TouchController(m_ControllerManager, (Display*)m_hDisplayInst, (Window)m_hMainWindow)));

    return result;
}

bool SimpleStreamingClientLinux::InitCursor()
{
    bool result = SimpleStreamingClient::InitCursor();
    if (result == false || m_ControllerManager == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"Failed to initialize cursor.");
        return false;
    }

    // Create Mouse Cursor
    bool bShowCursor = true;
    GetParam(PARAM_NAME_SHOW_CURSOR, bShowCursor);
    m_ControllerManager->SetCursor(ssdk::ctls::MouseCursor::Ptr(new ssdk::ctls::MouseCursorLnx(bShowCursor, true, (Display*)m_hDisplayInst, m_hMainWindow)));

    return result;
}

void SimpleStreamingClientLinux::CreateGameControllers()
{
    libevdev* dev = nullptr;
    for (uint8_t i = 0; i < GAME_USER_MAX_COUNT; i++)
    {
        m_ControllerManager->AddController(ssdk::ctls::ControllerBase::Ptr(new ssdk::ctls::GameController(m_ControllerManager, dev, i, ssdk::ctls::GameController::BLUETOOTH)));
    }
    /*std::string eventFile = "/dev/input";
    std::string eventStr("event");
    DIR* deviceDir = opendir(eventFile.c_str());
    std::vector<std::string> fileNames;
    if (deviceDir && m_ControllerManager != nullptr)
    {
        struct dirent* entryDir;
        while ((entryDir = readdir(deviceDir)) != nullptr)
        {
            const std::string fileName(entryDir->d_name);
            if (fileName.find(eventStr) == 0)
            {
                std::string deviceFile = eventFile + "/" + fileName;
                struct libevdev* dev = nullptr;

                int fileDescriptor = open(deviceFile.c_str(), O_RDWR | O_NONBLOCK);
                if (fileDescriptor < 0) {
                    continue;
                }

                int returnCode = libevdev_new_from_fd(fileDescriptor, &dev);
                if (returnCode < 0) {
                    close(fileDescriptor);
                    continue;
                }

                if (libevdev_get_id_vendor(dev) != 0x045e)
                {
                    close(fileDescriptor);
                    continue;
                }

                switch (libevdev_get_id_product(dev))
                {
                case 0x0202:    //XBOX controllers product ID (360, One, S, e.t.c.)
                case 0x0285:
                case 0x0287:
                case 0x0288:
                case 0x0289:
                case 0x028e:
                case 0x02a0:
                case 0x02a1:
                case 0x02d1:
                case 0x02dd:
                case 0x02e0:
                case 0x02e3:
                case 0x02ea:
                case 0x02fd:
                case 0x02ff:
                case 0x0719:
                case 0x0b12:
                case 0x0b13:
                    break;
                default:
                    close(fileDescriptor);
                    continue;
                }

                std::string tmpStr = fileName.substr(eventStr.length(), fileName.length());
                bool isDigit = !tmpStr.empty() && std::all_of(tmpStr.begin(), tmpStr.end(), ::isdigit);
                int eventIndex = isDigit ? std::stoi(tmpStr) : 0;

                const char* phys = libevdev_get_phys(dev);
                ssdk::ctls::GameController::CONNECTION_TYPE connectionType = ssdk::ctls::GameController::CONNECTION_TYPE::BLUETOOTH;
                if (phys && strstr(phys, "usb") != nullptr)
                {
                    connectionType = ssdk::ctls::GameController::CONNECTION_TYPE::USB;
                }

                m_ControllerManager->AddController(new ssdk::ctls::GameController(m_ControllerManager, dev, eventIndex, connectionType));
                dev = nullptr;
            }
        }
        closedir(deviceDir);
    }*/
}

std::string SimpleStreamingClientLinux::GenerateGUID() const
{
    std::ifstream t("/proc/sys/kernel/random/uuid");
    std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
    return str;
}

#endif
