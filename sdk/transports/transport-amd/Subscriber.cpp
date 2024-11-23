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

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include "Subscriber.h"
#include "public/common/TraceAdapter.h"
#include "public/common/PropertyStorageImpl.h"
#include "transports/transport-amd/TransportServerImpl.h"
#include "transports/transport-amd/messages/audio/AudioInit.h"
#include "transports/transport-amd/messages/audio/AudioData.h"
#include "controllers/UserInput.h"

#include <sstream>
#include <iomanip>

namespace ssdk::transport_amd
{
    static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::transport_amd::Subscriber";
    static constexpr const float STATS_UPDATE_PERIOD = float(AMF_SECOND * 3);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Subscriber implementation
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Subscriber::Subscriber(Session::Ptr pClientSession, amf::AMFContextPtr pContext) :
        m_pClientSession(pClientSession),
        m_pContext(pContext)
    {
        m_pStatistics = new amf::AMFInterfaceImpl<amf::AMFPropertyStorageImpl<amf::AMFPropertyStorage>>();
        m_StatTime = amf_high_precision_clock();
    }

    Subscriber::~Subscriber()
    {
        m_pCipher = nullptr;
    }

    const char* Subscriber::GetID() const
    {
        const char* id = nullptr;
        amf::AMFLock lock(&m_Guard);
        if (m_ID.empty() == false)
        {
            id = m_ID.c_str();
        }
        return id;
    }

    const char* Subscriber::GetSessionID() const
    {
        const char* id = nullptr;
        amf::AMFLock lock(&m_Guard);
        if (m_SessionID.empty() == false)
        {
            id = m_SessionID.c_str();
        }
        return id;
    }

    const char* Subscriber::GetSubscriberIPAddress() const
    {
        amf::AMFLock lock(&m_Guard);
        return m_pClientSession->GetPeerUrl();
    }

    const char* Subscriber::GetSubscriberPlatform() const
    {
        const char* platform = nullptr;
        amf::AMFLock lock(&m_Guard);
        if (m_UserPlatform.empty() == false)
        {
            platform = m_UserPlatform.c_str();
        }
        return platform;
    }

    void Subscriber::SetID(const char* pID)
    {
        amf::AMFLock lock(&m_Guard);
        if (pID != nullptr)
        {
            m_ID = pID;
        }
    }

    ssdk::transport_common::Result Subscriber::SetSessionID(const char* sessionID)
    {
        AMF_RETURN_IF_FALSE(sessionID != nullptr, ssdk::transport_common::Result::INVALID_ARG, L"sessionID cannot be NULL");

        amf::AMFLock lock(&m_Guard);
        m_SessionID = sessionID;

        return ssdk::transport_common::Result::OK;
    }

    ssdk::transport_common::Result Subscriber::SetSubscriberPlatformInfo(const char* platform)
    {
        AMF_RETURN_IF_FALSE(platform != nullptr, ssdk::transport_common::Result::INVALID_ARG, L"User's Platform Info cannot be NULL");

        amf::AMFLock lock(&m_Guard);
        m_UserPlatform = platform;
        amf::AMFVariant varPlatform;
        varPlatform = platform;

        return ssdk::transport_common::Result::OK;
    }

    ssdk::transport_common::Result Subscriber::TransmitMessage(const void* msg, size_t msgLength)
    {
        return TransmitMessage(Channel::USER_DEFINED, msg, msgLength);
    }

