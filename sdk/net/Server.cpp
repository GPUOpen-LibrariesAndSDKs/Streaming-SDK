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
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "Server.h"
#include "public/common/TraceAdapter.h"
#include <vector>
#include <sstream>

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::net::Server";

namespace ssdk::net
{
    SessionManager::Result Server::RunServer()
    {
        SessionManager::Result result = SessionManager::Result::OK;
        if (m_Running == false)
        {
            {
                m_Terminate = false;
                m_Running = true;
            }
			AMFTraceInfo(AMF_FACILITY, L"Starting server...");
            result = AcceptConnections();   //  This blocks until the server is stopped
			void TerminateSessions();
			AMFTraceInfo(AMF_FACILITY, L"Server shutdown");
            {
                m_Running = false;
            }
        }
        else
        {
            result = SessionManager::Result::ALREADY_RUNNING;
			AMFTraceError(AMF_FACILITY, L"Server already running");
        }
        return result;
    }

    SessionManager::Result Server::ShutdownServer()
    {
        SessionManager::Result result = SessionManager::Result::OK;
        if (m_Running != true)
        {
            result = SessionManager::Result::NOT_RUNNING;
			AMFTraceError(AMF_FACILITY, L"Server not running, cannot shutdown");
        }
        else
        {
            m_Terminate = true;
			AMFTraceInfo(AMF_FACILITY, L"Server shutdown requested");
        }
        return result;
    }
}
