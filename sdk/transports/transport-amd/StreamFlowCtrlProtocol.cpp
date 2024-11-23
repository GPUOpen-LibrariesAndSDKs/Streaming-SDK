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

#include "FlowCtrlProtocol.h"
#include "public/include/core/Platform.h"
#include "public/common/Thread.h"
#include "public/common/TraceAdapter.h"

#define TRACE_SCOPE L"StreamFlowCtrlProtocol"

namespace ssdk::transport_amd
{
    //--------------------------------------------------------------------------------------------------------------------
    // StreamFlowCtrlProtocol
    //--------------------------------------------------------------------------------------------------------------------
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ssdk::transport_amd::StreamFlowCtrlProtocol::StreamFlowCtrlProtocol()
    {
    }

    FlowCtrlProtocol::MessageID ssdk::transport_amd::StreamFlowCtrlProtocol::PrepareMessage(Channel channel, const void* msg, size_t msgSize)
    {
        if (msgSize == 0)
        {
            AMFTraceWarning(TRACE_SCOPE, L"StreamFlowCtrlProtocol: 0 message size");
        }
        static FlowCtrlProtocol::MessageID msgID = 0;
        StreamMessageHeader header;
        header.m_ChannelID = static_cast<uint8_t>(channel);
        header.m_MsgSize = htonl(static_cast<uint32_t>(msgSize));
        header.m_MsgID = htons(msgID);
        size_t newSize = msgSize + sizeof(header);
        if (newSize > m_SendBufSize)
        {
            m_SendBuf.reset(new uint8_t[newSize]);
            m_SendBufSize = newSize;
        }
        m_OutMsgSize = msgSize;
        memcpy(m_SendBuf.get(), &header, sizeof(header));
        memcpy(m_SendBuf.get() + sizeof(header), msg, msgSize);
        return msgID++;
    }

    ssdk::transport_amd::StreamFlowCtrlProtocol::~StreamFlowCtrlProtocol()
    {
    }

    ssdk::transport_amd::StreamFlowCtrlProtocol::Result ssdk::transport_amd::StreamFlowCtrlProtocol::ReadAndProcess(net::StreamSocket* socket, bool& msgIsComplete)
    {
        ssdk::transport_amd::StreamFlowCtrlProtocol::Result result = Result::FAIL;
        size_t receivedSize = 0;
        net::Socket::Result socketResult = net::Socket::Result::OK;
        StreamMessageHeader header;
        if (m_NewIncomingMessage == true)
        {
            socketResult = socket->ReceiveAll(&header, sizeof(header), &receivedSize);
            if (socketResult != net::Socket::Result::OK)
            {
                m_InMsgSize = 0;
                AMFTraceWarning(TRACE_SCOPE, L"ReadAndProcess: Failed to read message header");
            }
            else
            {
                m_NewIncomingMessage = false;
                m_CurIncomingOffset = 0;
#if defined(__APPLE__)
                m_InMsgSize = ntohl(header.m_MsgSize);
                m_MsgID = ntohs(header.m_MsgID);
#else
                m_InMsgSize = ntohl(header.m_MsgSize);
                m_MsgID = ntohs(header.m_MsgID);
#endif
                m_ChannelID = header.m_ChannelID;

                if (m_InMsgSize > m_RecvBufSize)
                {
                    m_RecvBuf.reset(new uint8_t[m_InMsgSize]);
                    m_RecvBufSize = m_InMsgSize;
                }
                msgIsComplete = false;
            }
        }
        if (m_InMsgSize > 0)
        {
            result = Result::OK;
            receivedSize = 0;
            socketResult = socket->Receive(m_RecvBuf.get() + m_CurIncomingOffset, m_InMsgSize - m_CurIncomingOffset, &receivedSize);
            if (socketResult != net::Socket::Result::OK)
            {
                AMFTraceError(TRACE_SCOPE, L"ReadAndProcess: Failed to read message body");
                m_NewIncomingMessage = true; // Abandon current
                result = Result::OK;
            }
            else if (receivedSize > 0)
            {
                m_CurIncomingOffset += receivedSize;
                if (m_CurIncomingOffset == m_InMsgSize) // Read complete message
                {
                    msgIsComplete = true;
                    m_NewIncomingMessage = true;
                    result = Result::OK;
                }
                else
                {
                    msgIsComplete = false;
                }
            }

        }
        if (socketResult != net::Socket::Result::OK)
        {
            AMFTraceWarning(TRACE_SCOPE, L"ReadAndProcess: Failed to read message header");
            switch (socketResult)
            {
            case net::Socket::Result::CONNECTION_RESET:
                result = Result:: CONNECTION_TERMINATED;
                break;
            case net::Socket::Result::CONNECTION_TIMEOUT:
                result = Result::TIMEOUT;
                break;
            case net::Socket::Result::BUSY:
                result = Result::TIMEOUT;
                break;
            case net::Socket::Result::IN_USE:
                result = Result::TIMEOUT;
                break;
            default:
                result = Result::CONNECTION_TERMINATED;
                break;
            }
        }
        return result;
    }
}