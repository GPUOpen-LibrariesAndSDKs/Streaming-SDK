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

#include "QoS.h"
#include "amf/public/common/TraceAdapter.h"

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::util::QoS";

namespace ssdk::util
{
    AMF_RESULT QoS::Init(InitParams initParams)
    {
        amf::AMFLock    lock(&m_Guard);
        if (m_Initialized == false)
        {
            m_InitParams = initParams;
            m_Initialized = true;
        }
        return AMF_OK;
    }

    AMF_RESULT QoS::Terminate()
    {
        amf::AMFLock    lock(&m_Guard);
        if (m_Initialized == true)
        {
            ResetCounters();

            m_Initialized = false;
        }
        return AMF_OK;
    }

    AMF_RESULT QoS::AdjustStreamQuality(VideoOutputStats videoOutputStats)
    {
        amf::AMFLock    lock(&m_Guard);
        if (m_Initialized != true)
        {
            return AMF_NOT_INITIALIZED;
        }

        if (m_SessionInfoMap.size() > 0)
        {
            m_EncoderQueueDepth = videoOutputStats.encoderQueueDepth;
            int64_t bandwidth = videoOutputStats.bandwidth;

            amf_pts now = amf_high_precision_clock();
            if (m_FirstFrameTime == 0)
            {
                m_FirstFrameTime = now;
                m_LastFrameTime = now;
                m_Bitrate = videoOutputStats.encoderTargetBitrate;
                m_Framerate = videoOutputStats.encoderTargetFramerate;
            }
            else
            {
                float secondsBetweenFrames = (float)(now - m_LastFrameTime) / AMF_SECOND;
                m_LastFrameTime = now;
                m_FramerateHistory.AddValue(1 / secondsBetweenFrames, now);
                if (m_FramerateHistory.IsHistoryFull() == true)
                {
                    m_Framerate = m_FramerateHistory.GetAverage();
                }

                m_AccumulatedBandwidth += bandwidth;
                amf_pts lastBitrateMeasureTime = now - m_BitrateHistory.GetLastUpdateTime();
                if (lastBitrateMeasureTime > AMF_SECOND)
                {
                    int64_t bitrate = (AMF_SECOND * m_AccumulatedBandwidth) / lastBitrateMeasureTime;
                    m_BitrateHistory.AddValue(bitrate, now);
                    m_AccumulatedBandwidth = 0;
                }
            }


            if (m_LastFpsAdjustmentTime == 0 || m_LastVideoBitrateAdjustmentTime == 0)
            {
                // handle fist call
                m_LastVideoBitrateAdjustmentTime = m_LastFpsAdjustmentTime = now;
            }
            else
            {
                QoSPanic reasonForPanic = QoSPanic::NO_CLIENT_DATA;

                bool lowerVideoBitrate = false;
                int64_t targetBitrate = videoOutputStats.encoderTargetBitrate;
                bool lowerFrameRate = false;
                float targetFramerate = videoOutputStats.encoderTargetFramerate;
                bool panic = false;
                bool immediate = false;

                if (now - m_FirstFrameTime > m_InitParams.timeBeforePanic && m_SessionInfoMap.empty() == true && m_Panic == false)
                {
                    //  We've started streaming, but not receiving any stats from clients for too long - this only happens at the beginning
                    reasonForPanic = QoSPanic::NO_CLIENT_DATA;
                    panic = true;
                    lowerVideoBitrate = true;
                    lowerFrameRate = true;
                    AMFTraceWarning(AMF_FACILITY, L"Have not received statistics from the client at the beginning of the stream, QoS is panicing");
                }
                else
                {
                    if (panic == false) //  No need to check this when we're already panicing
                    {
                        static constexpr const int MAX_DECODER_OVERFLOW_EVENTS = 5;
                        static constexpr const int MAX_CONGESTION_EVENTS = 5;
                        for (SessionInfoMap::iterator it = m_SessionInfoMap.begin(); it != m_SessionInfoMap.end(); ++it)
                        {
                            // if any client's force IDR request count exceeds panicThresholdIDR, lower bitrate and panic
                            // if any client's force IDR request count exceeds thresholdIDR, lower bitrate
                            if (it->second.m_ForceIDRReqCount > m_InitParams.panicThresholdIDR)
                            {   //  The number of Force IDR requests exceeded panic threshold
                                panic = true;
                                lowerVideoBitrate = true;
                                lowerFrameRate = true;
                                reasonForPanic = QoSPanic::TOO_MANY_IDR_REQUESTS;
                                AMFTraceWarning(AMF_FACILITY, L"Client is sending too many key frame repeat requests, QoS is panicing, bandwidth %5.2f Mbps", float(m_BitrateHistory.GetAverage()) / 1024 / 1024);
                                break;
                            }
                            else if (it->second.m_ForceIDRReqCount > m_InitParams.thresholdIDR)
                            {   //  Too many IDR requests, but not enough to panic yet
                                lowerVideoBitrate = true;
                                lowerFrameRate = true;
                                AMFTraceInfo(AMF_FACILITY, L"Client is requesting key frames, QoS is lowering bitrate and frame rate, bandwidth %5.2f Mbps", float(m_BitrateHistory.GetAverage()) / 1024 / 1024);
                            }
                            if (it->second.m_DecoderQueueDepth > 0)
                            {
                                lowerFrameRate = true;
                                if (it->second.m_DecoderQueueDepth > m_InitParams.maxDecoderQueueDepth)
                                {
                                    panic = true;
                                    reasonForPanic = QoSPanic::CLIENT_CANT_KEEP_UP;
                                    AMFTraceWarning(AMF_FACILITY, L"Client cannot keep up at %5.2f, decoder queue depth %lld, overflow count %d, QoS is panicing", m_Framerate, it->second.m_DecoderQueueDepth, it->second.m_DecoderQueueOverflowCnt);
                                }
                                if (now - m_LastFpsAdjustmentTime > m_InitParams.framerateAdjustmentPeriod / 4)
                                {
                                    immediate = true;
                                    if (panic == false)
                                    {
                                        if (++it->second.m_DecoderQueueOverflowCnt > MAX_DECODER_OVERFLOW_EVENTS)
                                        {
                                            it->second.m_DecoderQueueOverflowFps = m_Framerate;
                                            AMFTraceWarning(AMF_FACILITY, L"Client cannot keep up at %5.2f, decoder queue depth %lld, overflow count %d, QoS is limiting frame rate", m_Framerate, it->second.m_DecoderQueueDepth, it->second.m_DecoderQueueOverflowCnt);
                                        }
                                        else
                                        {
                                            AMFTraceWarning(AMF_FACILITY, L"Client cannot keep up at %5.2f, decoder queue depth %lld, overflow count %d, QoS is lowering frame rate", m_Framerate, it->second.m_DecoderQueueDepth, it->second.m_DecoderQueueOverflowCnt);
                                        }
                                    }
                                    else
                                    {
                                        it->second.m_DecoderQueueOverflowCnt = 0;
                                    }
                                }
                            }
                            auto& sessionFramerateHistory = it->second.m_FramerateHistory;
                            if (sessionFramerateHistory.IsHistoryFull() == true)
                            {
                                if (now - sessionFramerateHistory.GetLastUpdateTime() > m_InitParams.timeBeforePanic)
                                {
                                    panic = true;
                                    reasonForPanic = QoSPanic::NO_CLIENT_DATA;
                                    lowerVideoBitrate = true;
                                    lowerFrameRate = true;
                                    AMFTraceWarning(AMF_FACILITY, L"Have not received statistics from the client, QoS is panicing");
                                    break;
                                }

                                float sessionFramerate = sessionFramerateHistory.GetAverage();
                                if (sessionFramerate != 0 && m_Framerate != 0 && m_Framerate > sessionFramerate * 1.15 && (now - m_LastFpsAdjustmentTime) > m_InitParams.framerateAdjustmentPeriod)
                                {
                                    lowerVideoBitrate = true;
                                    sessionFramerateHistory.Clear();
                                    AMFTraceWarning(AMF_FACILITY, L"Frame rate reported by the receiver (%5.2f) is lower than frame rate measured at the server (%5.2f), possible network congestion, QoS is lowering bitrate", sessionFramerate, m_Framerate);
                                    if (++it->second.m_CongestionCnt > MAX_CONGESTION_EVENTS)
                                    {
                                        it->second.m_CongestionBitrate = m_Bitrate;
                                        AMFTraceWarning(AMF_FACILITY, L"One or more of the clients has limited network bandwidth, QoS is limiting video bitrate to %lld to avoid congestion", targetBitrate);
                                    }
                                }
                            }
                            if (it->second.m_DecoderQueueOverflowCnt > MAX_DECODER_OVERFLOW_EVENTS && targetFramerate >= it->second.m_DecoderQueueOverflowFps)
                            {
                                targetFramerate = it->second.m_DecoderQueueOverflowFps - m_InitParams.framerateStep;
                                if (targetFramerate < m_InitParams.minFramerate)
                                {
                                    targetFramerate = m_InitParams.minFramerate;
                                }
                            }
                            if (it->second.m_CongestionCnt > MAX_CONGESTION_EVENTS && targetBitrate >= it->second.m_CongestionBitrate)
                            {
                                targetBitrate = it->second.m_CongestionBitrate - m_InitParams.bitrateStep;
                                if (targetBitrate < m_InitParams.minBitrate)
                                {
                                    targetBitrate = m_InitParams.minBitrate;
                                }
                            }
                        }

                        if (panic == false)
                        {
                            if (m_Framerate != 0 && lowerVideoBitrate == false)
                            {
                                float frameTime = 1000 / m_Framerate;
                                if (m_WorstSendTime > (frameTime * 2.0f) ||
                                    (m_WorstSendTimeHistory.IsHistoryFull() == true && m_WorstSendTimeHistory.GetAverage() > frameTime))
                                {   //  We're seeing spikes in send() execution time, indicating frequent hiccups on the network (usually TCP) - try lowering both FPS and bitrate by 1 step
                                    lowerVideoBitrate = true;
                                    lowerFrameRate = true;
                                    m_WorstSendTime = 0.0f;
                                    AMFTraceWarning(AMF_FACILITY, L"Send is taking too long, possible network congestion, QoS is lowering bitrate and frame rate");
                                }
                            }

                            if (m_EncoderQueueDepth > m_InitParams.maxEncoderQueueDepth)
                            {   //  Video Encoder overloaded
                                lowerFrameRate = true;
                                immediate = true;
                                NotifyCallback(QoSEvent::VIDEO_ENCODER_QUEUE_THRESHOLD_EXCEEDED, amf::AMFVariant(m_EncoderQueueDepth));
                                AMFTraceWarning(AMF_FACILITY, L"Video encoder cannot keep up, QoS is lowering frame rate");
                            }
                        }
                    }
                }

                if (panic == true)
                {
                    if (now - m_LastPanicTime > m_InitParams.timeBeforePanic)    //  We're panicing - lower bitrate & frame rate to the minimum and wait for things to improve
                    {
                        m_LastPanicTime = now;
                        if (m_Panic == false)
                        {
                            NotifyCallback(QoSEvent::PANIC, amf::AMFVariant(int64_t(reasonForPanic)));
                            if ((m_InitParams.strategy == QoSStrategy::ADJUST_FRAMERATE || m_InitParams.strategy == QoSStrategy::ADJUST_BOTH) && lowerFrameRate == true)
                            {
                                AMFTraceWarning(AMF_FACILITY, L"QoS is panicing, setting frame rate to minimum");
                                AdjustFramerate(m_InitParams.minFramerate);
                                m_FramerateHistory.Clear();
                            }
                            if ((m_InitParams.strategy == QoSStrategy::ADJUST_VIDEOBITRATE || m_InitParams.strategy == QoSStrategy::ADJUST_BOTH) && lowerVideoBitrate == true)
                            {
                                AMFTraceWarning(AMF_FACILITY, L"QoS is panicing, setting bitrate to minimum, actual bandwidth %5.2f Mbps", float(m_BitrateHistory.GetAverage()) / 1024 / 1024);
                                AdjustVideoBitrate(m_InitParams.minBitrate);
                                m_BitrateHistory.Clear();
                            }
                            m_Panic = true;
                        }
                    }
                }
                else    //  Not panicing - normal operation
                {
                    if (m_Panic == true)
                    {
                        m_Panic = false;
                        NotifyCallback(QoSEvent::PANIC_ENDED, amf::AMFVariant());
                        AMFTraceInfo(AMF_FACILITY, L"QoS panic ended");
                    }

                    //  Adjust frame rate
                    if (((m_InitParams.strategy == QoSStrategy::ADJUST_FRAMERATE || m_InitParams.strategy == QoSStrategy::ADJUST_BOTH) &&
                        ((now - m_LastFpsAdjustmentTime) > m_InitParams.framerateAdjustmentPeriod && m_FramerateHistory.IsHistoryFull() == true)) || immediate == true)
                    {
                        if (lowerFrameRate == true) //  Lower FPS because the encoder cannot keep up
                        {
                            AMFTraceDebug(AMF_FACILITY, L"QoS is decreasing frame rate by %5.2f fps", m_InitParams.framerateStep);
                            AdjustFramerate(m_Framerate - m_InitParams.framerateStep);
                        }
                        else if (m_Framerate < targetFramerate)   //  Increase FPS - things are improving
                        {
                            AMFTraceDebug(AMF_FACILITY, L"QoS is increasing frame rate by %5.2f fps", m_InitParams.framerateStep);
                            AdjustFramerate(m_Framerate + m_InitParams.framerateStep);
                        }
                    }

                    //  Adjust video bitrate
                    if ((m_InitParams.strategy == QoSStrategy::ADJUST_VIDEOBITRATE || m_InitParams.strategy == QoSStrategy::ADJUST_BOTH) &&
                        ((now - m_LastVideoBitrateAdjustmentTime) > m_InitParams.bitrateAdjustmentPeriod) && m_BitrateHistory.IsHistoryFull() == true)
                    {
                        if (lowerVideoBitrate == true) //  Either channel is bad or client cannot keep up with decryption
                        {
                            AMFTraceDebug(AMF_FACILITY, L"QoS is decreasing video bitrate by %lld bps, actual bandwidth is %5.2f Mbps", m_InitParams.bitrateStep, float(m_BitrateHistory.GetAverage()) / 1024 / 1024);
                            AdjustVideoBitrate(m_Bitrate - m_InitParams.bitrateStep);
                        }
                        else if (m_Bitrate < targetBitrate)   //  Increase video bitrate - things are improving
                        {
                            AMFTraceDebug(AMF_FACILITY, L"QoS is increasing video bitrate by %lld bps, actual bandwidth is %5.2f Mbps", m_InitParams.bitrateStep, float(m_BitrateHistory.GetAverage()) / 1024 / 1024);
                            AdjustVideoBitrate(m_Bitrate + m_InitParams.bitrateStep);
                        }
                    }
                }
            }
        }
        return AMF_OK;
    }

