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
#include "amf/public/include/core/Platform.h"
#include "amf/public/common/Thread.h"
#include "amf/public/common/TraceAdapter.h"

#include <time.h>
#include <queue>

//#define PRINT_EXTRA_LOGS
namespace ssdk::transport_amd
{
    static constexpr const wchar_t* TRACE_SCOPE = L"DgramFlowCtrlProtocol";

    //--------------------------------------------------------------------------------------------------------------------
    // FlowCtrlProtocol
    //--------------------------------------------------------------------------------------------------------------------
#if 0
    std::unique_ptr<FlowCtrlProtocol> FlowCtrlProtocol::FlowCtrlProtocolFactory(uint32_t version)
    {
        std::unique_ptr<FlowCtrlProtocol> p;
        switch (UdpStreamVersion(version))
        {
        case 0:       // No version changes
            p.reset();
        case UdpStreamVersion::VERSION_1:
            p.reset(new FlowCtrlProtocol(UdpStreamVersion::VERSION_1));
            break;
        case UdpStreamVersion::VERSION_2:
            p.reset(new FlowCtrlProtocol(UdpStreamVersion::VERSION_2));
            break;
        case UdpStreamVersion::VERSION_3:
            p.reset(new FlowCtrlProtocol(UdpStreamVersion::VERSION_3));
            break;

        default:
            p.reset(new FlowCtrlProtocol(UdpStreamVersion::VERSION_1));
            break;
        }
        if (nullptr != p)
        {
            AMFTraceDebug(TRACE_SCOPE, L"FlowCtrlProtocolFactory(%d)", p->GetVersion());
        }
        return p;
    }
#endif
    //--------------------------------------------------------------------------------------------------------------------
    FlowCtrlProtocol::FlowCtrlProtocol(uint32_t version)
    {
        for (int ch = 0; ch < static_cast<int>(Channel::CHANNELS_COUNT); ++ch)
        {
            m_CurMessageID[ch] = 1;
            m_LastMessageID[ch] = 0;
        }
        Init(version);
    }
    FlowCtrlProtocol::~FlowCtrlProtocol() noexcept
    {
        AMFTraceDebug(TRACE_SCOPE, L"~FlowCtrlProtocol()");
    }
    //--------------------------------------------------------------------------------------------------------------------
    void FlowCtrlProtocol::UpgradeProtocol(uint32_t upgradeVersion)
    {
        AMFTraceDebug(TRACE_SCOPE, L"UpgradeProtocol(%d)", upgradeVersion);
        if (upgradeVersion != m_version)
        {
            Init(upgradeVersion, m_bEnableProfile);

            // Sender increments message ids after sending.
            // Need to keep track on that logic when upgrade. Increment not used channels ids.
            for (MessageID& curMsgId : m_CurMessageID)
            {
                if (curMsgId == 0)
                {
                    ++curMsgId;
                }
            }

            // Empty missing messages set
            for (MessageMarksMap& msgMap : m_RequestedMissingID)
            {
                msgMap.clear();
            }
        }
    }
    //--------------------------------------------------------------------------------------------------------------------
    void FlowCtrlProtocol::Init(uint32_t version, bool bEnableProfile)
    {
        m_version = version;
        m_bEnableProfile = bEnableProfile;
        m_bFirstMessage = true;
        m_lastMsgRecievedClock = 0;
        m_MaxFragmentSize = MAX_DATAGRAM_SIZE;
        m_maxChannelID = static_cast<uint8_t>(Channel::CHANNELS_COUNT) - 1;

        for (MessageMap& msg : m_IncomingMessages)
        {
            msg.clear();
        }
    }
    //--------------------------------------------------------------------------------------------------------------------
    uint32_t FlowCtrlProtocol::MaxSupportedVersion(uint32_t minLocal, uint32_t maxLocal, uint32_t minRemote, uint32_t maxRemote) const noexcept
    {
        if (minLocal > maxRemote || maxLocal < minRemote)
        {
            return PROTOCOL_VERSION_UNSUPPORTED;
        }
        return (maxLocal > maxRemote) ? maxRemote : maxLocal;
    }

    //--------------------------------------------------------------------------------------------------------------------
    void FlowCtrlProtocol::PurgeStaleBuffers(uint8_t channelID)
    {
        amf::AMFLock lock(&m_incomingCs);

        for (MessageMap::iterator it = m_IncomingMessages[channelID].begin(); it != m_IncomingMessages[channelID].end();)
        {
            int distID = CalcDistance(m_LastMessageID[channelID], it->first);

            if (distID < 0)
            {
    #ifdef PRINT_EXTRA_LOGS
                amf_pts now = amf_high_precision_clock();
                AMFTraceInfo(TRACE_SCOPE, L"Purging stale message %d size=%d remain=%d timediff=%5.2f dist=%d queuesize=%d %s",
                    (int)(uint32_t)it->first, (int)it->second->GetSize(), (int)it->second->GetBytesRemaining(),
                    (now - it->second->GetLastUpdateTime()) / 10000.f,
                    distID, (int)m_IncomingMessages[channelID].size(),
                    it->second->GetBytesRemaining() == 0 ? L"Complete" : L"Incomplete");
    #endif
                it = m_IncomingMessages[channelID].erase(it);
            }
            else
            {
                it++;
            }
        }
    }

    //--------------------------------------------------------------------------------------------------------------------
    bool FlowCtrlProtocol::PromoteMessage(ProcessIncomingCallback& callback, uint8_t channelID)
    {
        bool sent = false;
        Buffer::Ptr fragmentBuffer = nullptr;
        {
            amf::AMFLock lock(&m_incomingCs);
            FlowCtrlProtocol::MessageID currentID = m_LastMessageID[channelID];

            currentID++;
    #ifdef PRINT_EXTRA_LOGS
            bool moreThanOneInBuffer = (m_IncomingMessages[channelID].size()) < 2 ? false : true;
            std::ostringstream msgIDs;
            msgIDs << "MsgIds: currentID: " << currentID;
    #endif
            for (MessageMap::iterator it = m_IncomingMessages[channelID].begin(); it != m_IncomingMessages[channelID].end();)
            {
                if (it->second->GetBytesRemaining() == 0 && (currentID == it->first || GetEnableProfile()))
                {
                    m_LastMessageID[channelID] = it->first;
                    fragmentBuffer = it->second;
                    it = m_IncomingMessages[channelID].erase(it);

                    if (fragmentBuffer != nullptr)
                    {
    #ifdef PRINT_EXTRA_LOGS
                        AMFTraceInfo(TRACE_SCOPE, L"PromoteMessage. ver %d channelID %d LastMessageID=%d size=%d %s",
                            m_version, channelID, (int)(uint32_t)m_LastMessageID[channelID], fragmentBuffer->GetSize(),
                            m_bEnableProfile ? L"Profile" : L"");
    #endif
                        callback.OnCompleteMessage(m_LastMessageID[channelID], fragmentBuffer->GetData(), fragmentBuffer->GetSize(), fragmentBuffer->GetPeerAddress(), fragmentBuffer->GetChannelID());
                        m_lastMsgRecievedClock = amf_high_precision_clock();
                        currentID++;
                        sent = true;
                    }
                }
                else if (sent)
                {
                    break;
                }
                else
                {
                    it++;
                }
            }

    #ifdef PRINT_EXTRA_LOGS
            if (moreThanOneInBuffer)
            {
                std::string msg = msgIDs.str();
                AMFTraceDebug(TRACE_SCOPE, L"PromoteMessage() more than 1 msg in queue. newID=%s", msg.c_str());
            }
    #endif
        }

        return sent;
    }

