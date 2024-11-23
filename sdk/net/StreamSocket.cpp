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
#include "StreamSocket.h"
#include "amf/public/common/TraceAdapter.h"

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::net::StreamSocket";

namespace ssdk::net
{
	StreamSocket::StreamSocket() :
		Socket(Socket::AddressFamily::ADDR_IP, Socket::Type::TYPE_STREAM, Socket::Protocol::PROTO_TCP)
	{

	}

	StreamSocket::StreamSocket(Socket::Socket_t handle, Socket::Address::Ptr peerAddress) :
		Socket(handle, peerAddress, Socket::AddressFamily::ADDR_IP, Socket::Type::TYPE_STREAM, Socket::Protocol::PROTO_TCP)
	{

	}

	StreamSocket::StreamSocket(Socket::AddressFamily addrFamily, Socket::Protocol protocol) :
		Socket(addrFamily, Socket::Type::TYPE_STREAM, protocol)
	{

	}

	StreamSocket::StreamSocket(Socket::Socket_t handle, Socket::Address::Ptr peerAddress, Socket::AddressFamily addrFamily, Protocol protocol) :
		Socket(handle, peerAddress, addrFamily, Socket::Type::TYPE_STREAM, protocol)
	{

	}

	StreamSocket::~StreamSocket()
	{

	}

	Socket::Result StreamSocket::Listen(int backLog)
	{
		Socket::Result result = Socket::Result::UNKNOWN_ERROR;
		if (m_Socket == INVALID_SOCKET)
		{
			result = Socket::Result::SOCKET_NOT_OPEN;
            AMFTraceError(AMF_FACILITY, L"Listen() err=%s", GetErrorString(result));
        }
		else if (::listen(m_Socket, backLog) != 0)
		{
            int errcode = GetSocketOSError();
            result = GetError(errcode);
            AMFTraceError(AMF_FACILITY, L"Listen() listen() failed: err=%s socketerr=%d", GetErrorString(result), errcode);

		}
		else
		{
			result = Socket::Result::OK;
		}
		return result;
	}

	Socket::Result StreamSocket::Accept(StreamSocket** newSocket)
	{
		Socket::Result result = Socket::Result::UNKNOWN_ERROR;
		if (m_Socket == INVALID_SOCKET)
		{
			result = Socket::Result::SOCKET_NOT_OPEN;
            AMFTraceError(AMF_FACILITY, L"Accept() err=%s", GetErrorString(result));
        }
		else if (m_SocketType != Socket::Type::TYPE_STREAM)
		{
			result = Socket::Result::SOCKET_TYPE_NOT_SUPPORTED;
            AMFTraceError(AMF_FACILITY, L"Accept() err=%s", GetErrorString(result));
        }
		else
		{
			struct sockaddr peerAddr = {};
			socklen_t peerAddrLen = sizeof(peerAddr);
			Socket_t	newSock = ::accept(m_Socket, &peerAddr, &peerAddrLen);
			if (newSock == INVALID_SOCKET)
			{
                int errcode = GetSocketOSError();
                result = GetError(errcode);
                AMFTraceError(AMF_FACILITY, L"Accept() accept() failed: err=%s socketerr=%d", GetErrorString(result), errcode);
            }
			else
			{
                StreamSocket::Ptr socket(CreateSocket(newSock, Socket::Address::Ptr(new Socket::Address(peerAddr))));
				*newSocket = socket.Detach();
				result = Socket::Result::OK;
			}
		}
		return result;
	}

	Socket::Result StreamSocket::ReceiveAll(void* buf, size_t size, size_t* bytesReceived, int flags)
	{
		Socket::Result result = Socket::Result::UNKNOWN_ERROR;
		size_t bytesLeftToRead = size;
		uint8_t* dest = static_cast<uint8_t*>(buf);
		while (bytesLeftToRead > 0 && bytesLeftToRead <= size)
		{
			size_t bytesReceivedNow = 0;
			if ((result = Receive(dest, bytesLeftToRead, &bytesReceivedNow, flags)) != Result::OK)
			{
				break;
			}
			else if (bytesReceivedNow == 0)
			{
				result = Socket::Result::CONNECTION_TIMEOUT;
				break;
			}
			else
			{
				dest += bytesReceivedNow;
				bytesLeftToRead -= bytesReceivedNow;
			}
		}
		if (bytesReceived != nullptr)
		{
			*bytesReceived = size - bytesLeftToRead;
		}
		return result;
	}

	Socket::Result StreamSocket::SendAll(const void* buf, size_t size, size_t* bytesSent, int flags)
	{
		Socket::Result result = Socket::Result::UNKNOWN_ERROR;
		size_t bytesLeftToSend = size;
		const uint8_t* src = static_cast<const uint8_t*>(buf);
		while (bytesLeftToSend > 0)
		{
			size_t bytesSentNow = 0;
			if ((result = Send(src, bytesLeftToSend, &bytesSentNow, flags)) != Socket::Result::OK)
			{
				break;
			}
			else if (bytesSentNow == 0)
			{
				result = Socket::Result::CONNECTION_TIMEOUT;
				break;
			}
			else
			{
				src += bytesSentNow;
				bytesLeftToSend -= bytesSentNow;
				if (bytesLeftToSend > 0)
				{
					AMFTraceInfo(AMF_FACILITY, L"SendAll sent less %lu than requested %lu", bytesSentNow, size);
				}
			}
		}
		if (bytesSent != nullptr)
		{
			*bytesSent = size - bytesLeftToSend;
		}
		return result;

	}

	StreamSocket::Ptr StreamSocket::CreateSocket(Socket::Socket_t handle, Socket::Address::Ptr peerAddress)
	{
		return StreamSocket::Ptr(new StreamSocket(handle, peerAddress));
	}
}