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

#include "ClientImpl.h"
#include "ClientSessionImpl.h"
#include "ServerDiscovery.h"
#include "Misc.h"

#include "amf/public/common/TraceAdapter.h"
#include "amf/public/include/core/PropertyStorage.h"

#if !defined(_WIN32) && !defined(__APPLE__)
#include <sys/prctl.h>
#endif

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::transport_amd::ClientImpl";
static constexpr const int CLIENT_CONNECTION_TIMEOUT_SESSION = 10; // sec

namespace ssdk::transport_amd
{

    ClientImpl::ClientImpl() :
        m_ServersEnumerated(false),
        m_Session(nullptr),
        m_WaitingForIncoming(false),
        m_DiscoveryPort(DEFAULT_PORT),
        m_Timeout(CLIENT_CONNECTION_TIMEOUT_SESSION),
        m_DatagramSize(FlowCtrlProtocol::MAX_DATAGRAM_SIZE),
        m_RxMaxFragmentSize(FlowCtrlProtocol::UDP_MSS_SIZE),
        m_TxMaxFragmentSize(FlowCtrlProtocol::UDP_MSS_SIZE)
    {
    }

    ClientImpl::~ClientImpl()
    {
    }

    transport_common::Result AMF_STD_CALL ClientImpl::EnumerateServers(const char* clientID, unsigned short port, time_t timeout, size_t* const numOfServers, ServerEnumCallback* callback)
    {
        m_DiscoveryPort = port;
        transport_common::Result result = transport_common::Result::FAIL;
        if (timeout == 0 || numOfServers == nullptr)
        {
            m_ServersEnumerated = false;
            result = transport_common::Result::INVALID_ARG;
        }
        else
        {
            std::string clientDeviceID;
            if (clientID != nullptr)
            {
                clientDeviceID = clientID;
            }
            DiscoveryClient discoveryClient(clientDeviceID, m_Servers, m_DiscoveryPort);

            ServerDiscoverySession::Ptr session(discoveryClient.Connect(net::Url("*", "udp", m_DiscoveryPort)));
            if (session != nullptr)
            {
                session->SetTimeout(timeout);

                DatagramClientSessionFlowCtrl::Result resDiscovery = session->EnumerateServers(callback, numOfServers, this);
                m_ServersEnumerated = true;
                switch (resDiscovery)
                {
                case DatagramClientSessionFlowCtrl::Result::OK:
                    result = transport_common::Result::OK;
                    break;
                case DatagramClientSessionFlowCtrl::Result::TIMEOUT:
                    result = transport_common::Result::CONNECTION_TIMEOUT;
                    break;
                case DatagramClientSessionFlowCtrl::Result::REFUSED:
                    result = transport_common::Result::CONNECTION_REFUSED;
                    break;
                case DatagramClientSessionFlowCtrl::Result::INVALID_VERSION:
                    result = transport_common::Result::CONNECTION_INVALID_VERSION;
                    break;
                default:
                    result = transport_common::Result::FAIL;
                    break;
                }
            }
        }
        return result;
    }

    transport_common::Result AMF_STD_CALL ClientImpl::GetServerParameters(size_t serverIdx, ServerParameters** params)
    {
        transport_common::Result result = transport_common::Result::INVALID_ARG;
        if (serverIdx >= m_Servers.size())
        {
            AMFTraceError(AMF_FACILITY, L"Index out of bounds");
            result = transport_common::Result::INVALID_ARG;
        }
        else if (params == nullptr)
        {
            AMFTraceError(AMF_FACILITY, L"params == NULL");
            result = transport_common::Result::INVALID_ARG;
        }
        else if (m_ServersEnumerated == false)
        {
            AMFTraceError(AMF_FACILITY, L"Servers have not been enumerated");
            result = transport_common::Result::SERVERS_NOT_ENUMERATED;
        }
        else
        {
            *params = m_Servers[serverIdx];
            (*params)->Acquire();
            result = transport_common::Result::OK;
        }
        return result;
    }

