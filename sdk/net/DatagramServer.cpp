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
#include "DatagramServer.h"
#include "Selector.h"
#include <memory>
#include <time.h>

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::net::DatagramServer";

namespace ssdk::net
{
    DatagramServer::DatagramServer(DatagramSocket* sock, size_t receiveBufferSize, size_t maxConnections, time_t disconnectTimeout) :
		m_Socket(sock),
        m_MaxConnections(maxConnections),
        m_ReceiveBufferSize(receiveBufferSize),
        m_DisconnectTimeout(disconnectTimeout)
    {

    }

    void DatagramServer::SelectorTimeout()
    {
        DatagramServerSession::Ptr session;

        for (SessionSet::const_iterator sessionIt = m_Sessions.begin(); sessionIt != m_Sessions.end(); ++sessionIt)
        {
            session = DatagramServerSession::Ptr(*sessionIt);
            if (session->GetElapsedTimeSinceLastRequest() < m_DisconnectTimeout)
            {
                session->OnTickNotify();
            }
            else
            {
                 // Nothing to do here.
            }
        }
    }

    SessionManager::Result DatagramServer::ProcessIncomingMessages(uint8_t* buf)
    {
        SessionManager::Result res = SessionManager::Result::OK;
		DatagramServerSession::Ptr session;
		size_t bytesReceived = 0;
		Socket::Address receivedFrom;
		Socket::Result sockResult = m_Socket->ReceiveFrom(buf, m_ReceiveBufferSize, &receivedFrom, &bytesReceived);

        bool sessionFound = false;
        // If during the terminte process, return SERVER_SHUTDOWN - used to be SESSION_CREATE_FAILED.
        if (m_Terminate == true)
        {
            res = SessionManager::Result::SERVER_SHUTDOWN;
            AMFTraceInfo(AMF_FACILITY, L"DatagramServer::ProcessIncomingMessages(): Server terminated, returning...");
        }
        else
        {
            for (SessionSet::const_iterator sessionIt = m_Sessions.begin(); sessionIt != m_Sessions.end(); ++sessionIt)
            {
                if ((*sessionIt)->GetPeerAddress() == receivedFrom)
                {
                    session = DatagramServerSession::Ptr(*sessionIt);
                    sessionFound = true;
                    break;
                }
            }

            if (bytesReceived > 0)
            {
                //  Find the session the datagram should be routed to based on the From address if it exists
                if (sessionFound == false)  //  No active sessions for this From address - create a new one
                {
                    if (m_Sessions.size() < m_MaxConnections)
                    {
                        session = DatagramServerSession::Ptr(OnCreateSession(receivedFrom, m_Socket, buf, bytesReceived));
                        if (session != nullptr)
                        {
                            AMFTraceInfo(AMF_FACILITY, L"New session created");
                            session->Touch();
                            if ((res = RegisterSession(session)) != SessionManager::Result::OK)
                            {
                                AMFTraceError(AMF_FACILITY, L"Failed to register session");
                                session = nullptr;
                            }
                        }
                        else
                        {
//                            AMFTraceError(AMF_FACILITY, L"Failed to create session");
                            res = SessionManager::Result::SESSION_CREATE_FAILED;
                        }
                    }
                    else
                    {
                        AMFTraceError(AMF_FACILITY, L"Exceeded maximum number of simultaneous connections");
                    }
                }
                if (session != nullptr) //  Route the message to the session - either existing or newly created
                {
                    session->Touch();
                    session->OnDataReceived(buf, bytesReceived, session->GetPeerAddress());
                }
            }
            else
            {
                switch (sockResult)
                {
                case Socket::Result::CONNECTION_RESET:
                case Socket::Result::CONNECTION_ABORTED:
                    res = SessionManager::Result::CLIENT_DISCONNECTED;
                    if (session)
                    {
                        session->Terminate();
                    }
                    break;
                default:
                    break;
                }
            }
        }
        return res;
    }

    SessionManager::Result DatagramServer::AcceptConnections()
    {
        SessionManager::Result res = SessionManager::Result::NO_LISTENING_SOCKET;
        res = SessionManager::Result::OK;
        std::unique_ptr<uint8_t[]> readBuf(new uint8_t[m_ReceiveBufferSize]);
        uint8_t* rawBuffer = readBuf.get();
        Selector selector;
        selector.AddReadableSocket(m_Socket);
        Socket::Set readableSockets;
        do
        {
            amf::AMFLock	lock(&m_Guard);
            if (m_Terminate == true)
            {
                break;
            }
            else
            {
                struct timeval timeout_tv {};
                timeout_tv.tv_usec = 1000 * COMM_SELECTOR_FLUSH_INTERVAL_IN_MS;
                Selector::Result res1 = selector.WaitToRead(timeout_tv, readableSockets);
                switch (res1)
                {
                case Selector::Result::OK:
                {
                    if (m_Terminate == false)
                    {
                        //  Dispatch the incoming message to the appropriate session, create a new one if necessary
                        if (readableSockets.size() > 0)
                        {
                            res = ProcessIncomingMessages(rawBuffer);
                            if (res == SessionManager::Result::CLIENT_DISCONNECTED)
                            {
                                AMFTraceDebug(AMF_FACILITY, L"DatagramServer::AcceptConnections() - client disconnected");
                            }
                        }
                    }
                }
                break;
                case Selector::Result::TIMEOUT:
                    SelectorTimeout();
                    //AMFTraceDebug(AMF_FACILITY, L"DatagramServer::AcceptConnections() - select timed out(%d)", timeout);
                    break;
                default:
                    AMFTraceInfo(AMF_FACILITY, L"AcceptConnections err=%d", res1);

                    break;
                }
                CleanupTimedoutSessions(m_DisconnectTimeout);
            }
        } while ((res == SessionManager::Result::OK || res == SessionManager::Result::CLIENT_DISCONNECTED || res == SessionManager::Result::SESSION_CREATE_FAILED));
        return res;
    }
}
