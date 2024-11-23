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

#include "ServerDiscovery.h"
#include "ClientImpl.h"
#include "ClientSessionImpl.h"
#include "amf/public/common/TraceAdapter.h"

static constexpr const time_t DISCOVERY_TIMEOUT = 10;

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::transport_amd::ServerDiscovery";

namespace ssdk::transport_amd
{
    //  Options:
    const char* ServerParametersImpl::OPTION_MICROPHONE = "Microphone";
    const char* ServerParametersImpl::OPTION_HORIZONTAL_FOV = "HorizontalFOV";
    const char* ServerParametersImpl::OPTION_VERTICAL_FOV = "VerticalFOV";
    const char* ServerParametersImpl::OPTION_ENCODER_FRAME_SIZE_EYE = "EncoderSize";  // AMFSize
    const char* ServerParametersImpl::OPTION_KEYBOARD = "KeyboardPresent";
    const char* ServerParametersImpl::OPTION_MOUSE = "MousePresent";
    const char* ServerParametersImpl::OPTION_TOUCHSCREEN = "TouchScreenPresent";
    const char* ServerParametersImpl::OPTION_PROFILE_PAYLOAD_SIZE = "ProfileFragmentSize";
    const char* ServerParametersImpl::OPTION_ENCRYPTION = "Cipher";


    ServerParametersImpl::ServerParametersImpl(HelloResponse& serverDescriptor, const net::Url& url) :
        m_Name(serverDescriptor.GetName()),
        m_Url(url.GetUrl()),
        m_Version(serverDescriptor.GetProtocolVersion()),
        m_DatagramSize(serverDescriptor.GetDatagramSize()),
        m_Options(serverDescriptor.GetOptions()),
        m_OsName(serverDescriptor.GetOsName())
    {
    }


    ServerParametersImpl::ServerParametersImpl(HelloResponse& serverDescriptor, const std::string& transport, const net::Socket::Address& address) :
        m_Name(serverDescriptor.GetName()),
        m_Url(static_cast<net::Socket::IPv4Address>(address).GetAddressAsString()),
        m_Version(serverDescriptor.GetProtocolVersion()),
        m_DatagramSize(serverDescriptor.GetDatagramSize()),
        m_Options(serverDescriptor.GetOptions()),
        m_OsName(serverDescriptor.GetOsName())
    {
        net::Url srvUrl(static_cast<net::Socket::IPv4Address>(address).GetAddressAsString(), transport, serverDescriptor.GetPort());
        srvUrl.SetPort(serverDescriptor.GetPort());
        m_Url = srvUrl.GetUrl();
    }


    const char* AMF_STD_CALL ServerParametersImpl::GetOsName() const
    {
        return m_OsName.c_str();
    }

    const char* AMF_STD_CALL ServerParametersImpl::GetName() const
    {
        return m_Name.c_str();
    }

    const char* AMF_STD_CALL ServerParametersImpl::GetUrl() const
    {
        return m_Url.c_str();
    }

    int32_t AMF_STD_CALL ServerParametersImpl::GetVersion() const
    {
        return m_Version;
    }

    size_t AMF_STD_CALL ServerParametersImpl::GetDatagramSize() const
    {
        return m_DatagramSize;
    }

    bool AMF_STD_CALL ServerParametersImpl::GetEncryption() const
    {
        bool encryption = false;
        m_Options.GetBool(OPTION_ENCRYPTION, encryption);
        return encryption;
    }

    bool AMF_STD_CALL ServerParametersImpl::GetOptionBool(const char* name, bool& value) const
    {
        return m_Options.GetBool(name, value);
    }

    bool AMF_STD_CALL ServerParametersImpl::GetOptionUInt32(const char* name, uint32_t& value) const
    {
        return m_Options.GetUInt32(name, value);
    }

    bool AMF_STD_CALL ServerParametersImpl::GetOptionInt32(const char* name, int32_t& value) const
    {
        return m_Options.GetInt32(name, value);
    }

    bool AMF_STD_CALL ServerParametersImpl::GetOptionInt64(const char* name, int64_t& value) const
    {
        return m_Options.GetInt64(name, value);
    }

    bool AMF_STD_CALL ServerParametersImpl::GetOptionFloat(const char* name, float& value) const
    {
        return m_Options.GetFloat(name, value);
    }

