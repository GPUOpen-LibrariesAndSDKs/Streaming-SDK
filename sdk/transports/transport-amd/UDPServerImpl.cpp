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
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include "TransportServerImpl.h"
#include "ServerSessionImpl.h"
#include "DiscoverySessionImpl.h"
#include "Misc.h"
#include "amf/public/common/TraceAdapter.h"
#include "transports/transport-amd/ServerTransportImpl.h"

#include <iostream>

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::transport_amd::ServerImpl";

static constexpr const time_t DISCONNECT_TIMEOUT = 5;
static constexpr const size_t MAX_CONCURRENT_CONNECTIONS = 10;


namespace ssdk::transport_amd
{
    ServerImpl::UDPServer::UDPServer(const char* url, uint16_t defaultPort, size_t maxFragmentSize, ServerImpl& server) :
        net::DatagramServer(new net::DatagramSocket(), maxFragmentSize, MAX_CONCURRENT_CONNECTIONS, DISCONNECT_TIMEOUT),
        m_Server(server),
        m_Url(url),
        m_Port(defaultPort),
        m_MaxFragmentSize(static_cast<uint32_t>(maxFragmentSize))
    {
    }

    ServerImpl::UDPServer::~UDPServer()
    {
        StopServer();
    }

    net::Session::Ptr ServerImpl::UDPServer::OnCreateSession(const net::Socket::Address& peer, net::Socket*, uint8_t* buf, size_t bufSize)
    {
        FlowCtrlProtocol::Fragment fragment;
        if (fragment.ParseFromBuffer(buf, bufSize) != FlowCtrlProtocol::Result::OK)
        {
            return nullptr;
        }
        net::Session::Ptr session;
        Channel channel = static_cast<Channel>(fragment.GetChannelID());
        uint8_t optCode = *(reinterpret_cast<const uint8_t*>(fragment.GetFragmentData()));
        transport_common::Result res = transport_common::Result::OK;
        if (channel == Channel::SERVICE)
        {
            if (optCode == static_cast<uint8_t>(SERVICE_OP_CODE::DISCOVERY))   //  Special case for discovery broadcast
            {
                AMFTraceInfo(AMF_FACILITY, L"UDPServer::OnCreateSession: Discovery session created, client IP: %S", peer.GetParsedAddressAsString().c_str());
                session = new amf::AMFInterfaceMultiImpl<DiscoveryServerSessionImpl, net::Session, ServerImpl*, net::DatagramSocket::Ptr, const net::Socket::Address&, size_t >(&m_Server, GetSocket(), peer, bufSize);
                static_cast<UDPServerSessionImpl*>(session.GetPtr())->SetPeerAddress(net::Socket::IPv4Address(peer).GetAddressAsString());
                if (m_Server.m_pConnectCallback != nullptr)
                {
                    res = m_Server.m_pConnectCallback->OnDiscovery((UDPServerSessionImpl*)session.GetPtr());
                }
                if (res == transport_common::Result::CONNECTION_REFUSED)
                {
                    HelloRefused refused;
                    static_cast<UDPServerSessionImpl*>(session.GetPtr())->Send(Channel::SERVICE, refused.GetSendData(), refused.GetSendSize());
                    AMFTraceInfo(AMF_FACILITY, L"UDPServer::OnCreateSession: connection refused, client IP: %S", peer.GetParsedAddressAsString().c_str());
                }
            }
            else if (optCode == static_cast<uint8_t>(SERVICE_OP_CODE::HELLO))   //  HELLO must be the first message received by a new session, otherwise it's a reconnect after a timed-out session
            {
                AMFTraceInfo(AMF_FACILITY, L"UDPServer::OnCreateSession: UDP session created, client IP: %S", peer.GetParsedAddressAsString().c_str());
                session = new amf::AMFInterfaceMultiImpl<UDPServerSessionImpl, net::Session, ServerImpl*, net::DatagramSocket::Ptr, const net::Socket::Address&, uint32_t>(&m_Server, GetSocket(), peer, m_MaxFragmentSize);
                static_cast<UDPServerSessionImpl*>(session.GetPtr())->SetPeerAddress(net::Socket::IPv4Address(peer).GetAddressAsString());
                if (m_Server.m_pConnectCallback != nullptr)
                {
                    res = m_Server.m_pConnectCallback->OnClientConnected(static_cast<UDPServerSessionImpl*>(session.GetPtr()));
                }
                if (m_Server.m_pServerTransport != nullptr && res == transport_common::Result::OK && session != nullptr)
                {
                    m_Server.m_pServerTransport->NewClientSessionCreated(static_cast<UDPServerSessionImpl*>(session.GetPtr()));
                }
            }
        }
        if (res != transport_common::Result::OK)
        {
            AMFTraceError(AMF_FACILITY, L"UDPServer::OnCreateSession error %d: UDP session destroyed, client IP: %S", (int)res, peer.GetParsedAddressAsString().c_str());
            session = nullptr;
        }

        if (session != nullptr)
        {
            amf::AMFVariantStruct vsInterval;
            amf_int64 interval = 10;
            m_Server.GetProperty(DATAGRAM_MSG_INTERVAL, &interval);
            amf::AMFVariantAssignInt64(&vsInterval, interval);
            static_cast<UDPServerSessionImpl*>(session.GetPtr())->SetProperty(DATAGRAM_MSG_INTERVAL, vsInterval);

            amf::AMFVariantStruct vsThreshold;
            amf_int64 lostMsgThreshold = 10;
            m_Server.GetProperty(DATAGRAM_LOST_MSG_THRESHOLD, &lostMsgThreshold);
            amf::AMFVariantAssignInt64(&vsThreshold, lostMsgThreshold);
            static_cast<UDPServerSessionImpl*>(session.GetPtr())->SetProperty(DATAGRAM_LOST_MSG_THRESHOLD, vsThreshold);

            amf::AMFVariantStruct vsTpThreshold;
            amf_int64 tpThreshold = 20;
            m_Server.GetProperty(DATAGRAM_TURNING_POINT_THRESHOLD, &tpThreshold);
            amf::AMFVariantAssignInt64(&vsTpThreshold, tpThreshold);
            static_cast<UDPServerSessionImpl*>(session.GetPtr())->SetProperty(DATAGRAM_TURNING_POINT_THRESHOLD, vsTpThreshold);
        }

        return session;
    }