    //--------------------------------------------------------------------------------------------------------------------
    FlowCtrlProtocol::Result FlowCtrlProtocol::ProcessFragment(const void* buf, uint32_t bufSize, const net::Socket::Address& receivedFrom,
        ProcessIncomingCallback& incomingCallback, ProcessOutgoingCallback* outgoingCallback)
    {
        FlowCtrlProtocol::Result result = FlowCtrlProtocol::Result::OK;
        Fragment fragment;
        if (fragment.ParseFromBuffer(buf, bufSize) != FlowCtrlProtocol::Result::OK)
        {   //  Invalid/incomplete fragment
            result = FlowCtrlProtocol::Result::INCOMPLETE_FRAGMENT;
        }
        else
        {
            bool bMessageComplete = false;
            uint8_t channelID = fragment.GetChannelID();
            {
                amf::AMFLock lock(&m_incomingCs);

                // The case when sender handles missing fragment requested from receiver
                if (channelID == static_cast<uint8_t>(Channel::SYSTEM))
                {
                    return ProcessMissingFragmentsRequest(fragment, outgoingCallback);
                }

                MessageID messageID = fragment.GetMessageID();
                uint32_t messageSize = fragment.GetMessageSize();
                if (m_bFirstMessage)
                {
                    m_LastMessageID[channelID] = messageID - 1;
                    m_bFirstMessage = false;
                }

                int distance = CalcDistance(m_LastMessageID[channelID], messageID);
                if (distance > 0 || GetEnableProfile())// only if message is newer then last sent
                {
                    MessageMap::iterator msgIt = m_IncomingMessages[channelID].find(messageID);
                    if (msgIt == m_IncomingMessages[channelID].end())
                    {   //  First to come fragment belonging to a new message
                        msgIt = m_IncomingMessages[channelID].insert(std::pair<MessageID, Buffer::Ptr>(messageID,
                            Buffer::Ptr(new Buffer(messageSize, receivedFrom, fragment.GetChannelID())))).first;
                    }
                    else if ((*msgIt).second->GetSize() != messageSize) //  This happens when a stale message with the same ID remains in the map after a roll-over of message ID
                    {
                        (*msgIt).second = Buffer::Ptr(new Buffer(messageSize, receivedFrom, fragment.GetChannelID()));
                    }

                    if (msgIt->second->AddFragment(fragment.GetFragmentOffset(), ((unsigned char*)buf) + sizeof(FragmentHeader), fragment.GetFragmentSize()) == true)
                    {
                        //  Last fragment has been added, message is complete
                        bMessageComplete = true;
    #ifdef PRINT_EXTRA_LOGS
                        AMFTraceInfo(TRACE_SCOPE, L"<=== ProcessFragment() ver %d channelID %d seqId= %d fragOffset =%d fragSize =%d/%d queue size = %d msg Complete ", m_version, channelID, messageID, fragment.GetFragmentOffset(), fragment.GetFragmentSize(), messageSize, m_IncomingMessages[channelID].size());

    #endif
                    }
    #ifdef PRINT_EXTRA_LOGS
                    else
                    {
                        AMFTraceInfo(TRACE_SCOPE, L"ProcessFragment() ver=%d channelID=%d seqId= %d fragOffset =%d fragSize =%d/%d queue size = %d msg Incomplete ", m_version, channelID, messageID, fragment.GetFragmentOffset(), fragment.GetFragmentSize(), messageSize, m_IncomingMessages[channelID].size());
                    }
    #endif
                    // Check for whole missing message(s) and request to resend them again
                    // If found gap or waiting for requested missing message(s)
                    if ((distance > 1 && distance < 0x7FFF) || WaitingForRequestedMessages(channelID))
                    {
                        bMessageComplete = RequestMissingMessages(channelID, messageID, bMessageComplete, incomingCallback);
                    }
                }
                else
                {
                    AMFTraceInfo(TRACE_SCOPE, L"Old message received. newID=%d LastMessageID=%d dist=%d size=%d", (int)(uint32_t)messageID, (int)(uint32_t)m_LastMessageID[channelID], distance, (int)messageSize);
                }
            }

            if (bMessageComplete)
            {
                // Send next in sequence
                bool bSent = false;
                while (PromoteMessage(incomingCallback, channelID))
                {
                    bSent = true;
                }
                // try to find a gap
                if (bSent == false)
                {
                    TickNotify(incomingCallback, channelID);
                }
            }
        }

        return result;
    }

    //--------------------------------------------------------------------------------------------------------------------
    net::Socket::Result FlowCtrlProtocol::FragmentMessage(const void* message, uint32_t messageSize,
        uint32_t maxFragmentSize, uint8_t channelID, ProcessOutgoingCallback& onFragmentReadyCB, uint32_t& bytesSent)
    {
        amf::AMFLock lock(&m_outgoingCs);

        net::Socket::Result res = net::Socket::Result::OK;
        bytesSent = 0;
        uint32_t bytesRemaining = messageSize;
        m_MaxFragmentSize = maxFragmentSize;

        // Group outgoing messages by receiver's buffer - Usage 0 for default buffer index

        MessageID messageID = m_CurMessageID[channelID];
        ++m_CurMessageID[channelID];

        // Add to message history. This will be used to resend a lost message when receiver requests.
        StoreOutgoingMessage(messageID, message, messageSize, channelID);

        while (bytesRemaining > 0)
        {
            uint32_t curFragmentSize = bytesRemaining > maxFragmentSize - sizeof(FragmentHeader) ? maxFragmentSize - sizeof(FragmentHeader) : bytesRemaining;
            Fragment fragment(messageID, message, messageSize, messageSize - bytesRemaining, curFragmentSize, channelID);
            bytesRemaining -= curFragmentSize;

            res = onFragmentReadyCB.OnFragmentReady(fragment, bytesRemaining != 0);
    #ifdef PRINT_EXTRA_LOGS
            AMFTraceInfo(TRACE_SCOPE, L"===> Fragment sent ver %d channelID %d seqId=%d messageSize=%d fragmentSize=%d",
                m_version, channelID, (int)fragment.GetMessageID(), (int)fragment.GetMessageSize(), (int)fragment.GetFragmentSize());
    #endif
            if (res != net::Socket::Result::OK)
            {
                AMFTraceInfo(TRACE_SCOPE, L"OnFragmentReady() failed with %d", (int)res);
                break;
            }
            bytesSent += fragment.GetMessageSize();
        }

        // Monitor sent messages and reduce max fragment size if necessary
        m_MessageMonitor.AddSentMessage(messageSize);
        std::pair<bool, size_t> tpResult = m_MessageMonitor.ProcessWhenTime(maxFragmentSize);
        if (tpResult.first == true) // if change needed
        {
            onFragmentReadyCB.OnSetMaxFragmentSize(tpResult.second);
            AMFTraceInfo(TRACE_SCOPE, L"=== Max Fragment Size changed: PrevMaxFragmentSize=%d NewMaxFragmentSize=%d", maxFragmentSize, tpResult.second);
        }

        return res;
    }

