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

#include "Session.h"
#include <memory>
#include <mutex>

namespace ssdk::net
{
    class ClientSession :
        public Session
    {
    public:
        typedef amf::AMFInterfacePtr_T<ClientSession> Ptr;
        AMF_DECLARE_IID(0xf0d30490, 0x927b, 0x469f, 0x96, 0x33, 0x22, 0xf9, 0x46, 0x21, 0x61, 0x64);


        virtual  void           AMF_STD_CALL Terminate();
        virtual Result          AMF_STD_CALL WaitForIncoming();
        virtual Socket::Result  AMF_STD_CALL Send(const void* buf, size_t size, size_t* const bytesSent, int flags);
        virtual void            AMF_STD_CALL SetTimeout(time_t timeout) ;
        virtual time_t          AMF_STD_CALL GetTimeout() const { return m_Timeout; }
        virtual Socket::Ptr	    AMF_STD_CALL GetSocket() { return m_Socket; }
        virtual void            AMF_STD_CALL SetReceiveBufSize(size_t receiveBufSize);
        virtual void            AMF_STD_CALL SetTxMaxFragmentSize(size_t txMaxFragmentSize);

    protected:
        ClientSession(Socket* socket, const Socket::Address& peer, size_t receiveBufSize);
        virtual bool            AMF_STD_CALL OnTickNotify() = 0;
        virtual Result          AMF_STD_CALL ProcessIncomingMessages() = 0;

    protected:
        time_t                              m_Timeout;
		Socket::Ptr							m_Socket;
        std::unique_ptr<char[]>             m_ReceiveBuf;
        size_t                              m_ReceiveBufSize;
        size_t                              m_TxMaxFragmentSize;
        bool                                m_Terminated;
        mutable std::recursive_mutex        m_Guard;
    };
}