    void QoS::UpdateSessionStats(ssdk::transport_common::SessionHandle session, amf_pts lastStatsTime, float framerate, int64_t forceIDRReqCount, float sendTime, int64_t decoderQueueDepth)
    {
        if (m_Initialized != true)
        {
            return;
        }

        amf_pts now = amf_high_precision_clock();

        SessionInfo& sessionInfo = m_SessionInfoMap[session];
        sessionInfo.m_FramerateHistory.AddValue(framerate, lastStatsTime);

        if (sessionInfo.m_ForceIDRReqCountUpdateTime < lastStatsTime)
        {
            sessionInfo.m_ForceIDRReqCount = forceIDRReqCount;
            sessionInfo.m_ForceIDRReqCountUpdateTime = lastStatsTime;
            sessionInfo.m_DecoderQueueDepth = decoderQueueDepth;
            if (decoderQueueDepth > 0)
            {
//                sessionInfo.m_DecoderQueueOverflowCnt++;
//                sessionInfo.m_DecoderQueueOverflowFps = m_Framerate;
//                AMFTraceWarning(AMF_FACILITY, L"Decoder overflow at %5.2f fps, depth is %lld, count=%d", m_Framerate, decoderQueueDepth, sessionInfo.m_DecoderQueueOverflowCnt);
            }
        }

        if (sendTime > m_WorstSendTime)
        {
            m_WorstSendTime = sendTime;
            m_WorstSendTimeHistory.AddValue(sendTime, now);
        }

    }

