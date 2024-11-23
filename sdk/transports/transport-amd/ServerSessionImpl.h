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

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif


#include "TransportServer.h"
#include "FlowCtrlProtocol.h"
#include "TransportServerImpl.h"
#include "messages/service/Connect.h"

#include "net/DatagramServer.h"
#include "net/DatagramServerSession.h"
#include "net/StreamServer.h"
#include "net/StreamServerSession.h"

#include "amf/public/include/core/PropertyStorage.h"

#include <sstream>
#include <iomanip>


namespace ssdk::transport_amd
{
    class ServerSessionImpl :
        public Session
    {
    public:
        // {77516649-7CA7-4ADE-8F00-8D9E9BC1A48D}
        AMF_DECLARE_IID(0x77516649, 0x7ca7, 0x4ade, 0x8f, 0x0, 0x8d, 0x9e, 0x9b, 0xc1, 0xa4, 0x8d);

        ServerSessionImpl(ServerImpl* server);
        virtual ~ServerSessionImpl();

        virtual void AMF_STD_CALL RegisterReceiverCallback(ReceiverCallback* callback) override;
        virtual const char* AMF_STD_CALL GetPeerUrl() const noexcept override;

        virtual const char* AMF_STD_CALL GetPeerPlatform() const noexcept override;
        inline void SetPeerAddress(const std::string& address) { m_PeerAddress = address; }

        virtual ssdk::transport_common::SessionHandle AMF_STD_CALL GetSessionHandle() const noexcept override { return m_SessionHandle; };
    protected:
        void TimeoutNotify();
        void TerminateNotify();
        void DisconnectNotify(ReceiverCallback::TerminationReason reason);

        static void PopulateOptionsFromHello(const HelloRequest& request, amf::AMFPropertyStorage* propStorage);

    protected:
        ServerImpl::Ptr m_Server;
        ReceiverCallback* m_Callback = nullptr;
        std::string m_PeerAddress;
        std::string m_PeerPlatform;
        static ssdk::transport_common::SessionHandle m_UniqID;
        ssdk::transport_common::SessionHandle m_SessionHandle{ 0 };
    };
}
