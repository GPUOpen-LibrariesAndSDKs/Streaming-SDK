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
#include "amf/public/common/TraceAdapter.h"
#include "Socket.h"
#include <sstream>
#include <iomanip>

namespace ssdk::net
{
    //  *************************************** Socket::Address *********************************************
    Socket::Address::Address()
    {
        m_SockAddr.ss_family = AF_UNSPEC;
    }

    Socket::Address::Address(const Socket::Address& other)
    {
        operator=(other);
    }

    Socket::Address::Address(const sockaddr& other)
    {
        operator=(other);
    }

    Socket::Address& Socket::Address::operator=(const Socket::Address& other)
    {
        m_SockAddr = other.m_SockAddr;
        return *this;
    }

    Socket::Address& Socket::Address::operator=(const sockaddr& other)
    {
        memcpy(reinterpret_cast<struct sockaddr*>(&m_SockAddr), &other, sizeof(sockaddr));
        return *this;
    }

    bool Socket::Address::operator==(const Socket::Address& other) const
    {
        return operator==(other.ToSockAddr());
    }

    bool Socket::Address::operator==(const sockaddr& other) const
    {
        return memcmp(reinterpret_cast<const struct sockaddr*>(&m_SockAddr), &other, sizeof(sockaddr)) == 0;
    }

    bool Socket::Address::operator<(const Address& other) const
    {
        return memcmp(&m_SockAddr, &(other.m_SockAddr), sizeof(m_SockAddr)) < 0;
    }

    size_t Socket::Address::GetSize() const
    {
        if (m_SockAddr.ss_family == AF_INET)
        {
            return sizeof(sockaddr_in);
        }
        return sizeof(sockaddr);
    }

    static const char* GetAddressFamilyAsString(unsigned short af)
    {
        struct AddressFamilyMapping_t {
            unsigned short  af;
            const char* af_name;
        };

        static const AddressFamilyMapping_t AddressFamilyMapping[] =
        {
            { AF_UNIX,      "Unix"          },
            { AF_INET,      "IPv4"          },
            { AF_INET6,     "IPv6"          }
        };
        static const char* invalidAddressFamily = "Invalid";

        const char* addressFamily = invalidAddressFamily;
        for (const AddressFamilyMapping_t& afMapping : AddressFamilyMapping)
        {
            if (af == afMapping.af)
            {
                addressFamily = afMapping.af_name;
                break;
            }
        }
        return addressFamily;
    }

    std::string Socket::Address::GetAddressAsString() const
    {
        std::stringstream stream;

//        stream << std::string(inet_ntoa(reinterpret_cast<const sockaddr_in&>(m_SockAddr).sin_addr)) << ":" << ntohs(reinterpret_cast<const sockaddr_in&>(m_SockAddr).sin_port);

//        stream << "Family(" << m_SockAddr.ss_family << "), Address(";
        stream << "Family(" << GetAddressFamilyAsString(m_SockAddr.ss_family) << "), Address(";

        const struct sockaddr* sockAddr = reinterpret_cast<const struct sockaddr*>(&m_SockAddr);
        const unsigned char* ptr = reinterpret_cast<const unsigned char*>(&sockAddr->sa_data);
        for (int i = 0; i < sizeof(sockAddr->sa_data); ++i, ++ptr)
        {
            if (i > 0)
            {
                stream << ':';
            }
            unsigned int byteVal = *ptr;
            stream << std::setfill('0') << std::setw(2) << byteVal;
        }
        stream << ")";

        return stream.str();
    }

    std::string Socket::Address::GetParsedAddressAsString() const
    {
        std::stringstream stream;
        const sockaddr_in &addIn = reinterpret_cast<const sockaddr_in&>(m_SockAddr);
//        stream << "Family(" << addIn.sin_family << "), Address(";
        stream << "Family(" << GetAddressFamilyAsString(addIn.sin_family) << "), Address(";

        const unsigned char* ptr = reinterpret_cast<const unsigned char*>(&addIn.sin_addr);
        for (int i = 0; i < sizeof(addIn.sin_addr); ++i, ++ptr)
        {
            if (i > 0)
            {
                stream << '.';
            }
            unsigned int byteVal = *ptr;
            stream << std::setfill('0') << std::setw(2) << byteVal;
        }
        stream << ":" << ntohs(addIn.sin_port) << ")";

        return stream.str();
    }

    bool Socket::Address::IsBroadcast() const
    {
        bool result = false;
        if (m_SockAddr.ss_family == AF_INET)
        {
            const sockaddr_in& ipaddr = reinterpret_cast<const sockaddr_in&>(m_SockAddr);
            if (ipaddr.sin_addr.s_addr == INADDR_BROADCAST)
            {
                result = true;
            }
        }
        return result;
    }