    void QoS::ResetCounters()
    {
        m_FirstFrameTime = 0;
        m_LastFrameTime = 0;
        m_FramerateHistory.Clear();
        m_Framerate = 0;
        m_LastFpsAdjustmentTime = 0;
        m_AccumulatedBandwidth = 0;
        m_LastVideoBitrateAdjustmentTime = 0;
        m_Panic = false;
        m_LastPanicTime = 0;

        m_SessionInfoMap.clear();
        m_BitrateHistory.Clear();
        m_WorstSendTime = 0.0f;

        NotifyCallback(QoSEvent::FPS_CHANGE, amf::AMFVariant(m_InitParams.maxFramerate));
        NotifyCallback(QoSEvent::VIDEO_BITRATE_CHANGED, amf::AMFVariant(m_InitParams.maxBitrate));
    }

    void QoS::AdjustFramerate(float targetFps)
    {
        if (targetFps < m_InitParams.minFramerate)
        {
            targetFps = m_InitParams.minFramerate;
            if (m_Framerate != m_InitParams.minFramerate)
            {
                NotifyCallback(QoSEvent::FPS_REACHED_LOW_LIMIT, amf::AMFVariant(targetFps));
            }
        }
        else if (targetFps > m_InitParams.maxFramerate)
        {
            targetFps = m_InitParams.maxFramerate;
            if (m_Framerate != m_InitParams.maxFramerate)
            {
                NotifyCallback(QoSEvent::FPS_REACHED_HIGH_LIMIT, amf::AMFVariant(targetFps));
            }
        }

        if (targetFps != m_Framerate)
        {
            NotifyCallback(QoSEvent::FPS_CHANGE, amf::AMFVariant(targetFps));
            m_LastFpsAdjustmentTime = amf_high_precision_clock();
        }
    }

