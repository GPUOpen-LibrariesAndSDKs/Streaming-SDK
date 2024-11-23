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
#include "net/DatagramServer.h"
#include "net/StreamServer.h"

#include "amf/public/common/InterfaceImpl.h"
#include "amf/public/common/Thread.h"
#include "amf/public/common/PropertyStorageExImpl.h"

#include <set>

namespace ssdk::transport_amd
{
    class ServerTransportImpl;
    class ServerImpl :
        public amf::AMFInterfaceImpl<amf::AMFPropertyStorageExImpl<ssdk::transport_amd::Server>>
    {
    private:
        class UDPServer :
            public ssdk::net::DatagramServer,
            protected amf::AMFThread
        {
        public:
            typedef std::shared_ptr<UDPServer>	Ptr;

        public:
            UDPServer(const char* url, uint16_t defaultPort, size_t maxFragmentSize, ServerImpl& server);
            ~UDPServer();

            transport_common::Result StartServer();
            transport_common::Result StopServer();

            virtual net::Session::Ptr AMF_STD_CALL OnCreateSession(const net::Socket::Address& peer, net::Socket* socket, uint8_t* buf, size_t bufSize) override;   //  Create a session on the connection returned by OnAcceptConnection()

            virtual void Run() override;
        private:
            mutable amf::AMFCriticalSection     m_CritSect;
            ServerImpl&						    m_Server;
            std::string							m_Url;
            uint16_t							m_Port;
            uint32_t							m_MaxFragmentSize;
        };

        class TCPServer :
            public ssdk::net::StreamServer,
            protected amf::AMFThread
        {
        public:
            typedef std::shared_ptr<TCPServer>	Ptr;

        public:
            TCPServer(const char* url, uint16_t defaultPort, size_t maxFragmentSize, ServerImpl& server);
            ~TCPServer();

            transport_common::Result StartServer();
            transport_common::Result StopServer();

            virtual net::Session::Ptr AMF_STD_CALL OnCreateSession(const net::Socket::Address& peer, net::Socket* socket, uint8_t* buf, size_t bufSize) override;   //  Create a session on the connection returned by OnAcceptConnection()

            virtual void Run() override;
        private:
            mutable amf::AMFCriticalSection m_CritSect;
            ServerImpl&                     m_Server;
            std::string                     m_Url;
            uint16_t                        m_Port;
            size_t                          m_MaxFragmentSize;
        };

    public:
        // {9D6BF699-7436-47A6-A248-A422C874722C}
        AMF_DECLARE_IID(0x9d6bf699, 0x7436, 0x47a6, 0xa2, 0x48, 0xa4, 0x22, 0xc8, 0x74, 0x72, 0x2c);
        typedef amf::AMFInterfaceMultiImpl<ServerImpl, transport_amd::Server> Implementation;
        typedef amf::AMFInterfacePtr_T<Implementation> Ptr;

        AMF_BEGIN_INTERFACE_MAP
            AMF_INTERFACE_MULTI_ENTRY(ServerImpl)
            AMF_INTERFACE_MULTI_ENTRY(transport_amd::Server)
            AMF_INTERFACE_CHAIN_ENTRY(amf::AMFInterfaceImpl<amf::AMFPropertyStorageExImpl<transport_amd::Server>>)
        AMF_END_INTERFACE_MAP

        ServerImpl();

        // AWVRServer interface
        virtual transport_common::Result AMF_STD_CALL SetName(const char* name) override;
        virtual transport_common::Result AMF_STD_CALL SetDatagramSize(size_t size) override;

        //  Run the server in a thread
        virtual transport_common::Result AMF_STD_CALL Activate(const char* url, OnClientConnectCallback* connectCallback) override;
        virtual transport_common::Result AMF_STD_CALL Deactivate() override;

        virtual transport_common::Result AMF_STD_CALL SetOptionProvider(OnFillOptionsCallback *provider) override;
        virtual void             AMF_STD_CALL SetSessionTimeoutEnabled(bool timeoutEnabled) override;

        // helpers
        const std::string& GetName() const noexcept;
        uint16_t GetPort() const noexcept;
        size_t GetDatagramSize() const noexcept;

        void FillOptions(bool discovery, Session* session, HelloResponse::Options* options);

        inline bool IsUDPSupported() const { return m_UDPServer != nullptr; }
        inline bool IsTCPSupported() const { return m_TCPServer != nullptr; }

        bool AuthorizeDiscoveryRequest(Session* session, const std::string& deviceID);
        bool AuthorizeConnectionRequest(Session* session, const std::string& deviceID);

        void SetServerTransport(ServerTransportImpl* pServerTransport) { m_pServerTransport = pServerTransport; };
        ServerTransportImpl* GetServerTransport() { return m_pServerTransport; };
    protected:

        // helpers
        bool IsConnected() const noexcept;
        void SetConnected(bool val) noexcept;

    private:
        ServerImpl(const ServerImpl&) = delete;
        ServerImpl& operator=(const ServerImpl&) = delete;

    private:
        mutable amf::AMFCriticalSection     m_CritSect;
//        bool                                m_Running;
        bool                                m_Connected = false;
        std::string                         m_Name;
        uint16_t							m_Port = 0;
        size_t								m_MaxFragmentSize;  // Datagram size setting in configuration
        OnClientConnectCallback*            m_pConnectCallback = nullptr;
        OnFillOptionsCallback *             m_pOptionsProvider = nullptr;
        UDPServer::Ptr						m_UDPServer;
        TCPServer::Ptr						m_TCPServer;
        ServerTransportImpl*                m_pServerTransport = nullptr;

        bool								m_PairingModeEnabled = false;
    };
}
