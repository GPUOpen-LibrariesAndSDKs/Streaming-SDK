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
#include "net/StreamSocket.h"
#include "transports/transport-amd/Channels.h"
#include "amf/public/common/Thread.h"

#include <map>
#include <memory>
#include <stdint.h>
#include <unordered_set>


namespace ssdk::transport_amd
{
    class FlowCtrlProtocol
    {
    public:
        typedef std::unique_ptr<FlowCtrlProtocol>   Ptr;

        static constexpr const int32_t PROTOCOL_VERSION_UNSUPPORTED = 0;
        static constexpr const int32_t PROTOCOL_VERSION_CURRENT = 3;
        static constexpr const int32_t PROTOCOL_VERSION_MIN = 3;

        static constexpr const size_t MAX_DATAGRAM_SIZE = size_t(65507);

        static constexpr const size_t IP_MSS_SIZE = size_t(576);  // IP MSS - rfc879
        static constexpr const size_t IP_MAX_HEADER_LEN = size_t(60);  // 20 bytes + a maximum of 40 bytes options
        static constexpr const size_t UDP_MSS_SIZE = size_t(IP_MSS_SIZE - (IP_MAX_HEADER_LEN + size_t(8)));  // UDP - rfc879
        static constexpr const size_t UDP_MAX_MSS_SIZE_WITH_NO_FRAGMENTATION = size_t(1472);
        static constexpr const size_t TCP_MSS_SIZE = size_t(IP_MSS_SIZE - (20 + 20)); // TCP not including 802.1Q and options

        enum class Result
        {
            OK,
            FAIL,
            NOT_INITIALIZED,
            INVALID_ARG,
            INCOMPLETE_FRAGMENT
        };
        typedef uint16_t MessageID;

#pragma pack(push, 1)
        struct FragmentHeader
        {
            MessageID           m_MessageID;        //  Unique message ID - all fragments belonging to the same message have the same Message ID
            uint32_t            m_MessageSize;      //  The total size of the entire message
            uint32_t            m_FragmentOffset;   //  Offset of the current fragment
            uint32_t            m_FragmentSize;     //  Size of the current fragment
            uint8_t             m_ChannelID;        //  Channel ID (CHANNEL_AUDIO_OUT, CHANNEL_VIDEO_OUT, ...)
        };
#pragma pack(pop)

    public:
//        static std::unique_ptr<FlowCtrlProtocol> FlowCtrlProtocolFactory(uint32_t version = 1);
        uint32_t MaxSupportedVersion(uint32_t minLocal, uint32_t maxLocal, uint32_t minRemote, uint32_t maxRemote) const noexcept;

        FlowCtrlProtocol() = delete;
        FlowCtrlProtocol(uint32_t version);

        virtual ~FlowCtrlProtocol() noexcept;
        inline void     EnableProfile(bool bEnable) { m_bEnableProfile = bEnable; }
        inline bool     GetEnableProfile() const { return m_bEnableProfile; }
        uint32_t GetVersion() { return m_version; };
        void UpgradeProtocol(uint32_t upgradeVersion);

        void ModifyInterval(amf_int64 value);
        void ModifyLostMsgCountThreshold(amf_int64 value);
        void ModifyDecisionThreshold(amf_int64 value);

        class Fragment
        {
        public:
            Fragment();
            Fragment(Fragment&& other) noexcept;
            Fragment(MessageID messageID, const void* messageData, uint32_t messageSize, uint32_t fragmentOffset, uint32_t fragmentSize, uint8_t channelID);
            ~Fragment() noexcept;

            Result ParseFromBuffer(const void* buf, size_t bufSize);

            Fragment& operator=(Fragment&& other) noexcept;
            inline MessageID    GetMessageID() const        { return ntohs(reinterpret_cast<const FragmentHeader*>(m_Buf)->m_MessageID); }
            inline uint32_t     GetFragmentOffset() const   { return ntohl(reinterpret_cast<const FragmentHeader*>(m_Buf)->m_FragmentOffset); }
            inline uint32_t     GetFragmentSize() const     { return ntohl(reinterpret_cast<const FragmentHeader*>(m_Buf)->m_FragmentSize); }
            inline uint32_t     GetMessageSize() const      { return ntohl(reinterpret_cast<const FragmentHeader*>(m_Buf)->m_MessageSize); }
            inline uint8_t      GetChannelID() const        { return reinterpret_cast<const FragmentHeader*>(m_Buf)->m_ChannelID; }
            inline const void*  GetFragmentData() const     { return m_Buf + sizeof(FragmentHeader); }