    //--------------------------------------------------------------------------------------------------------------------
    net::Socket::Result FlowCtrlProtocol::FragmentStoredMessage(const unsigned char* msgBuf, MessageID messageID, uint32_t messageSize, size_t offset, size_t messageChunkSize,
        uint32_t maxFragmentSize, uint8_t channelID, ProcessOutgoingCallback& onFragmentReadyCB)
    {
        amf::AMFLock lock(&m_outgoingCs);

        net::Socket::Result res = net::Socket::Result::OK;
        size_t bytesRemaining = messageChunkSize;
        m_MaxFragmentSize = maxFragmentSize;
        uint32_t fragmentOffset = uint32_t(offset);

        while (bytesRemaining > 0)
        {
            uint32_t curFragmentSize = uint32_t(bytesRemaining > maxFragmentSize - sizeof(FragmentHeader) ? maxFragmentSize - sizeof(FragmentHeader) : bytesRemaining);
            Fragment fragment(messageID, msgBuf, messageSize, fragmentOffset, curFragmentSize, channelID);
            bytesRemaining -= curFragmentSize;
            fragmentOffset += curFragmentSize;

            res = onFragmentReadyCB.OnFragmentReady(fragment, bytesRemaining != 0);

            if (res != net::Socket::Result::OK)
            {
                AMFTraceInfo(TRACE_SCOPE, L"OnFragmentReady() failed with %d", (int)res);
                break;
            }
        }
        return res;
    }
    //--------------------------------------------------------------------------------------------------------------------

    bool FlowCtrlProtocol::TickNotify(ProcessIncomingCallback& callback)
    {
        bool bFound = false;
        for (uint8_t i = 0; i <= m_maxChannelID; ++i)
        {
            bFound |= TickNotify(callback, i);
        }
        return bFound;
    }

    bool FlowCtrlProtocol::TickNotify(ProcessIncomingCallback& callback, uint8_t channelID)
    {
        amf_pts now = amf_high_precision_clock();
        bool bRet = false;

        {
            amf::AMFLock lock(&m_incomingCs);

            if (m_IncomingMessages[channelID].empty())
            {
                return false;
            }

            amf_pts distanceTime = FlowCtrlProtocol::msgFlushTimeoutInPts;
            int distanceID = 0x10000;
            MessageMap::iterator found = m_IncomingMessages[channelID].end();
            // find ready oldest but stale message
            for (MessageMap::iterator it = m_IncomingMessages[channelID].begin(); it != m_IncomingMessages[channelID].end(); it++)
            {
                if (it->second->GetBytesRemaining() == 0)
                {
                    amf_pts dist = now - it->second->GetLastUpdateTime();
                    int distID = CalcDistance(m_LastMessageID[channelID], it->first);
    #ifdef PRINT_EXTRA_LOGS
                    AMFTraceInfo(TRACE_SCOPE, L"Message candidate for gap. lastMsgId=%d newID=%d timediff=%5.2fms idDiff=%d",
                        (int)(uint32_t)(m_LastMessageID[channelID]), (int)(uint32_t)it->first, dist / 10000.f, distID);
    #endif

                    if (distID >= 0 && distID < distanceID)
                    {
                        distanceTime = dist;
                        distanceID = distID;
                        found = it;
                    }
                }
            }

            if (found != m_IncomingMessages[channelID].end() && distanceTime >= FlowCtrlProtocol::msgFlushTimeoutInPts)
            {
                AMFTraceInfo(TRACE_SCOPE, L"Message sent from gap (channelID=%d). lastMsgId=%d newID=%d timediff=%5.2fms idDiff=%d queue=%d", channelID,
                    (int)(uint32_t)(m_LastMessageID[channelID]), (int)(uint32_t)found->first, distanceTime / 10000.f, distanceID, (int)m_IncomingMessages[channelID].size());
                m_LastMessageID[channelID] = found->first;
                m_LastMessageID[channelID]--;
            }
        }
        bRet = true;
        while (PromoteMessage(callback, channelID))
        {
        }

        PurgeStaleBuffers(channelID);
        return bRet;
    }

    int FlowCtrlProtocol::CalcDistance(FlowCtrlProtocol::MessageID from, FlowCtrlProtocol::MessageID to)
    {
        int dist = (int)(uint32_t)to - (int)(uint32_t)from;
        if (dist > 0x7FFF)
        {
            dist -= 0x10000;
        }
        else if (dist < -0x7FFF)
        {
            dist += 0x10000;
        }
        return dist;
    }

    void FlowCtrlProtocol::StoreOutgoingMessage(MessageID messageID, const void* message, uint32_t messageSize, uint8_t channelID)
    {
        // This will be used to resend a lost message when receiver requests.
        auto msgIt = m_OutgoingMessages[channelID].find(messageID);
        // Create a new message if not exist in history
        if (msgIt == m_OutgoingMessages[channelID].end())
        {
            msgIt = m_OutgoingMessages[channelID].insert(std::pair<MessageID, Buffer::Ptr>(messageID,
                Buffer::Ptr(new Buffer(messageSize, net::Socket::Address(), (unsigned char)channelID)))).first;

            // Add message to buffer
            msgIt->second->AddBuffer(0, message, messageSize);

            m_OutgoingMessagesIts[channelID].push_back(msgIt);

            // Remove from the message history the first message if the amount of messages is greather than the maximum defined amount.
            if (m_OutgoingMessagesIts[channelID].size() > sSentMsgHistoryLimit)
            {
                MessageMap::iterator it = m_OutgoingMessagesIts[channelID].front();
                m_OutgoingMessages[channelID].erase(it);
                m_OutgoingMessagesIts[channelID].erase(m_OutgoingMessagesIts[channelID].begin());
            }
        }
    }