    void ServerImpl::UDPServer::Run()
    {
        RunServer();
    }

    transport_common::Result ServerImpl::UDPServer::StartServer()
    {
        transport_common::Result result = transport_common::Result::FAIL;

        net::DatagramSocket::Ptr socket(GetSocket());
        int yes = 1;
        socket->SetSocketOpt(SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));
        yes = 1;
        //	socket->SetSocketOpt(SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
        int recvBufSize = SOCKET_BUF_SIZE_RCV;
        socket->SetSocketOpt(SOL_SOCKET, SO_RCVBUF, &recvBufSize, sizeof(recvBufSize));
        int sendBufSize = SOCKET_BUF_SIZE_SND;
        socket->SetSocketOpt(SOL_SOCKET, SO_SNDBUF, &sendBufSize, sizeof(sendBufSize));

        net::Url urlTemp(m_Url, "udp", m_Port);
        net::Socket::Ptr   sock(m_Socket);
        if (m_Socket->Bind(urlTemp) != net::Socket::Result::OK)
        {
            SetSocket(nullptr);
            result = transport_common::Result::PORT_BUSY;
        }
        else
        {
            Start();
            result = transport_common::Result::OK;
        }
        return result;
    }

    transport_common::Result ServerImpl::UDPServer::StopServer()
    {
        transport_common::Result result = transport_common::Result::OK;
        {
            amf::AMFLock    lock(&m_CritSect);
            if (IsServerRunning() != true)
            {
                result = transport_common::Result::NOT_RUNNING;
            }
        }
        if (result == transport_common::Result::OK)
        {
            ShutdownServer();
            if (m_Socket != nullptr)
            {
                m_Socket->Close();
            }
            RequestStop();
            WaitForStop();
            SetSocket(nullptr);
        }
        return result;

    }


}