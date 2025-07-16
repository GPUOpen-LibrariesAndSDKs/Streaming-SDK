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
#include "amf/public/samples/CPPSamples/common/AudioPresenterLinux.h"
#include "amf/public/include/core/VulkanAMF.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <fstream>
#include <set>
#include <fcntl.h>
#include <dirent.h>

static constexpr const wchar_t* const AMF_FACILITY = L"SimpleStreamingClientLinux";
static constexpr const uint8_t GAME_USER_MAX_COUNT = 4;

SimpleStreamingClientLinux::SimpleStreamingClientLinux(const char** cmdLine)
{
    XInitThreads();
    m_hDisplayInst = amf_handle(XOpenDisplay(NULL));
}

SimpleStreamingClientLinux::~SimpleStreamingClientLinux()
{
    ReleaseKeyModifiers();
    Terminate();
}

bool SimpleStreamingClientLinux::OnAppInit()
{
    bool result = SimpleStreamingClient::OnAppInit();

    if (result == true)
    {
#ifdef AMF_CONTEXT_VULKAN_USE_TIMELINE_SEMAPHORES
        m_Context->SetProperty(AMF_CONTEXT_VULKAN_USE_TIMELINE_SEMAPHORES, true);   //  Workaround for the bug in the NVidia Linux driver that causes jitter in the decoder
#endif
        Display* dpy = (Display*)m_hDisplayInst;
        int screen_num = DefaultScreen(dpy);

        Window parentWnd = DefaultRootWindow(dpy);
        XWindowAttributes getWinAttr;
        XGetWindowAttributes(dpy, parentWnd, &getWinAttr);

        Window hMainWindow = XCreateSimpleWindow(dpy,
            parentWnd,
            // 10, 10, 800, 600,
            0, 0, getWinAttr.width, getWinAttr.height,
            1, BlackPixel(dpy, screen_num), WhitePixel(dpy, screen_num));
        XSelectInput(dpy, hMainWindow, ExposureMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | FocusChangeMask);
        m_hMainWindow = (amf_handle)hMainWindow;

        // For full screen
        {
            Atom wm_state = XInternAtom(dpy, "_NET_WM_STATE", true );
            Atom wm_fullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", true );

            XChangeProperty(dpy, hMainWindow, wm_state, XA_ATOM, 32, PropModeReplace, (unsigned char *)&wm_fullscreen, 1);
        }

        /* check XInput extension */
        {
            int ev{};
            int err{};
            if (!XQueryExtension(dpy, "XInputExtension", &m_xiOpcode, &ev, &err)) {
                AMFTraceError(AMF_FACILITY, L"X Input extension not available.");
                return false;
            }
        }
        /* check the version of XInput */
        {
            int rc{};
            int major = 2;
            int minor = 3;
            rc = XIQueryVersion(dpy, &major, &minor);
            if (rc != Success)
            {
                AMFTraceError(AMF_FACILITY, L"No XI2 support. (%d.%d only)", major, minor);
                return false;
            }
        }
        /* select device */
        int touchDeviceId = XIAllDevices;
        if (m_bOnlyTouchDevice) // When touch device detected and selected mouse move will be ignored
        {
            XIDeviceInfo* di{};
            auto selectTouchscreenDevice = [&dpy, &touchDeviceId, &di]()
            {
                int cnt{};
                di = XIQueryDevice(dpy, XIAllDevices, &cnt);
                for (int i = 0; i < cnt; i ++)
                {
                    XIDeviceInfo* dev = &di[i];
                    for (int j = 0; j < dev->num_classes; j ++)
                    {
                        XITouchClassInfo* classInfo = reinterpret_cast<XITouchClassInfo*>(dev->classes[j]);
                        // We don't need virtual device, we need physical device
                        // xintput list:
                        //      - Virtual device - dev->name == "Virtual core pointer"  | XIMasterPointer
                        //      - Physical device - dev->name == "ELAN Touchscreen"     | XISlavePointer
                        // When touch device detected and selected mouse events will be ignored
                        if (classInfo->type == XITouchClass && dev->use == XISlavePointer)
                        {
                            touchDeviceId = dev->deviceid;
                            return;
                        }
                    }
                }
            };
            selectTouchscreenDevice();
            XIFreeDeviceInfo(di);
        }

        unsigned char mask_mask[4] = { 0 };
        XIEventMask mask{ touchDeviceId, XIMaskLen(XI_TouchEnd) };
        mask.mask = mask_mask;
        XISetMask(mask.mask, XI_TouchBegin);
        XISetMask(mask.mask, XI_TouchUpdate);
        XISetMask(mask.mask, XI_TouchEnd);
        XISetMask(mask.mask, XI_RawMotion);
        XISelectEvents(dpy, parentWnd, &mask, 1);

        XMapWindow(dpy, hMainWindow);
        XStoreName(dpy, hMainWindow, "SimpleStreamingClient");
    
        m_wmDeleteWindow = XInternAtom(dpy, "WM_DELETE_WINDOW", False); 
        XSetWMProtocols(dpy, hMainWindow, &m_wmDeleteWindow, 1); 
        XkbSetDetectableAutoRepeat(dpy, True, NULL); // remove autorepeat for holding keyboard keys
        XGrabPointer(dpy, hMainWindow, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask | FocusChangeMask, GrabModeAsync, GrabModeAsync, hMainWindow, None, CurrentTime);

        InitPresenter();
        ResizePresenter(getWinAttr.width, getWinAttr.height);
    }

    return result;
}