    //-------------------------------------------------------------------------------------------------------
    void  FlowCtrlProtocol::RequestMissingFragments(MessageChunks& missingChunks, ProcessIncomingCallback& processIncomingCallback)
    {
        // Pack missing chunks data into buffer
        MessageChunks::Buf buf = missingChunks.Pack();

        // Send Request fragment by fragment if message size exceeds UPD datagram size
        const void* message = (const void*)buf.first.get();
        uint32_t messageSize = (uint32_t)buf.second;
        uint32_t bytesRemaining = messageSize;
        MessageID messageID = static_cast<MessageID>(amf_high_precision_clock() % UINT16_MAX); // not a real message id, we send missing chunks info
                                                                                               // used in case this message will be received fragment by fragment
        // Send request. Fragment message if it is more than datagram size
        while (bytesRemaining > 0)
        {
            uint32_t curFragmentSize = bytesRemaining > (uint32_t)m_MaxFragmentSize - sizeof(FragmentHeader) ? (uint32_t)m_MaxFragmentSize - sizeof(FragmentHeader) : bytesRemaining;
            Fragment fragment(messageID, message, messageSize, messageSize - bytesRemaining, curFragmentSize, (unsigned char)Channel::SYSTEM);
            bytesRemaining -= curFragmentSize;

            net::Socket::Result res = processIncomingCallback.OnRequestFragment(fragment);

    #ifdef PRINT_EXTRA_LOGS
            if (res == net::Socket::OK)
            {
                AMFTraceInfo(TRACE_SCOPE, L"===> Request Missing Fragments ver %d channelID %d messageID %d messageSize=%d fragmentSize=%d",
                    m_version, Channel::CHANNEL_SYSTEM, messageID, (int)fragment.GetMessageSize(), (int)fragment.GetFragmentSize());
            }
    #endif
            if (res != net::Socket::Result::OK)
            {
                AMFTraceInfo(TRACE_SCOPE, L"Request Missing Fragments failed with %d", (int)res);
                break;
            }
        }
    }

    //-------------------------------------------------------------------------------------------------------
    void  FlowCtrlProtocol::RequestMissingChunks(uint8_t channelID, MessageID currMessageID, ProcessIncomingCallback& processIncomingCallback)
    {
        MessageChunks missingChunks;
        for (MessageMap::iterator it = m_IncomingMessages[channelID].begin(); it != m_IncomingMessages[channelID].end(); ++it)
        {
            MessageID messageID = it->first;
            int diff = currMessageID - messageID;
            if (diff <= sSentMsgHistoryLimit && diff > 0 && // do not request out of history long staled message, and current message
                it->second->GetBytesRemaining() > 0)        // message has missing fragment(s)
            {
                if (m_RequestedMissingID[channelID].find(messageID) == m_RequestedMissingID[channelID].end()) // if not requested before
                {
                    Buffer::Ptr fragmentBuffer = it->second;

                    // Get missing fragments
                    fragmentBuffer->GetMissingChunks(missingChunks.Chunks(channelID, messageID));
                    m_RequestedMissingID[channelID].insert({ messageID, false });

                    AMFTraceInfo(TRACE_SCOPE, L"===> Request Missing Chunks ver %d channelID %d missingMessageID %d RequestedMissingCount %d queue size = %d",
                        m_version, channelID, messageID, m_RequestedMissingID[channelID].size(), m_IncomingMessages[channelID].size());
                }
            }
        }

        // Request missing fragments, if any
        if (missingChunks.HasChunk())
        {
            RequestMissingFragments(missingChunks, processIncomingCallback);
        }
    }

    //-------------------------------------------------------------------------------------------------------
    bool FlowCtrlProtocol::RequestMissingMessages(uint8_t channelID, MessageID currMessageID, bool bMessageComplete, ProcessIncomingCallback& incomingCallback)
    {
        bool bStopWaiting = false;

        // Check if the message is requested before
        MessageMarksMap::iterator itReq = m_RequestedMissingID[channelID].find(currMessageID);
        if (itReq != m_RequestedMissingID[channelID].end()) // requested before
        {
            itReq->second = true; // mark as received
            bStopWaiting = !WaitingForRequestedMessages(channelID); // promote message(s) if no waiting for missing message(s) anymore
            if (bStopWaiting)
                m_RequestedMissingID[channelID].clear();
            return bStopWaiting;
        }

        // Check for missing messages
        MessageID lastMessageID = m_LastMessageID[channelID];
        // No reason to wait for missing message(s) if message history cleaned up on sender's side
        if (currMessageID - lastMessageID > sSentMsgHistoryLimit)
        {
            bStopWaiting = true;
            m_RequestedMissingID[channelID].clear();
        }
        else
        {
            // Request missing chunks of incomplete message(s) before, if any
            RequestMissingChunks(channelID, currMessageID, incomingCallback);

            // Request missing messages, only once
            std::wstring missingIDs;
            MessageChunks missingChunks;
            for (MessageID missingID = lastMessageID + 1; missingID < currMessageID; ++missingID)
            {
                if (m_RequestedMissingID[channelID].find(missingID) == m_RequestedMissingID[channelID].end() && // if not requested before
                    m_IncomingMessages[channelID].find(missingID) == m_IncomingMessages[channelID].end())       // and if not came before
                {
                    missingChunks.AddChunk(channelID, missingID, 0, 0);
                    m_RequestedMissingID[channelID].insert({ missingID, false });
                    missingIDs += std::to_wstring(missingID) + L" ";
                }
            }
            if (bMessageComplete)
                m_RequestedMissingID[channelID].insert({ currMessageID, true });

            if (missingChunks.HasChunk())
            {
                RequestMissingFragments(missingChunks, incomingCallback);

                // Trace request
                AMFTraceInfo(TRACE_SCOPE, L"===> Request Missing Messages version %d channelID %d missingMessageIDs %sRequestedMissingCount %d queue size = %d",
                    m_version, channelID, missingIDs.c_str(), m_RequestedMissingID[channelID].size(), m_IncomingMessages[channelID].size());
            }
        }

        return bStopWaiting;
    }

    //-------------------------------------------------------------------------------------------------------
    bool FlowCtrlProtocol::WaitingForRequestedMessages(uint8_t channelID) const
    {
        bool bWaiting = false;
        const MessageMarksMap& msgMap = m_RequestedMissingID[channelID];
        for (MessageMarksMap::const_iterator it = msgMap.cbegin(); it != msgMap.cend(); ++it)
        {
            if (it->second == false)
            {
                bWaiting = true;
                break;
            }
        }
        return bWaiting;
    }