    bool AMF_STD_CALL ServerParametersImpl::GetOptionSize(const char* name, AMFSize& value) const
    {
        return m_Options.GetSize(name, value);
    }

    bool AMF_STD_CALL ServerParametersImpl::GetOptionRect(const char* name, AMFRect& value) const
    {
        return m_Options.GetRect(name, value);
    }

    bool AMF_STD_CALL ServerParametersImpl::GetOptionRate(const char* name, AMFRate& value) const
    {
        return m_Options.GetRate(name, value);
    }

    bool AMF_STD_CALL ServerParametersImpl::GetOptionString(const char* name, std::string& value) const
    {
        return m_Options.GetString(name, value);
    }


    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ServerDiscoverySession::ServerDiscoverySession(const std::string& deviceID, net::Socket* sock, const net::Socket::Address& peer, size_t receiveBufSize, ServerParametersImpl::Collection& servers, unsigned short /*port*/) :
        DatagramClientSessionFlowCtrl(sock, peer, receiveBufSize, DISCOVERY_TIMEOUT),
        m_Servers(servers),
        m_ClientDeviceID(deviceID),
        m_ReceiveBufSize(receiveBufSize)
    {
        m_Peer = peer;
    }

    DatagramClientSessionFlowCtrl::Result ServerDiscoverySession::EnumerateServers(ServerEnumCallback* callback, size_t* size, amf::AMFPropertyStoragePtr options)
    {
        DatagramClientSessionFlowCtrl::Result res = DatagramClientSessionFlowCtrl::Result::OK;
        m_LastResult = DatagramClientSessionFlowCtrl::Result::OK;
        DiscoveryRequest helloRequest(FlowCtrlProtocol::PROTOCOL_VERSION_CURRENT, FlowCtrlProtocol::PROTOCOL_VERSION_MIN, m_ClientDeviceID.c_str(), m_ReceiveBufSize, options);
        Broadcast(helloRequest.GetSendData(), helloRequest.GetSendSize(), uint8_t(Channel::SERVICE));
        time_t startTime;
        time(&startTime);
        time_t curTime;
        time_t timeout = GetTimeout();
        m_Callback = callback;
        m_KeepLooking = ServerEnumCallback::DiscoveryCtrl::CONTINUE;
#pragma warning(disable:4127)
        while (true)
#pragma warning(default:4127)
        {
            WaitForIncoming();
            if (m_KeepLooking != ServerEnumCallback::DiscoveryCtrl::CONTINUE)
            {
                res = m_LastResult;
                AMFTraceDebug(AMF_FACILITY, L"Was instructed to stop looking for new servers");
                break;
            }
            time(&curTime);
            if ((curTime - startTime) > timeout)
            {
                AMFTraceDebug(AMF_FACILITY, L"Stopped looking for new servers due to timeout");
                break;
            }
        }
        AMFTraceDebug(AMF_FACILITY, L"Exited the discovery loop");
        if ((*size = m_Servers.size()) == 0)
        {
            res = DatagramClientSessionFlowCtrl::Result::TIMEOUT;
        }
        m_Callback = nullptr;
        return res;
    }

    DatagramClientSessionFlowCtrl::Result ServerDiscoverySession::QueryServerInfo(ServerEnumCallback* callback, amf::AMFPropertyStoragePtr options)
    {
        DatagramClientSessionFlowCtrl::Result res = DatagramClientSessionFlowCtrl::Result::OK;
        m_LastResult = DatagramClientSessionFlowCtrl::Result::OK;
        DiscoveryRequest helloRequest(FlowCtrlProtocol::PROTOCOL_VERSION_CURRENT, FlowCtrlProtocol::PROTOCOL_VERSION_MIN, m_ClientDeviceID.c_str(), m_ReceiveBufSize, options);
        Send(helloRequest.GetSendData(), helloRequest.GetSendSize(), uint8_t(Channel::SERVICE));
        m_Callback = callback;
        m_KeepLooking = ServerEnumCallback::DiscoveryCtrl::ABORT;

        WaitForIncoming();

        res = m_LastResult;

        AMFTraceDebug(AMF_FACILITY, L"Queried server info");

        if (m_Servers.size() == 0)
        {
            res = DatagramClientSessionFlowCtrl::Result::TIMEOUT;
        }

        m_Callback = nullptr;

        return res;
    }

