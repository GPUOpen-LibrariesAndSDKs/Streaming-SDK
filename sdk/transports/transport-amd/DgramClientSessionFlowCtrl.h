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

#pragma once

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif


#include "net/Socket.h"
#include "net/DatagramClientSession.h"
#include "transports/transport-amd/FlowCtrlProtocol.h"
#include "transports/transport-amd/Channels.h"

#include <map>
#include <memory>


namespace ssdk::transport_amd
{
    class DatagramClientSessionFlowCtrl :
        public net::DatagramClientSession,
        protected FlowCtrlProtocol::ProcessIncomingCallback
    {
    public:

        AMF_DECLARE_IID(0x10b9a314, 0x9463, 0x45bb, 0x99, 0x85, 0xc3, 0xd4, 0x15, 0xc7, 0xe7, 0xed);

        typedef amf::AMFInterfacePtr_T<DatagramClientSessionFlowCtrl> Ptr;

        virtual void            AMF_STD_CALL EnableProfile(bool bEnable);
        void UpgradeProtocol(uint32_t version);

    private:
        class OutgoingCB :
            public FlowCtrlProtocol::ProcessOutgoingCallback
        {
        public:
            OutgoingCB();

            inline void SetSession(DatagramClientSessionFlowCtrl* session) { m_Session = session; }

            inline int GetSocketFlags() const throw() { return m_SocketFlags; }
            inline void SetSocketFlags(int flags) throw() { m_SocketFlags = flags; }

        protected:
            DatagramClientSessionFlowCtrl* m_Session;
            int m_SocketFlags;
        };

        class SendCB :
            public OutgoingCB
        {
        public:
            SendCB() {}

        protected:
            virtual net::Socket::Result OnFragmentReady(const FlowCtrlProtocol::Fragment& fragment, bool last) override;
            virtual void OnSetMaxFragmentSize(size_t fragmentSize) override;
        };
        friend class SendCB;

        class BroadcastCB :
            public OutgoingCB
        {
        public:
            BroadcastCB() {}

        protected:
            virtual net::Socket::Result OnFragmentReady(const FlowCtrlProtocol::Fragment& fragment, bool last) override;
            virtual void OnSetMaxFragmentSize(size_t fragmentSize) override;
        };
        friend class BroadcastCB;


    protected:
        DatagramClientSessionFlowCtrl(net::Socket* sock, const net::Socket::Address& peer, size_t receiveBufSize, time_t timeout);
        virtual net::Socket::Result OnRequestFragment(const FlowCtrlProtocol::Fragment& fragment) override;

    public:
        net::Socket::Result SendDatagram(const void* buf, size_t bufSize, size_t* const bytesSent = nullptr, int flags = 0);
        net::Socket::Result SendDatagramTo(const net::Socket::Address& peer, const void* buf, size_t bufSize, size_t* const bytesSent = nullptr, int flags = 0);
        net::Socket::Result BroadcastDatagram(const void* buf, size_t bufSize, size_t* const bytesSent = nullptr, int flags = 0);

        net::Socket::Result Send(const void* buf, size_t bufSize, uint8_t optional, size_t* const bytesSent = nullptr, int flags = 0);
        net::Socket::Result Broadcast(const void* buf, size_t bufSize, uint8_t optional, size_t* const bytesSent = nullptr, int flags = 0);

    private:
        net::Socket::Result SendOrBroadcastMessage(const void* buf, size_t bufSize, uint8_t optional, size_t* const bytesSent, int flags, std::unique_ptr <FlowCtrlProtocol>& pFlowCtrl, OutgoingCB& callback);

    protected:
        virtual bool AMF_STD_CALL OnTickNotify() override;
        virtual Result AMF_STD_CALL OnDataReceived(const void* request, size_t requestSize, const net::Socket::Address& receivedFrom) override;

    protected:
        net::Socket::Ptr   m_Socket;
        time_t              m_Timeout;
        std::unique_ptr <FlowCtrlProtocol>  m_pFlowCtrl;
        SendCB              m_SendCB;
        std::unique_ptr <FlowCtrlProtocol>  m_pFlowCtrlBroadcast;
        BroadcastCB         m_BroadcastCB;
        typedef std::map<net::Socket::Address, std::unique_ptr<FlowCtrlProtocol>>   FlowCtrlProtocolMap;
        FlowCtrlProtocolMap m_ReceiverFlowCtrl;
    };


}