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

#include "UDPServerSessionImpl.h"
#include "Channels.h"
#include "DgramClientSessionFlowCtrl.h"
#include "messages/service/Connect.h"

#include "net/Selector.h"

#include "public/include/core/Platform.h"
#include "public/common/TraceAdapter.h"

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::transport_amd::UDPServerSessionImpl";

namespace ssdk::transport_amd
{
    UDPServerSessionImpl::UDPServerSessionImpl(ServerImpl* server, net::DatagramSocket* sock, const net::Socket::Address& peer, uint32_t configMaxFragmentSize) :
        ServerSessionImpl(server),
        net::DatagramServerSession(peer),
        m_Socket(sock),
        m_TxMaxFragmentSize(configMaxFragmentSize),
        m_RxMaxFragmentSize(FlowCtrlProtocol::UDP_MSS_SIZE),
        m_SeqNum(0)
    {
        if (configMaxFragmentSize < FlowCtrlProtocol::UDP_MSS_SIZE)
        {
            std::stringstream infoMsg;
            infoMsg << "Configuration Error: DatagramSize setting smaller than minimum MSS: " << FlowCtrlProtocol::UDP_MSS_SIZE <<
                       " resetting the DatagramSize to max MSS: " << FlowCtrlProtocol::MAX_DATAGRAM_SIZE;
            AMFTraceError(AMF_FACILITY, L"%S", infoMsg.str().c_str());
            m_TxMaxFragmentSize = FlowCtrlProtocol::MAX_DATAGRAM_SIZE;  //Set to max
        }

        m_pFlowCtrl = FlowCtrlProtocol::Ptr(new FlowCtrlProtocol(FlowCtrlProtocol::PROTOCOL_VERSION_CURRENT));

        m_Socket->SetTimeout(5);
    }

    UDPServerSessionImpl::~UDPServerSessionImpl()
    {
        AMFTraceInfo(AMF_FACILITY, L"UDPServerSessionImpl destroyed");
    }

    transport_common::Result AMF_STD_CALL UDPServerSessionImpl::Send(Channel channel, const void* msg, size_t msgLen)
    {
        uint32_t sent = 0;
        net::Socket::Result res = m_pFlowCtrl->FragmentMessage(msg, static_cast<uint32_t>(msgLen), (uint32_t)m_TxMaxFragmentSize, static_cast<unsigned char>(channel), *this, sent);
        return (res == net::Socket::Result::OK) ? transport_common::Result::OK : transport_common::Result::FAIL;
    }

    //  Overridables:
    net::Session::Result UDPServerSessionImpl::OnInit()
    {
        return net::Session::Result::OK;
    }

    bool UDPServerSessionImpl::OnTickNotify()
    {
        return m_pFlowCtrl->TickNotify(*this);
    }

    net::Socket::Result UDPServerSessionImpl::OnFragmentReady(const FlowCtrlProtocol::Fragment& fragment, bool /*last*/)
    {
        size_t bytesSent = 0;
        return Send(fragment.GetDataToSend(), fragment.GetSizeToSend(), &bytesSent, 0);
    }

    void UDPServerSessionImpl::OnSetMaxFragmentSize(size_t fragmentSize)
    {
        m_TxMaxFragmentSize = fragmentSize;
    }

    net::Socket::Result AMF_STD_CALL UDPServerSessionImpl::Send(const void* buf, size_t size, size_t* const bytesSent, int flags)
    {
        net::Socket::Result result = net::Socket::Result::UNKNOWN_ERROR;
        //    result = m_Socket->SendTo(buf, size, GetPeerAddress(), bytesSent); //Mm original code - has to use selector to check if buffer is ready.

        {
            net::Selector selector;
            selector.AddWritableSocket(m_Socket);
            net::Socket::Set readyToSend;
            struct timeval timeout_tv = {};

            timeout_tv.tv_sec = (long)m_Socket->GetTimeout(); //MM timeout - if we cannot send - something is wrong.
            bool retry = false;
            do
            {
                retry = false;
                switch (selector.WaitToWrite(timeout_tv, readyToSend))
                {
                case net::Selector::Result::OK:
                    if (readyToSend.size() > 0)
                    {
                        result = m_Socket->SendTo(buf, size, GetPeerAddress(), bytesSent, flags);
                    }
                    else
                    {
                        retry = true;
                    }
                    break;
                case net::Selector::Result::TIMEOUT:
                    result = net::Socket::Result::CONNECTION_TIMEOUT;
                    break;
                case net::Selector::Result::INVALID_ARG:
                    result = net::Socket::Result::INVALID_ARG;
                    break;
                default:
                    result = net::Socket::Result::INVALID_ARG;
                    break;
                }
            } while (retry == true);
        }

        return result;

    }

