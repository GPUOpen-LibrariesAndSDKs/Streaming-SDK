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

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::util::QoS";

namespace ssdk::util
{
    AMF_RESULT QoS::Init(InitParams initParams)
    {
        amf::AMFLock    lock(&m_Guard);
        if (m_Initialized == false)
        {
            ResetCounters();

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
        
        m_EncoderQueueDepth = videoOutputStats.encoderQueueDepth;
        m_Bitrate = videoOutputStats.encoderTargetBitrate;
        m_EncoderTargetFramerate = videoOutputStats.encoderTargetFramerate;
        int64_t bandwidth = videoOutputStats.bandwidth;

        amf_pts now = amf_high_precision_clock();
        if (m_FirstFrameTime == 0)
        {
            m_FirstFrameTime = now;
            m_LastFrameTime = now;
            m_Framerate = m_EncoderTargetFramerate;
        }
        else
        {   
            float secondsBetweenFrames = (float)(now - m_LastFrameTime) / AMF_SECOND;
            m_LastFrameTime = now;
            m_FramerateHistory.AddValue(1 / secondsBetweenFrames, now);
            m_Framerate = m_FramerateHistory.GetAverage();

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
            bool lowerFrameRate = false;
            bool panic = false;

            if (now - m_FirstFrameTime > m_InitParams.timeBeforePanic && m_SessionInfoMap.empty() == true && m_Panic == false)
            {
                //  We've started streaming, but not receiving any stats from clients for too long - this only happens at the beginning
                reasonForPanic = QoSPanic::NO_CLIENT_DATA;
                panic = true;   
                lowerVideoBitrate = true;
                lowerFrameRate = true;
            }
            else    
            {
                // if any client's force IDR request count exceeds panicThresholdIDR, lower bitrate and panic
                // if any client's force IDR request count exceeds thresholdIDR, lower bitrate
                for (SessionInfoMap::const_iterator it = m_SessionInfoMap.begin(); it != m_SessionInfoMap.end(); ++it)
                {
                    if (it->second.m_ForceIDRReqCount > m_InitParams.panicThresholdIDR)
                    {   //  The number of Force IDR requests exceeded panic threshold
                        panic = true;
                        lowerVideoBitrate = true;
                        lowerFrameRate = true;
                        reasonForPanic = QoSPanic::TOO_MANY_IDR_REQUESTS;
                        break;
                    }
                    else if (it->second.m_ForceIDRReqCount > m_InitParams.thresholdIDR)
                    {   //  Too many IDR requests, but not enough to panic yet
                        lowerVideoBitrate = true;
                        lowerFrameRate = true;
                    }
                }
                if (panic == false) //  No need to check this when we're already panicing
                {

                    float txAverage = m_FramerateHistory.GetAverage();
                    if (m_Framerate > txAverage)
                    {
                        m_Framerate = txAverage;
                    }

                    for (SessionInfoMap::const_iterator it = m_SessionInfoMap.begin(); it != m_SessionInfoMap.end(); ++it)
                    {
                        const FramerateHistory& sessionFramerateHistory = it->second.m_FramerateHistory;
                        if (sessionFramerateHistory.IsHistoryFull() == true)
                        {
                            if (now - sessionFramerateHistory.GetLastUpdateTime() > m_InitParams.timeBeforePanic)
                            {
                                panic = true;
                                reasonForPanic = QoSPanic::NO_CLIENT_DATA;
                                lowerVideoBitrate = true;
                                lowerFrameRate = true;
                                break;
                            }
                            
                            float sessionFramerate = sessionFramerateHistory.GetAverage();
                            if (m_Framerate > sessionFramerate)
                            {
                                m_Framerate = sessionFramerate;
                            }
                            else if (sessionFramerate < (sessionFramerateHistory.GetAverage() * 0.8) && sessionFramerate != 0)
                            {
                                lowerVideoBitrate = true;
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
                            }
                        }

                        if (lowerFrameRate == false && m_DecoderQueueDepthExcess == true)
                        {   //  At least one of the clients' decoder cannot keep up
                            lowerFrameRate = true;
                            m_DecoderQueueDepthExcess = false;
                        }

                        if (m_EncoderQueueDepth > m_InitParams.maxEncoderQueueDepth)
                        {   //  Video Encoder overloaded
                            lowerFrameRate = true;
                            NotifyCallback(QoSEvent::VIDEO_ENCODER_QUEUE_THRESHOLD_EXCEEDED, amf::AMFVariant(m_EncoderQueueDepth));
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
                            AdjustFramerate(-m_Framerate);
                        }
                        if ((m_InitParams.strategy == QoSStrategy::ADJUST_VIDEOBITRATE || m_InitParams.strategy == QoSStrategy::ADJUST_BOTH) && lowerVideoBitrate == true)
                        {
                            AdjustVideoBitrate(m_Bitrate, -m_Bitrate);
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
                }

                //  Adjust frame rate
                if ((m_InitParams.strategy == QoSStrategy::ADJUST_FRAMERATE || m_InitParams.strategy == QoSStrategy::ADJUST_BOTH) &&
                    (((now - m_LastFpsAdjustmentTime) > m_InitParams.framerateAdjustmentPeriod)))
                {
                    if (lowerFrameRate == true) //  Lower FPS because the encoder cannot keep up
                    {
                        m_Framerate = AdjustFramerate(-m_InitParams.framerateStep);
                    }
                    else if (lowerVideoBitrate == true && m_InitParams.strategy == QoSStrategy::ADJUST_BOTH)
                    {
                        // Lowering bitrate, don't increase FPS
                    }
                    else    //  Increase FPS - things are improving
                    {
                        float fpsThreshold = m_Framerate - m_InitParams.framerateStep;
                        if (fpsThreshold < 0)
                        {
                            fpsThreshold = m_Framerate * 0.8f;
                        }
                        if (m_Framerate >= fpsThreshold)
                        {
                            m_Framerate = AdjustFramerate(m_InitParams.framerateStep);
                        }
                    }
                }

                //  Adjust video bitrate
                if ((m_InitParams.strategy == QoSStrategy::ADJUST_VIDEOBITRATE || m_InitParams.strategy == QoSStrategy::ADJUST_BOTH) &&
                    ((now - m_LastVideoBitrateAdjustmentTime) > m_InitParams.bitrateAdjustmentPeriod))
                {
                    if (lowerVideoBitrate == true) //  Either channel is bad or client cannot keep up with decryption
                    {
                        AdjustVideoBitrate(m_Bitrate, -m_InitParams.bitrateStep);
                    }
                    else if (lowerFrameRate == true && m_InitParams.strategy == QoSStrategy::ADJUST_BOTH)
                    {
                        // Lowering framerate, don't increase bitrate
                    }
                    else    //  Increase video bitrate - things are improving
                    {
                        int64_t bitrateThreshold = m_Bitrate - m_InitParams.bitrateStep;
                        if (bitrateThreshold <= 0)// or strict <
                        {
                            bitrateThreshold = int64_t(m_Bitrate * 0.8f);
                        }
                        if (m_BitrateHistory.GetAverage() > bitrateThreshold) //  Increase the bitrate only when the actual bandwidth is approaching the current bitrate
                        {
                            AdjustVideoBitrate(m_Bitrate, m_InitParams.bitrateStep);
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
        }

        if (sendTime > m_WorstSendTime)
        {
            m_WorstSendTime = sendTime;
            m_WorstSendTimeHistory.AddValue(sendTime, now);
        }

        if (decoderQueueDepth > m_InitParams.maxDecoderQueueDepth)
        {
            m_DecoderQueueDepthExcess = true;
        }
    }

    void QoS::ResetCounters()
    {
        m_InitParams = {};

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
        m_DecoderQueueDepthExcess = false;        
    }

    float QoS::AdjustFramerate(float step)
    {
        float result = m_Framerate;

        float targetFps = m_Framerate + step;
        if (targetFps < m_InitParams.minFramerate)
        {
            targetFps = m_InitParams.minFramerate;
            if (targetFps != m_EncoderTargetFramerate)
            {
                NotifyCallback(QoSEvent::FPS_REACHED_LOW_LIMIT, amf::AMFVariant(targetFps));
            }
        }
        else if (targetFps > m_InitParams.maxFramerate)
        {
            targetFps = m_InitParams.maxFramerate;
            if (targetFps != m_EncoderTargetFramerate)
            {
                NotifyCallback(QoSEvent::FPS_REACHED_HIGH_LIMIT, amf::AMFVariant(targetFps));
            }
        }

        if (targetFps != m_EncoderTargetFramerate)
        {
            NotifyCallback(QoSEvent::FPS_CHANGE, amf::AMFVariant(targetFps));
            m_LastFpsAdjustmentTime = amf_high_precision_clock();
            result = targetFps;
        }
        
        return result;
    }

    int64_t QoS::AdjustVideoBitrate(int64_t current, int64_t step)
    {
        int64_t result = current;

        int64_t targetBitrate = current + step;
        if (targetBitrate < m_InitParams.minBitrate)
        {
            targetBitrate = m_InitParams.minBitrate;
            if (targetBitrate != m_Bitrate)
            {
                NotifyCallback(QoSEvent::VIDEO_BITRATE_REACHED_LOW_LIMIT, amf::AMFVariant(targetBitrate));
            }
        }
        else if (targetBitrate > m_InitParams.maxBitrate)
        {
            targetBitrate = m_InitParams.maxBitrate;
            if (targetBitrate != m_Bitrate)
            {
                NotifyCallback(QoSEvent::VIDEO_BITRATE_REACHED_HIGH_LIMIT, amf::AMFVariant(targetBitrate));
            }
        }

        if (targetBitrate != m_Bitrate)
        {
            NotifyCallback(QoSEvent::VIDEO_BITRATE_CHANGED, amf::AMFVariant(targetBitrate));
            m_LastVideoBitrateAdjustmentTime = amf_high_precision_clock();
            result = targetBitrate;
        }

        return result;
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
    }
}
