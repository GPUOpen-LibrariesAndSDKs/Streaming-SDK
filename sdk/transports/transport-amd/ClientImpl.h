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
#include "ServerDiscovery.h"
#include "ClientSessionImpl.h"
#include "DgramClientSessionFlowCtrl.h"

#include "net/DatagramClient.h"
#include "net/StreamClient.h"

#include "amf/public/include/core/PropertyStorageEx.h"
#include "amf/public/common/PropertyStorageImpl.h"
#include "amf/public/common/InterfaceImpl.h"
#include "amf/public/common/Thread.h"

#include <list>
#include <sstream>
#include <iomanip>
#include <string>
#include <time.h>

namespace ssdk::transport_amd
{
    class DatagramClientSessionImpl;
    class ClientImpl :
        public amf::AMFInterfaceImpl<amf::AMFPropertyStorageImpl<Client>>,
        public ReceiverCallback,
        protected amf::AMFThread
    {
        friend class ClientSessionImpl;

        class DatagramClient : public net::DatagramClient
        {
        public:
            DatagramClient(ClientImpl* clientImpl);
            virtual ~DatagramClient();

            virtual net::ClientSession::Ptr AMF_STD_CALL OnCreateSession(Client& client, const net::Socket::Address& peer, net::Socket* connectSocket, void* params) override;

        private:
            ClientImpl* m_ClientImpl;
        };

        class StreamClient : public net::StreamClient
        {
        public:
            StreamClient(ClientImpl* clientImpl);
            virtual ~StreamClient();

            virtual net::ClientSession::Ptr AMF_STD_CALL OnCreateSession(Client& client, const net::Socket::Address& peer, net::Socket* connectSocket, void* params) override;

        private:
            ClientImpl* m_ClientImpl;
        };

    public:
        AMF_BEGIN_INTERFACE_MAP
            AMF_INTERFACE_ENTRY(Client)
            AMF_INTERFACE_CHAIN_ENTRY(amf::AMFInterfaceImpl<amf::AMFPropertyStorageImpl<Client>>)
            AMF_END_INTERFACE_MAP

            typedef amf::AMFInterfacePtr_T<ClientImpl> Ptr;

        ClientImpl();
        virtual ~ClientImpl();

        //  Discovery methods:
        virtual transport_common::Result AMF_STD_CALL EnumerateServers(const char* clientID, unsigned short port, time_t timeoutSec, size_t* const numOfServers, ServerEnumCallback* callback) override;
        virtual transport_common::Result AMF_STD_CALL GetServerParameters(size_t serverIdx, ServerParameters** params) override;
        virtual transport_common::Result AMF_STD_CALL QueryServerInfo(const char* clientID, const char* url, unsigned short port, time_t timeoutSec, ServerEnumCallback* callback) override;

        //  Connection methods:
        virtual transport_common::Result AMF_STD_CALL ConnectToServerAndQueryParameters(const char* url, const char* clientID, Session** session, ServerParameters** params) override;
        virtual transport_common::Result AMF_STD_CALL SetTimeout(time_t timeoutSec) override;

        virtual transport_common::Result AMF_STD_CALL AddVideoCodec(const VideoCodec& codec) override;
        virtual transport_common::Result AMF_STD_CALL AddAudioCodec(const AudioCodec& codec) override;


        //  Run the client service in a thread
        virtual transport_common::Result AMF_STD_CALL Activate() override;
        virtual transport_common::Result AMF_STD_CALL Deactivate() override;

        // helpers
        void SetSession(ClientSessionImpl* session);
        void SetDatagramSize(size_t datagramSize);

        virtual void AMF_STD_CALL OnMessageReceived(Session* session, Channel channel, int msgID, const void* message, size_t messageSize) override;
        virtual void AMF_STD_CALL OnTerminate(Session* session, TerminationReason reason) override;

    protected:
        virtual void Run() override;
    private:
        ClientImpl(const ClientImpl&) = delete;
        ClientImpl& operator=(const ClientImpl&) = delete;

        transport_common::Result AMF_STD_CALL EstablishUDPConnection(const net::Url& url, ClientSessionImpl** session, size_t datagramSize = 0);
        transport_common::Result AMF_STD_CALL EstablishTCPConnection(const net::Url& url, ClientSessionImpl** session);

        //net::DatagramClient interface
//        virtual net::ClientSession* AMF_STD_CALL OnCreateSession(Client& client, const net::Socket::Address& peer, net::Socket::Ptr& connectSocket, void* params);

    private:
        mutable amf::AMFCriticalSection     m_CritSect;
        ServerParametersImpl::Collection    m_Servers;
        bool                                m_ServersEnumerated = false;
        ClientSessionImpl*                  m_Session = nullptr;
        volatile bool                       m_WaitingForIncoming = false;
        volatile bool                       m_WaitForHelloResponse = false;
        unsigned short                      m_DiscoveryPort = 0;
        time_t                              m_Timeout = 0;
        size_t                              m_DatagramSize = 0;
        net::Client::Ptr                    m_Client;
        size_t                              m_RxMaxFragmentSize = 0;
        size_t                              m_TxMaxFragmentSize = 0;

        volatile bool                       m_Terminated = false;
        ServerParameters::Ptr               m_CurrentServer;
        net::Url                            m_CurrentServerUrl;
        std::string                         m_Transport;
        transport_common::Result            m_ConnectionStatus;

        std::vector<VideoCodec>             m_VideoCodecs;
        std::vector<AudioCodec>             m_AudioCodecs;
    };
}
