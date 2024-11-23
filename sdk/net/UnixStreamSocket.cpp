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

#include "UnixStreamSocket.h"
#include "amf/public/common/TraceAdapter.h"
#include <errno.h>

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::net::UnixStreamSocket";

#if defined(__linux)
namespace ssdk::net
{
	UnixStreamSocket::UnixStreamSocket() :
		StreamSocket(Socket::AddressFamily::ADDR_UNIX, Socket::Protocol::PROTO_UNIX)
	{

	}

	UnixStreamSocket::UnixStreamSocket(Socket::Socket_t handle, Socket::Address::Ptr peerAddress) :
		StreamSocket(handle, peerAddress, Socket::AddressFamily::ADDR_UNIX, Socket::Protocol::PROTO_UNIX)
	{

	}

	UnixStreamSocket::~UnixStreamSocket()
	{

	}

	Socket::Result UnixStreamSocket::SendFd(int fd)
	{
		struct msghdr msg = {};
		msg.msg_name = nullptr;
		msg.msg_namelen = 0;

		char data = 'f'; //the data doesn't matter, what matters is the ancillary data attached
		struct iovec iov = {};
		iov.iov_base = &data;
		iov.iov_len = sizeof(char);
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;

		char cms[CMSG_SPACE(sizeof(int))] = {};
		msg.msg_control = (caddr_t) cms;
		msg.msg_controllen = sizeof(cms);

		struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		cmsg->cmsg_len = CMSG_LEN(sizeof(int));
		AMFTraceInfo(AMF_FACILITY, L"fd=%d", fd);
		*reinterpret_cast<int*>(CMSG_DATA(cmsg)) = fd;

		if (sendmsg(m_Socket, &msg, 0) != 1)
		{
			AMFTraceError(AMF_FACILITY, L"sendmsg failed, error=%S", strerror(errno));
			return Result::UNKNOWN_ERROR;
		}

		return Result::OK;
	}

	Socket::Result UnixStreamSocket::RecvFd(int* fd)
	{
		struct msghdr msg = {};
		msg.msg_name = nullptr;
		msg.msg_namelen = 0;

		char data;
		struct iovec iov = {};
		iov.iov_base = &data;
		iov.iov_len = sizeof(char);
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;

		char cms[CMSG_SPACE(sizeof(int))] = {};
		msg.msg_control = (caddr_t) cms;
		msg.msg_controllen = sizeof(cms);

		if (recvmsg(m_Socket, &msg, 0) != 1)
		{
			AMFTraceError(AMF_FACILITY, L"recvmsg failed, error=%S", strerror(errno));
			return Result::UNKNOWN_ERROR;
		}

		for (struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg); cmsg != nullptr; cmsg = CMSG_NXTHDR(&msg, cmsg))
		{
			if((cmsg->cmsg_level == SOL_SOCKET) && (cmsg->cmsg_type == SCM_RIGHTS))
			{

				*fd = *reinterpret_cast<int*>(CMSG_DATA(cmsg));
				return Result::OK;
			}
		}

		//couldn't find FD
		return Result::UNKNOWN_ERROR;
	}

	StreamSocket::Ptr UnixStreamSocket::CreateSocket(Socket::Socket_t handle, Socket::Address::Ptr peerAddress)
	{
		return StreamSocket::Ptr(new UnixStreamSocket(handle, peerAddress));
	}
}
#endif
