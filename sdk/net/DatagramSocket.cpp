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
#include "DatagramSocket.h"
#include "amf/public/common/TraceAdapter.h"
#include <sstream>

#include <iphlpapi.h>
#include <ws2tcpip.h>

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::net::DatagramSocket";

#define NIC_LIST_UPDATE_INTERVAL    1   //  In seconds. A call to query all network adapters on Windows is relatively slow - up to 25ms on slower CPUs like Carrizo

namespace ssdk::net
{
    DatagramSocket::AddressVector   DatagramSocket::m_MyNICs;
    time_t DatagramSocket::m_NICsUpdatedTime = 0;
    time_t DatagramSocket::m_NICsUpdateInterval = NIC_LIST_UPDATE_INTERVAL;
    amf::AMFCriticalSection DatagramSocket::m_Guard;


    DatagramSocket::DatagramSocket(AddressFamily addrFamily, Protocol protocol) :
        Socket(addrFamily, Socket::Type::TYPE_DATAGRAM, protocol)
    {
//        EnumerateNICs();
    }

    DatagramSocket::Result DatagramSocket::SendTo(const void* buf, size_t size, const Socket::Address& to, size_t* bytesSent, int flags)
    {
        DatagramSocket::Result result = DatagramSocket::Result::OK;
        int sentCount = 0;

        if (m_Socket == INVALID_SOCKET)
        {
            result = Socket::Result::SOCKET_NOT_OPEN;
            AMFTraceError(AMF_FACILITY, L"SendTo() err=%s", GetErrorString(result));
        }
        else if (buf == nullptr)
        {
            result = Socket::Result::INVALID_ARG;
            AMFTraceError(AMF_FACILITY, L"SendTo() buffer is NULL err=%s", GetErrorString(result));
        }
        else if (size == 0)
        {
            result = Socket::Result::INVALID_ARG;
            AMFTraceError(AMF_FACILITY, L"SendTo() buffer size is 0 err=%s", GetErrorString(result));
        }
        else if (to.GetAddressFamily() != m_AddrFamily)
        {
            result = Socket::Result::INVALID_ARG;
            AMFTraceError(AMF_FACILITY, L"SendTo() invalid address family err=%s", GetErrorString(result));
        }
        else if ((sentCount = ::sendto(m_Socket, reinterpret_cast<const char*>(buf), (int)size, flags, &to.ToSockAddr(), (int)to.GetSize())) <= 0)
        {
            result = GetError(GetSocketOSError());
            AMFTraceError(AMF_FACILITY, L"SendTo() sendto() failed err=%s", GetErrorString(result));
        }
        else if (bytesSent != nullptr)
        {
            //AMFTraceDebug(AMF_FACILITY, L"DatagramSocket::SendTo() bytesSent=%d", sentCount);
            *bytesSent = sentCount;
        }
        return result;
    }

    DatagramSocket::Result DatagramSocket::ReceiveFrom(void* buf, size_t size, Socket::Address* from, size_t* bytesReceived, int flags)
    {
        DatagramSocket::Result result = DatagramSocket::Result::OK;
        int receivedCount = 0;

        if (m_Socket == INVALID_SOCKET)
        {
            result = Socket::Result::SOCKET_NOT_OPEN;
            AMFTraceError(AMF_FACILITY, L"ReceiveFrom() err=%s", GetErrorString(result));
        }
        else if (buf == nullptr)
        {
            result = Socket::Result::INVALID_ARG;
            AMFTraceError(AMF_FACILITY, L"ReceiveFrom() buffer is NULL err=%s", GetErrorString(result));
        }
        else if (size == 0)
        {
            result = Socket::Result::INVALID_ARG;
            AMFTraceError(AMF_FACILITY, L"ReceiveFrom() buffer size is 0 err=%s", GetErrorString(result));
        }
        else
        {
            struct sockaddr* fromAddr = nullptr;
            socklen_t addrSize = 0;
            if (from != nullptr)
            {
                fromAddr = &(from->ToSockAddr());
                addrSize = (int)from->GetSize();
            }
            if ((receivedCount = ::recvfrom(m_Socket, reinterpret_cast<char*>(buf), (int)size, flags, fromAddr, &addrSize)) <= 0)
            {
                int errcode = GetSocketOSError();
                result = GetError(errcode);
                AMFTraceError(AMF_FACILITY, L"ReceiveFrom() recvfrom() failed: err=%s socketerr=%d", GetErrorString(result), errcode);
            }
            else if (bytesReceived != nullptr)
            {
                //AMFTraceDebug(AMF_FACILITY, L"DatagramSocket::ReceiveFrom() receivedCount=%d", receivedCount);
                *bytesReceived = receivedCount;
            }
        }
        return result;
    }