void SimpleStreamingClientLinux::OnAppTerminate()
{
    SimpleStreamingClient::OnAppTerminate();

    TerminatePresenter();
    TerminateVulkan();
    m_hMainWindow = 0;
    m_MemoryType = amf::AMF_MEMORY_TYPE::AMF_MEMORY_UNKNOWN;
}

void SimpleStreamingClientLinux::RunMessageLoop()
{
    Display* dpy = (Display*)m_hDisplayInst;
    XEvent e;
    XGenericEventCookie* cookie{&e.xcookie};
    bool bRun = true;
    bool focusIn = true;
    bool cursorGrabbed = true;
    uint8_t maxGrabAttempts = 5;

    while (bRun) 
    {
        XLockDisplay(dpy);
        while (XPending(dpy) != 0)
        {
            XNextEvent(dpy, &e);
            switch(e.type)
            {
                case Expose:
                    break;
                case KeyPress:
                case KeyRelease:
                case ButtonPress:
                case ButtonRelease:
                case MotionNotify:
                    if (focusIn)
                    {
                        ProcessMessage(e);
                    }
                    break;
                case GenericEvent:
                    if (focusIn)
                    {
                        if (XGetEventData(dpy, cookie))
                        {
                            if (cookie->extension == m_xiOpcode)
                            {
                                switch (cookie->evtype)
                                {
                                case XI_RawMotion:
                                    ProcessMessage(e);
                                    break;
                                case XI_TouchBegin:
                                case XI_RawTouchBegin:
                                case XI_TouchUpdate:
                                case XI_RawTouchUpdate:
                                case XI_TouchEnd:
                                case XI_RawTouchEnd:
                                    XIDeviceEvent* devEv = static_cast<XIDeviceEvent*>(cookie->data);
                                    ProcessMessage(*devEv);
                                    break;
                                }
                            }
                        }
                    }
                    break;
                case ConfigureNotify:
                    {
                        XConfigureEvent xce = e.xconfigure;
                        CheckForResize();
                    }
                    break;
                case ClientMessage:
                    if((static_cast<unsigned int>(e.xclient.data.l[0]) == m_wmDeleteWindow))
                    {
                        bRun = false;
                    }
                    break;
                case FocusIn:
                {
                    if(!cursorGrabbed)
                    {
                        for (uint8_t trial = 0; trial < maxGrabAttempts; ++trial)
                        {
                            int grabResult = XGrabPointer(dpy, (Window)m_hMainWindow, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, (Window)m_hMainWindow, None, CurrentTime);
                            if (grabResult == GrabSuccess)
                            {
                                cursorGrabbed = true;
                                break;
                            }
                            amf_sleep(50);
                        }
                    }
                    XSync(dpy, False);
                    focusIn = true;
                    ReleaseKeyModifiers();
                    break;
                }
                case FocusOut:
                    if(cursorGrabbed)
                    {
                        XUngrabPointer(dpy, CurrentTime);
                        cursorGrabbed = false;
                    }
                    XSync(dpy, False);
                    focusIn = false;
                    ReleaseKeyModifiers();
                    break;
            }
            XFreeEventData(dpy, cookie);
        }
        XUngrabPointer(dpy, CurrentTime);
        XUnlockDisplay(dpy);
    }
}

void SimpleStreamingClientLinux::OnFrameUpdated()
{
}

bool SimpleStreamingClientLinux::ProcessMessage(WindowMessage msg)
{
    bool bResult = false;

    if (m_ControllerManager != nullptr)
    {
        bResult = m_ControllerManager->ProcessMessage(msg);
    }

    return bResult;
}

bool SimpleStreamingClientLinux::ProcessMessage(ExtWindowMessage msg)
{
    bool bResult = false;

    if (m_ControllerManager != nullptr)
    {
        bResult = m_ControllerManager->ProcessMessage(msg);
    }

    return bResult;
}

