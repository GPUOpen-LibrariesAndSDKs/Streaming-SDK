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
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "Socket.h"
#include "amf/public/common/TraceAdapter.h"
#ifdef _WIN32
#include <WS2tcpip.h>
#else
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::net::Socket";

namespace ssdk::net
{
    Socket::Socket(AddressFamily addrFamily, Type type, Protocol protocol) :
        m_Socket(INVALID_SOCKET),
        m_AddrFamily(addrFamily),
        m_SocketType(type),
        m_Protocol(protocol),
        m_Timeout(0)
    {
        Open(addrFamily, type, protocol);
    }

    Socket::Socket(Socket_t handle, Address::Ptr peerAddress, AddressFamily addrFamily, Type type, Protocol protocol) :
        m_Socket(handle),
        m_AddrFamily(addrFamily),
        m_SocketType(type),
        m_Protocol(protocol),
        m_PeerAddress(peerAddress)
    {
    }

    Socket::~Socket()
    {
        Close();
    }

    bool Socket::operator==(const Socket& other) const
    {
        return  m_Socket == other.m_Socket &&
                m_AddrFamily == other.m_AddrFamily &&
                m_SocketType == other.m_SocketType &&
                m_Protocol == other.m_Protocol;
    }

    int Socket::GetSocketOSError()
    {
        int error = 0;
#ifdef _WIN32
        error = ::WSAGetLastError();
#else
        error = errno;
#endif
        return error;
    }

    const wchar_t*   Socket::GetErrorString(Socket::Result error)
    {
        static struct MsgMapElement
        {
            Socket::Result	m_Error;
            const wchar_t*	m_Text;
        } msgMap[] =
        {
            { Result::OK,                                   L"OK" },
            { Result::NOT_INITIALIZED,						L"Winsock not initialized" },
            { Result::ADDRESS_FAMILY_NOT_SUPPORTED,			L"Network is down" },
            { Result::PROTOCOL_NOT_SUPPORTED,				L"Protocol not supported" },
            { Result::OPERATION_NOT_SUPPORTED,				L"Operation not supported" },
            { Result::SOCKET_TYPE_NOT_SUPPORTED,			L"Socket type not supported for this address family" },
            { Result::INVALID_ARG,							L"Invalid argument" },
            { Result::INVALID_PROVIDER,						L"Invalid provider" },
            { Result::INVALID_PROC_TABLE,					L"Invalid proc table" },
            { Result::INVALID_ADDRESS,						L"Invalid address" },
            { Result::BUSY,									L"Busy" },
            { Result::IN_USE,								L"In use" },
            { Result::NETWORK_DOWN,							L"Network is down" },
            { Result::NO_MORE_HANDLES,						L"No more handles" },
            { Result::NO_BUFFER_SPACE,						L"No buffer space available" },
            { Result::ACCESS_DENIED,                        L"Access denied"},
            { Result::ACCESS_VIOLATION,                     L"Access vialation" },
            { Result::SOCKET_NOT_OPEN,						L"Socket not open" },
            { Result::SOCKET_ALREADY_OPEN,					L"Socket is already open" },
            { Result::SOCKET_NOT_CONNECTED,                 L"Socket not connected" },
            { Result::SOCKET_SHUTDOWN,                      L"Socket shutdown" },
            { Result::SOCKET_WOULD_BLOCK,                   L"Socket would block" },
            { Result::MESSAGE_TOO_BIG,                      L"Message too big" },
            { Result::DESTINATION_UNREACHABLE,              L"Destination unreachable" },
            { Result::CONNECTION_ABORTED,					L"Connection aborted" },
            { Result::CONNECTION_RESET,						L"Connection reset" },
            { Result::CONNECTION_TIMEOUT,				    L"Connection timeout" },
            { Result::CONNECTION_REFUSED,				    L"Connection refused" },
            { Result::END_OF_PIPE,                          L"End of pipe"},
            { Result::UNKNOWN_ERROR,						L"Unknown error" },
        };

        for (int i = 0; i < sizeof(msgMap) / sizeof(MsgMapElement); i++)
        {
            if (msgMap[i].m_Error == error)
            {
                return msgMap[i].m_Text;
            }
        }
        return L"unknown error";
    }

