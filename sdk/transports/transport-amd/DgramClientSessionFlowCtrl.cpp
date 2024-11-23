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

#include "DgramClientSessionFlowCtrl.h"
#include "net/Selector.h"
#include "net/DatagramSocket.h"
#include "amf/public/common/TraceAdapter.h"
#include <sstream>


namespace ssdk::transport_amd
{

    static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::transport_amd::DatagramClientSessionFlowCtrl";

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    DatagramClientSessionFlowCtrl::OutgoingCB::OutgoingCB() :
        m_Session(nullptr),
        m_SocketFlags(0)
    {
    }


    DatagramClientSessionFlowCtrl::DatagramClientSessionFlowCtrl(net::Socket* sock, const net::Socket::Address& peer, size_t receiveBufSize, time_t timeout) :
        DatagramClientSession(sock, peer, receiveBufSize),
        m_Socket(sock),
        m_Timeout(timeout)
    {
        m_pFlowCtrl = FlowCtrlProtocol::Ptr(new FlowCtrlProtocol(FlowCtrlProtocol::PROTOCOL_VERSION_CURRENT));
        m_pFlowCtrlBroadcast = FlowCtrlProtocol::Ptr(new FlowCtrlProtocol(FlowCtrlProtocol::PROTOCOL_VERSION_CURRENT));
        m_SendCB.SetSession(this);
        m_BroadcastCB.SetSession(this);

        SetTimeout(timeout);
    }

    void            AMF_STD_CALL DatagramClientSessionFlowCtrl::EnableProfile(bool bEnable)
    {
        for (FlowCtrlProtocolMap::iterator it = m_ReceiverFlowCtrl.begin(); it != m_ReceiverFlowCtrl.end(); ++it)
        {
            it->second->EnableProfile(bEnable);
        }

        m_pFlowCtrl->EnableProfile(bEnable);
    }

    net::Socket::Result DatagramClientSessionFlowCtrl::SendDatagram(const void* buf, size_t bufSize, size_t* const bytesSent, int flags)
    {
        return SendDatagramTo(GetPeerAddress(), buf, bufSize, bytesSent, flags);
    }

    net::Socket::Result DatagramClientSessionFlowCtrl::SendDatagramTo(const net::Socket::Address& /*peer*/, const void* buf, size_t size, size_t* const bytesSent, int flags)
    {
        net::Socket::Result result = net::Socket::Result::UNKNOWN_ERROR;
        net::Selector selector;
        selector.AddWritableSocket(m_Socket);
        net::Socket::Set readyToSend;
        struct timeval timeout_tv {};
        timeout_tv.tv_sec = (long)m_Timeout;
        bool retry = false;
        do
        {
            retry = false;
            switch (selector.WaitToWrite(timeout_tv, readyToSend))
            {
            case net::Selector::Result::OK:
                if (readyToSend.size() > 0)
                {
                    result = net::DatagramSocket::Ptr(m_Socket)->SendTo(buf, size, GetPeerAddress(), bytesSent, flags);
                }
                else
                {
                    retry = true;
                }
                break;
            case net::Selector::Result::TIMEOUT:
                result = net::Socket::Result::CONNECTION_TIMEOUT;
                AMFTraceWarning(AMF_FACILITY, L"Send timed out");
                break;
            default:
                break;
            }
        } while (retry == true);
        return result;
    }

    net::Socket::Result DatagramClientSessionFlowCtrl::BroadcastDatagram(const void* buf, size_t bufSize, size_t* const bytesSent, int flags)
    {
        net::Socket::Result result = net::Socket::Result::UNKNOWN_ERROR;
        net::Selector selector;
        selector.AddWritableSocket(m_Socket);
        net::Socket::Set readyToSend;
        struct timeval timeout_tv {};
        timeout_tv.tv_sec = (long)m_Timeout;
        bool retry = false;
        do
        {
            retry = false;
            switch (selector.WaitToWrite(timeout_tv, readyToSend))
            {
            case net::Selector::Result::OK:
                if (readyToSend.size() > 0)
                {
                    int yes = 1;
                    m_Socket->SetSocketOpt(SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));
                    m_Socket->SetSocketOpt(SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
                    switch (m_Socket->GetAddressFamily())
                    {
                    case net::Socket::AddressFamily::ADDR_IP:
                        net::DatagramSocket::Ptr(m_Socket)->SetNICDataExpiration(0);
                        result = net::DatagramSocket::Ptr(m_Socket)->Broadcast(buf, bufSize, static_cast<const net::Socket::IPv4Address&>(GetPeerAddress()).GetPort(), bytesSent, flags);
                        break;
                    default:
                        AMFTraceError(AMF_FACILITY, L"Broadcast not supported");
                    }
                }
                else
                {
                    retry = true;
                }
                break;
            case net::Selector::Result::TIMEOUT:
                result = net::Socket::Result::CONNECTION_TIMEOUT;
                AMFTraceWarning(AMF_FACILITY, L"Send timed out");
                break;
            default:
                break;
            }
        } while (retry == true);
        return result;
    }

    void DatagramClientSessionFlowCtrl::UpgradeProtocol(uint32_t version)
    {
        for (FlowCtrlProtocolMap::iterator it = m_ReceiverFlowCtrl.begin(); it != m_ReceiverFlowCtrl.end(); ++it)
        {
            it->second->UpgradeProtocol(version);
        }
        m_pFlowCtrl->UpgradeProtocol(version);
    }

