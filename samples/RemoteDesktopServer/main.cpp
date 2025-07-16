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
#include "RemoteDesktopServerWin.h"

int WINAPI wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPWSTR /*lpCmdLine*/, int /*nCmdShow*/)
#else
#include "RemoteDesktopServerLinux.h"

int main(int argc, const char** argv)
#endif
{
    int result = -1;
    RemoteDesktopServer::Ptr    pApp;
#if defined(_WIN32)
    pApp = RemoteDesktopServer::Ptr(new RemoteDesktopServerWin);
    int argc = 0; const char** argv = nullptr;
#else
    pApp = RemoteDesktopServer::Ptr(new RemoteDesktopServerLinux);
#endif

    if (pApp != nullptr)
    {
        RemoteDesktopServer::EStatus initStatus = pApp->Init(argc, argv);
        switch (initStatus)
        {
        case RemoteDesktopServer::EStatus::OK:
            pApp->RunServer();
            result = 0;
            break;
        case RemoteDesktopServer::EStatus::SKIP:
            pApp->ShutdownRunningServer();
            result = 0;
            break;
        default:
            break;
        }
        if (result == 0)
        {
            pApp->Terminate();
        }
    }
    return result;
}
