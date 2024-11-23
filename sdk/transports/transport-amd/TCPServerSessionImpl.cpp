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

#include "TCPServerSessionImpl.h"
#include "Channels.h"
#include "messages/service/Connect.h"
#include "Misc.h"
#include "net/Selector.h"

#include "public/include/core/Platform.h"
#include "public/common/TraceAdapter.h"

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::transport_amd::TCPSessionImpl";

namespace ssdk::transport_amd
{
    TCPServerSessionImpl::TCPServerSessionImpl(ServerImpl* server, net::StreamSocket* sock, const net::Socket::Address& peer) :
        ServerSessionImpl(server),
        net::StreamServerSession(sock),
        m_Peer(peer)
    {
        int yes = 1;
        sock->SetSocketOpt(IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));

        int recvBufSize = SOCKET_BUF_SIZE_RCV;
        sock->SetSocketOpt(SOL_SOCKET, SO_RCVBUF, &recvBufSize, sizeof(recvBufSize));
        int sendBufSize = SOCKET_BUF_SIZE_SND;
        sock->SetSocketOpt(SOL_SOCKET, SO_SNDBUF, &sendBufSize, sizeof(sendBufSize));
        sock->SetTimeout(5);
    }

    TCPServerSessionImpl::~TCPServerSessionImpl()
    {
        AMFTraceInfo(AMF_FACILITY, L"TCPServerSessionImpl destroyed");
    }



    transport_common::Result AMF_STD_CALL TCPServerSessionImpl::Send(Channel channel, const void* msg, size_t msgLen)
    {
        static FlowCtrlProtocol::MessageID msgID = 0;
        transport_common::Result result = transport_common::Result::FAIL;
        net::StreamSocket::Ptr socket(GetSocket());
        amf::AMFLock lock(&m_SendCS);	//  GK: This lock is necessary for TCP as Send can be called from different threads. Datagrams are atomic, but
                                        //  with TCP messages can get mixed up and confuse the parser on the receiving end
        m_FlowCtrl.PrepareMessage(channel, msg, msgLen);

        net::Socket::Result sockResult = socket->SendAll(m_FlowCtrl.GetSendBuffer(), m_FlowCtrl.GetSendSize());
        if (sockResult != net::Socket::Result::OK)
        {
            bool traceError = true;
            switch (sockResult)
            {
            case net::Socket::Result::OK:
                result = transport_common::Result::OK;
                break;
            case net::Socket::Result::NETWORK_DOWN:
            case net::Socket::Result::SOCKET_NOT_CONNECTED:
                result = transport_common::Result::FAIL;
                break;
            case net::Socket::Result::SOCKET_SHUTDOWN:
            case net::Socket::Result::CONNECTION_ABORTED:
            case net::Socket::Result::CONNECTION_RESET:
            case net::Socket::Result::DESTINATION_UNREACHABLE:
                result = transport_common::Result::SERVER_NOT_AVAILABLE;
                break;
            case net::Socket::Result::CONNECTION_TIMEOUT:
                result = transport_common::Result::SESSION_TIMEOUT;
                break;
            case net::Socket::Result::END_OF_PIPE:
                traceError = false;
                result = transport_common::Result::CONNECTION_REFUSED;
                break;
            default:
                result = transport_common::Result::FAIL;
                break;
            }
            if (traceError == true)
            {
                AMFTraceError(AMF_FACILITY, L"Send: Failed to send message, sockResult: %s", socket->GetErrorString(sockResult));
            }

        }
        return result;
    }

    net::Session::Result TCPServerSessionImpl::OnDataAvailable()
    {
        net::Session::Result result = net::Session::Result::RECEIVE_FAILED;
        net::StreamSocket::Ptr socket(GetSocket());

        bool msgIsComplete = false;
        StreamFlowCtrlProtocol::Result flowResult = m_FlowCtrl.ReadAndProcess(socket, msgIsComplete);
        if (flowResult != StreamFlowCtrlProtocol::Result::OK)
        {
            AMFTraceWarning(AMF_FACILITY, L"OnDataAvailable(): Failed to read message. ReadAndProcess() returned %d", (int)flowResult);
            switch (flowResult)
            {
            case StreamFlowCtrlProtocol::Result::OK:
                result = Result::OK;
                break;
            case StreamFlowCtrlProtocol::Result::FAIL:
                result = Result::RECEIVE_FAILED;
                break;
            case StreamFlowCtrlProtocol::Result::TIMEOUT:
                result = Result::TIMEOUT;
                break;
            case StreamFlowCtrlProtocol::Result::CONNECTION_TERMINATED:
                result = Result::CONNECTION_TERMINATED;
                break;
            default:
                result = Result::RECEIVE_FAILED;
                break;
            }
        }
        else
        {
            if (msgIsComplete == true)
            {
                if (m_FlowCtrl.GetChannel() == Channel::SERVICE && *((const uint8_t*)m_FlowCtrl.GetReceiveBuffer()) == uint8_t(SERVICE_OP_CODE::HELLO))
                {
                    ConnectRequest helloRequest;
                    if (helloRequest.ParseBuffer(m_FlowCtrl.GetReceiveBuffer(), m_FlowCtrl.GetReceiveSize()))
                    {
                        net::Socket::IPv4Address peer(*socket->GetPeerAddress());
                        std::string debugMsg = "HelloRequest command received from ";
                        debugMsg += peer.GetAddressAsString();
                        if (helloRequest.GetProtocolVersion() >= FlowCtrlProtocol::PROTOCOL_VERSION_MIN)
                        {
                            AMFTraceInfo(AMF_FACILITY, L"%S", debugMsg.c_str());

                            amf::AMFPropertyStoragePtr propStorage(this);
                            PopulateOptionsFromHello(helloRequest, propStorage);
                            if (m_Server->AuthorizeConnectionRequest(this, helloRequest.GetDeviceID()) == true)
                            {
                                HelloResponse resp(m_Server->GetName().c_str(), FlowCtrlProtocol::PROTOCOL_VERSION_CURRENT, FlowCtrlProtocol::PROTOCOL_VERSION_MIN,
                                                   m_Server->GetPort(), 0);
                                m_Server->FillOptions(false, this, &resp.GetOptions());
                                resp.UpdateData();
                                Send(Channel::SERVICE, resp.GetSendData(), resp.GetSendSize());
                            }
                            else
                            {
                                HelloRefused refused;
                                Send(Channel::SERVICE, refused.GetSendData(), refused.GetSendSize());
                                AMFTraceInfo(AMF_FACILITY, L"OnDataAvailable(): connection refused as unauthorized");
                            }
                        }
                        else
                        {
                            AMFTraceError(AMF_FACILITY, L"%S with protocol version less then supported : server = %d serverMin = %d client = %d",
                                          debugMsg.c_str(), (int)FlowCtrlProtocol::PROTOCOL_VERSION_CURRENT, (int)FlowCtrlProtocol::PROTOCOL_VERSION_MIN, (int)helloRequest.GetProtocolVersion());
                        }
                    }
                }
                else
                {
                    if (m_Callback != nullptr)
                    {
                        m_Callback->OnMessageReceived(this, m_FlowCtrl.GetChannel(), m_FlowCtrl.GetMsgID(), m_FlowCtrl.GetReceiveBuffer(), m_FlowCtrl.GetReceiveSize());
                    }
                }
            }
            result = net::Session::Result::OK;
        }
        return result;
    }

    //  Overridables:
    net::Session::Result TCPServerSessionImpl::OnInit()
    {
        return net::Session::Result::OK;
    }


    net::Session::Result TCPServerSessionImpl::OnSessionTimeout()
    {
        TimeoutNotify();
        return net::Session::Result::OK;
    }

    net::Session::Result TCPServerSessionImpl::OnSessionClose()
    {
        TerminateNotify();
        return net::Session::Result::OK;
    }


    void TCPServerSessionImpl::Terminate()
    {
        TerminateNotify();
        net::StreamServerSession::Terminate();
    }

}