    Socket::Result Socket::GetError(int error)
    {
        Socket::Result result = Result::UNKNOWN_ERROR;
        static struct NativeToSocketErrorMap
        {
            int					m_Native;
            Socket::Result		m_Result;
        } errMap[] =
        {
#ifdef _WIN32
            { WSANOTINITIALISED,		Result::NOT_INITIALIZED },
            { WSAENETDOWN,				Result::NETWORK_DOWN },
            { WSAEAFNOSUPPORT,			Result::ADDRESS_FAMILY_NOT_SUPPORTED },
            { WSAEINPROGRESS,			Result::BUSY },
            { WSAEMFILE,				Result::NO_MORE_HANDLES },
            { WSAEINVAL,				Result::INVALID_ARG },
            { WSAEPROVIDERFAILEDINIT,	Result::INVALID_PROVIDER },
            { WSAEINVALIDPROVIDER,		Result::INVALID_PROVIDER },
            { WSAEINVALIDPROCTABLE,		Result::INVALID_PROC_TABLE },
            { WSAENOBUFS,				Result::NO_BUFFER_SPACE },
            { WSAEPROTONOSUPPORT,		Result::PROTOCOL_NOT_SUPPORTED },
            { WSAEPROTOTYPE,			Result::PROTOCOL_NOT_SUPPORTED },
            { WSAESOCKTNOSUPPORT,		Result::SOCKET_TYPE_NOT_SUPPORTED },
            { WSAEACCES,				Result::ACCESS_DENIED },
            { WSAEFAULT,				Result::ACCESS_VIOLATION },
            { WSAENETRESET,				Result::CONNECTION_RESET },
            { WSAECONNRESET,			Result::CONNECTION_RESET },
            { WSAENOTCONN,				Result::SOCKET_NOT_CONNECTED },
            { WSAEOPNOTSUPP,			Result::OPERATION_NOT_SUPPORTED },
            { WSAESHUTDOWN,				Result::SOCKET_SHUTDOWN },
            { WSAEWOULDBLOCK,			Result::SOCKET_WOULD_BLOCK },
            { WSAEMSGSIZE,				Result::MESSAGE_TOO_BIG },
            { WSAEHOSTUNREACH,			Result::DESTINATION_UNREACHABLE },
            { WSAECONNABORTED,			Result::CONNECTION_ABORTED },
            { WSAETIMEDOUT,				Result::CONNECTION_TIMEOUT },
            { WSAEADDRINUSE,			Result::IN_USE },
            { WSAEADDRNOTAVAIL,			Result::INVALID_ADDRESS }
#else
            { ENETDOWN,				    Result::NETWORK_DOWN },
            { EAFNOSUPPORT,				Result::ADDRESS_FAMILY_NOT_SUPPORTED },
            { ENFILE,					Result::NO_MORE_HANDLES },
            { ENOBUFS,					Result::NO_BUFFER_SPACE },
            { ENOMEM,					Result::NO_BUFFER_SPACE },
            { EPROTONOSUPPORT,			Result::PROTOCOL_NOT_SUPPORTED },
            { ESOCKTNOSUPPORT,		    Result::SOCKET_TYPE_NOT_SUPPORTED },
            { EACCES,					Result::ACCESS_DENIED },
            { ENETRESET,                Result::CONNECTION_RESET },
            { ECONNRESET,               Result::CONNECTION_RESET },
            { ENOTCONN,				    Result::SOCKET_NOT_CONNECTED },
            { EOPNOTSUPP,				Result::OPERATION_NOT_SUPPORTED },
            { ESHUTDOWN,				Result::SOCKET_SHUTDOWN },
            { EWOULDBLOCK,			    Result::SOCKET_WOULD_BLOCK },
            { EWOULDBLOCK,			    Result::SOCKET_WOULD_BLOCK },
            { EMSGSIZE,					Result::MESSAGE_TOO_BIG },
            { EHOSTUNREACH,			    Result::DESTINATION_UNREACHABLE },
            { ECONNABORTED,             Result::CONNECTION_ABORTED },
            { ETIMEDOUT,                Result::CONNECTION_TIMEOUT },
            { EADDRINUSE,			    Result::IN_USE },
            { EADDRNOTAVAIL,			Result::INVALID_ADDRESS },
            { ECONNREFUSED,			    Result::CONNECTION_REFUSED },
            { EPIPE,                    Result::END_OF_PIPE},
#endif
        };
        bool found = false;
        for (int i = 0; i < sizeof(errMap) / sizeof(NativeToSocketErrorMap); i++)
        {
            if (errMap[i].m_Native == error)
            {
                result = errMap[i].m_Result;
                found = true;
                break;
            }
        }
        if (found == false)
        {
            AMFTraceError(AMF_FACILITY, L"Unknown error code (%d) - debug me", error);
        }

        return result;
    }