    transport_common::Result AMF_STD_CALL ClientImpl::QueryServerInfo(const char* clientID, const char* url, unsigned short port, time_t timeoutSec, ServerEnumCallback* callback)
    {
        m_DiscoveryPort = port;
        transport_common::Result result = transport_common::Result::FAIL;
        if (timeoutSec == 0)
        {
            m_ServersEnumerated = false;
            result = transport_common::Result::INVALID_ARG;
        }
        else
        {
            std::string clientDeviceID;
            if (clientID != nullptr)
            {
                clientDeviceID = clientID;
            }
            DiscoveryClient discoveryClient(clientDeviceID, m_Servers, m_DiscoveryPort);

            ServerDiscoverySession::Ptr session(discoveryClient.Connect(net::Url(url, "udp", m_DiscoveryPort)));
            if (session != nullptr)
            {
                session->SetTimeout(timeoutSec);

                DatagramClientSessionFlowCtrl::Result resDiscovery = session->QueryServerInfo(callback, this);
                m_ServersEnumerated = true;
                switch (resDiscovery)
                {
                case DatagramClientSessionFlowCtrl::Result::OK:
                    result = transport_common::Result::OK;
                    break;
                case DatagramClientSessionFlowCtrl::Result::TIMEOUT:
                    result = transport_common::Result::CONNECTION_TIMEOUT;
                    break;
                case DatagramClientSessionFlowCtrl::Result::REFUSED:
                    result = transport_common::Result::CONNECTION_REFUSED;
                    break;
                case DatagramClientSessionFlowCtrl::Result::INVALID_VERSION:
                    result = transport_common::Result::CONNECTION_INVALID_VERSION;
                    break;
                default:
                    result = transport_common::Result::FAIL;
                    break;
                }
            }
        }
        return result;
    }

    transport_common::Result AMF_STD_CALL ClientImpl::ConnectToServerAndQueryParameters(const char* serverUrl, const char* clientID, Session** session, ServerParameters** params)
    {
        transport_common::Result result = transport_common::Result::OK;
        if (m_Session != nullptr)
        {
            result = transport_common::Result::INVALID_ARG;
            AMFTraceError(AMF_FACILITY, L"m_Session != nullptr");
        }
        else if (session == nullptr || serverUrl == nullptr || params == nullptr)
        {
            result = transport_common::Result::INVALID_ARG;
            AMFTraceError(AMF_FACILITY, L"session == NULL || server == NULL");
        }
        else
        {
            m_ConnectionStatus = transport_common::Result::SERVERS_NOT_ENUMERATED;
            net::Url url(serverUrl, "UDP", m_DiscoveryPort);
            m_CurrentServerUrl = url;

            ClientSessionImpl::Ptr sessionImpl;
            AMFTraceDebug(AMF_FACILITY, L"Connect to server %S", url.GetUrl().c_str());

            if (url.GetProtocol() == "UDP")
            {
                result = EstablishUDPConnection(url, &sessionImpl, FlowCtrlProtocol::MAX_DATAGRAM_SIZE);
            }
            else
            {
                result = EstablishTCPConnection(url, &sessionImpl);
            }
            if (result == transport_common::Result::OK && sessionImpl != nullptr)
            {
                sessionImpl->RegisterReceiverCallback(this);
                m_WaitForHelloResponse = true;
                m_Terminated = false;
                time_t startTime = time(nullptr);
                do
                {
                    ConnectRequest helloRequest(FlowCtrlProtocol::PROTOCOL_VERSION_CURRENT, FlowCtrlProtocol::PROTOCOL_VERSION_MIN, clientID, m_DatagramSize, m_VideoCodecs, m_AudioCodecs, this);
                    sessionImpl->Send(Channel::SERVICE, helloRequest.GetSendData(), helloRequest.GetSendSize());

                    if (IsRunning())
                    {
                        amf_sleep(1);
                    }
                    else
                    {
                        net::ClientSession::Result  res = sessionImpl->WaitForIncomingMessages();
                        if (res != net::ClientSession::Result::OK && res != net::ClientSession::Result::TIMEOUT)
                        {
                            sessionImpl = nullptr;
                            result = m_ConnectionStatus;
                            AMFTraceError(AMF_FACILITY, L"ConnectToServerAndQueryParameters(): Failed reading while trying to connect to %S, res=%d", url.GetUrl().c_str(), res);
                            break;
                        }
                    }
                    time_t now = time(nullptr);
                    {
                        amf::AMFLock    lock(&m_CritSect);
                        if (now - startTime > m_Timeout)
                        {
                            result = transport_common::Result::CONNECTION_TIMEOUT;
                            sessionImpl = nullptr;
                            AMFTraceError(AMF_FACILITY, L"ConnectToServerAndQueryParameters(): Timed out trying to connect to %S", url.GetUrl().c_str());
                            break;
                        }
                    }
                } while (m_WaitForHelloResponse == true);

                if (m_Terminated == false && sessionImpl != nullptr && m_CurrentServer != nullptr)
                {
                    // Recieve buf is set to max - no need for adjustments
                    net::ClientSession::Ptr(sessionImpl)->SetTxMaxFragmentSize(m_TxMaxFragmentSize);

                    sessionImpl->SetServerParameters(m_CurrentServer);
                    sessionImpl->RegisterReceiverCallback(nullptr);

                    (*session) = sessionImpl;
                    (*session)->Acquire();

                    (*params) = m_CurrentServer;
                    (*params)->Acquire();

                    result = transport_common::Result::OK;
                }
                else
                {
                    (*session) = nullptr;
                    (*params) = nullptr;
                    result = transport_common::Result::FAIL;
                    if (m_Terminated == true)
                    {
                        AMFTraceInfo(AMF_FACILITY, L"ConnectToServerAndQueryParameters(): aborting because the client is terminating");
                    }
                    else
                    {
                        AMFTraceError(AMF_FACILITY, L"ConnectToServerAndQueryParameters(): failed, session %s nullptr, m_CurrentServer %s nullptr", (sessionImpl != nullptr ? L"!=" : L"=="), (m_CurrentServer != nullptr ? L"!=" : L"=="));
                    }
                }
            }
            else
            {
                AMFTraceError(AMF_FACILITY, L"Failed to establish connection to %S, transport_common::Result=%d", url.GetUrl().c_str(), result);
            }
        }
        return result;
    }