    ssdk::transport_common::Result Subscriber::TransmitMessage(Channel channel, const void* msg, size_t msgLen)
    {
        uint8_t* cipherText = nullptr;
        uint8_t* msgToSend = static_cast<uint8_t*>(const_cast<void*>(msg));
        size_t bytesToSend = msgLen;
        Session::Ptr pSession;
        ssdk::util::AESPSKCipher::Ptr pCipher;
        amf::AMFContextPtr pContext;
        amf::AMFBufferPtr pSendBuffer;
        uint8_t commandCode = msgToSend[0];

        {
            amf::AMFLock lock(&m_Guard);
            pSession = m_pClientSession;
            if (pSession == nullptr)
            {
                return ssdk::transport_common::Result::FAIL;
            }
            pCipher = m_pCipher;
            pContext = m_pContext;
        }

        if (pCipher != nullptr && pContext != nullptr)
        {
            amf_pts encryptStartTime = amf_high_precision_clock();
            size_t cipherTextBufferSize = pCipher->GetCipherTextBufferSize(msgLen);
            AMF_RETURN_IF_FALSE(cipherTextBufferSize != 0, ssdk::transport_common::Result::FAIL, L"Failed to allocate a zero-length buffer for Cipher::Encrypt, msgLen: %ld", msgLen);
            AMF_RESULT res;
            res = pContext->AllocBuffer(amf::AMF_MEMORY_HOST, cipherTextBufferSize, &pSendBuffer);
            AMF_RETURN_IF_FALSE(res == AMF_OK, ssdk::transport_common::Result::FAIL, L"Failed to allocate a buffer for ANSCipher::Encrypt, size: %ld, msgLen: %ld", cipherTextBufferSize, msgLen);
            cipherText = static_cast<uint8_t*>(pSendBuffer->GetNative());
            size_t cipherTextSize;
            bool bOk = pCipher->Encrypt(msg, msgLen, cipherText, &cipherTextSize);
            AMF_RETURN_IF_FALSE(bOk == true, ssdk::transport_common::Result::INVALID_ARG, L"ANSCipher::Encrypt failed");
            msgToSend = cipherText;
            bytesToSend = cipherTextBufferSize;
            m_EncryptTimeAccum += amf_high_precision_clock() - encryptStartTime;
        }
        amf_pts sendStart = amf_high_precision_clock();
        ssdk::transport_common::Result resOut = pSession->Send(channel, msgToSend, bytesToSend);
        amf_pts sendDuration = amf_high_precision_clock() - sendStart;
        {   // Statistics:
            amf::AMFLock lock(&m_Guard);

            m_TotalBytesTx += bytesToSend;
            m_TotalTxTimeAccum += sendDuration;
            ++m_TotalTxCnt;

            switch (channel)
            {
            case Channel::VIDEO_OUT:
                m_VideoBytesTx += bytesToSend;
                m_VideoTxTimeAccum += sendDuration;
                if (VIDEO_OP_CODE(commandCode) == VIDEO_OP_CODE::DATA)
                {
                    ++m_VideoTxCnt;
                }
                break;
            case Channel::AUDIO_OUT:
                m_AudioBytesTx += bytesToSend;
                m_AudioTxTimeAccum += sendDuration;
                if (AUDIO_OP_CODE(commandCode) == AUDIO_OP_CODE::DATA)
                {
                    ++m_AudioTxCnt;
                }
                break;
            case Channel::SENSORS_OUT:
                m_CtrlBytesTx += bytesToSend;
                m_CtrlTxTimeAccum += sendDuration;
                ++m_CtrlTxCnt;
                break;
            case Channel::USER_DEFINED:
                m_UserBytesTx += bytesToSend;
                m_UserTxTimeAccum += sendDuration;
                ++m_UserTxCnt;
                break;
            default:
                break;
            }
            if (m_WorstSendTime < sendDuration)
            {
                m_WorstSendTime = sendDuration;
            }
            if (sendDuration > AMF_SECOND / 10) // 100 ms
            {
                m_SlowSendCnt++;
                AMFTraceWarning(AMF_FACILITY, L"calling Send() took %5.2f ms msgLen=%d", sendDuration / 10000., (int)msgLen);
            }
            UpdateLocalStats();
        }
        return resOut;
    }

    ssdk::transport_common::Result Subscriber::OnEvent(DeviceEvent& data, size_t dataSize)
    {
        // Statistics:
        m_CtrlBytesRx += dataSize;
        ++m_CtrlRxCnt;

        AddRemoteTimestamp(data);

        return ssdk::transport_common::Result::OK;
    }

    void Subscriber::AddRemoteTimestamp(const DeviceEvent& event)
    {
        const DeviceEvent::DataCollection& coll = event.GetDataCollection();
        amf_pts now = amf_high_precision_clock();
        for (DeviceEvent::DataCollection::const_iterator it = coll.begin(); it != coll.end(); it++)
        {
            const std::string& id = it->first;
            if (id.find(ssdk::ctls::DEVICE_POSE) != id.npos)
            {
                for (DeviceEvent::Data::const_iterator it_pose = it->second.begin(); it_pose != it->second.end(); it_pose++)
                {
                    amf_pts headPoseTimestamp;
                    if (it_pose->GetTimestampValue(headPoseTimestamp) == true)
                    {
                        AddRemoteTimestamp(now, headPoseTimestamp);
                    }
                }
            }
        }

    }