    net::Session::Result UDPServerSessionImpl::OnDataReceived(const void* request, size_t requestSize, const net::Socket::Address& receivedFrom)
    {
        net::Session::Result result = net::Session::Result::OK;
        if (m_pFlowCtrl->ProcessFragment(request, static_cast<uint32_t>(requestSize), receivedFrom, *this, this) != FlowCtrlProtocol::Result::OK)
        {
            AMFTraceWarning(AMF_FACILITY, L"Invalid message received");
            result = net::Session::Result::RECEIVE_FAILED;
        }

        return result;
    }

    net::Session::Result UDPServerSessionImpl::OnSessionTimeout()
    {
        TimeoutNotify();
        return net::Session::Result::OK;
    }

    net::Session::Result UDPServerSessionImpl::OnSessionClose()
    {
        //	TerminateNotify();
        return net::Session::Result::OK;
    }

    void UDPServerSessionImpl::Terminate()
    {
        TerminateNotify();
        net::DatagramServerSession::Terminate();
    }

    void AMF_STD_CALL UDPServerSessionImpl::UpgradeProtocol(uint32_t version)
    {
        m_pFlowCtrl->UpgradeProtocol(version);
    }

    bool                 AMF_STD_CALL UDPServerSessionImpl::IsTerminated() const noexcept
    {
        return net::Session::IsTerminated();
    }

