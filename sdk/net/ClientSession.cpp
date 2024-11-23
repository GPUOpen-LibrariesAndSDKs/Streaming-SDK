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
#include "ClientSession.h"
#include "Selector.h"
#include "amf/public/common/TraceAdapter.h"
#include <sstream>

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::net::ClientSession";

namespace ssdk::net
{
    ClientSession::ClientSession(Socket* socket, const Socket::Address& peer, size_t receiveBufSize) :
        Session(peer),
		m_Socket(socket),
        m_ReceiveBufSize(receiveBufSize),
        m_TxMaxFragmentSize(receiveBufSize),
        m_Terminated(false),
		m_Timeout(100)
    {
        m_ReceiveBuf = std::unique_ptr<char[]>(new char[m_ReceiveBufSize]);
    }
    void            AMF_STD_CALL AMF_STD_CALL ClientSession::SetReceiveBufSize(size_t receiveBufSize)
    {
        m_ReceiveBufSize = receiveBufSize;
        m_ReceiveBuf = std::unique_ptr<char[]>(new char[m_ReceiveBufSize]);
    }

    void            AMF_STD_CALL AMF_STD_CALL ClientSession::SetTxMaxFragmentSize(size_t txMaxFragmentSize)
    {
        m_TxMaxFragmentSize = txMaxFragmentSize;
    }

    void           AMF_STD_CALL ClientSession::Terminate()
    {
        std::lock_guard<std::recursive_mutex> lock(m_Guard);
        m_Socket->Close();
        m_Terminated = true;
        Session::Terminate();
    }

    ClientSession::Result AMF_STD_CALL ClientSession::WaitForIncoming()
    {
        ClientSession::Result result = Result::OK;
        Selector selector;
        selector.AddReadableSocket(m_Socket);
        Socket::Set readableSockets;
        struct timeval timeout_tv = {};
        timeout_tv.tv_usec = 1000 * COMM_SELECTOR_FLUSH_INTERVAL_IN_MS; // e.g 120 ms

        do {
            {   //  Check if the client is being terminated before a call to select so that we wouldn't have to wait for something to be received or timeout
                std::lock_guard<std::recursive_mutex> lock(m_Guard);
                if (m_Terminated == true)
                {
                    result = Result::CONNECTION_TERMINATED;
                    break;
                }
            }
            Selector::Result selectorResult = selector.WaitToRead(timeout_tv, readableSockets);
            {   //  GK: We need to check whether the client has been terminated while we were waiting inside WaitToRead
                std::lock_guard<std::recursive_mutex> lock(m_Guard);
                if (m_Terminated == true)
                {
                    result = Result::CONNECTION_TERMINATED;
                    break;
                }
            }
            switch (selectorResult)
            {
            case Selector::Result::OK:
                result = ProcessIncomingMessages();
                if (result != Result::OK)
                {
                    OnSessionTimeout();
                    result = Result::RECEIVE_FAILED;
                }
                break;
            case Selector::Result::TIMEOUT:
                if (GetElapsedTimeSinceLastRequest() >= m_Timeout)
                {
                    OnSessionTimeout();
                    result = Result::TIMEOUT;
                }
                else
                {
                    // Check whether a message in the queue was processed
                    if (OnTickNotify() == true)
                    {
                        result = Result::OK;
                    }
                    else
                    {
                        result = Result::TICK;
                    }
                }
                break;
            default:
                OnSessionTimeout();
                result = Result::RECEIVE_FAILED;
                break;
            }
        } while (result == Result::TICK);

        return result;
    }

    Socket::Result AMF_STD_CALL ClientSession::Send(const void* buf, size_t size, size_t* const bytesSent, int flags)
    {
        Socket::Result result = Socket::Result::UNKNOWN_ERROR;
        {
            Selector selector;
			selector.AddWritableSocket(m_Socket);
            Socket::Set readyToSend;
            struct timeval timeout_tv = {};
            timeout_tv.tv_sec = (long) m_Timeout; //MM timeout - if we cannot send - something is wrong.
            bool retry = false;
            do
            {
                retry = false;
                switch (selector.WaitToWrite(timeout_tv, readyToSend))
                {
                case Selector::Result::OK:
                    if (readyToSend.size() > 0)
                    {
                        result = m_Socket->Send(buf, size, bytesSent, flags);
                    }
                    else
                    {
                        retry = true;
                    }
                    break;
                case Selector::Result::TIMEOUT:
                    result = Socket::Result::CONNECTION_TIMEOUT;
                    break;
                case Selector::Result::INVALID_ARG:
                    result = Socket::Result::INVALID_ARG;
                    break;
                default:
                    result = Socket::Result::INVALID_ARG;
                    break;
                }
            } while (retry == true);
        }
        return result;
    }

    void AMF_STD_CALL ClientSession::SetTimeout(time_t timeout)
    {
        m_Timeout = timeout;
    }
}