    void Subscriber::AddRemoteTimestamp(amf_pts local, amf_pts remote)
    {
        amf::AMFLock lock(&m_Guard);
        m_LocalToRemoteTimeMap.push_back(LocalToRemoteTimeMapping(local, remote));
    }

    void Subscriber::UpdateLocalStats()
    {
        amf_pts now = amf_high_precision_clock();
        amf_pts actualTimeSinceStatUpdate = now - m_StatTime;
        if (actualTimeSinceStatUpdate > STATS_UPDATE_PERIOD)
        {
            float actualTimeSinceStatUpdateInSeconds = float(actualTimeSinceStatUpdate) / 10000000.f;
            int eyeCount = (m_EncoderStereo == true) ? 2 : 1;

            float RxBandwidth = float((m_AudioBytesRx + m_CtrlBytesRx + m_UserBytesRx) * 8) / actualTimeSinceStatUpdateInSeconds;
            float TxBandwidth = float(m_TotalBytesTx * 8) / actualTimeSinceStatUpdateInSeconds;

            m_pStatistics->SetProperty(STATISTICS_BANDWIDTH_TOTAL_IN, RxBandwidth);
            m_pStatistics->SetProperty(STATISTICS_BANDWIDTH_TOTAL_OUT, TxBandwidth);
            m_pStatistics->SetProperty(STATISTICS_BANDWIDTH_ESTIMATE, TxBandwidth + RxBandwidth);
            m_pStatistics->SetProperty(STATISTICS_TOTAL_SEND_TIME, float(m_TotalTxTimeAccum) / float(m_TotalTxCnt) / float(AMF_MILLISECOND));

            m_pStatistics->SetProperty(STATISTICS_BANDWIDTH_VIDEO_OUT, float(m_VideoBytesTx * 8) / actualTimeSinceStatUpdateInSeconds);
            m_pStatistics->SetProperty(STATISTICS_VIDEO_SEND_TIME, float(m_VideoTxTimeAccum) / float(m_VideoTxCnt) / float(AMF_SECOND));

            m_pStatistics->SetProperty(STATISTICS_BANDWIDTH_AUDIO_IN, float(m_AudioBytesRx * 8) / actualTimeSinceStatUpdateInSeconds);
            m_pStatistics->SetProperty(STATISTICS_BANDWIDTH_AUDIO_OUT, float(m_AudioBytesTx * 8) / actualTimeSinceStatUpdateInSeconds);
            m_pStatistics->SetProperty(STATISTICS_AUDIO_SEND_TIME, float(m_AudioTxTimeAccum) / float(m_AudioTxCnt) / float(AMF_SECOND));

            m_pStatistics->SetProperty(STATISTICS_BANDWIDTH_CTRL_IN, float(m_CtrlBytesRx * 8) / actualTimeSinceStatUpdateInSeconds);
            m_pStatistics->SetProperty(STATISTICS_BANDWIDTH_CTRL_OUT, float(m_CtrlBytesTx * 8) / actualTimeSinceStatUpdateInSeconds);
            m_pStatistics->SetProperty(STATISTICS_CTRL_SEND_TIME, float(m_CtrlTxTimeAccum) / float(m_CtrlTxCnt) / float(AMF_SECOND));

            m_pStatistics->SetProperty(STATISTICS_BANDWIDTH_USER_IN, float(m_UserBytesRx * 8) / actualTimeSinceStatUpdateInSeconds);
            m_pStatistics->SetProperty(STATISTICS_BANDWIDTH_USER_OUT, float(m_UserBytesTx * 8) / actualTimeSinceStatUpdateInSeconds);
            m_pStatistics->SetProperty(STATISTICS_USER_SEND_TIME, float(m_UserTxTimeAccum) / float(m_UserTxCnt) / float(AMF_SECOND));

            m_pStatistics->SetProperty(STATISTICS_VIDEO_FPS_AT_TX, float(m_VideoTxCnt) / actualTimeSinceStatUpdateInSeconds / eyeCount);

            m_pStatistics->SetProperty(STATISTICS_DECRYPTION_LATENCY, float(m_DecryptTimeAccum) / float(m_TotalRxCnt) / float(AMF_MILLISECOND));
            m_pStatistics->SetProperty(STATISTICS_ENCRYPTION_LATENCY, float(m_EncryptTimeAccum) / float(m_TotalTxCnt) / float(AMF_MILLISECOND));

            m_pStatistics->SetProperty(STATISTICS_SLOW_SEND_COUNT, m_SlowSendCnt);
            m_pStatistics->SetProperty(STATISTICS_WORST_SEND_TIME, float(m_WorstSendTime) / float(AMF_MILLISECOND));

            m_pStatistics->SetProperty(STATISTICS_LOCAL_UPDATE_TIME, now);

            m_TotalBytesTx = m_VideoBytesTx = m_AudioBytesTx = m_CtrlBytesTx = m_UserBytesTx = 0;
            m_TotalTxTimeAccum = m_VideoTxTimeAccum = m_AudioTxTimeAccum = m_CtrlTxTimeAccum = m_UserTxTimeAccum = m_EncryptTimeAccum = m_DecryptTimeAccum = 0;
            m_TotalTxCnt = m_VideoTxCnt = m_AudioTxCnt = m_CtrlTxCnt = m_UserTxCnt = m_TotalRxCnt = 0;
            m_AudioBytesRx = m_CtrlBytesRx = m_UserBytesRx = 0;
            m_AudioRxCnt = m_CtrlRxCnt = m_UserRxCnt = 0;
            m_SlowSendCnt = 0;
            m_WorstSendTime = 0;
            m_StatTime = now;
        }
    }

