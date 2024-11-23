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

#include "TransportClient.h"
#include "DgramClientSessionFlowCtrl.h"

#include "net/StreamClientSession.h"

#include "amf/public/common/InterfaceImpl.h"
#include "amf/public/common/Thread.h"

namespace ssdk::transport_amd
{
    class ClientImpl;


    class ClientSessionImpl :
        public Session
    {
    public:
        typedef amf::AMFInterfacePtr_T<ClientSessionImpl> Ptr;

        AMF_DECLARE_IID(0xf82e3416, 0x803d, 0x406a, 0xaa, 0xf0, 0x3c, 0x5, 0x1a, 0x4a, 0x5e, 0x35);

    protected:
        ClientSessionImpl(ClientImpl* client);

        net::Session::Result NotifyTerminationCallbacks(ReceiverCallback::TerminationReason reason);
    public:

        virtual ~ClientSessionImpl();

        virtual void AMF_STD_CALL UpgradeProtocol(uint32_t version) = 0;
        virtual void AMF_STD_CALL RegisterReceiverCallback(ReceiverCallback* callback) override;

        virtual net::ClientSession::Result AMF_STD_CALL WaitForIncomingMessages() = 0;

        inline ServerParameters::Ptr GetServerParameters() const { return m_ServerParams; }
        inline void SetServerParameters(ServerParameters::Ptr params) { m_ServerParams = params; }

        virtual const char* AMF_STD_CALL GetPeerPlatform() const noexcept override;
        virtual const char* AMF_STD_CALL GetPeerUrl() const noexcept override;

        virtual ssdk::transport_common::SessionHandle AMF_STD_CALL GetSessionHandle() const noexcept override { return m_SessionHandle; };

    protected:
        mutable amf::AMFCriticalSection     m_CritSect;
        ReceiverCallback* m_Callback = nullptr;
        ClientImpl*   m_Client = nullptr;
        ServerParameters::Ptr m_ServerParams;
        ssdk::transport_common::SessionHandle m_SessionHandle;
    };


    class DatagramClientSessionImpl :
        public amf::AMFInterfaceBase,
        public ClientSessionImpl,
        public DatagramClientSessionFlowCtrl
    {
    public:
        typedef amf::AMFInterfacePtr_T<DatagramClientSessionImpl > Ptr;
        AMF_DECLARE_IID(0x47fe30cd, 0x2849, 0x4fd4, 0x81, 0x78, 0xc, 0xd1, 0xd3, 0x12, 0xd, 0x1f);

        AMF_BEGIN_INTERFACE_MAP
            AMF_INTERFACE_MULTI_ENTRY(DatagramClientSessionImpl)
            AMF_INTERFACE_MULTI_ENTRY(ClientSessionImpl)
            AMF_INTERFACE_MULTI_ENTRY(DatagramClientSessionFlowCtrl)
            AMF_INTERFACE_MULTI_ENTRY(net::DatagramClientSession)
            AMF_INTERFACE_MULTI_ENTRY(net::ClientSession)
            AMF_INTERFACE_MULTI_ENTRY(net::Session)
        AMF_END_INTERFACE_MAP

        DatagramClientSessionImpl(ClientImpl* client, net::Socket* sock, const net::Socket::Address& peer, size_t receiveBufSize);
        virtual ~DatagramClientSessionImpl();

        virtual net::Session::Result   AMF_STD_CALL OnInit() override;
        virtual net::Session::Result   AMF_STD_CALL OnSessionTimeout() override;
        virtual net::Session::Result   AMF_STD_CALL OnSessionClose() override;
        virtual void                   AMF_STD_CALL Terminate() override;
        virtual bool                   AMF_STD_CALL IsTerminated() const noexcept override { return net::Session::IsTerminated(); }


        virtual net::ClientSession::Result  AMF_STD_CALL WaitForIncomingMessages() override;

        virtual transport_common::Result    AMF_STD_CALL Send(Channel channel, const void* msg, size_t msgLen) override;

        virtual void AMF_STD_CALL UpgradeProtocol(uint32_t version) override;
    protected:
        virtual void OnCompleteMessage(FlowCtrlProtocol::MessageID msgID, const void* buf, size_t size, const net::Socket::Address& receivedFrom, uint8_t optional) override;

    };


    class StreamClientSessionImpl :
        public amf::AMFInterfaceBase,
        public ClientSessionImpl,
        public net::StreamClientSession
    {
    public:
        typedef amf::AMFInterfacePtr_T<StreamClientSessionImpl> Ptr;

        AMF_DECLARE_IID(0x1ecf9518, 0x5d80, 0x4eca, 0x8b, 0xdf, 0xde, 0xdd, 0xa5, 0xd9, 0x5, 0xd7);

        AMF_BEGIN_INTERFACE_MAP
            AMF_INTERFACE_MULTI_ENTRY(StreamClientSessionImpl)
            AMF_INTERFACE_MULTI_ENTRY(ClientSessionImpl)
            AMF_INTERFACE_MULTI_ENTRY(net::StreamClientSession)
            AMF_INTERFACE_MULTI_ENTRY(net::ClientSession)
            AMF_INTERFACE_MULTI_ENTRY(net::Session)
        AMF_END_INTERFACE_MAP


        StreamClientSessionImpl(ClientImpl* client, net::Socket* sock, const net::Socket::Address& peer, size_t receiveBufSize);
        virtual ~StreamClientSessionImpl();

        virtual net::Session::Result   AMF_STD_CALL OnInit() override;
        virtual net::Session::Result   AMF_STD_CALL OnSessionTimeout() override;
        virtual net::Session::Result   AMF_STD_CALL OnSessionClose() override;
        virtual void                   AMF_STD_CALL Terminate() override;
        virtual bool                   AMF_STD_CALL IsTerminated() const noexcept override { return net::Session::IsTerminated(); }

        virtual net::ClientSession::Result AMF_STD_CALL WaitForIncomingMessages() override;


        virtual bool                       AMF_STD_CALL OnTickNotify()  override;
        virtual net::ClientSession::Result AMF_STD_CALL OnDataReceived() override;
        virtual void AMF_STD_CALL UpgradeProtocol(uint32_t /*version*/) override {};

        virtual transport_common::Result   AMF_STD_CALL Send(Channel channel, const void* msg, size_t msgLen) override;

    private:
        StreamFlowCtrlProtocol	m_FlowCtrl;
    };

}