    void AMF_STD_CALL ClientImpl::OnMessageReceived(Session* session, Channel channel, int /*msgID*/, const void* message, size_t messageSize)
    {
        static const size_t flowctrlFragmentsHeaderSize = FlowCtrlProtocol::Fragment::GetSizeOfFragmentHeader();
        if (channel == Channel::SERVICE)
        {
            uint8_t optCode = *((const uint8_t*)message);
            AMFTraceDebug(AMF_FACILITY, L"ClientImpl::OnMessageReceived() OptCode = %d", optCode);
            switch (SERVICE_OP_CODE(optCode))
            {
            case SERVICE_OP_CODE::DISCOVERY:// todo: verify DISCOVERY is necessery
            case SERVICE_OP_CODE::HELLO:
                {
                    HelloResponse   resp;
                    resp.ParseBuffer(message, messageSize);
                    AMFTraceDebug(AMF_FACILITY, L"Received <<=== SERVICE_OP_CODE_HELLO");
                    int32_t serverVersion = resp.GetProtocolVersion();
                    if (serverVersion < FlowCtrlProtocol::PROTOCOL_VERSION_MIN)
                    {
                        m_ConnectionStatus = transport_common::Result::CONNECTION_INVALID_VERSION;
                    }
                    else
                    {
                        AMFTraceInfo(AMF_FACILITY, L"UpgradeProtocol version:%d", serverVersion);

                        session->UpgradeProtocol(serverVersion);
                        m_CurrentServer = new ServerParametersImpl(resp, m_CurrentServerUrl);
                    }
                    m_WaitForHelloResponse = false;
                    break;
                }
            case SERVICE_OP_CODE::CONNECTION_REFUSED:
                {
                    AMFTraceDebug(AMF_FACILITY, L"Received <<=== SERVICE_OP_CODE_CONNECTION_REFUSED");

                    m_ConnectionStatus = transport_common::Result::CONNECTION_REFUSED;
                    m_WaitForHelloResponse = false;
                    break;
                }
            default:
                AMFTraceInfo(AMF_FACILITY, L"OnMessageReceived() received CHANNEL_SERVICE::%d is not handled", (int)optCode);
                break;
            }
        }
    }

