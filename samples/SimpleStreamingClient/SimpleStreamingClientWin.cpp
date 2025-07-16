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

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_WARNINGS

#include "SimpleStreamingClientWin.h"

#include "amf/public/common/AMFFactory.h"
#include "amf/public/common/Thread.h"
#include "amf/public/common/TraceAdapter.h"
#include "amf/public/samples/CPPSamples/common/AudioPresenterWin.h"
#include "sdk/controllers/client/ControllerManager.h"
#include "sdk/controllers/client/win/MouseCursorWin.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>


#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "dxgi.lib")

static constexpr const wchar_t* const AMF_FACILITY = L"SimpleStreamingClientWin";

static LRESULT CALLBACK WndProc(HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;
    SimpleStreamingClientWin* instance = static_cast<SimpleStreamingClientWin*>(SimpleStreamingClientWin::GetInstance());
    switch (message)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT ps = {};
            BeginPaint(hWindow, &ps);
            EndPaint(hWindow, &ps);
        }
        break;
    case WM_CREATE:
        result = DefWindowProc(hWindow, message, wParam, lParam);
        instance->InitPresenter(hWindow);
        break;
    case WM_CLOSE:
        if (instance != nullptr)
        {
            instance->Terminate();
        }
        result = DefWindowProc(hWindow, message, wParam, lParam);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        result = DefWindowProc(hWindow, message, wParam, lParam);
        instance->ResizePresenter(LOWORD(lParam), HIWORD(lParam));
        break;
    default:
        if (instance != nullptr)
        {
            MSG msg = {};
            msg.message = message;
            msg.wParam = wParam;
            msg.lParam = lParam;
            msg.hwnd = hWindow;
            if (instance->ProcessMessage(msg) == true)
            {
                return 0;
            }
        }
        result = DefWindowProc(hWindow, message, wParam, lParam);
    }
    return result;
}

static bool RedirectIOToConsole()
{
    if (FALSE == AttachConsole(ATTACH_PARENT_PROCESS))
    {
        // If it failed to attach to parent process's console, warn the user with error code.
        AllocConsole();
    }

    FILE* fpResult = freopen("CONOUT$", "w", stdout);
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

SimpleStreamingClientWin::SimpleStreamingClientWin(HINSTANCE hInstance, int nCmdShow, const wchar_t* cmdLine) :
    m_hInstance(hInstance),
    m_nCmdShow(nCmdShow),
    m_CmdLine(cmdLine)
{
}

SimpleStreamingClientWin::~SimpleStreamingClientWin()
{
    Terminate();
}

void SimpleStreamingClientWin::RunMessageLoop()
{
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_QUIT)
        {
            break;
        }
    }
}

bool SimpleStreamingClientWin::ProcessMessage(MSG msg)
{
    bool bResult = false;

    switch (msg.message)
    {
    case WM_ACTIVATE:
        if (m_VideoPresenter != nullptr)
        {
            m_VideoPresenter->SetFullScreen(LOWORD(msg.wParam) != WA_INACTIVE);
        }
        break;

    default:
        bResult = false;
        break;
    }

    if (m_ControllerManager != nullptr)
    {
        bResult = m_ControllerManager->ProcessMessage(msg);
    }
    return bResult;
}

void SimpleStreamingClientWin::InitPresenter(HWND hWnd)
{
    AMF_RESULT result = VideoPresenter::Create(m_VideoPresenter, m_MemoryType, hWnd, m_Context, 0);
    if (m_VideoPresenter != nullptr)
    {
        m_VideoPresenter->SetFullScreen(true);
        m_VideoPresenter->SetExclusiveFullscreen(true);
        m_VideoPresenter->SetOrientation(0);
        m_VideoPresenter->SetWaitForVSync(true);
        m_VideoPresenter->DoActualWait(false);
        m_hWnd = hWnd;
    }
    else
    {
        AMFTraceError(AMF_FACILITY, L"Failed to create video presenter, result=%s", amf::AMFGetResultText(result));
    }
    m_AudioPresenter = AudioPresenterPtr(new AudioPresenterWin);
    if (m_AudioPresenter != nullptr)
    {
        m_AudioPresenter->SetLowLatency(true);
        m_AudioPresenter->DoActualWait(false);
    }
    m_AVSynchronizer = ssdk::util::AVSynchronizer::Ptr(new ssdk::util::AVSynchronizer(m_VideoPresenter, m_AudioPresenter));
}

void SimpleStreamingClientWin::TerminatePresenter()
{
    m_AVSynchronizer = nullptr;
    if (m_AudioPresenter != nullptr)
    {
        std::static_pointer_cast<AudioPresenterWin>(m_AudioPresenter)->Terminate(); //  AMF AudioPresenter does not define a virtual Terminate() method in the base class
        m_AudioPresenter = nullptr;
    }
    if (m_VideoPresenter != nullptr)
    {
        m_VideoPresenter->Terminate();
    }
}

