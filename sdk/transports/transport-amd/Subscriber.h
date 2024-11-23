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

#include "transports/transport-common/ServerTransport.h"
#include "TransportSession.h"
#include "util/encryption/AESPSKCipher.h"
#include "amf/public/common/Thread.h"
#include "transports/transport-amd/messages/service/Stats.h"
#include "transports/transport-amd/messages/sensors/DeviceEvent.h"
#include "transports/transport-amd/messages/sensors/TrackableDeviceCaps.h"
#include <list>

namespace ssdk::transport_amd
{
    //----------------------------------------------------------------------------------------------
    // Subscriber class
    //----------------------------------------------------------------------------------------------
    class Subscriber : public ssdk::transport_common::Transport
    {
    public:
        Subscriber(Session::Ptr pClientSession, amf::AMFContextPtr pContext);
        virtual ~Subscriber();

        typedef std::shared_ptr<Subscriber> Ptr;

        // Subscriber interface
        virtual const char* GetID() const;
        virtual const char* GetSessionID() const;
        virtual const char* GetSubscriberIPAddress() const;
        virtual const char* GetSubscriberPlatform() const;

        virtual void SetID(const char* pID);
        virtual ssdk::transport_common::Result SetSessionID(const char* sessionID);
        virtual ssdk::transport_common::Result SetSubscriberPlatformInfo(const char* platform);

        // Miscellaneous
        virtual ssdk::transport_common::Result TransmitMessage(const void* msg, size_t msgLength); // Send a subscriber-defined message to the client
        virtual ssdk::transport_common::Result TransmitMessage(Channel channel, const void* msg, size_t msgLen);
        inline transport_common::ServerTransport::ConnectionManagerCallback::ClientRole GetRole() const noexcept { return m_Role; }
        virtual ssdk::transport_common::Result GetSessionStatistics(amf::AMFPropertyStorage** pStatistics); // Receive streaming statistics for this subscriber session
        virtual ssdk::transport_common::Result OnEvent(DeviceEvent& data, size_t dataSize);
        ssdk::transport_common::Result SendEvent(const char* id, const ssdk::ctls::CtlEvent* pEvent);

        Session* GetSession() { return m_pClientSession; };
        void SetSession(Session* session) { m_pClientSession = session; };

        void SetCipher(ssdk::util::AESPSKCipher::Ptr pCipher);
        ssdk::transport_common::Result GetCipher(ssdk::util::AESPSKCipher::Ptr* pCipher) const;

        void AddDecryptionTime(amf_pts decryptTime);
        void OnAudioInMessage(uint8_t opcode, const void* msg, size_t len, ssdk::transport_common::SessionHandle session, ssdk::transport_common::ServerTransport::AudioReceiverCallback* pARCallback);
        void OnUserDefinedMessage(const void* message, size_t messageSize);

        inline void ForceIDRFrame() { amf::AMFLock lock(&m_Guard); ++m_ForceIDRReqCnt; }
        inline bool IsWaitingForIDR() const { amf::AMFLock lock(&m_Guard); return m_WaitingForIDR; }
        inline void WaitForIDR(bool wait) { amf::AMFLock lock(&m_Guard); m_WaitingForIDR = wait; }

        inline void SetRole(transport_common::ServerTransport::ConnectionManagerCallback::ClientRole role) { m_Role = role; }

        inline bool WaitForVideoInitAck() const { amf::AMFLock lock(&m_Guard); return m_WaitForVideoInitAck; }
        inline void WaitForVideoInitAck(bool wait) { amf::AMFLock lock(&m_Guard); m_WaitForVideoInitAck = wait; }
        inline bool WaitForAudioInitAck() const { amf::AMFLock lock(&m_Guard); return m_WaitForAudioInitAck; }
        inline void WaitForAudioInitAck(bool wait) { amf::AMFLock lock(&m_Guard); m_WaitForAudioInitAck = wait; }

        inline amf_pts GetLastVideoInitAckReceived() const { amf::AMFLock lock(&m_Guard); return m_LastVideoInitIDReceived; }
        inline void SetLastVideoInitAckReceived(amf_pts id) { amf::AMFLock lock(&m_Guard); m_LastVideoInitIDReceived = id; }

        inline amf_pts GetLastAudioInitAckReceived() const { amf::AMFLock lock(&m_Guard); return m_LastAudioInitIDReceived; }
        inline void SetLastAudioInitAckReceived(amf_pts id) { amf::AMFLock lock(&m_Guard); m_LastAudioInitIDReceived = id; }