    //-------------------------------------------------------------------------------------------------------
    FlowCtrlProtocol::Result FlowCtrlProtocol::ProcessMissingFragmentsRequest(const Fragment& fragment, ProcessOutgoingCallback* outgoingCallback)
    {
        FlowCtrlProtocol::Result result = FlowCtrlProtocol::Result::OK;
        if (outgoingCallback != nullptr)
        {
            uint32_t messageSize = fragment.GetMessageSize();
            MessageID messageID = fragment.GetMessageID();
            uint32_t fragmentSize = fragment.GetFragmentSize();
            const void* fragmentData = fragment.GetFragmentData();

    #ifdef PRINT_EXTRA_LOGS
            AMFTraceInfo(TRACE_SCOPE, L"<=== Process_MissingFragmentsRequest ver %d messageId=%d messageSize=%d fragmentSize=%d fragmentOffset=%d",
                m_version, (int)messageID, (int)messageSize, fragmentSize, fragment.GetFragmentOffset());
    #endif

            // Free memory if another message received
            if (m_FragmentBuffer.bufSize > 0 && messageID != m_FragmentBuffer.id)
            {
                m_FragmentBuffer.Reset();
            }

            // If whole message received in one fragment
            if (fragmentSize == messageSize)
            {
                net::Socket::Result res = SendMissingFragments(fragmentData, *outgoingCallback);
                result = (res == net::Socket::Result::OK) ? FlowCtrlProtocol::Result::OK : FlowCtrlProtocol::Result::FAIL;
            }
            else // collect the message from fragments
            {
                // Allocate memory when first fragment received
                if (nullptr == m_FragmentBuffer.buf)
                {
                    m_FragmentBuffer.bufSize = messageSize;
                    m_FragmentBuffer.buf.reset(new unsigned char[m_FragmentBuffer.bufSize], std::default_delete<unsigned char[]>());
                    m_FragmentBuffer.id = messageID;
                }

                // Add fragment
                memcpy(&(*m_FragmentBuffer.buf) + fragment.GetFragmentOffset(), fragmentData, fragmentSize);
                m_FragmentBuffer.sizeSoFar += fragmentSize;

                // Check if whole message collected then send missing fragments to receiver
                if (m_FragmentBuffer.Complete())
                {
                    net::Socket::Result res = SendMissingFragments(&(*m_FragmentBuffer.buf), *outgoingCallback);
                    result = (res == net::Socket::Result::OK) ? FlowCtrlProtocol::Result::OK : FlowCtrlProtocol::Result::FAIL;

                    m_FragmentBuffer.Reset();
                }
            }
        }

        return result;
    }

    //-------------------------------------------------------------------------------------------------------
    net::Socket::Result FlowCtrlProtocol::SendMissingFragments(const void* chunksData, ProcessOutgoingCallback& onFragmentReadyCB)
    {
        net::Socket::Result res = net::Socket::Result::OK;

        amf::AMFLock lock(&m_outgoingCs);

        MessageChunks missingChunks;
        if (missingChunks.Unpack(chunksData))
        {
            for (const std::pair<uint8_t, MessageChunks::MessageChunksMap>& chunksMap : missingChunks.messageChunks)
            {
                uint8_t channelID = chunksMap.first;
                for (const std::pair<MessageID, Buffer::BufferChunks>& messageChunksMap : chunksMap.second)
                {
                    MessageID messageID = messageChunksMap.first;
                    for (const Buffer::BufferChunk& chunk : messageChunksMap.second)
                    {
                        bool bAddLostMsg = true;
                        auto msgIt = m_OutgoingMessages[channelID].find(messageID);
                        if (msgIt != m_OutgoingMessages[channelID].end()) // message found
                        {
                            const unsigned char* msgBuf = msgIt->second->GetData();
                            const size_t msgSize = msgIt->second->GetSize();
                            size_t messageChunkSize = (chunk.size > 0) ? chunk.size : msgSize;

                            res = FragmentStoredMessage(msgBuf, messageID, (uint32_t)msgSize, chunk.offset, messageChunkSize, (uint32_t)m_MaxFragmentSize, static_cast<unsigned char>(channelID), onFragmentReadyCB);

                            if (res == net::Socket::Result::OK)
                            {
                                AMFTraceInfo(TRACE_SCOPE, L"===> Missing Fragment(s) sent version %d channelID %d messageId=%d missingMessageSize=%d missingFragmentOffset=%d missingFragmentSize=%d",
                                    m_version, channelID, (int)messageID, (int)msgSize, chunk.offset, messageChunkSize);
                            }

                            if (bAddLostMsg)
                            {
                                m_MessageMonitor.AddLostMessage(msgSize, messageID);
                                bAddLostMsg = false;
                            }
                        }
                    }
                }
            }
        }

        return res;
    }

    void FlowCtrlProtocol::ModifyInterval(amf_int64 value)
    {
        m_MessageMonitor.ModifyInterval(value);
    }

    void FlowCtrlProtocol::ModifyLostMsgCountThreshold(amf_int64 value)
    {
        m_MessageMonitor.ModifyLostMsgCountThreshold(value);
    }

    void FlowCtrlProtocol::ModifyDecisionThreshold(amf_int64 value)
    {
        m_MessageMonitor.ModifyDecisionThreshold(value);
    }

    //--------------------------------------------------------------------------------------------------------------------
    // FlowCtrlProtocol::Fragment
    //--------------------------------------------------------------------------------------------------------------------
    FlowCtrlProtocol::Fragment::Fragment()
    {
    }
    //--------------------------------------------------------------------------------------------------------------------
    FlowCtrlProtocol::Fragment::Fragment(Fragment&& other) noexcept
    {
        operator=(std::move(other));
    }
    //--------------------------------------------------------------------------------------------------------------------
    FlowCtrlProtocol::Fragment::Fragment(MessageID messageID, const void* messageData, uint32_t messageSize, uint32_t fragmentOffset, uint32_t fragmentSize, uint8_t channelID)
    {
        m_Buf = (unsigned char*)malloc(sizeof(FragmentHeader) + fragmentSize);
        if (m_Buf != nullptr)
        {
            reinterpret_cast<FragmentHeader*>(m_Buf)->m_MessageID = htons(messageID);
            reinterpret_cast<FragmentHeader*>(m_Buf)->m_MessageSize = htonl(messageSize);
            reinterpret_cast<FragmentHeader*>(m_Buf)->m_FragmentSize = htonl(fragmentSize);
            reinterpret_cast<FragmentHeader*>(m_Buf)->m_FragmentOffset = htonl(fragmentOffset);
            reinterpret_cast<FragmentHeader*>(m_Buf)->m_ChannelID = channelID;
            memcpy(m_Buf + sizeof(FragmentHeader), reinterpret_cast<const char*>(messageData) + fragmentOffset, fragmentSize);
            m_Allocated = true;
        }
    }