bool SimpleStreamingClientWin::InitControllers()
{
    bool result = SimpleStreamingClient::InitControllers();
    if (result == false || m_ControllerManager == nullptr)
    {
        AMFTraceError(AMF_FACILITY, L"Failed to initialize controller manager.");
        return false;
    }

    // Create Controllers
    bool useRawDataInput = true;
    GetParam(PARAM_NAME_RELATIVE_MOUSE_CAPTURE, useRawDataInput);

    m_ControllerManager->AddController(ssdk::ctls::ControllerBase::Ptr(new ssdk::ctls::MouseController(m_ControllerManager, m_VideoPresenter, m_VideoPipeline, m_hWnd, useRawDataInput)));
    m_ControllerManager->AddController(ssdk::ctls::ControllerBase::Ptr(new ssdk::ctls::KeyboardController(m_ControllerManager)));

    for (DWORD i = 0; i < XUSER_MAX_COUNT; i++)
    {
        m_ControllerManager->AddController(ssdk::ctls::ControllerBase::Ptr(new ssdk::ctls::GameController(m_ControllerManager, i)));
    }

    return result;
}

bool SimpleStreamingClientWin::InitCursor()
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
    m_ControllerManager->SetCursor(ssdk::ctls::MouseCursor::Ptr(new ssdk::ctls::MouseCursorWin(bShowCursor, true, m_hWnd)));

    return result;
}

bool SimpleStreamingClientWin::OnAppInit()
{
    bool result = SimpleStreamingClient::OnAppInit();

    static constexpr LPCWSTR szWindowClass = L"AMD_SIMPLE_STREAMING_CLIENT";
    static constexpr LPCWSTR szTitle = L"Simple Streaming Client";

    result = RedirectIOToConsole();
    if (result == true)
    {
        AMF_RESULT amfRes = AMF_OK;
        if (m_Context != nullptr)
        {
            if ((amfRes = m_Context->InitDX11(nullptr)) != AMF_OK)
            {
                AMFTraceError(AMF_FACILITY, L"Failed to initialize AMFContext for DX11. Check AMD graphics driver installation or, if running on a non-AMD platform, ensure that amfrt64.dll and amfrtlte64.dll are next to the executable or in the PATH");
                result = false;
            }
            else
            {
                m_MemoryType = amf::AMF_MEMORY_TYPE::AMF_MEMORY_DX11;
                m_DX11Initialized = true;

                WNDCLASSEXW wcex = {};

                wcex.cbSize = sizeof(WNDCLASSEX);

                wcex.style = CS_HREDRAW | CS_VREDRAW;
                wcex.lpfnWndProc = WndProc;
                wcex.cbClsExtra = 0;
                wcex.cbWndExtra = 0;
                wcex.hInstance = m_hInstance;
                wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
                wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
                wcex.lpszClassName = szWindowClass;

                RegisterClassExW(&wcex);

                HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_POPUP, //WS_OVERLAPPEDWINDOW,
                    0, 0, 1920, 1080, nullptr, nullptr, m_hInstance, nullptr);
                if (hWnd == NULL)
                {
                    AMFTraceError(AMF_FACILITY, L"Failed to create a window");
                    result = false;
                }
                else
                {
                    m_hWnd = hWnd;

                    ShowWindow(hWnd, m_nCmdShow);
                    UpdateWindow(hWnd);
                    ImmAssociateContext(hWnd, NULL);
                    result = SimpleStreamingClient::OnAppInit();
                    if (result == true)
                    {
                        AMFTraceDebug(AMF_FACILITY, L"OnAppInit(): success");
                    }
                    else
                    {
                        AMFTraceError(AMF_FACILITY, L"OnAppInit(): failure - examine the log above this point for more details");
                    }
                }
            }
        }
    }
    return result;
}

void SimpleStreamingClientWin::OnAppTerminate()
{
    SimpleStreamingClient::OnAppTerminate();
    TerminatePresenter();
    m_VideoPresenter = nullptr;
    m_hWnd = NULL;
    if (m_DX11Initialized == true)
    {
        if (m_Context != nullptr)
        {
            m_Context->Terminate();
            AMFTraceDebug(AMF_FACILITY, L"OnAppTerminate(): AMFContext terminated");
        }
        AMFTraceDebug(AMF_FACILITY, L"OnAppTerminate(): DX11 device terminated");
        m_DX11Initialized = false;
        m_MemoryType = amf::AMF_MEMORY_TYPE::AMF_MEMORY_UNKNOWN;
    }
}

std::string SimpleStreamingClientWin::GenerateGUID() const
{
    GUID guid;
    CoCreateGuid(&guid);
    wchar_t guidWSTR[1000];
    StringFromGUID2(guid, (LPOLESTR)guidWSTR, amf_countof(guidWSTR));

    return std::string(amf::amf_from_unicode_to_utf8(guidWSTR));
}

#endif // _WIN32