            inline const void*  GetDataToSend() const       { return m_Buf; }
            inline size_t       GetSizeToSend() const       { return sizeof(FragmentHeader) + GetFragmentSize(); }
            static size_t       GetSizeOfFragmentHeader()   { return sizeof(FragmentHeader); }
        private:
            Fragment(const Fragment&) = delete;
            Fragment& operator=(const Fragment&) = delete;
            void ReleaseIfNecessary();

            unsigned char*      m_Buf = nullptr;
            bool                m_Allocated = false;
        };

        class ProcessIncomingCallback;
        class Buffer
        {
        public:
            typedef std::shared_ptr<Buffer>	Ptr;
            struct BufferChunk
            {
                BufferChunk(size_t ofs = 0, size_t sz = 0) : offset(ofs), size(sz) {}
                size_t offset;
                size_t size;
            };
            typedef std::list<BufferChunk> BufferChunks;

        public:
            Buffer(size_t size, const ssdk::net::Socket::Address& receivedFrom, uint8_t channelID);
            ~Buffer();

            inline const ssdk::net::Socket::Address& GetPeerAddress() const { return m_ReceivedFrom; }
            inline const unsigned char* GetData()const { return m_Buf; }
            inline size_t               GetSize() const { return m_Size; }
            inline amf_pts              GetLastUpdateTime() const { return m_LastUpdated; }
            inline size_t               GetBytesRemaining() const { return m_BytesRemaining; }
            inline uint8_t              GetChannelID() const { return m_ChannelID; }
            inline void                 UpdateTime(amf_pts pts) { m_LastUpdated = pts; }

            bool AddFragment(size_t ofs, const void* const buf, size_t size);
            void AddBuffer(size_t ofs, const void* const buf, size_t size);

            bool GetMissingChunks(BufferChunks& chunks) const;

        private:
            Buffer(const Buffer&) = delete;
            Buffer& operator=(const Buffer&) = delete;

            void UpdateBufferChunks(size_t offset, size_t size);
            void ClearBufferChunks() { m_BufferChunks.clear(); };

            unsigned char*                      m_Buf = nullptr;
            size_t                              m_Size = 0;
            size_t                              m_BytesRemaining = 0;
            amf_pts                             m_LastUpdated = 0;
            ssdk::net::Socket::Address          m_ReceivedFrom;
            uint8_t                             m_ChannelID = 0;
            BufferChunks                        m_BufferChunks; // contains buffer offsets and sizes
        };

        struct MessageChunks
        {
            typedef std::shared_ptr<unsigned char> BufPtr;
            typedef std::pair<BufPtr, size_t> Buf;
            typedef std::map<MessageID, Buffer::BufferChunks> MessageChunksMap;
            typedef std::map<uint8_t, MessageChunksMap> ChunksMap;

            MessageChunks() {}

            inline void AddChunk(uint8_t channelID, MessageID messageID, size_t offset, size_t size) { messageChunks[channelID][messageID].push_back(Buffer::BufferChunk(offset, size)); }
            inline Buffer::BufferChunks& Chunks(uint8_t channelID, MessageID messageID) { return messageChunks[channelID][messageID]; }
            bool HasChunk() const;

            Buf Pack();
            bool Unpack(const void* buf);
            size_t BufferSize();

            ChunksMap messageChunks;
        };

        struct BufferFragment
        {
            BufferFragment() : buf(nullptr), bufSize(0), id(0), sizeSoFar(0) {}
            void Reset() { buf.reset(); bufSize = 0, id = 0, sizeSoFar = 0; }
            bool Complete() { return (sizeSoFar == bufSize); }
            MessageChunks::BufPtr buf;
            size_t bufSize;
            MessageID id;
            size_t sizeSoFar;
        };

        class ProcessIncomingCallback
        {
        public:
            enum class ReleaseReason
            {
                RELEASED,
                PURGED
            };
        public:
            virtual void OnCompleteMessage(MessageID msgID, const void* buf, size_t size, const ssdk::net::Socket::Address& receivedFrom, uint8_t optional) = 0;
            virtual ssdk::net::Socket::Result OnRequestFragment(const Fragment& fragment) = 0;
        };
        class ProcessOutgoingCallback
        {
        public:
            virtual ssdk::net::Socket::Result OnFragmentReady(const Fragment& fragment, bool last) = 0;
            virtual void OnSetMaxFragmentSize(size_t fragmentSize) = 0;
        };