    void QoS::AdjustVideoBitrate(int64_t targetBitrate)
    {
        if (targetBitrate < m_InitParams.minBitrate)
        {
            targetBitrate = m_InitParams.minBitrate;
            if (m_Bitrate != m_InitParams.minBitrate)
            {
                NotifyCallback(QoSEvent::VIDEO_BITRATE_REACHED_LOW_LIMIT, amf::AMFVariant(targetBitrate));
            }
        }
        else if (targetBitrate > m_InitParams.maxBitrate)
        {
            targetBitrate = m_InitParams.maxBitrate;
            if (m_Bitrate != m_InitParams.maxBitrate)
            {
                NotifyCallback(QoSEvent::VIDEO_BITRATE_REACHED_HIGH_LIMIT, amf::AMFVariant(targetBitrate));
            }
        }

        if (targetBitrate != m_Bitrate)
        {
            NotifyCallback(QoSEvent::VIDEO_BITRATE_CHANGED, amf::AMFVariant(targetBitrate));
            m_LastVideoBitrateAdjustmentTime = amf_high_precision_clock();
            m_Bitrate = targetBitrate;
        }
    }

    void QoS::NotifyCallback(QoSEvent event, const amf::AMFVariantStruct& value)
    {
        if (m_pQoSCB != nullptr)
        {
            m_pQoSCB->OnQoSEvent(m_StreamID, event, &value);
        }
    }

    void QoS::UnregisterSession(transport_common::SessionHandle session)
    {
        amf::AMFLock lock(&m_Guard);
        m_SessionInfoMap.erase(session);
        if (m_SessionInfoMap.empty() == true)
        {   //  Once the last session has disconnected, restore the defaults
            ResetCounters();
        }
    }
}