    Socket::Result Socket::Open(AddressFamily /*addrFamily*/, Type /*type*/, Protocol /*protocol*/)
    {
        Socket::Result result = Result::SOCKET_NOT_OPEN;
        if (m_Socket != INVALID_SOCKET)
        {
            result = Result::SOCKET_ALREADY_OPEN;
            AMFTraceError(AMF_FACILITY, L"Open() err=%s", GetErrorString(result));
        }
        else
        {
            m_Socket = ::socket(static_cast<int>(m_AddrFamily), static_cast<int>(m_SocketType), static_cast<int>(m_Protocol));
            if (m_Socket != INVALID_SOCKET)
            {
                result = Result::OK;
            }
            else
            {
                result = GetError(GetSocketOSError());
                AMFTraceError(AMF_FACILITY, L"Open() - socket() failed err=%s", GetErrorString(result));
            }
        }
        return result;
    }

    Socket::Result Socket::Close()
    {
        Socket::Result result = Result::SOCKET_NOT_OPEN;
        if (m_Socket != INVALID_SOCKET)
        {
#ifdef _WIN32
            ::closesocket(m_Socket);
#else
            ::close(m_Socket);
#endif
            m_Socket = INVALID_SOCKET;
            m_PeerAddress = nullptr;
            result = Result::OK;
        }
        return result;
    }

    Socket::Result Socket::Connect(const Address& address)
    {
        Socket::Result result = Result::OK;

        if (m_Socket == INVALID_SOCKET)
        {
            result = Result::SOCKET_NOT_OPEN;
            AMFTraceError(AMF_FACILITY, L"Connect() err=%s", GetErrorString(result));
        }
        else
        {
            if (::connect(m_Socket, &address.ToSockAddr(), (int)address.GetSize()) == -1)
            {
                result = GetError(GetSocketOSError());
                //AMFTraceError(AMF_FACILITY, L"Connect() connect() failed err=%s", GetErrorString(result));
            }
            else
            {
                m_PeerAddress = address.Duplicate();
            }
        }
        return result;
    }

    Socket::Result Socket::Connect(const Url& url)
    {
        Socket::Result result = Result::OK;

        struct addrinfo* dnsResults = NULL;
        std::string port = url.GetPortAsString();
        if (url.GetHost().length() == 0)
        {
            result = Result::INVALID_ARG;
        }
        else
        {
            if (port.empty() == true)
            {
                result = Result::INVALID_ARG;
                AMFTraceError(AMF_FACILITY, L"Connect() invalid port or protocol url=%S err=%s",
                              url.GetUrl().c_str(), GetErrorString(result));
            }
            else if (::getaddrinfo(url.GetHost().c_str(), port.c_str(), NULL, &dnsResults) != 0)
            {
                result = GetError(GetSocketOSError());
                AMFTraceError(AMF_FACILITY, L"Connect() getaddrinfo() failed url=%S err=%s",
                              url.GetUrl().c_str(), GetErrorString(result));
            }
            else
            {
                for (struct addrinfo* curEntry = dnsResults;
                     curEntry != NULL; curEntry = curEntry->ai_next)
                {
                    if (curEntry->ai_family == static_cast<int>(m_AddrFamily))
                    {
                        switch (m_AddrFamily)
                        {
                        case AddressFamily::ADDR_IP:
                            result = Connect(IPv4Address(*curEntry->ai_addr));
                            break;
                        default:
                            result = Connect(Address(*curEntry->ai_addr));
                            break;
                        }
                        break;
                    }
                }
                ::freeaddrinfo(dnsResults);
            }
        }
        return result;
    }

