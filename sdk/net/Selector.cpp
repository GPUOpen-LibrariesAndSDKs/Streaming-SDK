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
#include "Selector.h"
#include "amf/public/common/TraceAdapter.h"
#include <algorithm>
#ifndef _WIN32
#include <sys/time.h>
#endif

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::net::Selector";

namespace ssdk::net
{
    Selector::Selector() :
        m_HighestReadableNative(0),
        m_HighestWritableNative(0)
    {
    }

    Selector::Selector(const Socket::Set* const readable, const Socket::Set* const writable) :
        m_HighestReadableNative(0),
        m_HighestWritableNative(0)
    {
        if (readable != nullptr)
        {
            for (Socket::Set::const_iterator it = readable->begin(); it != readable->end(); ++it)
            {
                Result res = AddReadableSocket(*it);
                if (res != Result::OK)
                {
                    throw Exception(res);
                }
            }
        }
        if (writable != nullptr)
        {
            for (Socket::Set::const_iterator it = writable->begin(); it != writable->end(); ++it)
            {
                Result res = AddWritableSocket(*it);
                if (res != Result::OK)
                {
                    throw Exception(res);
                }
            }
        }
    }

    Selector::~Selector()
    {
    }

	bool Selector::AddSocketToSet(Socket* socket, Socket::Set& socketSet, Socket::Socket_t& highestNativeHandle)
	{
		bool result = false;
		if (socketSet.insert(socket).second == true)
		{
			Socket::Socket_t native = socket->GetNativeHandle();
			if (native > highestNativeHandle)
			{
				highestNativeHandle = native;
			}
			result = true;
		}
		return result;
	}

    Selector::Result Selector::AddReadableSocket(Socket* socket)
    {
        Selector::Result result = Result::INVALID_ARG;
        if (socket == nullptr)
        {
            result = Result::INVALID_ARG;
			AMFTraceError(AMF_FACILITY, L"AddReadableSocket - NULL pointer to socket");
        }
        else
        {
			AddSocketToSet(socket, m_ReadableSockets, m_HighestReadableNative);
            result = Result::OK;
        }
        return result;
    }

    Selector::Result Selector::AddWritableSocket(Socket* socket)
    {
        Selector::Result result = Result::INVALID_ARG;
        if (socket == nullptr)
        {
            result = Result::INVALID_ARG;
			AMFTraceError(AMF_FACILITY, L"AddWritableSocket - NULL pointer to socket");
        }
        else
        {
			AddSocketToSet(socket, m_WritableSockets, m_HighestWritableNative);
            result = Result::OK;
        }
        return result;
    }

    void Selector::SocketSetToFDSet(const Socket::Set& in, fd_set* out)
    {
        FD_ZERO(out);
        int count = 0;
        for (Socket::Set::const_iterator it = in.begin(); it != in.end() && count < FD_SETSIZE; ++it, ++count)
        {
            Socket::Socket_t   nativeSocket = (*it)->GetNativeHandle();

            if (nativeSocket != INVALID_SOCKET)
            {
                FD_SET(nativeSocket, out);
            }
        }
    }

    size_t Selector::FindReadySockets(const Socket::Set& in, const fd_set* native, Socket::Set& ready)
    {
        size_t count = 0;
        ready.clear();
        for (Socket::Set::const_iterator it = in.begin(); it != in.end(); ++it)
        {
            Socket::Socket_t   nativeSocket = (*it)->GetNativeHandle();
            if (nativeSocket != INVALID_SOCKET && FD_ISSET(nativeSocket, const_cast<fd_set*>(native)) != 0)
            {
                ready.insert(*it);
                ++count;
            }
        }
        return count;
    }

    void Selector::TimeToTimeVal(time_t in, struct timeval& out)
    {
        memset(&out, 0, sizeof(out));
        out.tv_sec = static_cast<long>(in);
    }

    Selector::Result Selector::WaitToRead(struct timeval& timeout, Socket::Set& readable)
    {
        Result res = Result::OK;
        fd_set nativeSockets;

        SocketSetToFDSet(m_ReadableSockets, &nativeSockets);
        int retVal = ::select((int)std::max<Socket::Socket_t>(m_HighestReadableNative, m_HighestWritableNative) + 1, &nativeSockets, nullptr, nullptr, &timeout);

        if (retVal > 0 && FindReadySockets(m_ReadableSockets, &nativeSockets, readable) == 0)
        {
            //AMFTraceInfo(AMF_FACILITY, L"Selector::WaitToRead() no socket avaiable");
            res = Result::EMPTY_SET;
        }
        else if (retVal == 0)
        {
            //AMFTraceInfo(AMF_FACILITY, L"Socket timeout");
            res = Result::TIMEOUT;
        }
        else if (retVal == -1)
        {
            //AMFTraceError(AMF_FACILITY, L"WaitToRead - select failed");
            res = Result::SELECT_FAILED;
        }

        return res;
    }

    Selector::Result Selector::WaitToWrite(struct timeval& timeout, Socket::Set& writable)
    {
        Result res = Result::TIMEOUT;
        fd_set nativeSockets;
        SocketSetToFDSet(m_WritableSockets, &nativeSockets);

        int retVal = ::select((int)std::max<Socket::Socket_t>(m_HighestReadableNative, m_HighestWritableNative) + 1, nullptr, &nativeSockets, nullptr, &timeout);

        if (retVal == -1)
        {
//			AMFTraceError(AMF_FACILITY, L"WaitToWrite - select failed");
			res = Result::SELECT_FAILED;
        }
        else if (retVal > 0)
        {
            if (FindReadySockets(m_WritableSockets, &nativeSockets, writable) == 0)
            {
				AMFTraceError(AMF_FACILITY, L"WaitToWrite - select succeeded, but no ready sockets were returned");
				res = Result::EMPTY_SET;
            }
            else
            {
                res = Result::OK;
            }
        }
        return res;
    }

    Selector::Result Selector::WaitToReadAndWrite(struct timeval& timeout, Socket::Set& readable, Socket::Set& writable)
    {
        Result res = Result::TIMEOUT;
        fd_set nativeReadableSockets;
        SocketSetToFDSet(m_ReadableSockets, &nativeReadableSockets);
        fd_set nativeWritableSockets;
        SocketSetToFDSet(m_WritableSockets, &nativeWritableSockets);

        int retVal = ::select((int)std::max<Socket::Socket_t>(m_HighestReadableNative, m_HighestWritableNative) + 1, &nativeReadableSockets, &nativeWritableSockets, nullptr, &timeout);

        if (retVal == -1)
        {
			AMFTraceError(AMF_FACILITY, L"WaitToReadAndWrite - select failed");
			res = Result::SELECT_FAILED;
        }
        else if (retVal > 0)
        {
            if (FindReadySockets(m_ReadableSockets, &nativeReadableSockets, readable) == 0 &&
                FindReadySockets(m_WritableSockets, &nativeWritableSockets, writable))
            {
				AMFTraceError(AMF_FACILITY, L"WaitToReadAndWrite - select succeeded, but no ready sockets were returned");
				res = Result::EMPTY_SET;
            }
            else
            {
                res = Result::OK;
            }
        }
        return res;
    }
}