    void FlowCtrlProtocol::Fragment::ReleaseIfNecessary()
    {
        if (m_Buf != nullptr && m_Allocated == true)
        {
            free(m_Buf);
        }
    }

    //--------------------------------------------------------------------------------------------------------------------
    FlowCtrlProtocol::Fragment::~Fragment() noexcept
    {
        ReleaseIfNecessary();
    }
    //--------------------------------------------------------------------------------------------------------------------
    FlowCtrlProtocol::Fragment& FlowCtrlProtocol::Fragment::operator=(Fragment&& other) noexcept
    {
        ReleaseIfNecessary();
        m_Buf = std::move(other.m_Buf); other.m_Buf = nullptr;
        m_Allocated = other.m_Allocated; other.m_Allocated = false;
        return *this;
    }
    //-------------------------------------------------------------------------------------------------------
    FlowCtrlProtocol::Result FlowCtrlProtocol::Fragment::ParseFromBuffer(const void* buf, size_t bufSize)
    {
        FlowCtrlProtocol::Result result = Result::OK;
        if (buf == nullptr || bufSize == 0)
        {
            result = Result::INVALID_ARG;
        }
        else if (bufSize <= sizeof(FragmentHeader) || ntohl(reinterpret_cast<const FragmentHeader*>(buf)->m_FragmentSize) + sizeof(FragmentHeader) != bufSize)
        {
            result = Result::INCOMPLETE_FRAGMENT;
        }
        else
        {
            ReleaseIfNecessary();
            m_Buf = const_cast<unsigned char*>(static_cast<const unsigned char*>(buf));
            m_Allocated = false;
        }
        return result;
    }

    //--------------------------------------------------------------------------------------------------------------------
    // FlowCtrlProtocol::Buffer
    //--------------------------------------------------------------------------------------------------------------------
    FlowCtrlProtocol::Buffer::Buffer(size_t size, const net::Socket::Address& receivedFrom, uint8_t channelID) :
        m_Size(size),
        m_BytesRemaining(size),
        m_LastUpdated(0),
        m_ReceivedFrom(receivedFrom),
        m_Buf(nullptr),
        m_ChannelID(channelID)
    {
        m_Buf = (unsigned char*)malloc(size);
    }
    //--------------------------------------------------------------------------------------------------------------------
    FlowCtrlProtocol::Buffer::~Buffer()
    {
        if (m_Buf != nullptr)
        {
            free(m_Buf);
        }
    }

    //--------------------------------------------------------------------------------------------------------------------
    bool FlowCtrlProtocol::Buffer::AddFragment(size_t ofs, const void* const buf, size_t size)
    {
        AddBuffer(ofs, buf, size);
        bool result = (m_BytesRemaining == 0) ? true : false;
        if (m_BytesRemaining == 0)
        {
            ClearBufferChunks();
        }
        else
        {
            UpdateBufferChunks(ofs, size);
        }
        return result;
    }

    //--------------------------------------------------------------------------------------------------------------------
    void FlowCtrlProtocol::Buffer::AddBuffer(size_t ofs, const void* const buf, size_t size)
    {
        memcpy(m_Buf + ofs, buf, size);
        m_LastUpdated = amf_high_precision_clock();
        m_BytesRemaining -= size;
    }

    //--------------------------------------------------------------------------------------------------------------------
    void FlowCtrlProtocol::Buffer::UpdateBufferChunks(size_t offset, size_t size)
    {
        bool bAdded = false;
        BufferChunk preChunk;
        // Go throught the chunk list and add the new chunk
        for (BufferChunks::iterator chIt = m_BufferChunks.begin(); chIt != m_BufferChunks.end(); ++chIt)
        {
            // Insert new chunk into the list
            if (preChunk.offset < offset && offset < chIt->offset)
            {
                chIt = m_BufferChunks.insert(chIt, BufferChunk(offset, size));
                bAdded = true;
                break;
            }

            // Keep previous chunk
            preChunk.offset = chIt->offset;
            preChunk.size = chIt->size;
        }

        // Add new chunk to the end of the list
        if (bAdded == false)
        {
            if (offset == 0)
            {
                m_BufferChunks.push_front(BufferChunk(offset, size));
            }
            else
            {
                m_BufferChunks.push_back(BufferChunk(offset, size));
            }
        }
    }

    //--------------------------------------------------------------------------------------------------------------------
    bool FlowCtrlProtocol::Buffer::GetMissingChunks(BufferChunks& chunks) const
    {
        chunks.clear();
        BufferChunk prevChunk;
        size_t ofs = 0;
        // Go throught the chunk list and check for missing chunks
        for (BufferChunks::const_iterator chIt = m_BufferChunks.begin(); chIt != m_BufferChunks.end(); ++chIt)
        {
            // Find out and collect missing chunks
            ofs = prevChunk.offset + prevChunk.size;
            if (ofs < chIt->offset)
            {
                chunks.push_back(BufferChunk(ofs, chIt->offset - ofs));
            }

            // Keep previous chunk
            prevChunk.offset = chIt->offset;
            prevChunk.size = chIt->size;
        }

        return !chunks.empty();
    }

    //--------------------------------------------------------------------------------------------------------------------
    // FlowCtrlProtocol::MessageChunks
    //--------------------------------------------------------------------------------------------------------------------
    bool FlowCtrlProtocol::MessageChunks::HasChunk() const
    {
        bool bHasChunk = false;
        if (messageChunks.empty() == false)
        {
            const MessageChunksMap& messageChunksMap = messageChunks.begin()->second;
            if (messageChunksMap.empty() == false)
            {
                const Buffer::BufferChunks& chunks = messageChunksMap.begin()->second;
                bHasChunk = !chunks.empty();
            }
        }
        return bHasChunk;
    }