    void AMF_STD_CALL ClientImpl::OnTerminate(Session* /*session*/, TerminationReason reason)
    {
        AMFTraceDebug(AMF_FACILITY, L"ClientImpl::OnTerminate(%d)", reason);
        m_WaitForHelloResponse = false;
        m_Terminated = true;
    }

    transport_common::Result AMF_STD_CALL ClientImpl::EstablishUDPConnection(const net::Url& serverUrl, ClientSessionImpl** session, size_t datagramSize)
    {
        transport_common::Result result = transport_common::Result::OK;
        if (session == nullptr || serverUrl.GetUrl().length() == 0)
        {
            result = transport_common::Result::INVALID_ARG;
            AMFTraceError(AMF_FACILITY, L"session == NULL || server == NULL");
        }
        else
        {
            amf::AMFLock    lock(&m_CritSect);
            m_Transport = "UDP";
            net::ClientSession::Ptr clientSession(nullptr);
            DatagramClient* client = new DatagramClient(this);
            m_Client = net::Client::Ptr(client);
            clientSession = client->Connect(serverUrl, reinterpret_cast<void*>(datagramSize != 0 ? datagramSize : 508)); // 508 is the payload size of a minimum reassembly packet as defined in RFC791 and is the largest "safe" size guaranteed to pass through compliant routers
            clientSession->SetReceiveBufSize(FlowCtrlProtocol::MAX_DATAGRAM_SIZE);

            if (m_Client == nullptr)
            {
                result = transport_common::Result::INVALID_ARG;
            }
            else
            {
                if (clientSession == nullptr)
                {
                    result = transport_common::Result::FAIL;
                    AMFTraceError(AMF_FACILITY, L"Failed to connect");
                }
                else
                {
                    *session = static_cast<DatagramClientSessionImpl*>(clientSession.GetPtr());
                    (*session)->Acquire();
                    clientSession->SetTimeout(m_Timeout);
                }
            }
        }
        return result;
    }

    transport_common::Result AMF_STD_CALL ClientImpl::EstablishTCPConnection(const net::Url& serverUrl, ClientSessionImpl** session)
    {
        transport_common::Result result = transport_common::Result::OK;
        if (session == nullptr || serverUrl.GetUrl().length() == 0)
        {
            result = transport_common::Result::INVALID_ARG;
            AMFTraceError(AMF_FACILITY, L"session == NULL || server == NULL");
        }
        else
        {
            amf::AMFLock    lock(&m_CritSect);
            m_Transport = "TCP";
            net::ClientSession::Ptr clientSession(nullptr);
            StreamClient* client = new StreamClient(this);
            m_Client = net::Client::Ptr(client);
            clientSession = client->Connect(serverUrl, nullptr);

            if (m_Client == nullptr)
            {
                result = transport_common::Result::INVALID_ARG;
            }
            else
            {
                if (clientSession == nullptr)
                {
                    result = transport_common::Result::FAIL;
                    AMFTraceError(AMF_FACILITY, L"Failed to connect");
                }
                else
                {
                    *session = static_cast<StreamClientSessionImpl*>(clientSession.GetPtr());
                    (*session)->Acquire();
                    clientSession->SetTimeout(m_Timeout);
                }
            }
        }
        return result;
    }


    transport_common::Result     ClientImpl::SetTimeout(time_t timeoutSec)
    {
        amf::AMFLock    lock(&m_CritSect);
        m_Timeout = timeoutSec;
        return transport_common::Result::OK;
    }


    transport_common::Result AMF_STD_CALL ClientImpl::AddVideoCodec(const VideoCodec& codec)
    {
        amf::AMFLock    lock(&m_CritSect);
        for (auto& it : m_VideoCodecs)
        {
            if (it == codec)
            {
                return transport_common::Result::ALREADY_EXISTS;
            }
        }
        m_VideoCodecs.push_back(codec);
        return transport_common::Result::OK;
    }