    void Subscriber::UpdateStatsFromClient(const Statistics& stat)
    {
        m_pStatistics->SetProperty(STATISTICS_FULL_LATENCY, stat.GetFull());
        m_pStatistics->SetProperty(STATISTICS_SERVER_LATENCY, stat.GetServer());
        m_pStatistics->SetProperty(STATISTICS_ENCODER_LATENCY, stat.GetEncoder());
        m_pStatistics->SetProperty(STATISTICS_CLIENT_LATENCY, stat.GetClient());
        m_pStatistics->SetProperty(STATISTICS_NETWORK_LATENCY, stat.GetNetwork());
        m_pStatistics->SetProperty(STATISTICS_DECODER_LATENCY, stat.GetDecoder());
        m_pStatistics->SetProperty(STATISTICS_DECODER_QUEUE_DEPTH, stat.GetDecoderQueueDepth());
        m_pStatistics->SetProperty(STATISTICS_CLIENT_ENCRYPTION_LATENCY, stat.GetEncryptionLatency());
        m_pStatistics->SetProperty(STATISTICS_CLIENT_DECRYPTION_LATENCY, stat.GetDecryptionLatency());
        m_pStatistics->SetProperty(STATISTICS_AV_DESYNC, stat.GetAVDesync());
        m_pStatistics->SetProperty(STATISTICS_VIDEO_FPS_AT_RX, stat.GetFpsRx());
        m_pStatistics->SetProperty(STATISTICS_FORCE_IDR_REQ_COUNT, m_ForceIDRReqCnt);
        m_ForceIDRReqCnt = 0;
        m_pStatistics->SetProperty(STATISTICS_UPDATE_TIME, amf_high_precision_clock());
    }

    ssdk::transport_common::Result Subscriber::GetSessionStatistics(amf::AMFPropertyStorage** pStatistics)
    {
        if (nullptr == pStatistics)
        {
            return ssdk::transport_common::Result::INVALID_ARG;
        }

        amf::AMFPropertyStorage* out = new amf::AMFInterfaceImpl<amf::AMFPropertyStorageImpl<amf::AMFPropertyStorage>>();
        m_pStatistics->CopyTo(out, false);

        (*pStatistics) = out;
        (*pStatistics)->Acquire();

        return ssdk::transport_common::Result::OK;
    }

    void Subscriber::SetCipher(ssdk::util::AESPSKCipher::Ptr pCipher)
    {
        amf::AMFLock lock(&m_Guard);
        m_pCipher = pCipher;
    }

    ssdk::transport_common::Result Subscriber::GetCipher(ssdk::util::AESPSKCipher::Ptr* pCipher) const
    {
        AMF_RETURN_IF_FALSE(pCipher != nullptr, ssdk::transport_common::Result::FAIL, L"pCipher cannot be NULL");
        amf::AMFLock lock(&m_Guard);
        *pCipher = m_pCipher;

        return ssdk::transport_common::Result::OK;
    }

    void Subscriber::AddDecryptionTime(amf_pts decryptTime)
    {
        amf::AMFLock lock(&m_Guard);
        m_DecryptTimeAccum += decryptTime;
        ++m_TotalRxCnt;
    }