    Socket::Result Socket::Bind(const Address& address)
    {
        Socket::Result result = Result::OK;

        if (m_Socket == INVALID_SOCKET)
        {
            result = Result::SOCKET_NOT_OPEN;
            AMFTraceError(AMF_FACILITY, L"Bind() err=%s", GetErrorString(result));
        }
        else
        {
            // need to use a Ptr to avoid object slicing
            Address::Ptr bindAddress = address.Duplicate();
            if (::bind(m_Socket, &bindAddress->ToSockAddr(), (int)bindAddress->GetSize()) == -1)
            {
                result = GetError(GetSocketOSError());
                AMFTraceError(AMF_FACILITY, L"Bind()  bind() failed err=%s", GetErrorString(result));
            }
        }
        return result;
    }

    Socket::Result Socket::Bind(const Url& url)
    {
        Socket::Result result = Result::OK;

        struct addrinfo* dnsResults = nullptr;
        std::string service = url.GetPortAsString();
        std::string host = url.GetHost();
        if (service.empty() == true)
        {
            service = url.GetProtocol();
        }
        if (service.empty() == true)
        {
            result = Result::INVALID_ARG;
            AMFTraceError(AMF_FACILITY, L"Bind() invalid port or protocol url=%S err=%s", url.GetUrl().c_str(), GetErrorString(result));
        }
        else
        {
            if (host == "*" && m_AddrFamily == AddressFamily::ADDR_IP)
            {
                result = Bind(IPv4Address(host, url.GetPort()));
            }
            else if (::getaddrinfo(host.c_str(), service.c_str(), nullptr, &dnsResults) != 0)
            {
                result = GetError(GetSocketOSError());
                AMFTraceError(AMF_FACILITY, L"Bind() getaddrinfo() failed url=%S err=%s", url.GetUrl().c_str(), GetErrorString(result));
            }
            else
            {
                for (struct addrinfo* curEntry = dnsResults; curEntry != nullptr; curEntry = curEntry->ai_next)
                {
                    if (curEntry->ai_family == static_cast<int>(m_AddrFamily))
                    {
                        switch (m_AddrFamily)
                        {
                        case AddressFamily::ADDR_IP:
                            result = Bind(IPv4Address(*curEntry->ai_addr));
                            break;
                        default:
                            result = Bind(Address(*curEntry->ai_addr));
                            break;
                        }
                        break;
                    }
                }
                ::freeaddrinfo(dnsResults);
            }
        }
        return result;
    }

    Socket::Result Socket::Send(const void* buf, size_t size, size_t* bytesSent, int flags)
    {
        Socket::Result result = Result::OK;
        int sentCount = 0;

        if (bytesSent != NULL)
        {
            *bytesSent = 0;
        }
        if (m_Socket == INVALID_SOCKET)
        {
            result = Result::SOCKET_NOT_OPEN;
            AMFTraceError(AMF_FACILITY, L"Send() err=%s", GetErrorString(result));
        }
        else if (buf == NULL)
        {
            result = Result::INVALID_ARG;
            AMFTraceError(AMF_FACILITY, L"Send() buffer is NULL err=%s", GetErrorString(result));
        }
        else if (size == 0)
        {
            result = Result::INVALID_ARG;
            AMFTraceError(AMF_FACILITY, L"Send() buffer size is 0 err=%s", GetErrorString(result));
        }
        else
        {
            fd_set writeset;
            FD_ZERO(&writeset);
            FD_SET(m_Socket, &writeset);
            timeval* pTimeout = nullptr;
            timeval timeout = {};
            if (m_Timeout > 0)
            {
                timeout.tv_sec = m_Timeout;
                pTimeout = &timeout;
            }
            int sockCount = ::select(static_cast<int>(m_Socket) + 1, nullptr, &writeset, nullptr, pTimeout);
            if (sockCount == -1)
            {
                int errcode = GetSocketOSError();
                result = GetError(errcode);
                AMFTraceError(AMF_FACILITY, L"Send() select() failed: err=%s socketerr=%d", GetErrorString(result), errcode);
            }
            else if (sockCount == 0)
            {
                result = Result::CONNECTION_TIMEOUT;
            }
            else
            {
                if ((sentCount = ::send(m_Socket, reinterpret_cast<const char*>(buf), (int)size, flags)) == -1)
                {
                    int errcode = GetSocketOSError();
                    result = GetError(errcode);
                    if (result != Result::END_OF_PIPE)
                    {
                        AMFTraceError(AMF_FACILITY, L"Send() send() failed: err=%s socketerr=%d, size=%lu", GetErrorString(result), errcode, (unsigned long)size);
                    }
                }
                else if (bytesSent != nullptr)
                {
                    *bytesSent = sentCount;
                }
            }
        }
        return result;
    }