        inline amf_pts GetLastVideoInitSentTime() const { amf::AMFLock lock(&m_Guard); return m_LastVideoInitSentTime; }
        inline void SetVideoInitSentTime(amf_pts timestamp) { amf::AMFLock lock(&m_Guard); m_LastVideoInitSentTime = timestamp; }

        inline void SetAudioInitSentTime(amf_pts timestamp) { amf::AMFLock lock(&m_Guard); m_LastAudioInitSentTime = timestamp; }

        inline void SetEncoderStereo(bool stereo) { amf::AMFLock lock(&m_Guard); m_EncoderStereo = stereo; }

        void UpdateStatsFromClient(const Statistics& stat);

    protected:
        void AddRemoteTimestamp(const DeviceEvent& event);
        void AddRemoteTimestamp(amf_pts local, amf_pts remote);
        void UpdateLocalStats();

        class LocalToRemoteTimeMapping
        {
        public:
            typedef std::list<LocalToRemoteTimeMapping> Collection;

        public:
            LocalToRemoteTimeMapping() = default;
            LocalToRemoteTimeMapping(amf_pts remote) : m_Remote(remote) {};
            LocalToRemoteTimeMapping(amf_pts local, amf_pts remote) : m_Remote(remote), m_Local(local) {};

            inline amf_pts GetLocalTime() const noexcept { return m_Local; }
            inline amf_pts GetRemoteTime() const noexcept { return m_Remote; }

        private:
            amf_pts m_Local = amf_high_precision_clock();
            amf_pts m_Remote = 0LL;
        };

    private:
        mutable amf::AMFCriticalSection     m_Guard;
        amf::AMFContextPtr                  m_pContext;
        Session::Ptr                        m_pClientSession;
        ssdk::util::AESPSKCipher::Ptr       m_pCipher;
        LocalToRemoteTimeMapping::Collection m_LocalToRemoteTimeMap;

        std::string                         m_ID;
        std::string                         m_SessionID;
        std::string                         m_SubscriberIPAddress;
        std::string                         m_UserPlatform;

        bool                                m_WaitForVideoInitAck = false;
        bool                                m_WaitForAudioInitAck = false;

        transport_common::ServerTransport::ConnectionManagerCallback::ClientRole m_Role = transport_common::ServerTransport::ConnectionManagerCallback::ClientRole::CONTROLLER;

        // Statistics:
        amf::AMFPropertyStoragePtr          m_pStatistics;
        amf_pts                             m_VideoTxTimeAccum = 0;
        uint64_t                            m_VideoTxCnt = 0;
        uint64_t                            m_VideoBytesTx = 0;

        amf_pts                             m_AudioTxTimeAccum = 0;
        uint64_t                            m_AudioTxCnt = 0;
        uint64_t                            m_AudioRxCnt = 0;
        uint64_t                            m_AudioBytesTx = 0;
        uint64_t                            m_AudioBytesRx = 0;

        amf_pts                             m_CtrlTxTimeAccum = 0;
        uint64_t                            m_CtrlTxCnt = 0;
        uint64_t                            m_CtrlRxCnt = 0;
        uint64_t                            m_CtrlBytesTx = 0;
        uint64_t                            m_CtrlBytesRx = 0;

        amf_pts                             m_UserTxTimeAccum = 0;
        uint64_t                            m_UserTxCnt = 0;
        uint64_t                            m_UserRxCnt = 0;
        uint64_t                            m_UserBytesTx = 0;
        uint64_t                            m_UserBytesRx = 0;

        amf_pts                             m_TotalTxTimeAccum = 0;
        uint64_t                            m_TotalTxCnt = 0;
        uint64_t                            m_TotalRxCnt = 0;
        uint64_t                            m_TotalBytesTx = 0;

        int64_t                             m_SlowSendCnt = 0;
        amf_pts                             m_WorstSendTime = 0;

        amf_pts                             m_StatTime = 0;

        bool                                m_EncoderStereo = false;

        amf_pts                             m_EncryptTimeAccum = 0;
        amf_pts                             m_DecryptTimeAccum = 0;

        int64_t                             m_ForceIDRReqCnt = 0;
        bool                                m_WaitingForIDR = true;

        amf_pts                             m_LastVideoInitIDReceived = 0;
        amf_pts                             m_LastVideoInitSentTime = 0;

        amf_pts                             m_LastAudioInitIDReceived = 0;
        amf_pts                             m_LastAudioInitSentTime = 0;

    }; // class Subscriber

} // namespace ssdk::transport_amd