    void UDPServerSessionImpl::OnCompleteMessage(FlowCtrlProtocol::MessageID msgID, const void* buf, size_t size, const net::Socket::Address& receivedFrom, unsigned char optional)
    {
        static const size_t flowctrlFragmentsHeaderSize = FlowCtrlProtocol::Fragment::GetSizeOfFragmentHeader();
        unsigned char optCode = *((const unsigned char*)buf);
        Channel channel = static_cast<Channel>(optional);

        if (channel == Channel::SERVICE)
        {
            switch (static_cast<SERVICE_OP_CODE>(optCode))
            {
            case SERVICE_OP_CODE::HELLO:
                {
                    ConnectRequest helloRequest;
                    AMFTraceDebug(AMF_FACILITY, L"Received ConnectRequest");
                    if (helloRequest.ParseBuffer(buf, size))
                    {
                        net::Socket::IPv4Address peer(receivedFrom);
                        std::stringstream debugMsg;
                        std::string userIPAddress = peer.GetAddressAsString();
                        debugMsg << "HelloRequest command received from " << userIPAddress;

                        // Add user Platform info to this session's property storage.
                        amf::AMFVariant userPlatformStruct;
                        m_PeerPlatform = helloRequest.GetPlatformInfo();

                        uint32_t supportedVersion = m_pFlowCtrl->MaxSupportedVersion(FlowCtrlProtocol::PROTOCOL_VERSION_MIN,
                            FlowCtrlProtocol::PROTOCOL_VERSION_CURRENT, helloRequest.GetMinProtocolVersion(), helloRequest.GetProtocolVersion());

                        if (supportedVersion != FlowCtrlProtocol::PROTOCOL_VERSION_UNSUPPORTED)
                        {
                            UpgradeProtocol(supportedVersion);
                            amf::AMFPropertyStoragePtr propStorageSession(this);
                            PopulateOptionsFromHello(helloRequest, propStorageSession);
                            if (m_Server->AuthorizeConnectionRequest(this, helloRequest.GetDeviceID()) == true)
                            {
                                HelloResponse resp(m_Server->GetName().c_str(), (unsigned char)supportedVersion, FlowCtrlProtocol::PROTOCOL_VERSION_MIN,
                                                   m_Server->GetPort(), static_cast<uint32_t>(m_RxMaxFragmentSize));
                                m_Server->FillOptions(false, this, &resp.GetOptions());
                                resp.UpdateData();
                                Send(Channel::SERVICE, resp.GetSendData(), resp.GetSendSize());
                                AMFTraceDebug(AMF_FACILITY, L"Send ===>> HelloResponse (max rx datagram size: %d)", m_RxMaxFragmentSize);
                            }
                            else
                            {
                                // reset to the lowest version
                                UpgradeProtocol(supportedVersion);
                                //m_pFlowCtrl = std::move(FlowCtrlProtocol::FlowCtrlProtocolFactory(supportedVersion));
                                HelloRefused refused;
                                Send(Channel::SERVICE, refused.GetSendData(), refused.GetSendSize());
                                debugMsg << " UDPServerSessionImpl::OnCompleteMessage: connection refused as unauthorized";
                                AMFTraceInfo(AMF_FACILITY, L"%S", debugMsg.str().c_str());
                            }
                        }
                        else
                        {
                            debugMsg << " with protocol version less then supported : server =" << (int)FlowCtrlProtocol::PROTOCOL_VERSION_CURRENT;
                            debugMsg << " serverMin = " << (int)FlowCtrlProtocol::PROTOCOL_VERSION_MIN;
                            debugMsg << " client = " << (int)helloRequest.GetProtocolVersion();
                            AMFTraceError(AMF_FACILITY, L"%S", debugMsg.str().c_str());
                        }
                    }
                    break;
                }
            default:
                if (m_Callback != nullptr)
                {
                    m_Callback->OnMessageReceived(this, channel, msgID, buf, size);
                }
                break;
            }
        }
        else
        {
            if (m_Callback != nullptr)
            {
                m_Callback->OnMessageReceived(this, channel, msgID, buf, size);
            }
        }
    }

    net::Socket::Result UDPServerSessionImpl::OnRequestFragment(const FlowCtrlProtocol::Fragment& fragment)
    {
        size_t bytesSent = 0;

        return Send(fragment.GetDataToSend(), fragment.GetSizeToSend(), &bytesSent, 0);
    }

    void AMF_STD_CALL UDPServerSessionImpl::OnPropertyChanged(const wchar_t* name)
    {
        if (std::wcscmp(name, DATAGRAM_MSG_INTERVAL) == 0)
        {
            if (m_pFlowCtrl != nullptr)
            {
                amf::AMFVariantStruct vsInterval;
                GetProperty(DATAGRAM_MSG_INTERVAL, &vsInterval);
                amf_int64 interval = amf::AMFVariantGetInt64(&vsInterval);

                m_pFlowCtrl->ModifyInterval(interval);
            }
        }
        else if (std::wcscmp(name, DATAGRAM_LOST_MSG_THRESHOLD) == 0)
        {
            if (m_pFlowCtrl != nullptr)
            {
                amf::AMFVariantStruct vslostMsgThreshold;
                GetProperty(DATAGRAM_LOST_MSG_THRESHOLD, &vslostMsgThreshold);
                amf_int64 lostMsgThreshold = amf::AMFVariantGetInt64(&vslostMsgThreshold);

                m_pFlowCtrl->ModifyLostMsgCountThreshold(lostMsgThreshold);
            }
        }
        else if (std::wcscmp(name, DATAGRAM_TURNING_POINT_THRESHOLD) == 0)
        {
            if (m_pFlowCtrl != nullptr)
            {
                amf::AMFVariantStruct vsTpThreshold;
                GetProperty(DATAGRAM_TURNING_POINT_THRESHOLD, &vsTpThreshold);
                amf_int64 tpThreshold = amf::AMFVariantGetInt64(&vsTpThreshold);

                m_pFlowCtrl->ModifyDecisionThreshold(tpThreshold);
            }
        }
    }

}