    transport_common::Result AMF_STD_CALL ClientImpl::AddAudioCodec(const AudioCodec& codec)
    {
        amf::AMFLock    lock(&m_CritSect);
        for (auto& it : m_AudioCodecs)
        {
            if (it == codec)
            {
                return transport_common::Result::ALREADY_EXISTS;
            }
        }
        m_AudioCodecs.push_back(codec);
        return transport_common::Result::OK;
    }

    void ClientImpl::Run()
    {
        if (m_Session == nullptr)
        {
            AMFTraceError(AMF_FACILITY, L"Run: session == NULL");
        }
        else
        {
#if !defined(_WIN32) && !defined(__APPLE__)
            prctl(PR_SET_NAME, (long)"ClientImpl::Run", 0, 0, 0);
#endif
            while (StopRequested() == false)
            {
                ClientSessionImpl::Ptr session;
                {
                    amf::AMFLock    lock(&m_CritSect);
                    session = m_Session;
                }
                if (session != nullptr)
                {
                    net::ClientSession::Result  res = session->WaitForIncomingMessages();
                    if (res != net::ClientSession::Result::OK)
                    {
                        amf::AMFLock    lock(&m_CritSect);
                        m_Session = nullptr;
                    }
                }
                else
                {
                    amf_sleep(1);
                }
            }
        }
    }

    transport_common::Result AMF_STD_CALL ClientImpl::Activate()
    {
        transport_common::Result result = transport_common::Result::OK;
        if (Start() != true)
        {
            result = transport_common::Result::FAIL;
        }
        return result;
    }

    transport_common::Result AMF_STD_CALL ClientImpl::Deactivate()
    {
        ClientSessionImpl::Ptr session;
        {
            amf::AMFLock    lock(&m_CritSect);
            session = m_Session;
            m_Session = nullptr;
        }

        if (IsRunning() == true)
        {
            if (session != nullptr)
            {
                session->Terminate();
            }
            RequestStop();
            WaitForStop();
        }
        return transport_common::Result::OK;
    }

    void ClientImpl::SetSession(ClientSessionImpl* session)
    {
        amf::AMFLock    lock(&m_CritSect);
        m_Session = session;
    }

    void ClientImpl::SetDatagramSize(size_t datagramSize)
    {
        m_DatagramSize = datagramSize;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ClientImpl::DatagramClient::DatagramClient(ClientImpl* clientImpl) :
        m_ClientImpl(clientImpl)
    {

    }

    ClientImpl::DatagramClient::~DatagramClient()
    {

    }

    net::ClientSession::Ptr ClientImpl::DatagramClient::OnCreateSession(net::Client& /*client*/, const net::Socket::Address& peer, net::Socket* connectSocket, void* params)
    {
        return new amf::AMFInterfaceMultiImpl<DatagramClientSessionImpl, net::ClientSession, ClientImpl*, net::Socket*, const net::Socket::Address&, size_t >(m_ClientImpl, connectSocket, peer, params != nullptr ? reinterpret_cast<size_t>(params) : size_t(FlowCtrlProtocol::MAX_DATAGRAM_SIZE));

    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ClientImpl::StreamClient::StreamClient(ClientImpl* clientImpl) :
        m_ClientImpl(clientImpl)
    {

    }

    ClientImpl::StreamClient::~StreamClient()
    {

    }

    net::ClientSession::Ptr ClientImpl::StreamClient::OnCreateSession(net::Client& /*client*/, const net::Socket::Address& peer, net::Socket* connectSocket, void* params)
    {
        return new amf::AMFInterfaceMultiImpl<StreamClientSessionImpl, net::ClientSession, ClientImpl*, net::Socket*, const net::Socket::Address&, size_t>(m_ClientImpl, connectSocket, peer, params != nullptr ? reinterpret_cast<size_t>(params) : size_t(FlowCtrlProtocol::MAX_DATAGRAM_SIZE));

    }
}