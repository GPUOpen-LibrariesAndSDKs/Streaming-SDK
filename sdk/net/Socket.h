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

#include <memory>
#include <map>
#include <set>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <WS2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>
#ifdef __linux
#include <sys/un.h>
#endif

typedef int SOCKET;
#define INVALID_SOCKET -1
#endif

#include "Url.h"
#include "Initializer.h"
#include "amf/public/common/InterfaceImpl.h"

namespace ssdk::net
{
    class Socket :
        public amf::AMFInterfaceImpl<amf::AMFInterface>,
        protected Initializer
    {
    public:
        typedef amf::AMFInterfacePtr_T<Socket> Ptr;
        AMF_DECLARE_IID(0x3a955d2f, 0x37c9, 0x4050, 0xaf, 0x29, 0x7d, 0x79, 0x67, 0x1d, 0x97, 0x7);

        AMF_BEGIN_INTERFACE_MAP
            AMF_INTERFACE_ENTRY(Socket)
            AMF_INTERFACE_CHAIN_ENTRY(amf::AMFInterfaceImpl<amf::AMFInterface>)
        AMF_END_INTERFACE_MAP

#ifdef _WIN32
        typedef SOCKET Socket_t;
#else
        typedef int Socket_t;
#endif
        typedef std::set<Socket::Ptr>	Set;

        enum class Result
        {
            OK,
            NOT_INITIALIZED,
            ADDRESS_FAMILY_NOT_SUPPORTED,
            PROTOCOL_NOT_SUPPORTED,
            OPERATION_NOT_SUPPORTED,
            SOCKET_TYPE_NOT_SUPPORTED,
            INVALID_ARG,
            INVALID_PROVIDER,
            INVALID_PROC_TABLE,
            INVALID_ADDRESS,
            BUSY,
            IN_USE,
            NETWORK_DOWN,
            NO_MORE_HANDLES,
            NO_BUFFER_SPACE,
            ACCESS_DENIED,
            ACCESS_VIOLATION,
            SOCKET_NOT_OPEN,
            SOCKET_ALREADY_OPEN,
            SOCKET_NOT_CONNECTED,
            SOCKET_SHUTDOWN,
            SOCKET_WOULD_BLOCK,
            MESSAGE_TOO_BIG,
            DESTINATION_UNREACHABLE,
            CONNECTION_ABORTED,
            CONNECTION_RESET,
            CONNECTION_TIMEOUT,
            CONNECTION_CLOSED,
            CONNECTION_REFUSED,
            END_OF_PIPE,

            UNKNOWN_ERROR
        };

        enum class AddressFamily
        {
            ADDR_UNSPEC = AF_UNSPEC,
#if defined(__linux)
            ADDR_UNIX = AF_UNIX,
#endif
            ADDR_IP = AF_INET,
            ADDR_IP6 = AF_INET6
        };

        enum class Type
        {
            TYPE_STREAM = SOCK_STREAM,
            TYPE_DATAGRAM = SOCK_DGRAM
        };

        enum class Protocol
        {
#if defined(__linux)
            PROTO_UNIX = 0,
#endif
            PROTO_TCP = IPPROTO_TCP,
            PROTO_UDP = IPPROTO_UDP
        };

        class Address
        {
        public:
            typedef std::shared_ptr<const Address>      CPtr;
            typedef std::shared_ptr<Address>            Ptr;

        public:
            Address();
            Address(const Address& other);
            Address(const sockaddr& other);

            Address& operator=(const Address& other);
            Address& operator=(const sockaddr& other);

            bool operator==(const Address& other) const;
            bool operator==(const sockaddr& other) const;
            bool operator<(const Address& other) const;
            inline bool operator!=(const Address& other) const { return !operator==(other); }
            inline bool operator!=(const sockaddr& other) const { return !operator==(other); }

            inline Socket::AddressFamily GetAddressFamily() const { return static_cast<Socket::AddressFamily>(m_SockAddr.ss_family); }
            inline void SetAddressFamily(Socket::AddressFamily family)
            {
            #ifdef _WIN32
                m_SockAddr.ss_family = static_cast<ADDRESS_FAMILY>(family);
            #else
                m_SockAddr.ss_family = static_cast<sa_family_t>(family);
            #endif
            }

            inline const sockaddr& ToSockAddr() const { return reinterpret_cast<const sockaddr&>(m_SockAddr); }
            inline sockaddr& ToSockAddr() { return reinterpret_cast<sockaddr&>(m_SockAddr); }

            size_t GetSize() const;
            virtual std::string GetAddressAsString() const;
            virtual std::string GetParsedAddressAsString() const;

            virtual Ptr Duplicate() const { return Ptr(new Address(*this)); }

            bool IsBroadcast() const;
        protected:
            sockaddr_storage    m_SockAddr = {};

        private:
            static const wchar_t* const m_Realm;
        };

        class IPv4Address :
            public Address
        {
        public:
            IPv4Address();
            IPv4Address(const std::string& host, unsigned short port);
            IPv4Address(const IPv4Address& other);
            IPv4Address(const Address& other);
            IPv4Address(const sockaddr& other);
            IPv4Address(const sockaddr_in& other);

            IPv4Address& operator=(const IPv4Address& other);
            IPv4Address& operator=(const Address& other);
            IPv4Address& operator=(const sockaddr& other);
            IPv4Address& operator=(const sockaddr_in& other);

            bool operator==(const sockaddr& other) const;
            bool operator==(const sockaddr_in& other) const;
            bool operator==(const Address& other) const;
            bool operator==(const IPv4Address& other) const;

            inline bool operator!=(const sockaddr& other) const { return !operator==(other); }
            inline bool operator!=(const sockaddr_in& other) const { return !operator==(other); }
            inline bool operator!=(const Address& other) const { return !operator==(other); }
            inline bool operator!=(const IPv4Address& other) const { return !operator==(other); }

            inline const in_addr& GetAddress() const { return reinterpret_cast<const sockaddr_in&>(m_SockAddr).sin_addr; }
            inline void SetAddress(const in_addr& address) { reinterpret_cast<sockaddr_in&>(m_SockAddr).sin_addr = address; }
            void SetAddress(const std::string& address);

#ifdef __linux
            std::string GetInterfaceName() const;
#endif

            unsigned short GetPort() const;
            void SetIPPort(unsigned short port);

            virtual std::string GetAddressAsString() const;

            virtual Ptr Duplicate() const { return Ptr(new IPv4Address(*this)); }

            inline const sockaddr_in& ToSockAddr_In() const { return reinterpret_cast<const sockaddr_in&>(m_SockAddr); }
            inline sockaddr_in& ToSockAddr_In() { return reinterpret_cast<sockaddr_in&>(m_SockAddr); }

        private:
            static const wchar_t* const m_Realm;
        };

#if defined(__linux)
        class UnixDomainAddress :
            public Address
        {
        public:
            UnixDomainAddress();
            UnixDomainAddress(const std::string& address);
            UnixDomainAddress(const UnixDomainAddress& other);
            UnixDomainAddress(const Address& other);
            UnixDomainAddress(const sockaddr& other);
            UnixDomainAddress(const sockaddr_un& other);

            UnixDomainAddress& operator=(const UnixDomainAddress& other);
            UnixDomainAddress& operator=(const Address& other);
            UnixDomainAddress& operator=(const sockaddr& other);
            UnixDomainAddress& operator=(const sockaddr_un& other);

            bool operator==(const sockaddr& other) const;
            bool operator==(const sockaddr_un& other) const;
            bool operator==(const Address& other) const;
            bool operator==(const UnixDomainAddress& other) const;

            inline bool operator!=(const sockaddr& other) const { return !operator==(other); }
            inline bool operator!=(const sockaddr_un& other) const { return !operator==(other); }
            inline bool operator!=(const Address& other) const { return !operator==(other); }
            inline bool operator!=(const UnixDomainAddress& other) const { return !operator==(other); }

            const std::string& GetAddress() const;
            void SetAddress(const std::string& address);

            virtual std::string GetAddressAsString() const;

            virtual Ptr Duplicate() const { return Ptr(new UnixDomainAddress(*this)); }

            inline const sockaddr_un& ToSockAddr_Un() const { return reinterpret_cast<const sockaddr_un&>(m_SockAddr); }
            inline sockaddr_un& ToSockAddr_Un() { return reinterpret_cast<sockaddr_un&>(m_SockAddr); }

        private:
            static const wchar_t* const m_Realm;
        };
#endif

    protected:
        Socket(AddressFamily addrFamily, Type type, Protocol protocol);
        Socket(Socket_t handle, Address::Ptr peerAddress, AddressFamily addrFamily, Type type, Protocol protocol);

        static Result           GetError(int osError);
        static int              GetSocketOSError();

    public:
        virtual ~Socket();

        bool operator==(const Socket& other) const;
        inline bool operator!=(const Socket& other) const { return !operator==(other); }

        virtual Socket::Result Open(AddressFamily addrFamily, Type type, Protocol protocol);
        virtual Socket::Result Close();

        virtual Socket::Result Bind(const Url& url);
        virtual Socket::Result Bind(const Address& address);

        virtual Socket::Result Connect(const Url& url);
        virtual Socket::Result Connect(const Address& address);

        virtual Socket::Result Send(const void* buf, size_t size, size_t* bytesSent, int flags = 0);
        virtual Socket::Result Receive(void* buf, size_t size, size_t* bytesReceived, int flags = 0);	//	Reads whatever is available and returns. Equivalent to recv()

        virtual Socket::Result SetSocketOpt(int level, int option, const void* value, size_t valueSize);

        void SetTimeout(int seconds);
        inline int GetTimeout() const { return m_Timeout; }

        inline Type GetType() const throw() { return m_SocketType; }
        inline AddressFamily GetAddressFamily() const throw() { return m_AddrFamily; }
        inline Protocol GetProtocol() const throw() { return m_Protocol; }

        inline Socket_t GetNativeHandle() const throw() { return m_Socket; }

        inline Address::CPtr GetPeerAddress() const { return std::const_pointer_cast<const Address>(m_PeerAddress); }

        static const wchar_t* GetErrorString(Result error);

    protected:
        inline void SetNativeHandle(Socket_t handle) { m_Socket = handle; }

    private:
        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;

    protected:
        Socket_t        m_Socket;
        AddressFamily   m_AddrFamily;
        Type            m_SocketType;
        Protocol        m_Protocol;
        Address::Ptr    m_PeerAddress;
        int				m_Timeout = 0;
    };
}