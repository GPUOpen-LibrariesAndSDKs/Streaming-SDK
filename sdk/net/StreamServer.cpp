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
#include "StreamServer.h"
#include "StreamServerSession.h"
#include "Selector.h"
#include "amf/public/common/TraceAdapter.h"

static const wchar_t* const TRACE_SCOPE = L"ssdk::net::StreamServer";

namespace ssdk::net
{
    StreamServer::StreamServer(StreamSocket* listeningSocket, size_t maxDatagramSize, size_t maxConnections, time_t disconnectTimeout) :
        m_ListeningSocket(listeningSocket),
        m_MaxConnections(maxConnections),
        m_DisconnectTimeout(disconnectTimeout),
        m_BufSize(maxDatagramSize),
        m_ReadingThread(*this)
    {
        m_IncomingBuf = std::unique_ptr<uint8_t[]>(new uint8_t[maxDatagramSize]);
    }

    SessionManager::Result StreamServer::AcceptConnections()
    {
        SessionManager::Result res = SessionManager::Result::OK;
        Selector selector;
        selector.AddReadableSocket(m_ListeningSocket);
        Socket::Set readableSockets;
        do
        {
            struct timeval timeout_tv {};
            timeout_tv.tv_sec = (long) m_DisconnectTimeout;

            switch (selector.WaitToRead(timeout_tv, readableSockets))
            {
            case Selector::Result::OK:
            {
                amf::AMFLock lock(&m_Guard);
                if (m_Terminate == false)
                {
                    //  Dispatch the incoming message to the appropriate session, create a new one if necessary
                    if (readableSockets.size() > 0)
                    {
                        StreamSocket::Ptr sessionSocket;
                        if (m_ListeningSocket->Accept(&sessionSocket) != Socket::Result::OK)
                        {
                            AMFTraceError(TRACE_SCOPE, L"Failed to accept a connection");
                        }
                        else
                        {
                            Session::Ptr session = OnCreateSession(*(sessionSocket->GetPeerAddress()), sessionSocket, nullptr, 0);
                            if (session != nullptr)
                            {
                                RegisterSession(session);
                                if (m_ReadingThread.IsRunning() == false)
                                {
                                    m_ReadingThread.Start();
                                }
                            }
                        }
                    }
                }
            }
                break;
            case Selector::Result::TIMEOUT:
                //  Check if any of the current sessions have been idle longer than the timeout, close the idle ones
                break;
            default:
                break;
            }
        } while (m_Terminate == false && (res == SessionManager::Result::OK || res == SessionManager::Result::CLIENT_DISCONNECTED || res == SessionManager::Result::SESSION_CREATE_FAILED));
        m_ReadingThread.RequestStop();
        m_ReadingThread.WaitForStop();
        AMFTraceDebug(TRACE_SCOPE, L"Exit from StreamServer::AcceptConnections()");
        return res;
    }

    bool StreamServer::ProcessIncomingMessages()
    {
        bool result = false;
        Socket::Set sessionSockets;
        SessionSet sessions;
        {
            amf::AMFLock lock(&m_Guard);
            // If during the terminte process, return SESSION_CREATE_FAILED.
            if (m_Terminate == true)
            {
                AMFTraceInfo(TRACE_SCOPE, L"ProcessIncomingMessages(): Server terminated, returning...");
                return false;
            }
            sessions = m_Sessions;
            for (SessionSet::iterator sessionIt = sessions.begin(); sessionIt != sessions.end(); ++sessionIt)
            {
                sessionSockets.insert(StreamServerSession::Ptr(*sessionIt)->GetSocket());
            }
        }
        if (sessionSockets.size() > 0)
        {
            Selector selector(&sessionSockets, nullptr);
            Socket::Set readableSockets;

            struct timeval timeout_tv {};
            timeout_tv.tv_sec = (long) m_DisconnectTimeout;

            switch (selector.WaitToRead(timeout_tv, readableSockets))
            {
            case Selector::Result::OK:
                for (Socket::Set::const_iterator sockIt = readableSockets.begin(); sockIt != readableSockets.end(); ++sockIt)
                {
//                    amf::AMFLock lock(&m_Guard);
                    if (m_Terminate == false)
                    {
                        for (SessionSet::iterator sessionIt = sessions.begin(); sessionIt != sessions.end(); ++sessionIt)
                        {
                            if (StreamServerSession::Ptr(*sessionIt)->GetSocket() == *sockIt)
                            {
                                StreamServerSession::Ptr session(*sessionIt);
                                Session::Result sessionResult = session->OnDataAvailable();
                                switch (sessionResult)
                                {
                                case Session::Result::CONNECTION_TERMINATED:
                                    AMFTraceDebug(TRACE_SCOPE, L"Stream Session connection terminated");
                                    session->Terminate();
                                    break;
                                case Session::Result::OK:
                                    session->Touch();
                                    break;
                                default:
                                    AMFTraceError(TRACE_SCOPE, L"Session error: %d", sessionResult);
                                    break;
                                }
                            }
                        }
                    }
                }
                CleanupTimedoutSessions(m_DisconnectTimeout);
                break;
            case Selector::Result::TIMEOUT:
                CleanupTimedoutSessions(m_DisconnectTimeout);
                AMFTraceInfo(TRACE_SCOPE, L"Stream Session selector timeout, retrying");
                break;
            default:
                break;
            }
            result = true;
        }
        return result;
    }

    void StreamServer::ReadingThread::Run()
    {
        while (StopRequested() == false)
        {
            if (m_Server.ProcessIncomingMessages() == false)
            {
                break;// No active connections, just exit the thread. It will be restarted by the first connection
            }
        }
        AMFTraceDebug(TRACE_SCOPE, L"Exit from StreamServer::ReadingThread::Run()");
    }
}