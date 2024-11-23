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

#include "DiscoverySessionImpl.h"
#include "Misc.h"
#include "DgramClientSessionFlowCtrl.h"
#include "public/include/core/Platform.h"
#include "public/common/TraceAdapter.h"
#include "net/Selector.h"

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::transport_amd::DiscoverySessionImpl";

namespace ssdk::transport_amd
{
    DiscoveryServerSessionImpl::DiscoveryServerSessionImpl(ServerImpl* server, net::DatagramSocket* sock, const net::Socket::Address& peer, size_t sizeReceived) :
        UDPServerSessionImpl(server, sock, peer, FlowCtrlProtocol::UDP_MSS_SIZE),	//	548 bytes is the largest safe UDP datagram size that is guaranteed to pass through all routers
        m_MaxSizeReceived(sizeReceived)
    {
    }

    DiscoveryServerSessionImpl::~DiscoveryServerSessionImpl()
    {
        AMFTraceInfo(AMF_FACILITY, L"DiscoveryServerSessionImpl destroyed");
    }

    void DiscoveryServerSessionImpl::OnCompleteMessage(FlowCtrlProtocol::MessageID /*msgID*/, const void* buf, size_t size, const net::Socket::Address& receivedFrom, unsigned char optional)
    {
        uint8_t optCode = *(reinterpret_cast<const uint8_t*>(buf));
        //    const Command* command = reinterpret_cast<const Command*>(buf);
        Channel channel = static_cast<Channel>(optional);
        if (channel == Channel::SERVICE && optCode == uint8_t(SERVICE_OP_CODE::DISCOVERY))   //  Special case for discovery broadcast
        {
            ConnectRequest helloRequest;
            if (helloRequest.ParseBuffer(buf, size))
            {
                net::Socket::IPv4Address peer(receivedFrom);
                std::string debugMsg = "(Discovery) HelloRequest command received from ";
                debugMsg += peer.GetAddressAsString();
                if (helloRequest.GetProtocolVersion() >= FlowCtrlProtocol::PROTOCOL_VERSION_MIN)
                {
                    amf::AMFPropertyStoragePtr propStorage(this);
                    PopulateOptionsFromHello(helloRequest, propStorage);
                    if (m_Server->AuthorizeDiscoveryRequest(this, helloRequest.GetDeviceID()) == true)
                    {
                        AMFTraceInfo(AMF_FACILITY, L"%S", debugMsg.c_str());

                        size_t datagramSize = m_Server->GetDatagramSize();
                        if (helloRequest.GetMaxDatagramSize() < datagramSize)
                        {
                            datagramSize = helloRequest.GetMaxDatagramSize();
                            m_TxMaxFragmentSize = datagramSize;
                        }

                        std::vector<std::string> transports;
                        if (m_Server->IsTCPSupported() == true)
                        {
                            transports.push_back("TCP");
                        }
                        if (m_Server->IsUDPSupported() == true)
                        {
                            transports.push_back("UDP");
                        }
                        HelloResponse resp(m_Server->GetName().c_str(), FlowCtrlProtocol::PROTOCOL_VERSION_CURRENT, FlowCtrlProtocol::PROTOCOL_VERSION_MIN,
                                           m_Server->GetPort(), static_cast<uint32_t>(datagramSize), transports);
                        m_Server->FillOptions(true, this, &resp.GetOptions());
                        resp.UpdateData();

                        Send(Channel::SERVICE, resp.GetSendData(), resp.GetSendSize());
                        AMFTraceDebug(AMF_FACILITY, L"Send ===>> SERVICE_OP_CODE_HELLO");
                    }
                    else
                    {
                        AMFTraceInfo(AMF_FACILITY, L"Discovery request from a non-paired device %S - ignored", helloRequest.GetDeviceID().c_str());
                    }
                }
                else
                {
                    AMFTraceError(AMF_FACILITY, L"%S with protocol version less then supported : server = %d serverMin = %d client = %d", debugMsg.c_str(),
                        (int)FlowCtrlProtocol::PROTOCOL_VERSION_CURRENT, (int)FlowCtrlProtocol::PROTOCOL_VERSION_MIN, (int)helloRequest.GetProtocolVersion());
                }
            }
        }
        else
        {
            AMFTraceWarning(AMF_FACILITY, L"Invalid channel (%d) or opt (%d) code for received message", (int)channel, (int)optCode);
        }
    }


}