    //--------------------------------------------------------------------------------------------------------------------
    FlowCtrlProtocol::MessageChunks::Buf FlowCtrlProtocol::MessageChunks::Pack()
    {
        // Allocate buffer
        size_t bufferSize = BufferSize();
        MessageChunks::BufPtr buf(new unsigned char[bufferSize], std::default_delete<unsigned char[]>());
        uint8_t* buffer = buf.get();

        // Keep buffer size
        memcpy(buffer, &bufferSize, sizeof(bufferSize));
        buffer += sizeof(bufferSize);

        // Version
        uint8_t ver = 3;
        memcpy(buffer, &ver, sizeof(ver));
        buffer += sizeof(ver);

        // Transmitter code (reserve space for future possible use)
        uint8_t trCode = 0;
        memcpy(buffer, &trCode, sizeof(trCode));
        buffer += sizeof(trCode);

        // Channel count
        uint8_t channels = (uint8_t)messageChunks.size();
        memcpy(buffer, &channels, sizeof(channels));
        buffer += sizeof(channels);

        for (const std::pair<uint8_t, MessageChunks::MessageChunksMap>& chunksMap : messageChunks)
        {
            // Next Channel ID
            uint8_t channelID = chunksMap.first;
            memcpy(buffer, &channelID, sizeof(channelID));
            buffer += sizeof(channelID);

            // Message count for channel
            uint32_t messages = (uint32_t)chunksMap.second.size();
            memcpy(buffer, &messages, sizeof(messages));
            buffer += sizeof(messages);

            for (const std::pair<MessageID, Buffer::BufferChunks>& messageChunksMap : chunksMap.second)
            {
                // Next Message ID
                MessageID messageID = messageChunksMap.first;
                memcpy(buffer, &messageID, sizeof(messageID));
                buffer += sizeof(messageID);

                // Chunks count for message
                uint32_t chunks = (uint32_t)messageChunksMap.second.size();
                memcpy(buffer, &chunks, sizeof(chunks));
                buffer += sizeof(chunks);

                // Chunks for message
                //std::copy(messageChunksMap.second.begin(), messageChunksMap.second.end(), buffer); buffer += chunks*sizeof(Buffer::BufferChunk);
                for (const Buffer::BufferChunk& chunk : messageChunksMap.second)
                {
                    memcpy(buffer, &chunk, sizeof(chunk));
                    buffer += sizeof(chunk);
                }
            }
        }

        return std::make_pair(buf, bufferSize);
    }
    //--------------------------------------------------------------------------------------------------------------------
    bool FlowCtrlProtocol::MessageChunks::Unpack(const void* buf)
    {
        if (nullptr == buf)
            return false;

        const unsigned char* buffer = (const unsigned char*)buf;

        // Buffer size
        size_t bufferSize;
        memcpy(&bufferSize, buffer, sizeof(bufferSize));
        buffer += sizeof(bufferSize);

        // Keep version
        uint8_t ver;
        memcpy(&ver, buffer, sizeof(uint8_t));
        buffer += sizeof(uint8_t);

        // Transmitter code (reserved space for future possible use)
        uint8_t trCode;
        memcpy(&trCode, buffer, sizeof(trCode));
        buffer += sizeof(trCode);

        // Channel count
        uint8_t channels;
        memcpy(&channels, buffer, sizeof(uint8_t));
        buffer += sizeof(uint8_t);

        for (uint8_t ch = 0; ch < channels; ++ch)
        {
            // Next Channel ID
            uint8_t channelID;
            memcpy(&channelID, buffer, sizeof(channelID));
            buffer += sizeof(channelID);
            messageChunks.emplace(std::make_pair(channelID, MessageChunksMap()));

            // Message count for channel
            uint32_t messages;
            memcpy(&messages, buffer, sizeof(messages));
            buffer += sizeof(messages);

            // Messages for channel
            for (uint32_t m = 0; m < messages; ++m)
            {
                // Next Message ID
                MessageID messageID;
                memcpy(&messageID, buffer, sizeof(messageID));
                buffer += sizeof(messageID);
                messageChunks[channelID].emplace(std::make_pair(messageID, Buffer::BufferChunks()));

                // Chunks count for message
                uint32_t chunks;
                memcpy(&chunks, buffer, sizeof(chunks));
                buffer += sizeof(chunks);

                // Chunks for message
                for (uint32_t ck = 0; ck < chunks; ++ck)
                {
                    Buffer::BufferChunk chunk;
                    memcpy(&chunk, buffer, sizeof(chunk));
                    buffer += sizeof(chunk);
                    messageChunks[channelID][messageID].push_back(chunk);
                }
            }
        }

        return (buffer <= (const unsigned char*)buf + bufferSize);
    }

    //--------------------------------------------------------------------------------------------------------------------
    size_t FlowCtrlProtocol::MessageChunks::BufferSize()
    {
        size_t bufferSize = 0;

        // Size
        bufferSize += sizeof(size_t);
        // Version
        bufferSize += sizeof(uint8_t);
        // Transmitter code (reserved space for future possible use)
        bufferSize += sizeof(uint8_t);
        // Channel count
        bufferSize += sizeof(uint8_t);
        for (const std::pair<uint8_t, MessageChunks::MessageChunksMap>& chunksMap : messageChunks)
        {
            // Next Channel ID
            bufferSize += sizeof(uint8_t);
            // Message count for channel
            bufferSize += sizeof(uint32_t);
            for (const std::pair<MessageID, Buffer::BufferChunks>& messageChunksMap : chunksMap.second)
            {
                // Next Message ID
                bufferSize += sizeof(MessageID);
                // Chunks count for message
                uint32_t chunks = (uint32_t)messageChunksMap.second.size();
                bufferSize += sizeof(chunks);
                // Chunks for message
                bufferSize += chunks*sizeof(Buffer::BufferChunk);
            }
        }

        return bufferSize;
    }

    //--------------------------------------------------------------------------------------------------------------------
    // MessageMonitor
    //--------------------------------------------------------------------------------------------------------------------
    FlowCtrlProtocol::MessageMonitor::MessageMonitor()
    {
        Reset();
        m_StartTime = amf_high_precision_clock();
    }
    FlowCtrlProtocol::MessageMonitor::~MessageMonitor()
    {
    }

    void FlowCtrlProtocol::MessageMonitor::Reset()
    {
        for (uint8_t bucket = 0; bucket < Buckets::BUCKET_COUNT; ++bucket)
        {
            m_lostMessages[bucket] = 0;
            m_sentMessages[bucket] = 0;
        }
        m_lostMessagesIds.clear();
    }

    FlowCtrlProtocol::MessageMonitor::Buckets FlowCtrlProtocol::MessageMonitor::SizeToBucket(size_t messageSize) const
    {
        Buckets msgBucket;
        if (messageSize <= MinDatagramSize())
        {
            msgBucket = Buckets::MSG_SIZE_0toMinSize;
        }
        else if (messageSize <= KB(1))
        {
            msgBucket = Buckets::MSG_SIZE_MinSizeto1KB;
        }
        else if (messageSize <= KB(4))
        {
            msgBucket = Buckets::MSG_SIZE_1KBto4KB;
        }
        else if(messageSize <= KB(8))
        {
            msgBucket = Buckets::MSG_SIZE_4to8KB;
        }
        else if (messageSize <= KB(16))
        {
            msgBucket = Buckets::MSG_SIZE_8to16KB;
        }
        else if (messageSize <= KB(32))
        {
            msgBucket = Buckets::MSG_SIZE_16to32KB;
        }
        else if (messageSize <= KB(64))
        {
            msgBucket = Buckets::MSG_SIZE_32to64KB;
        }
        else
        {
            msgBucket = Buckets::MSG_SIZE_OVER_64KB;
        }

        return msgBucket;
    }