    void Subscriber::OnAudioInMessage(uint8_t opcode, const void* msg, size_t len, ssdk::transport_common::SessionHandle session, ssdk::transport_common::ServerTransport::AudioReceiverCallback* pARCallback)
    {
        size_t messageLen = strlen((char*)msg + 1) + 1;// skip opt code
        const void* audioDataPtr = static_cast<const amf_uint8*>(msg) + messageLen + 1;
        size_t audioDataSize = len - messageLen - 1;

        switch (AUDIO_OP_CODE(opcode))
        {
        case AUDIO_OP_CODE::INIT:
        {
            AudioInit audioInit;
            if (audioInit.ParseBuffer(msg, len) == false)
            {
                AMFTraceError(AMF_FACILITY, L"OnAudioInMessage: AUDIO_OP_CODE::INIT - invalid JSON");
            }
            else
            {
                amf::AMFBufferPtr pBuffer;
                m_pContext->AllocBuffer(amf::AMF_MEMORY_HOST, audioDataSize, &pBuffer);

                memcpy(pBuffer->GetNative(), audioDataPtr, audioDataSize);

                std::stringstream ss;
                amf_uint8* ssBuffer = (amf_uint8*)pBuffer->GetNative();
                for (size_t i = 0; i < audioDataSize; i++)
                {
                    ss << std::setfill('0') << std::setw(2) << std::hex << (int)ssBuffer[i];
                }
                AMFTraceInfo(AMF_FACILITY, L"OnAudioInMessage : AUDIO_OP_CODE_INIT codec=0x%S format=%d rate=%d channels=%d pBuffer-size=%d Buffer=0x%S",
                    audioInit.GetCodecID().c_str(), audioInit.GetFormat(), audioInit.GetSampleRate(), audioInit.GetChannels(), audioDataSize, ss.str().c_str());

                if (pARCallback != nullptr)
                {
                    pARCallback->OnAudioInit(session, audioInit.GetStreamID(), audioInit.GetCodecID().c_str(), audioInit.GetID(),
                        audioInit.GetChannels(), audioInit.GetChannelLayout(), audioInit.GetSampleRate(), audioDataPtr, audioDataSize);
                }
            }
        }
        break;
        case AUDIO_OP_CODE::DATA:
        {
            AudioData audioData;
            if (audioData.ParseBuffer(msg, len) == false)
            {
                AMFTraceError(AMF_FACILITY, L"OnAudioInMessage: AUDIO_OP_CODE::INIT - invalid JSON");
            }
            else
            {
                size_t sizeToDecode = audioData.IsSizePresent() ? (size_t)audioData.GetSize() : audioDataSize;

                amf::AMFBufferPtr pBuffer;
                m_pContext->AllocBuffer(amf::AMF_MEMORY_HOST, sizeToDecode, &pBuffer);

                memcpy(pBuffer->GetNative(), audioDataPtr, sizeToDecode);

                pBuffer->SetPts(audioData.GetPts());

                if (pARCallback != nullptr)
                {
                    ssdk::transport_common::ReceivableAudioBuffer buffer(pBuffer, audioData.GetSequenceNumber(), audioData.GetDiscontinuity());
                    pARCallback->OnAudioBuffer(session, audioData.GetStreamID(), buffer);
                }
            }
        }
        break;
        default:
            AMFTraceError(AMF_FACILITY, L"OnAudioInMessage - invalid opcode %d", opcode);
            break;
        }
        //  Statistics:
        m_AudioBytesRx += len;
        ++m_AudioRxCnt;
    }

    void Subscriber::OnUserDefinedMessage(const void* /*message*/, size_t messageSize)
    {
        // Statistics:
        m_UserBytesRx += messageSize;
        ++m_UserRxCnt;
    }

    ssdk::transport_common::Result Subscriber::SendEvent(const char* id, const ssdk::ctls::CtlEvent* pEvent)
    {
        ssdk::transport_common::Result result = ssdk::transport_common::Result::FAIL;
        Session::Ptr session;
        {
            amf::AMFLock lock(&m_Guard);
            session = m_pClientSession;
        }

        if (session != nullptr)
        {
            DeviceEvent data;
            data.AddValue(id, pEvent->value, pEvent->flags);
            data.Prepare();
            result = TransmitMessage(Channel::SENSORS_OUT, data.GetSendData(), data.GetSendSize());
        }
        return result;
    }

} // namespace ssdk::transport_amd