    Socket::Result Socket::Receive(void* buf, size_t size, size_t* bytesReceived, int flags)
    {
        Socket::Result result = Result::OK;
        int receivedCount = 0;

        if (bytesReceived != nullptr)
        {
            *bytesReceived = 0;
        }
        if (m_Socket == INVALID_SOCKET)
        {
            result = Result::SOCKET_NOT_OPEN;
            AMFTraceError(AMF_FACILITY, L"Receive() err=%s", GetErrorString(result));
        }
        else if (buf == NULL)
        {
            result = Result::INVALID_ARG;
            AMFTraceError(AMF_FACILITY, L"Receive() buffer is NULL err=%s", GetErrorString(result));
        }
        else if (size == 0)
        {
            result = Result::INVALID_ARG;
            AMFTraceError(AMF_FACILITY, L"Receive() buffer size is 0 err=%s", GetErrorString(result));
        }
        else
        {
            if ((receivedCount = ::recv(m_Socket, reinterpret_cast<char*>(buf), (int)size, flags)) == -1)
            {
                int errcode = GetSocketOSError();
                result = GetError(errcode);
                AMFTraceError(AMF_FACILITY, L"Receive() recv() failed: err=%s socketerr=%d", GetErrorString(result), errcode);
            }
            else
            {
                if (receivedCount == 0)
                {
                    result = Result::CONNECTION_CLOSED;
                    AMFTraceDebug(AMF_FACILITY, L"Receive() - connection was closed by peer");
                }
                else if (bytesReceived != nullptr)
                {
                    *bytesReceived = receivedCount;
                }
            }
        }
        return result;
    }

    Socket::Result Socket::SetSocketOpt(int level, int option, const void* value, size_t valueSize)
    {
        Socket::Result result = Result::OK;
        if (m_Socket == INVALID_SOCKET)
        {
            result = Result::SOCKET_NOT_OPEN;
            AMFTraceError(AMF_FACILITY, L"SetSocketOpt() err=%s ", GetErrorString(result));
        }
        else
        {
            if (::setsockopt(m_Socket, level, option, static_cast<const char*>(value), (int)valueSize) != 0)
            {
                int errcode = GetSocketOSError();
                result = GetError(errcode);
                AMFTraceError(AMF_FACILITY, L"SetSocketOpt() setsockopt() failed: err=%s socketerr=%d", GetErrorString(result), errcode);
            }
        }
        return result;
    }

    void Socket::SetTimeout(int seconds)
    {
        m_Timeout = seconds;
#ifdef _WIN32
        DWORD rcvTimeout = seconds * 1000;
        DWORD sndTimeout = seconds * 1000;
#else
        timeval rcvTimeout = {};
        rcvTimeout.tv_sec = seconds;	//	Block for no more than 2 sec

        timeval sndTimeout = {};
        sndTimeout.tv_sec = seconds;	//	Block for no more than 2 sec
#endif

        SetSocketOpt(SOL_SOCKET, SO_RCVTIMEO, &rcvTimeout, sizeof(rcvTimeout));
        SetSocketOpt(SOL_SOCKET, SO_SNDTIMEO, &sndTimeout, sizeof(sndTimeout));
    }
}