    size_t FlowCtrlProtocol::MessageMonitor::BucketSize(Buckets bucket) const
    {
        size_t bucketSize;
        switch (bucket)
        {
        case Buckets::MSG_SIZE_0toMinSize:
        case Buckets::MSG_SIZE_MinSizeto1KB:
            bucketSize = MinDatagramSize();
            break;
        case Buckets::MSG_SIZE_1KBto4KB:
            bucketSize = KB(1);
            break;
        case Buckets::MSG_SIZE_4to8KB:
            bucketSize = KB(4);
            break;
        case Buckets::MSG_SIZE_8to16KB:
            bucketSize = KB(8);
            break;
        case Buckets::MSG_SIZE_16to32KB:
            bucketSize = KB(16);
            break;
        case Buckets::MSG_SIZE_32to64KB:
            bucketSize = KB(32);
            break;
        case Buckets::MSG_SIZE_OVER_64KB:
        default:
            bucketSize = KB(64);
            break;
        }

        return bucketSize;
    }

    std::wstring FlowCtrlProtocol::MessageMonitor::BucketToStr(Buckets bucket) const
    {
        std::wstring str;
        switch (bucket)
        {
        case Buckets::MSG_SIZE_0toMinSize:
            str = L"[0-" + std::to_wstring(MinDatagramSize()) + L"bytes]";
            break;
        case Buckets::MSG_SIZE_MinSizeto1KB:
            str = L"[" + std::to_wstring(MinDatagramSize()) + L"bytes-1KB]";
            break;
        case Buckets::MSG_SIZE_1KBto4KB:
            str = L"[1-4]KB";
            break;
        case Buckets::MSG_SIZE_4to8KB:
            str = L"[4-8]KB";
            break;
        case Buckets::MSG_SIZE_8to16KB:
            str = L"[8-16]KB";
            break;
        case Buckets::MSG_SIZE_16to32KB:
            str = L"[16-32]KB";
            break;
        case Buckets::MSG_SIZE_32to64KB:
            str = L"[32-64]KB";
            break;
        case Buckets::MSG_SIZE_OVER_64KB:
        default:
            str = L"[Over64]KB";
            break;
        }

        return str;
    }

    void FlowCtrlProtocol::MessageMonitor::AddLostMessage(size_t messageSize, uint16_t messageID)
    {
        if (m_lostMessagesIds.find(messageID) == m_lostMessagesIds.end()) // do not add the same message
        {
            Buckets msgBucket = SizeToBucket(messageSize);
            m_lostMessages[msgBucket]++;
            m_lostMessagesIds.insert(messageID);
        }
    }

    void FlowCtrlProtocol::MessageMonitor::AddSentMessage(size_t messageSize)
    {
        Buckets msgBucket = SizeToBucket(messageSize);
        m_sentMessages[msgBucket]++;
    }

    std::pair<bool, size_t> FlowCtrlProtocol::MessageMonitor::ProcessWhenTime(size_t maxFragmentSize)
    {
        m_FragmentSize = maxFragmentSize;
        std::pair<bool, size_t> tpResult = {false, m_FragmentSize};

        amf_pts now = amf_high_precision_clock();
        amf_pts dist = now - m_StartTime;
        if (dist / AMF_SECOND > m_Interval && LostMessageCount() > m_MsgThreshold)
        {
            tpResult = Process();
            m_StartTime = amf_high_precision_clock();
            Reset();
        }

        return tpResult;
    }

    std::pair<bool, size_t> FlowCtrlProtocol::MessageMonitor::Process() const
    {
    #ifdef PRINT_EXTRA_LOGS
        AMFTraceInfo(TRACE_SCOPE, L"--- MessageMonitor - Begin --> MaxFragmentSize = %d ---", m_FragmentSize);
        AMFTraceInfo(TRACE_SCOPE, L"--- MessageSizeBucket LostPercent LostCount SentCount ---");
    #endif
        double lostMsgsPercents[BUCKET_COUNT];
        for (uint8_t bucket = 0; bucket < BUCKET_COUNT; ++bucket)
        {
            size_t lostCount = m_lostMessages[bucket];
            size_t sentCount = m_sentMessages[bucket];

            // Collect lost percentages
            lostMsgsPercents[bucket] = (sentCount > 0) ? (((double)lostCount / sentCount) * 100) : 0;

    #ifdef PRINT_EXTRA_LOGS
            AMFTraceInfo(TRACE_SCOPE, L"--- MessageSizeBucket = %s LostPercent = %.2f LostCount = %d SentCount = %d ---",
                BucketToStr(Buckets(bucket)).c_str(), lostMsgsPercents[bucket], lostCount, sentCount);
    #endif
        }
    #ifdef PRINT_EXTRA_LOGS
        AMFTraceInfo(TRACE_SCOPE, L"--- MessageMonitor - End ---");
    #endif

        // Find out if max fragment size needs to be reduced
        return GetTurningPointMessageSize(lostMsgsPercents, BUCKET_COUNT);
    }

    std::pair<bool, size_t> FlowCtrlProtocol::MessageMonitor::GetTurningPointMessageSize(double *lostMessages, size_t size) const
    {
        size_t messageSize = m_FragmentSize;
        if (m_FragmentSize > MinDatagramSize())
        {
            double lostPercent = 0, minLostSoFar = 100; // %, assigned to max value to collect minLostSoFar in the cycle.
            for (size_t bucket = 0; bucket < size; ++bucket)
            {
                lostPercent = lostMessages[bucket];
                if (lostPercent > minLostSoFar + m_TpThreshold)
                {
                    messageSize = BucketSize(Buckets((bucket > 0) ? bucket - 1 : bucket));
                    break;
                }
                minLostSoFar = (lostPercent < minLostSoFar) ? lostPercent : minLostSoFar;
            }
        }
        // Going down sizeing only
        return std::make_pair(messageSize < m_FragmentSize, messageSize);
    }

    void FlowCtrlProtocol::MessageMonitor::ModifyInterval(amf_int64 value)
    {
        m_Interval = (uint16_t)value;
    }

    void FlowCtrlProtocol::MessageMonitor::ModifyLostMsgCountThreshold(amf_int64 value)
    {
        m_MsgThreshold = (uint16_t)value;
    }

    void FlowCtrlProtocol::MessageMonitor::ModifyDecisionThreshold(amf_int64 value)
    {
        m_TpThreshold = (uint16_t)value;
    }
}