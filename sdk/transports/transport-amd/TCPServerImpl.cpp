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
#include "TCPServerSessionImpl.h"
#include "Misc.h"
#include "amf/public/common/TraceAdapter.h"
#include "transports/transport-amd/ServerTransportImpl.h"

#include <iostream>

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::transport_amd::ServerImpl";

static constexpr const time_t DISCONNECT_TIMEOUT = 5;
static constexpr const size_t MAX_CONCURRENT_CONNECTIONS = 10;


namespace ssdk::transport_amd
{
    ServerImpl::TCPServer::TCPServer(const char* url, uint16_t defaultPort, size_t maxFragmentSize, ServerImpl& server) :
        net::StreamServer(new net::StreamSocket, maxFragmentSize, MAX_CONCURRENT_CONNECTIONS, DISCONNECT_TIMEOUT),
        m_Server(server),
        m_Url(url),
        m_Port(defaultPort),
        m_MaxFragmentSize(maxFragmentSize)
    {
    }

    ServerImpl::TCPServer::~TCPServer()
    {
        StopServer();
    }


    net::Session::Ptr ServerImpl::TCPServer::OnCreateSession(const net::Socket::Address& peer, net::Socket* socket, uint8_t* /*buf*/, size_t /*bufSize*/)
    {
        net::Session::Ptr session(new amf::AMFInterfaceMultiImpl<TCPServerSessionImpl, net::Session, ServerImpl*, net::StreamSocket::Ptr, const net::Socket::Address&>(&m_Server, net::StreamSocket::Ptr(socket), peer));

        amf::AMFVariantStruct userIPStruct;
        amf::AMFVariantAssignString(&userIPStruct, net::Socket::IPv4Address(peer).GetAddressAsString().c_str());
        static_cast<TCPServerSessionImpl*>(session.GetPtr())->SetPeerAddress(net::Socket::IPv4Address(peer).GetAddressAsString());

        if (m_Server.m_pConnectCallback->OnClientConnected(static_cast<TCPServerSessionImpl*>(session.GetPtr())) != transport_common::Result::OK)
        {
            session = nullptr;
            AMFTraceError(AMF_FACILITY, L"TCPServer::OnCreateSession: failed to create a TCP session for client %S", peer.GetParsedAddressAsString().c_str());
        }
        else
        {
            if (m_Server.m_pServerTransport != nullptr && session != nullptr)
            {
                m_Server.m_pServerTransport->NewClientSessionCreated(static_cast<TCPServerSessionImpl*>(session.GetPtr()));
            }
            AMFTraceInfo(AMF_FACILITY, L"TCPServer::OnCreateSession: TCP session created for client %S", peer.GetParsedAddressAsString().c_str());
        }
        return session;
    }

    void ServerImpl::TCPServer::Run()
    {
        RunServer();
    }

    transport_common::Result ServerImpl::TCPServer::StartServer()
    {
        transport_common::Result result = transport_common::Result::FAIL;
        net::StreamSocket::Ptr socket(GetSocket());
        int yes = 1;
        socket->SetSocketOpt(SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        net::Url urlTemp(m_Url, "tcp", m_Port);

        if (socket->Bind(urlTemp) != net::Socket::Result::OK)
        {
            SetSocket(nullptr);
            result = transport_common::Result::PORT_BUSY;
        }
        else if (socket->Listen() != net::Socket::Result::OK)
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

    transport_common::Result ServerImpl::TCPServer::StopServer()
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
            net::StreamSocket::Ptr socket(GetSocket());

            if (socket != nullptr)
            {
                socket->Close();
            }
            RequestStop();
            WaitForStop();
            SetSocket(nullptr);
        }
        return result;

    }


}