void SimpleStreamingClientLinux::InitPresenter()
{
    if (m_hMainWindow != 0 && m_Context != nullptr)
    {
        AMF_RESULT amfResult = AMF_FAIL;
        {
            amf::AMFContext1Ptr context(m_Context);
            if (context == nullptr)
            {
                AMFTraceError(AMF_FACILITY, L"Failed to query the AMFContext1 interface, seems that you have an outdated AMD driver");
            }
            else
            {
                if ((amfResult = context->InitVulkan(nullptr)) != AMF_OK)
                {
                    AMFTraceError(AMF_FACILITY, L"Failed to initialize AMFContext for Vulkan, result=%s", amf::AMFGetResultText(amfResult));
                }
                else
                {
                    m_MemoryType = amf::AMF_MEMORY_TYPE::AMF_MEMORY_VULKAN;

                    AMF_RESULT result = VideoPresenter::Create(m_VideoPresenter, m_MemoryType, m_hMainWindow, m_Context, m_hDisplayInst);
                    if (m_VideoPresenter != nullptr)
                    {
                        m_VideoPresenter->SetFullScreen(true);
                        m_VideoPresenter->SetExclusiveFullscreen(true);
                        m_VideoPresenter->SetOrientation(0);
                        m_VideoPresenter->SetWaitForVSync(true);
                        m_VideoPresenter->DoActualWait(false);
                    }
                    else
                    {
                        AMFTraceError(AMF_FACILITY, L"Failed to create video presenter, result=%s", amf::AMFGetResultText(result));
                    }

                    m_AudioPresenter = AudioPresenterPtr(new AudioPresenterLinux);
                    if (m_AudioPresenter != nullptr)
                    {
                        m_AudioPresenter->SetLowLatency(true);
                        m_AudioPresenter->DoActualWait(false);
                    }
                    m_AVSynchronizer = ssdk::util::AVSynchronizer::Ptr(new ssdk::util::AVSynchronizer(m_VideoPresenter, m_AudioPresenter));
                }
            }
        }
    }
}

void SimpleStreamingClientLinux::TerminatePresenter()
{
    m_AVSynchronizer = nullptr;
    if (m_AudioPresenter != nullptr)
    {
        std::static_pointer_cast<AudioPresenterLinux>(m_AudioPresenter)->Terminate(); //  AMF AudioPresenter does not define a virtual Terminate() method in the base class
        m_AudioPresenter = nullptr;
    }
    if (m_VideoPresenter != nullptr)
    {
        m_VideoPresenter->Terminate();
        m_VideoPresenter = nullptr;
    }
}

void SimpleStreamingClientLinux::TerminateVulkan()
{
//    m_DeviceVulkan.Terminate();
}

void SimpleStreamingClientLinux::CheckForResize()
{
    if (m_VideoPresenter != nullptr)
    {
        m_VideoPresenter->ResizeIfNeeded();
    }
}

void SimpleStreamingClientLinux::ReleaseKeyModifiers()
{
    if (m_ControllerManager != nullptr)
    {
        m_ControllerManager->ReleaseModifiers();
    }
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
    m_ControllerManager->AddController(ssdk::ctls::ControllerBase::Ptr(new ssdk::ctls::X11TouchController(m_ControllerManager, (Display*)m_hDisplayInst, (Window)m_hMainWindow, m_VideoPipeline)));

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
    static const std::set<int> prodIDs = {  //XBOX controllers product ID (360, One, S, e.t.c.)
        0x0202, 0x0285, 0x0287, 0x0288, 0x0289, 0x028e, 0x02a0, 0x02a1, 0x02d1, 0x02dd, 0x02e0, 0x02e3, 0x02ea, 0x02fd, 0x02ff, 0x0719, 0x0b12, 0x0b13
    };
    static const std::string eventFile = "/dev/input";
    static const std::string eventStr("event");

    DIR* deviceDir = opendir(eventFile.c_str());
    std::vector<std::string> fileNames;

    if (deviceDir && m_ControllerManager != nullptr)
    {
        struct dirent* entryDir;
        uint32_t indexCtrl = 0;
        while ((entryDir = readdir(deviceDir)) != nullptr)
        {
            const std::string fileName(entryDir->d_name);
            if (fileName.find(eventStr) == 0)
            {
                std::string deviceFile = eventFile + "/" + fileName;
                struct libevdev* dev = nullptr;

                int fileDescriptor = open(deviceFile.c_str(), O_RDWR | O_NONBLOCK);
                if (fileDescriptor != -1) 
                {
                    int returnCode = libevdev_new_from_fd(fileDescriptor, &dev);
                    if (returnCode < 0 || libevdev_get_id_vendor(dev) != 0x045e || prodIDs.contains(libevdev_get_id_product(dev)) == false)
                    {
                        close(fileDescriptor);
                    }
                    else
                    {
                        std::string tmpStr = fileName.substr(eventStr.length(), fileName.length());
                        bool isDigit = !tmpStr.empty() && std::all_of(tmpStr.begin(), tmpStr.end(), ::isdigit);
                        int eventIndex = isDigit ? std::stoi(tmpStr) : 0;

                        const char* phys = libevdev_get_phys(dev);
                        ssdk::ctls::GameController::CONNECTION_TYPE connectionType = ssdk::ctls::GameController::CONNECTION_TYPE::BLUETOOTH;
                        if (phys != nullptr && strstr(phys, "usb") != nullptr)
                        {
                            connectionType = ssdk::ctls::GameController::CONNECTION_TYPE::USB;
                        }

                        m_ControllerManager->AddController(ssdk::ctls::ControllerBase::Ptr(new ssdk::ctls::GameController(m_ControllerManager, dev, indexCtrl++, connectionType)));
                    }
                }
            }
        }
        closedir(deviceDir);
    }
}

std::string SimpleStreamingClientLinux::GenerateGUID() const
{
    std::ifstream t("/proc/sys/kernel/random/uuid");
    std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
    return str;
}

#endif
