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
#pragma once

#include "Initializer.h"
#include "Url.h"
#include "SessionManager.h"
#include "Session.h"
#include <memory>
#include <set>

namespace ssdk::net
{
    class Server :
        public SessionManager,
        protected Initializer
    {
    public:
        typedef std::shared_ptr<Server>	Ptr;
    protected:
        Server() = default;

    public:
        virtual ~Server() = default;

        virtual SessionManager::Result RunServer();
        virtual SessionManager::Result ShutdownServer();

        inline bool IsServerRunning() const noexcept { return m_Running; }

    protected:
        virtual SessionManager::Result AcceptConnections() = 0;
        virtual Session::Ptr AMF_STD_CALL OnCreateSession(const Socket::Address& peer, Socket* socket, uint8_t* buf, size_t bufSize) = 0;   //  Create a session on the connection returned by OnAcceptConnection()

    private:
        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;

    protected:
//		mutable amf::AMFCriticalSection	m_Guard;

        volatile bool   m_Terminate = false;
    private:
        volatile bool   m_Running = false;
    };
}
