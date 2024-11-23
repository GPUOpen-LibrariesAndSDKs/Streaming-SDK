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

#include "Socket.h"
#include "amf/public/common/InterfaceImpl.h"

#include <memory>

namespace ssdk::net
{
    class SessionManager;
    class Session : public amf::AMFInterface
    {
        friend class SessionManager;
        friend class DatagramServer;
		friend class StreamServer;
    public:
        typedef amf::AMFInterfacePtr_T<Session> Ptr;

        AMF_DECLARE_IID(0xcd5a2bb2, 0xdca8, 0x47e8, 0xbe, 0x7, 0x2, 0xd2, 0x16, 0x39, 0xfd, 0x95);

		enum class Result
        {
            OK,
            NOT_IMPLEMENTED,
            INVALID_ARG,
            ALREADY_REGISTERED,
            NOT_REGISTERED,
            MANAGER_DESTROYED,
            SEND_FAILED,
            SEND_RQ_ALREADY_QUEUED,
            TIMEOUT,
            RECEIVE_FAILED,
            INVALID_PEER,
            REFUSED,
            INVALID_VERSION,
			CONNECTION_TERMINATED,
            TICK
        };

        virtual ~Session();

        // interface
        virtual  void             AMF_STD_CALL Terminate();

        const Socket::Address&      AMF_STD_CALL GetPeerAddress() const { return m_Peer; }
        time_t                      AMF_STD_CALL GetLastRequestTime() const  { return m_LastReceivedTime; }
        time_t               AMF_STD_CALL GetElapsedTimeSinceLastRequest();
        bool                        AMF_STD_CALL IsTerminated() const { return m_bTerminated; }
        void                        AMF_STD_CALL Touch();


    protected:
        Session(const Socket::Address& peer);

    protected:
        //  Overridables:
        virtual Result              AMF_STD_CALL OnInit() = 0;
        virtual Result              AMF_STD_CALL OnSessionTimeout() = 0;
		virtual Result				AMF_STD_CALL OnSessionClose() = 0;

    private:
        Session(const Session&) = delete;
        Session& operator=(const Session&) = delete;


    protected:
        Socket::Address m_Peer;
        time_t          m_LastReceivedTime;
        bool            m_bTerminated;
    };
}