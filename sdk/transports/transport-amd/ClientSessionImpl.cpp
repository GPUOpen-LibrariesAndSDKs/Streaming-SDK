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

#include "ClientSessionImpl.h"
#include "ClientImpl.h"
#include "ServerDiscovery.h"
#include "Misc.h"
#include "amf/public/common/TraceAdapter.h"

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::transport_amd::ClientSessionImpl";

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace ssdk::transport_amd
{

    ClientSessionImpl::ClientSessionImpl(ClientImpl* client) :
        m_Client(client)
    {
        if (client != nullptr)
        {
            client->SetSession(this);
        }
    }

    ClientSessionImpl::~ClientSessionImpl()
    {
        AMFTraceInfo(AMF_FACILITY, L"ClientSessionImpl destroyed");
        amf::AMFLock lock(&m_CritSect);
        if (m_Client != nullptr)
        {
            m_Client->SetSession(nullptr);
        }
    }

    void AMF_STD_CALL ClientSessionImpl::RegisterReceiverCallback(ReceiverCallback* callback)
    {
        amf::AMFLock lock(&m_CritSect);
        m_Callback = callback;
    }

    net::Session::Result ClientSessionImpl::NotifyTerminationCallbacks(ReceiverCallback::TerminationReason reason)
    {
        Session::Ptr temp(this);	//	This is to prevent destruction of the session from inside the callback
        amf::AMFLock lock(&m_CritSect);

        if (m_Callback != nullptr)
        {
            m_Callback->OnTerminate(this, reason);
        }
        return net::Session::Result::OK;
    }

    const char* AMF_STD_CALL ClientSessionImpl::GetPeerPlatform() const noexcept
    {
        if (m_ServerParams != nullptr)
        {
            return m_ServerParams->GetOsName();
        }
        else
        {
            return nullptr;
        }
    }

    const char* AMF_STD_CALL ClientSessionImpl::GetPeerUrl() const noexcept
    {
        if (m_ServerParams != nullptr)
        {
            return m_ServerParams->GetUrl();
        }
        else
        {
            return nullptr;
        }
    }

    // ---------------------------------- Overridables for the net::Session class: -------------------------------------------------

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    DatagramClientSessionImpl::DatagramClientSessionImpl(ClientImpl* client, net::Socket* sock, const net::Socket::Address& peer, size_t receiveBufSize) :
        DatagramClientSessionFlowCtrl(sock, peer, receiveBufSize, 1),
        ClientSessionImpl(client)
    {
    #if defined(__APPLE__)
        // FIX ME: need to find the correct way to set the socket buffer size that works on
        // both iphone SE and other ios devices.
        // On iphone SE, SOCKET_BUF_SIZE_RCV and SOCKET_BUF_SIZE_SND are too big.
        int recvBufSize = SOCKET_BUF_SIZE_RCV/2;
        int sendBufSize = SOCKET_BUF_SIZE_SND/2;
    #else
        int recvBufSize = SOCKET_BUF_SIZE_RCV;
        int sendBufSize = SOCKET_BUF_SIZE_SND;
    #endif
        sock->SetSocketOpt(SOL_SOCKET, SO_RCVBUF, &recvBufSize, sizeof(recvBufSize));
        sock->SetSocketOpt(SOL_SOCKET, SO_SNDBUF, &sendBufSize, sizeof(sendBufSize));
    }

    DatagramClientSessionImpl::~DatagramClientSessionImpl ()
    {
    }

    transport_common::Result     AMF_STD_CALL DatagramClientSessionImpl::Send(Channel channel, const void* msg, size_t msgLen)
    {
        transport_common::Result result = transport_common::Result::FAIL;
        net::Socket::Result sockResult = DatagramClientSessionFlowCtrl::Send(msg, msgLen, static_cast<unsigned char>(channel), nullptr);
        switch (sockResult)
        {
        case net::Socket::Result::OK:
            result = transport_common::Result::OK;
            break;
        case net::Socket::Result::NETWORK_DOWN:
        case net::Socket::Result::SOCKET_NOT_CONNECTED:
            OnSessionTimeout();
            result = transport_common::Result::FAIL;
            break;
        case net::Socket::Result::SOCKET_SHUTDOWN:
        case net::Socket::Result::CONNECTION_ABORTED:
        case net::Socket::Result::CONNECTION_RESET:
        case net::Socket::Result::DESTINATION_UNREACHABLE:
            result = transport_common::Result::SERVER_NOT_AVAILABLE;
            OnSessionTimeout();
            break;
        case net::Socket::Result::CONNECTION_TIMEOUT:
            result = transport_common::Result::SESSION_TIMEOUT;
            OnSessionTimeout();
            break;
        default:
            result = transport_common::Result::FAIL;
            OnSessionTimeout();
            break;
        }
        return result;
    }

    void  AMF_STD_CALL DatagramClientSessionImpl::UpgradeProtocol(uint32_t version)
    {
        DatagramClientSessionFlowCtrl::UpgradeProtocol(version);
    }


    void DatagramClientSessionImpl::OnCompleteMessage(FlowCtrlProtocol::MessageID msgID, const void* buf, size_t size, const net::Socket::Address& /*receivedFrom*/, unsigned char optional)
    {
        //    const Command* command = reinterpret_cast<const Command*>(buf);
        Channel channel = static_cast<Channel>(optional);
        transport_amd::Session::Ptr temp(this);	//	This is to prevent destruction of the session from inside the callback
        amf::AMFLock lock(&m_CritSect);
        if (m_Callback != nullptr)
        {
            m_Callback->OnMessageReceived(this, channel, msgID, buf, size);
        }
    }

    net::Session::Result DatagramClientSessionImpl::OnInit()
    {
        return net::Session::Result::OK;
    }


    net::Session::Result DatagramClientSessionImpl::OnSessionTimeout()
    {
        net::Session::Result result = NotifyTerminationCallbacks(ReceiverCallback::TerminationReason::TIMEOUT);
        DatagramClientSessionFlowCtrl::Terminate();
        return result;
    }

    net::Session::Result   DatagramClientSessionImpl::OnSessionClose()
    {
        net::Session::Result result = NotifyTerminationCallbacks(ReceiverCallback::TerminationReason::DISCONNECT);
        DatagramClientSessionFlowCtrl::Terminate();
        return result;
    }

    void DatagramClientSessionImpl::Terminate()
    {
        if (IsTerminated() == false)
        {
            NotifyTerminationCallbacks(ReceiverCallback::TerminationReason::DISCONNECT);
            DatagramClientSessionFlowCtrl::Terminate();
            m_Socket->Close();
        }
    }

    net::ClientSession::Result DatagramClientSessionImpl::WaitForIncomingMessages()
    {
        return WaitForIncoming();
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    StreamClientSessionImpl::StreamClientSessionImpl(ClientImpl* client, net::Socket* sock, const net::Socket::Address& peer, size_t receiveBufSize) :
        ClientSessionImpl(client),
        net::StreamClientSession(sock, peer, receiveBufSize)
    {
        int yes = 1;
        sock->SetSocketOpt(IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));

    #if defined(__APPLE__)
        // Disconnecting from TCP will raise SIGPIPE on apple and linux.
        // SIGPIPE will cause the app to crash as soon as TCP is not closed by
        // client sending a stop request.
        // Disabling SIGPIPE for apple so that it doesn't crash.
        sock->SetSocketOpt(SOL_SOCKET, SO_NOSIGPIPE, &yes, sizeof(yes));

        // FIX ME: need to find the correct way to set the socket buffer size that works on
        // both iphone SE and other ios devices.
        // On iphone SE, SOCKET_BUF_SIZE_RCV and SOCKET_BUF_SIZE_SND are too big.
        int recvBufSize = SOCKET_BUF_SIZE_RCV/2;
        int sendBufSize = SOCKET_BUF_SIZE_SND/2;
    #else
        int recvBufSize = SOCKET_BUF_SIZE_RCV;
        int sendBufSize = SOCKET_BUF_SIZE_SND;
    #endif

        sock->SetSocketOpt(SOL_SOCKET, SO_RCVBUF, &recvBufSize, sizeof(recvBufSize));
        sock->SetSocketOpt(SOL_SOCKET, SO_SNDBUF, &sendBufSize, sizeof(sendBufSize));

        sock->SetTimeout(5);
    }

    StreamClientSessionImpl::~StreamClientSessionImpl()
    {

    }

    transport_common::Result     AMF_STD_CALL StreamClientSessionImpl::Send(Channel channel, const void* msg, size_t msgLen)
    {
        amf::AMFLock lock(&m_CritSect);

        transport_common::Result result = transport_common::Result::FAIL;
        m_FlowCtrl.PrepareMessage(channel, msg, msgLen);
        size_t bytesSent = 0;
        net::Socket::Result sockResult = net::StreamSocket::Ptr(m_Socket)->SendAll(m_FlowCtrl.GetSendBuffer(), m_FlowCtrl.GetSendSize(), &bytesSent);
        switch (sockResult)
        {
        case net::Socket::Result::OK:
            result = transport_common::Result::OK;
            break;
        case net::Socket::Result::NETWORK_DOWN:
        case net::Socket::Result::SOCKET_NOT_CONNECTED:
            result = transport_common::Result::FAIL;
            OnSessionTimeout();
            break;
        case net::Socket::Result::SOCKET_SHUTDOWN:
        case net::Socket::Result::CONNECTION_ABORTED:
        case net::Socket::Result::CONNECTION_RESET:
        case net::Socket::Result::DESTINATION_UNREACHABLE:
            result = transport_common::Result::SERVER_NOT_AVAILABLE;
            OnSessionTimeout();
            break;
        case net::Socket::Result::CONNECTION_TIMEOUT:
            result = transport_common::Result::SESSION_TIMEOUT;
            OnSessionTimeout();
            break;
        default:
            result = transport_common::Result::FAIL;
            OnSessionTimeout();
            break;
        }
        return result;
    }

    net::Session::Result StreamClientSessionImpl::OnInit()
    {
        return net::Session::Result::OK;
    }


    net::Session::Result StreamClientSessionImpl::OnSessionTimeout()
    {
        return NotifyTerminationCallbacks(ReceiverCallback::TerminationReason::TIMEOUT);
    }

    net::Session::Result   StreamClientSessionImpl::OnSessionClose()
    {
        return NotifyTerminationCallbacks(ReceiverCallback::TerminationReason::DISCONNECT);
    }

    void StreamClientSessionImpl::Terminate()
    {
        if(IsTerminated() == false)
        {
            NotifyTerminationCallbacks(ReceiverCallback::TerminationReason::DISCONNECT);
            net::StreamClientSession::Terminate();
        }
    }

    net::ClientSession::Result StreamClientSessionImpl::WaitForIncomingMessages()
    {
        return WaitForIncoming();
    }

    bool StreamClientSessionImpl::OnTickNotify()
    {
        return false;
    }

    net::ClientSession::Result StreamClientSessionImpl::OnDataReceived()
    {
        net::Session::Result result = net::Session::Result::RECEIVE_FAILED;
        net::StreamSocket::Ptr socket(GetSocket());

        bool msgIsComplete = false;
        StreamFlowCtrlProtocol::Result flowResult = m_FlowCtrl.ReadAndProcess(socket, msgIsComplete);
        if (flowResult  != StreamFlowCtrlProtocol::Result::OK)
        {
            AMFTraceError(AMF_FACILITY, L"OnDataAvailable(): Failed to read message. ReadAndProcess() returned %d", (int)flowResult);
            switch (flowResult)
            {
                case StreamFlowCtrlProtocol::Result::OK:
                    result = net::Session::Result::OK;
                    break;
                case StreamFlowCtrlProtocol::Result::FAIL:
                    result = net::Session::Result::RECEIVE_FAILED;
                    break;
                case StreamFlowCtrlProtocol::Result::TIMEOUT:
                    result = net::Session::Result::TIMEOUT;
                    break;
                case StreamFlowCtrlProtocol::Result::CONNECTION_TERMINATED:
                    result = net::Session::Result::CONNECTION_TERMINATED;
                    break;
                default:
                    result = net::Session::Result::RECEIVE_FAILED;
                    break;
            }
        }
        else
        {
            if (msgIsComplete == true && m_Callback != nullptr)
            {
                m_Callback->OnMessageReceived(this, m_FlowCtrl.GetChannel(), m_FlowCtrl.GetMsgID(), m_FlowCtrl.GetReceiveBuffer(), m_FlowCtrl.GetReceiveSize());
            }
            Touch();
            result = net::Session::Result::OK;
        }
        return result;
    }
}