    DatagramClientSessionFlowCtrl::Result ServerDiscoverySession::OnInit()
    {
        return DatagramClientSessionFlowCtrl::Result::OK;
    }

    void ServerDiscoverySession::OnCompleteMessage(FlowCtrlProtocol::MessageID /*msgID*/, const void* buf, size_t size, const net::Socket::Address& receivedFrom, uint8_t optional)
    {
        uint8_t optCpde = *((const uint8_t*)buf);
        if (optional == uint8_t(Channel::SERVICE))
        {
            if (optCpde == uint8_t(SERVICE_OP_CODE::HELLO) || optCpde == uint8_t(SERVICE_OP_CODE::DISCOVERY))
            {
                HelloResponse   resp;
                resp.ParseBuffer(buf, size);
                AMFTraceDebug(AMF_FACILITY, L"Server @%S is accepting connections", receivedFrom.GetParsedAddressAsString().c_str());
                unsigned char serverVersion = static_cast<unsigned char>(resp.GetProtocolVersion());
                if (serverVersion >= FlowCtrlProtocol::PROTOCOL_VERSION_MIN)
                {
                    const std::vector<std::string>& transports(resp.GetSupportedTransports());
                    for (std::vector<std::string>::const_iterator it = transports.begin(); it != transports.end(); ++it)
                    {
                        ServerParameters::Ptr    server(new ServerParametersImpl(resp, *it, receivedFrom));
                        m_Servers.push_back(server);
                        if (m_Callback != nullptr)
                        {
                            m_KeepLooking = m_Callback->OnServerDiscovered(server);
                            if (m_KeepLooking == ssdk::transport_amd::ServerEnumCallback::DiscoveryCtrl::ABORT)
                            {
                                break;
                            }
                        }
                        m_LastResult = DatagramClientSessionFlowCtrl::Result::OK;
                    }
                }
                else
                {
                    m_LastResult = DatagramClientSessionFlowCtrl::Result::INVALID_VERSION;
                }

            }
            else if (optCpde == uint8_t(SERVICE_OP_CODE::CONNECTION_REFUSED))
            {
                HelloRefused   refused;
                refused.ParseBuffer(buf, size);
                AMFTraceDebug(AMF_FACILITY, L"Server @%S is refusing connections", receivedFrom.GetParsedAddressAsString().c_str());
                if (m_Callback != nullptr)
                {
                    m_KeepLooking = m_Callback->OnConnectionRefused();
                }
                m_LastResult = DatagramClientSessionFlowCtrl::Result::REFUSED;
            }
        }
    }


    DatagramClientSessionFlowCtrl::Result ServerDiscoverySession::OnSessionTimeout()
    {
        AMFTraceDebug(AMF_FACILITY, L"ServerDiscoverySession::OnSessionTimeout()");
        return DatagramClientSessionFlowCtrl::Result::OK;
    }

    DatagramClientSessionFlowCtrl::Result ServerDiscoverySession::OnSessionClose()
    {
        AMFTraceDebug(AMF_FACILITY, L"ServerDiscoverySession::OnSessionClose()");
        return DatagramClientSessionFlowCtrl::Result::OK;
    }


    DatagramClientSessionFlowCtrl::Result ServerDiscoverySession::OnTerminate()
    {
        AMFTraceDebug(AMF_FACILITY, L"ServerDiscoverySession::OnTerminate()");
        return DatagramClientSessionFlowCtrl::Result::OK;
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    DiscoveryClient::DiscoveryClient(const std::string& deviceID, ServerParametersImpl::Collection& servers, unsigned short port) :
        m_Servers(servers),
        m_Port(port),
        m_DeviceID(deviceID)
    {
        servers.clear();
    }

    net::ClientSession::Ptr DiscoveryClient::OnCreateSession(Client& /*client*/, const net::Socket::Address& peer, net::Socket* connectSocket, void* /*params*/)
    {
        net::ClientSession::Ptr session;
        session = new amf::AMFInterfaceMultiImpl<ServerDiscoverySession, DatagramClientSessionFlowCtrl, const std::string&, net::Socket*, const net::Socket::Address&, size_t, ServerParametersImpl::Collection&, unsigned short >(m_DeviceID, connectSocket, peer, FlowCtrlProtocol::UDP_MAX_MSS_SIZE_WITH_NO_FRAGMENTATION, m_Servers, m_Port);
        return session;
    }
}