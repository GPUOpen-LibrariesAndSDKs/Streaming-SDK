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
#include "DatagramClient.h"
#include "ClientSession.h"

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::net::DatagramClient";

namespace ssdk::net
{
    DatagramClient::DatagramClient()
    {
    }

    ClientSession::Ptr AMF_STD_CALL DatagramClient::Connect(const Socket::Address& server, Socket* socket, void* params)
    {
        ClientSession::Ptr session;
        session = OnCreateSession(*this, server, socket, params);
        if (session == nullptr)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to create a new UDP client session");
        }
        else
        {
            session->Touch();
        }
        return session;
    }

    ClientSession::Ptr AMF_STD_CALL DatagramClient::Connect(const Url& url, Socket* socket, void* params)
    {
        ClientSession::Ptr session = nullptr;
        Socket::Result sockRes = Socket::Result::INVALID_ADDRESS;
        Socket::Address peer;
        if (url.GetHost() != "*")
        {
/*  GK: This causes a problem on Apple - you cannot call sentto on a connected socket there, but on Windows and Linux/Android
        this works just fine. But realistically we don't even need to call Connect here as it is being called to populate the peer
        address only, can do it by simply copying it from the URL.

            sockRes = socket->Connect(url);
            if (sockRes == Socket::OK)
            {
                peer = *(socket->GetPeerAddress());
            }
*/
            peer = Socket::IPv4Address(url.GetHost(), url.GetPort());
            sockRes = Socket::Result::OK;
        }
        else
        {
            if (socket->GetAddressFamily() == Socket::AddressFamily::ADDR_IP)
            {
                peer = Socket::IPv4Address("255.255.255.255", url.GetPort());
                sockRes = Socket::Result::OK;
            }
        }
        if (sockRes != Socket::Result::OK)
        {
			AMFTraceError(AMF_FACILITY, L"Failed to connect");
        }
        else
        {
            session = Connect(peer, socket, params);
        }
        return session;
    }

    ClientSession::Ptr AMF_STD_CALL DatagramClient::Connect(const Url& url, void* params)
    {
        m_ConnectSocket = new DatagramSocket();
        Socket::Ptr sock(m_ConnectSocket);
        return Connect(url, sock, params);
    }

    ClientSession::Ptr AMF_STD_CALL DatagramClient::Connect(const Socket::Address& server, void* params)
    {
        m_ConnectSocket = new DatagramSocket();
        Socket::Ptr sock(m_ConnectSocket);
        return Connect(server, sock, params);
    }

}
