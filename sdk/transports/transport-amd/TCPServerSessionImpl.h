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

#include "ServerSessionImpl.h"
#include "TransportSession.h"

#include "amf/public/common/PropertyStorageImpl.h"
#include "amf/public/common/InterfaceImpl.h"

namespace ssdk::transport_amd
{
    class TCPServerSessionImpl :
        public amf::AMFInterfaceBase,
        public amf::AMFPropertyStorageImpl<amf::AMFPropertyStorage>,
        public ServerSessionImpl,
        public net::StreamServerSession
    {
    public:
        TCPServerSessionImpl(ServerImpl* server, net::StreamSocket* sock, const net::Socket::Address& peer);
        virtual ~TCPServerSessionImpl();

        // {3E370444-EFBA-4FA5-BE6C-743E71FBE00E}
        AMF_DECLARE_IID(0x3e370444, 0xefba, 0x4fa5, 0xbe, 0x6c, 0x74, 0x3e, 0x71, 0xfb, 0xe0, 0xe);


        AMF_BEGIN_INTERFACE_MAP
            AMF_INTERFACE_MULTI_ENTRY(transport_amd::Session)
            AMF_INTERFACE_MULTI_ENTRY(net::StreamServerSession)
            AMF_INTERFACE_MULTI_ENTRY(net::Session)
            AMF_INTERFACE_CHAIN_ENTRY(amf::AMFPropertyStorageImpl<amf::AMFPropertyStorage>)
        AMF_END_INTERFACE_MAP


        virtual transport_common::Result     AMF_STD_CALL Send(Channel channel, const void* msg, size_t msgLen) override;
        virtual void AMF_STD_CALL Terminate() override;
        virtual bool            AMF_STD_CALL IsTerminated() const noexcept override { return net::Session::IsTerminated(); }
        virtual void            AMF_STD_CALL UpgradeProtocol(uint32_t/*version*/) override {}

    protected:
        // net::Session interface
        virtual net::Session::Result AMF_STD_CALL OnInit() override;
        virtual net::Session::Result AMF_STD_CALL OnDataAvailable() override;
        virtual net::Session::Result AMF_STD_CALL OnSessionTimeout() override;
        virtual net::Session::Result AMF_STD_CALL OnSessionClose() override;

    private:
        mutable amf::AMFCriticalSection m_SendCS;
        net::Socket::Address	        m_Peer;
        StreamFlowCtrlProtocol	        m_FlowCtrl;
    };

}