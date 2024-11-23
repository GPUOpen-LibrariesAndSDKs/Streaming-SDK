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
#include "DatagramClientSession.h"
#include "DatagramSocket.h"
#include "Selector.h"
#include "amf/public/common/TraceAdapter.h"
#include <sstream>

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::net::DatagramClientSession";

namespace ssdk::net
{
    DatagramClientSession::DatagramClientSession(Socket* socket, const Socket::Address& peer, size_t receiveBufSize) :
        ClientSession(socket, peer, receiveBufSize)
    {
    }

    DatagramClientSession::Result AMF_STD_CALL DatagramClientSession::ReadIncoming(Socket* sock, char* buf, size_t bufSize, size_t* const bytesReceived, Socket::Address* const receivedFrom)
    {
        Result result = Result::OK;
        Socket::Result sockResult = DatagramSocket::Ptr(sock)->ReceiveFrom(buf, bufSize, receivedFrom, bytesReceived);
        switch (sockResult)
        {
        case Socket::Result::OK:
            if (receivedFrom != nullptr && *receivedFrom != GetPeerAddress() && GetPeerAddress().IsBroadcast() == false)
            {
                result = Result::INVALID_PEER;
            }
            break;
        default:
            {
				AMFTraceError(AMF_FACILITY, L"ReadIncoming: Failed to receive a message, socket error=%d", sockResult);
                result = Result::RECEIVE_FAILED;
            }
            break;
        }
        return result;
    }

	ClientSession::Result AMF_STD_CALL DatagramClientSession::ProcessIncomingMessages()
	{
		ClientSession::Result result = Result::OK;

		size_t bytesReceived = 0;
		Socket::Address receivedFrom;
		result = ReadIncoming(m_Socket, m_ReceiveBuf.get(), m_ReceiveBufSize, &bytesReceived, &receivedFrom);
		if (result == Result::OK)
		{
            Touch();
			result = OnDataReceived(m_ReceiveBuf.get(), bytesReceived, receivedFrom);
		}
		else
		{
			AMFTraceError(AMF_FACILITY, L"ProcessIncomingMessages: Failed to receive a message, socket error=%d", result);
		}
		return result;
	}
}
