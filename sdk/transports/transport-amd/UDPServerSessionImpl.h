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
    class UDPServerSessionImpl :
        public amf::AMFInterfaceBase,
        public ServerSessionImpl,
        public amf::AMFPropertyStorageImpl<amf::AMFPropertyStorage>,
        public net::DatagramServerSession,
        protected FlowCtrlProtocol::ProcessIncomingCallback,
        protected FlowCtrlProtocol::ProcessOutgoingCallback
    {
    public:
        // {86668061-FC7F-4CE2-AFE8-BCA0976718F7}
        AMF_DECLARE_IID(0x86668061, 0xfc7f, 0x4ce2, 0xaf, 0xe8, 0xbc, 0xa0, 0x97, 0x67, 0x18, 0xf7);
        AMF_BEGIN_INTERFACE_MAP
            AMF_INTERFACE_MULTI_ENTRY(transport_amd::Session)
            AMF_INTERFACE_MULTI_ENTRY(net::DatagramServerSession)
            AMF_INTERFACE_MULTI_ENTRY(net::Session)
            AMF_INTERFACE_CHAIN_ENTRY(amf::AMFPropertyStorageImpl<amf::AMFPropertyStorage>)
        AMF_END_INTERFACE_MAP

        UDPServerSessionImpl(ServerImpl* server, net::DatagramSocket* sock, const net::Socket::Address& peer, uint32_t maxFragmentSize);
        virtual ~UDPServerSessionImpl();

        // AWVRSession interface
        virtual transport_common::Result AMF_STD_CALL Send(Channel channel, const void* msg, size_t msgLen) override;
        virtual void                 AMF_STD_CALL Terminate() override;
        virtual void                 AMF_STD_CALL UpgradeProtocol(uint32_t version) override;
        virtual bool                 AMF_STD_CALL IsTerminated() const noexcept override;
    protected:
        // net::DatagramServerSession interface
        virtual net::Session::Result AMF_STD_CALL OnInit() override;
        virtual bool                 AMF_STD_CALL OnTickNotify() override;
        virtual net::Session::Result AMF_STD_CALL OnDataReceived(const void* request, size_t requestSize, const net::Socket::Address& receivedFrom) override;
        virtual net::Session::Result AMF_STD_CALL OnSessionTimeout() override;
        virtual net::Session::Result AMF_STD_CALL OnSessionClose() override;
        virtual net::Socket::Result  AMF_STD_CALL Send(const void* buf, size_t size, size_t* const bytesSent, int flags);


        // FlowCtrlProtocol::ProcessIncomingCallback interface
        virtual void OnCompleteMessage(FlowCtrlProtocol::MessageID msgID, const void* buf, size_t size, const net::Socket::Address& receivedFrom, unsigned char optional) override;
        virtual net::Socket::Result OnRequestFragment(const FlowCtrlProtocol::Fragment& fragment) override;

        // FlowCtrlProtocol::ProcessOutgoingCallback interface
        virtual net::Socket::Result OnFragmentReady(const FlowCtrlProtocol::Fragment& fragment, bool last) override;
        virtual void OnSetMaxFragmentSize(size_t fragmentSize) override;

        virtual void AMF_STD_CALL OnPropertyChanged(const wchar_t* name) override;

    private:
        UDPServerSessionImpl(const UDPServerSessionImpl&) = delete;
        UDPServerSessionImpl& operator=(const UDPServerSessionImpl&) = delete;

        uint32_t m_SeqNum = 0;

    protected:
        net::DatagramSocket::Ptr    m_Socket;
        FlowCtrlProtocol::Ptr       m_pFlowCtrl;
        size_t                      m_TxMaxFragmentSize = 0;        // Max payload supported by server (config setting)
        size_t                      m_RxMaxFragmentSize = 0;        // Max payload size received by server (sent by client)
    };

}