        class MessageMonitor
        {
        public:
            enum Buckets
            {
                MSG_SIZE_0toMinSize, // minimal agreed udp datagram size (508 bytes, 8 bytes for header)
                MSG_SIZE_MinSizeto1KB,
                MSG_SIZE_1KBto4KB,
                MSG_SIZE_4to8KB,
                MSG_SIZE_8to16KB,
                MSG_SIZE_16to32KB,
                MSG_SIZE_32to64KB,
                MSG_SIZE_OVER_64KB,

                BUCKET_COUNT
            };
            MessageMonitor();
            ~MessageMonitor();

            void AddLostMessage(size_t messageSize, uint16_t messageID);
            void AddSentMessage(size_t messageSize);
            std::pair<bool, size_t> ProcessWhenTime(size_t maxFragmentSize);
            std::pair<bool, size_t> Process() const;
            inline size_t LostMessageCount() const { return m_lostMessagesIds.size(); };

            void ModifyInterval(amf_int64 value);
            void ModifyLostMsgCountThreshold(amf_int64 value);
            void ModifyDecisionThreshold(amf_int64 value);

            typedef std::unordered_set<uint16_t> LostMessageIds;

        private:
            void Reset();
            Buckets SizeToBucket(size_t messageSize) const;
            size_t BucketSize(Buckets bucket) const;
            std::wstring BucketToStr(Buckets bucket) const;
            std::pair<bool,size_t> GetTurningPointMessageSize(double* lostMessages, size_t size) const;
            inline size_t KB(size_t kbVal) const { return ((size_t)(kbVal) << 10); }; // KB to bytes: x * 2'10 == x * 1024
            inline size_t MinDatagramSize() const { return 508; }; // minimal agreed udp datagram size (508 bytes, 8 bytes for header)

            size_t m_lostMessages[size_t(Buckets::BUCKET_COUNT)]; // [message size -> count]
            size_t m_sentMessages[size_t(Buckets::BUCKET_COUNT)]; // [message size -> count]
            LostMessageIds m_lostMessagesIds;    // lost messages ids
            size_t m_FragmentSize = 0;
            amf_pts m_StartTime = 0;
            uint16_t m_Interval = 10;           // seconds
            uint16_t m_MsgThreshold = 10;       // lost message minimal count to process
            uint16_t m_TpThreshold = 20;        // turning point threshold for specifying max fragment size
        };

        Result ProcessFragment(const void* buf, uint32_t bufSize, const ssdk::net::Socket::Address& receivedFrom,
            ProcessIncomingCallback& incomingCallback, ProcessOutgoingCallback* outgoingCallback = nullptr);
        ssdk::net::Socket::Result FragmentMessage(const void* buf, uint32_t bufSize, uint32_t maxFragmentSize,
            uint8_t channelID, ProcessOutgoingCallback& onFragmentReadyCB, uint32_t& bytesSent);
        ssdk::net::Socket::Result FragmentStoredMessage(const unsigned char* msgBuf, MessageID messageID, uint32_t messageSize, size_t offset, size_t messageChunkSize,
            uint32_t maxFragmentSize, uint8_t channelID, ProcessOutgoingCallback& onFragmentReadyCB);

        bool TickNotify(ProcessIncomingCallback& callback);
        bool TickNotify(ProcessIncomingCallback& callback, uint8_t channelID);

    protected:
        void PurgeStaleBuffers(uint8_t channelID);
        bool PromoteMessage(ProcessIncomingCallback& callback, uint8_t channelID);
        static int CalcDistance(FlowCtrlProtocol::MessageID from, FlowCtrlProtocol::MessageID to);
        void Init(uint32_t version, bool bEnableProfile = false);

        void StoreOutgoingMessage(MessageID messageID, const void* message, uint32_t messageSize, uint8_t channelID);
        void RequestMissingFragments(MessageChunks& missingChunks, ProcessIncomingCallback& processIncomingCallback);
        void RequestMissingChunks(uint8_t channelID, MessageID currMessageID, ProcessIncomingCallback& processIncomingCallback);
        bool RequestMissingMessages(uint8_t channelID, MessageID currMessageID, bool bMessageComplete, ProcessIncomingCallback& incomingCallback);
        bool WaitingForRequestedMessages(uint8_t channelID) const;
        Result ProcessMissingFragmentsRequest(const Fragment& fragment, ProcessOutgoingCallback* outgoingCallback);
        ssdk::net::Socket::Result SendMissingFragments(const void* chunksData, ProcessOutgoingCallback& onFragmentReadyCB);