    void DatagramSocket::SetNICDataExpiration(time_t expirationSec)
    {
        amf::AMFLock    lock(&m_Guard);
        m_NICsUpdatedTime = expirationSec;
    }

    bool DatagramSocket::EnumerateNICs()
    {
        bool result = true;
        time_t now = time(nullptr);
        amf::AMFLock    lock(&m_Guard);
        if (now - m_NICsUpdatedTime >= NIC_LIST_UPDATE_INTERVAL)
        {
            m_NICsUpdatedTime = now;
            m_MyNICs.clear();
            DWORD rv, size = 100 * 1024;    //  100k buffer by default, should be enough for absolute majority of cases
            PIP_ADAPTER_ADDRESSES adapterAddresses;
            ULONG family = AF_INET;
            ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_INCLUDE_ALL_INTERFACES;
            bool retry = false;
            std::unique_ptr<uint8_t[]> buf;
            do
            {
                buf.reset(new uint8_t[size]);
                adapterAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buf.get());
                rv = GetAdaptersAddresses(family, flags, nullptr, adapterAddresses, &size);
                switch (rv)
                {
                case ERROR_BUFFER_OVERFLOW:
                    retry = true;
                    result = false;
                    break;
                case ERROR_SUCCESS:
                    result = true;
                    retry = false;
                    break;
                default:
                    AMFTraceError(AMF_FACILITY, L"GetAdaptersAddresses() failed, error code %d", rv);
                    result = false;
                    retry = false;
                    break;
                }
            } while (retry == true);

            if (result == true)
            {
                for (PIP_ADAPTER_ADDRESSES aa = adapterAddresses; aa != nullptr; aa = aa->Next)
                {
                    if (aa->OperStatus == IfOperStatusUp && aa->FirstUnicastAddress != nullptr)
                    {
                        for (PIP_ADAPTER_UNICAST_ADDRESS multicastAddr = aa->FirstUnicastAddress; multicastAddr != nullptr; multicastAddr = multicastAddr->Next)
                        {
                            unsigned long mask;
                            ConvertLengthToIpv4Mask(multicastAddr->OnLinkPrefixLength, &mask);
                            if (mask != 0xFFFFFFFF)
                            {
                                *(reinterpret_cast<unsigned long*>(&reinterpret_cast<sockaddr_in*>(multicastAddr->Address.lpSockaddr)->sin_addr)) |= (~mask);

                                Socket::Address address(*multicastAddr->Address.lpSockaddr);
                                m_MyNICs.push_back(address);
                                std::string addressStr = address.GetParsedAddressAsString();
                                AMFTraceDebug(AMF_FACILITY, L"Added broadcast interface %S, address %S", aa->AdapterName, addressStr.c_str());
                            }
                        }
                    }
                }
            }
        }
        return result;
    }

    DatagramSocket::Result DatagramSocket::Broadcast(const void* buf, size_t size, unsigned short port, size_t* bytesSent, int flags)
    {
        DatagramSocket::Result result = DatagramSocket::Result::OK;
        AddressVector myNICs;

        EnumerateNICs();
        {
            amf::AMFLock    lock(&m_Guard);
            myNICs = m_MyNICs;
        }
        for (AddressVector::const_iterator it = myNICs.begin(); it != myNICs.end(); ++it)
        {
            Socket::IPv4Address address(*it);
            address.SetIPPort(port);
            result = SendTo(buf, size, address, bytesSent, flags);
        }

        return result;
    }
}