    //  *************************************** Socket::IPv4Address *********************************************
    const wchar_t* const Socket::IPv4Address::m_Realm = L"Socket::IPv4Address";
    Socket::IPv4Address::IPv4Address()
    {
        m_SockAddr.ss_family = AF_INET;
    }

    Socket::IPv4Address::IPv4Address(const std::string& host, unsigned short port)
    {
        m_SockAddr.ss_family = AF_INET;
        if (host != "*" && host.empty() == false)
        {
            struct addrinfo* dnsResults = NULL;
            std::stringstream service;
            service << port;
            if (::getaddrinfo(host.c_str(), service.str().c_str(), NULL, &dnsResults) != 0)
            {
				AMFTraceError(m_Realm, L"IPv4Address - getaddrinfo() failed");
            }
            else
            {
                for (struct addrinfo* curEntry = dnsResults; curEntry != NULL; curEntry = curEntry->ai_next)
                {
                    if (curEntry->ai_family == AF_INET)
                    {
                        operator=(*curEntry->ai_addr);
                    }
                }
            }
        }
        else
        {
            reinterpret_cast<struct sockaddr_in&>(m_SockAddr).sin_addr.s_addr = INADDR_ANY;
            reinterpret_cast<struct sockaddr_in&>(m_SockAddr).sin_port = ::htons(port);
        }
    }

    Socket::IPv4Address::IPv4Address(const IPv4Address& other) : Socket::Address::Address(other)
    {
        operator=(other);
    }

    Socket::IPv4Address::IPv4Address(const Address& other)
    {
        operator=(other);
    }

    Socket::IPv4Address::IPv4Address(const sockaddr& other)
    {
        operator=(other);
    }

    Socket::IPv4Address::IPv4Address(const sockaddr_in& other)
    {
        operator=(other);
    }

    Socket::IPv4Address& Socket::IPv4Address::operator=(const IPv4Address& other)
    {
        return operator=(other.ToSockAddr_In());
    }

    Socket::IPv4Address& Socket::IPv4Address::operator=(const Address& other)
    {
        return operator=(other.ToSockAddr());
    }

    Socket::IPv4Address& Socket::IPv4Address::operator=(const sockaddr& other)
    {
        if (other.sa_family != AF_INET)
        {
			AMFTraceError(m_Realm, L"Invalid address conversion to IPv4");
			return *this;
        }
        return operator=(reinterpret_cast<const sockaddr_in&>(other));
    }

    Socket::IPv4Address& Socket::IPv4Address::operator=(const sockaddr_in& other)
    {
        memcpy(reinterpret_cast<struct sockaddr_in*>(&m_SockAddr), &other, sizeof(struct sockaddr_in));
        return *this;
    }

    bool Socket::IPv4Address::operator==(const sockaddr& other) const
    {
        return (m_SockAddr.ss_family == other.sa_family &&
            reinterpret_cast<const sockaddr_in&>(m_SockAddr).sin_port == reinterpret_cast<const sockaddr_in&>(other).sin_port &&
            memcmp(&reinterpret_cast<const sockaddr_in&>(m_SockAddr).sin_addr, &reinterpret_cast<const sockaddr_in&>(other).sin_addr, sizeof(struct in_addr)) == 0);
    }

    bool Socket::IPv4Address::operator==(const sockaddr_in& other) const
    {
        return (m_SockAddr.ss_family == other.sin_family &&
            reinterpret_cast<const sockaddr_in&>(m_SockAddr).sin_port == other.sin_port &&
            memcmp(&reinterpret_cast<const sockaddr_in&>(m_SockAddr).sin_addr, &other.sin_addr, sizeof(struct in_addr)) == 0);
    }

    bool Socket::IPv4Address::operator==(const Address& other) const
    {
        return operator==(other.ToSockAddr());
    }

    bool Socket::IPv4Address::operator==(const IPv4Address& other) const
    {
        return operator==(other.ToSockAddr_In());
    }

    unsigned short Socket::IPv4Address::GetPort() const
    {
        return ::ntohs(reinterpret_cast<const sockaddr_in&>(m_SockAddr).sin_port);
    }

    void Socket::IPv4Address::SetIPPort(unsigned short port)
    {
        reinterpret_cast<sockaddr_in&>(m_SockAddr).sin_port = ::htons(port);
    }

    std::string Socket::IPv4Address::GetAddressAsString() const
    {
        std::stringstream stream;
        stream << std::string(inet_ntoa(reinterpret_cast<const sockaddr_in&>(m_SockAddr).sin_addr)) << ":" << ntohs(reinterpret_cast<const sockaddr_in&>(m_SockAddr).sin_port);
        return stream.str();
    }

    void Socket::IPv4Address::SetAddress(const std::string& address)
    {
        reinterpret_cast<sockaddr_in&>(m_SockAddr).sin_addr.s_addr = inet_addr(address.c_str());
    }
}