        static const amf_pts msgFlushTimeoutInPts = 150 * AMF_MILLISECOND;
        typedef std::map<MessageID, Buffer::Ptr> MessageMap;
        typedef std::list<MessageMap::iterator> MessageMapIts;
        typedef std::map<MessageID, bool> MessageMarksMap;

        // Array of channel ids.
        // Channel 0 is used for common stream
        // Channel 1 reserved
        // In version 1 only Channel 0 is used
        // In version 2 Channel 2 is used for audio channels - no reordering on the client
        // In version 3 all channels are used
        static const uint16_t sSentMsgHistoryLimit = 10; // Keep in sender the message history for some defined amount of messages.
        uint8_t     m_maxChannelID = 0;
        MessageMap  m_IncomingMessages[static_cast<size_t>(Channel::CHANNELS_COUNT)];
        MessageID   m_CurMessageID[static_cast<size_t>(Channel::CHANNELS_COUNT)];
        MessageID   m_LastMessageID[static_cast<size_t>(Channel::CHANNELS_COUNT)];
        MessageMap  m_OutgoingMessages[static_cast<size_t>(Channel::CHANNELS_COUNT)];        // This will be used to resend a lost message when receiver requests.
        MessageMapIts m_OutgoingMessagesIts[static_cast<size_t>(Channel::CHANNELS_COUNT)];   // This is used for keeping correct sequesnce of sent messages
        MessageMarksMap  m_RequestedMissingID[static_cast<size_t>(Channel::CHANNELS_COUNT)]; // Used for tracking requested messages and avoid requesting more than once
        bool                    m_bEnableProfile = false;
        uint32_t                m_version = 0;
        amf::AMFCriticalSection m_outgoingCs;
        amf::AMFCriticalSection m_incomingCs;
        bool                    m_bFirstMessage;
        amf_pts                 m_lastMsgRecievedClock;
        size_t                  m_MaxFragmentSize;
        BufferFragment          m_FragmentBuffer;
        MessageMonitor          m_MessageMonitor;
    };



    class StreamFlowCtrlProtocol
    {
    public:
        enum class Result
        {
            OK,
            FAIL,
            TIMEOUT,
            CONNECTION_TERMINATED,
        };

#pragma pack(push, 1)
        struct StreamMessageHeader
        {
            uint32_t					m_MsgSize;
            uint8_t						m_ChannelID;
            FlowCtrlProtocol::MessageID m_MsgID;
        };
#pragma pack(pop)
    public:
        StreamFlowCtrlProtocol();
        virtual ~StreamFlowCtrlProtocol();

        inline size_t GetReceiveSize() const { return m_InMsgSize; }
        inline size_t GetSendSize() const { return sizeof(StreamMessageHeader) + m_OutMsgSize; }
        inline Channel GetChannel() const { return static_cast<Channel>(m_ChannelID); }
        inline const uint8_t* GetSendBuffer() const { return m_SendBuf.get(); }
        inline const uint8_t* GetReceiveBuffer() const { return m_RecvBuf.get(); }
        inline FlowCtrlProtocol::MessageID GetMsgID() const { return m_MsgID; }

        Result ReadAndProcess(ssdk::net::StreamSocket* socket, bool& msgIsComplete);
        FlowCtrlProtocol::MessageID PrepareMessage(Channel channel, const void* msg, size_t msgSize);

    private:
        FlowCtrlProtocol::MessageID m_CurMessageID = 0;

        bool						m_NewIncomingMessage = true;
        size_t						m_CurIncomingOffset = 0;
        size_t						m_InMsgSize = 0;
        size_t						m_OutMsgSize = 0;

        uint8_t						m_ChannelID = 0;
        FlowCtrlProtocol::MessageID m_MsgID = 0;

        std::unique_ptr<uint8_t[]>	m_RecvBuf;
        size_t						m_RecvBufSize = 0;
        std::unique_ptr<uint8_t[]>	m_SendBuf;
        size_t						m_SendBufSize = 0;
    };
}

