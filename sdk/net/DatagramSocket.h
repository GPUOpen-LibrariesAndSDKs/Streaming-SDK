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

#include <vector>
#include <ctime>
#include "Socket.h"

namespace ssdk::net
{
    class DatagramSocket :
        public Socket
    {
    public:
        typedef amf::AMFInterfacePtr_T<DatagramSocket> Ptr;
        AMF_DECLARE_IID(0xa5c7af69, 0xa5a5, 0x4fe3, 0x92, 0x75, 0x45, 0xee, 0xf1, 0x10, 0xe8, 0xcd);

        AMF_BEGIN_INTERFACE_MAP
            AMF_INTERFACE_ENTRY(DatagramSocket)
            AMF_INTERFACE_CHAIN_ENTRY(Socket)
        AMF_END_INTERFACE_MAP

    public:
        DatagramSocket(AddressFamily addrFamily = Socket::AddressFamily::ADDR_IP, Protocol protocol = Socket::Protocol::PROTO_UDP);

        virtual Result SendTo(const void* buf, size_t size, const Socket::Address& to, size_t* bytesSent, int flags = 0);
        virtual Result ReceiveFrom(void* buf, size_t size, Socket::Address* from, size_t* bytesReceived, int flags = 0);

        virtual Result Broadcast(const void* buf, size_t size, unsigned short port, size_t* bytesSent, int flags = 0);

        static void SetNICDataExpiration(time_t expirationSec); //  Set how frequently Broadcast should check for changes in NICs
                                                                //  Setting it to 0 forces it to check for changes on every call.
                                                                //  Use 0 carefully as it might take up to 25ms on Windows, so
                                                                //  set to a higher value when broadcasting a lot of data. Default 1 sec

    private:
        DatagramSocket(const DatagramSocket&) = delete;
        DatagramSocket& operator=(const DatagramSocket&) = delete;

        bool EnumerateNICs();

    private:
        typedef std::vector<Socket::Address>    AddressVector;
        static AddressVector   m_MyNICs;
        static amf::AMFCriticalSection m_Guard;
        static time_t m_NICsUpdatedTime;
        static time_t m_NICsUpdateInterval;
    };
}