    net::Socket::Result  DatagramClientSessionFlowCtrl::SendCB::OnFragmentReady(const FlowCtrlProtocol::Fragment& fragment, bool /*last*/)
    {
        net::Socket::Result result;
        size_t bytesSent = 0;
        if ((result = m_Session->SendDatagram(fragment.GetDataToSend(), fragment.GetSizeToSend(), &bytesSent, m_SocketFlags)) != net::Socket::Result::OK)
        {
            std::stringstream errMsg;
            errMsg << "Failed to send fragment: Socket::Result==" << int(result);
            AMFTraceError(AMF_FACILITY, L"%S", errMsg.str().c_str());
        }
        else if (bytesSent != fragment.GetSizeToSend())
        {
            AMFTraceError(AMF_FACILITY, L"OnFragmentReady() sent less then asked: asked=%d sent=%d", (int)fragment.GetSizeToSend(), (int)bytesSent);
        }

        return result;
    }

    void DatagramClientSessionFlowCtrl::SendCB::OnSetMaxFragmentSize(size_t fragmentSize)
    {
        m_Session->SetTxMaxFragmentSize(fragmentSize);
    }

    net::Socket::Result DatagramClientSessionFlowCtrl::BroadcastCB::OnFragmentReady(const FlowCtrlProtocol::Fragment& fragment, bool /*last*/)
    {
        net::Socket::Result result;
        size_t bytesSent = 0;
        if ((result = m_Session->BroadcastDatagram(fragment.GetDataToSend(), fragment.GetSizeToSend(), &bytesSent, m_SocketFlags)) != net::Socket::Result::OK)
        {
            std::stringstream errMsg;
            errMsg << "Failed to broadcast fragment: Socket::Result==" << int(result);
            AMFTraceError(AMF_FACILITY, L"%S", errMsg.str().c_str());
        }
        return result;
    }

    void DatagramClientSessionFlowCtrl::BroadcastCB::OnSetMaxFragmentSize(size_t fragmentSize)
    {
        m_Session->SetTxMaxFragmentSize(fragmentSize);
    }

    net::Socket::Result DatagramClientSessionFlowCtrl::SendOrBroadcastMessage(const void* buf, size_t bufSize, uint8_t optional, size_t* const bytesSent, int flags, std::unique_ptr <FlowCtrlProtocol>& pFlowCtrl, OutgoingCB& callback)
    {
        callback.SetSocketFlags(flags);
        uint32_t sentReal = 0;
        net::Socket::Result res = pFlowCtrl->FragmentMessage(buf, (uint32_t)bufSize, (uint32_t)m_TxMaxFragmentSize, optional, callback, sentReal);

        if (bytesSent != nullptr)
        {
            *bytesSent = (size_t)sentReal;
        }
        if (sentReal == 0)
        {
            res = net::Socket::Result::UNKNOWN_ERROR;
        }
        return res;
    }

    net::Socket::Result DatagramClientSessionFlowCtrl::Send(const void* buf, size_t bufSize, uint8_t optional, size_t* const bytesSent, int flags)
    {
        return SendOrBroadcastMessage(buf, bufSize, optional, bytesSent, flags, m_pFlowCtrl, m_SendCB);
    }

    net::Socket::Result DatagramClientSessionFlowCtrl::Broadcast(const void* buf, size_t bufSize, uint8_t optional, size_t* const bytesSent, int flags)
    {
        return SendOrBroadcastMessage(buf, bufSize, optional, bytesSent, flags, m_pFlowCtrlBroadcast, m_BroadcastCB);
    }

    bool DatagramClientSessionFlowCtrl::OnTickNotify()
    {
        bool messagePosted = false;

        for (FlowCtrlProtocolMap::iterator it = m_ReceiverFlowCtrl.begin(); it != m_ReceiverFlowCtrl.end(); ++it)
        {
            if (it->second->TickNotify(*this) == true)
            {
                messagePosted = true;
            }
        }
        return messagePosted;
    }

    DatagramClientSessionFlowCtrl::Result DatagramClientSessionFlowCtrl::OnDataReceived(const void* request, size_t requestSize, const net::Socket::Address& receivedFrom)
    {
        FlowCtrlProtocolMap::iterator flowCtrlForAddress = m_ReceiverFlowCtrl.find(receivedFrom);
        if (flowCtrlForAddress == m_ReceiverFlowCtrl.end())
        {
            m_ReceiverFlowCtrl[receivedFrom] = FlowCtrlProtocol::Ptr(new FlowCtrlProtocol(3));
            flowCtrlForAddress = m_ReceiverFlowCtrl.find(receivedFrom);
        }

        DatagramClientSessionFlowCtrl::Result result = DatagramClientSessionFlowCtrl::Result::OK;
        flowCtrlForAddress->second->EnableProfile(m_pFlowCtrl->GetEnableProfile());
        if (flowCtrlForAddress->second->ProcessFragment(request, (uint32_t)requestSize, receivedFrom, *this) != FlowCtrlProtocol::Result::OK)
        {
            AMFTraceWarning(AMF_FACILITY, L"Invalid message received");
            result = DatagramClientSessionFlowCtrl::Result::RECEIVE_FAILED;
        }
        return result;
    }

    net::Socket::Result DatagramClientSessionFlowCtrl::OnRequestFragment(const FlowCtrlProtocol::Fragment& fragment)
    {
        size_t bytesSent = 0;

        return SendDatagram(fragment.GetDataToSend(), fragment.GetSizeToSend(), &bytesSent);
    }
}