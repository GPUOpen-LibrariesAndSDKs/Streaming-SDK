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

#include "transports/transport-common/ServerTransport.h"
#include "amf/public/common/Thread.h"
#include "ValueHistory.h"

#include <map>
#include <set>

namespace ssdk::util
{
    class QoS
    {
    public:
        typedef std::shared_ptr<QoS> Ptr;

    public:
        enum class QoSEvent
        {
            PANIC,                                    //  Panic mode triggered due to lack of data from the client or too many FORCE_IDR requests, value contains one of the QoSPanic enums
            PANIC_ENDED,                              //  Panic mode ended
            FPS_CHANGE,                               //  QoS is about to change the frame rate to float(value)
            FPS_REACHED_LOW_LIMIT,                    //  Frame rate has reached the low limit of float(value)
            FPS_REACHED_HIGH_LIMIT,                   //  Frame rate has reached the high limit of float(value)
            VIDEO_BITRATE_CHANGED,                    //  QoS is about to change video bitrate to int64_t(value)
            VIDEO_BITRATE_REACHED_LOW_LIMIT,          //  Video bitrate has reached the low limit of int64_t(value)
            VIDEO_BITRATE_REACHED_HIGH_LIMIT,         //  Video bitrate has reached the high limit of int64_t(value)
            VIDEO_ENCODER_QUEUE_THRESHOLD_EXCEEDED    //  Video encoder queue depth threshold has been exceeeded
        };

        enum class QoSPanic
        {
            NO_CLIENT_DATA,                   //  QoS Panic mode was triggered by the lack of data from the client
            TOO_MANY_IDR_REQUESTS             //  QoS Panic mode was triggered by exceeding the IDR frame requests threshold
        };

        enum class QoSStrategy
        {
            ADJUST_FRAMERATE,                           //  Allow QoS to adjust frame rate
            ADJUST_VIDEOBITRATE,                        //  Allow QoS to adjust video bitrate
            ADJUST_BOTH                                 //  Allow QoS to adjust frame rate and video bitrate
        }; 

        //  QoSCallback: implement when QoS provides feedback
        class QoSCallback
        {
        public:
            virtual void OnQoSEvent(ssdk::transport_common::StreamID streamID, QoSEvent event, const amf::AMFVariantStruct* value) = 0;
        };

        class InitParams
        {
        public:
            // QoS stats
            amf_pts timeBeforePanic = 0;                    // in amf_pts - lower fps and bitrate to min if time since the last statistics from client exceeds this value
            int64_t thresholdIDR = 0;                       // the number of FORCE_IDR requests in a stat interval to lower quality
            int64_t panicThresholdIDR = 0;                  // the number of FORCE_IDR requests in a stat interval to trigger panic mode
            int64_t maxEncoderQueueDepth = 0;               // the maximum number of frames allowed to accumulate in encoder before lowering the frame rate
            int64_t maxDecoderQueueDepth = 0;               // the maximum number of frames allowed to accumulate in decoder before lowering the frame rate
            QoSStrategy strategy = QoSStrategy::ADJUST_BOTH;// QoS strategy - a 64-bit mask (see above)
            
            float   minFramerate = 0.0f;                    // minimum fps QoS would go down to
            float   maxFramerate = 0.0f;                    // maximum fps QoS would go up to
            float   framerateStep = 0;                      // fps adjustment step
            amf_pts framerateAdjustmentPeriod = 5;          // in amf_pts - frequency of frame rate adjustments
            
            int64_t minBitrate = 0;                         // minimum video bitrate QoS would go down to in bps
            int64_t maxBitrate = 0;                         // maximum video bitrate QoS would go up to in bps
            int64_t bitrateStep = 0;                        // video bitrate adjustment step in bps
            amf_pts bitrateAdjustmentPeriod = 10;           // in amf_pts - frequency of video bitrate adjustments
        };
        
        class VideoOutputStats
        {
        public:
            int64_t encoderQueueDepth = 0;                  // current number of frames in the encoder queue
            int64_t encoderTargetBitrate = 0;               // Target bit rate in bits
            float   encoderTargetFramerate = 0;             // frame rate set on encoder
            int64_t bandwidth = 0;
        };

    public:
        QoS(QoSCallback* pQoSCB, ssdk::transport_common::StreamID streamID) : m_pQoSCB(pQoSCB), m_StreamID(streamID) {};
        ~QoS() {};

        AMF_RESULT Init(InitParams initParams);
        AMF_RESULT Terminate();

        AMF_RESULT AdjustStreamQuality(VideoOutputStats videoOutputStats);
        void UpdateSessionStats(ssdk::transport_common::SessionHandle session, amf_pts lastStatsTime,
            float framerate, int64_t forceIDRReqCount, float sendTime, int64_t decoderQueueDepth);
        void UnregisterSession(transport_common::SessionHandle session);

    private:
        void ResetCounters();
        float AdjustFramerate(float step);
        int64_t AdjustVideoBitrate(int64_t current, int64_t step);
        void NotifyCallback(QoSEvent event, const amf::AMFVariantStruct& value);
        
    private:
        QoSCallback*            m_pQoSCB = nullptr;
        ssdk::transport_common::StreamID m_StreamID;
        mutable amf::AMFCriticalSection m_Guard;
        bool                    m_Initialized = false;
        InitParams              m_InitParams;

        typedef ValueHistory<float, 4> FramerateHistory;
        typedef ValueHistory<int64_t, 4> BitrateHistory;

        amf_pts                 m_FirstFrameTime = 0;
        amf_pts                 m_LastFrameTime = 0;
        float                   m_Framerate = 0;
        float                   m_EncoderTargetFramerate = 0;
        FramerateHistory        m_FramerateHistory;
        amf_pts                 m_LastFpsAdjustmentTime = 0;

        int64_t                 m_AccumulatedBandwidth = 0;

        BitrateHistory          m_BitrateHistory;
        int64_t                 m_Bitrate = 0;
        amf_pts                 m_LastVideoBitrateAdjustmentTime = 0;

        int64_t                 m_EncoderQueueDepth = 0;
        bool                    m_Panic = false;
        amf_pts                 m_LastPanicTime = 0;    
        
        // collected session stats updated in UpdateSessionStats  
        class SessionInfo
        {
        public:
            FramerateHistory m_FramerateHistory;
            amf_int64 m_ForceIDRReqCount = 0;
            amf_pts m_ForceIDRReqCountUpdateTime = 0;
        };
        
        typedef std::map<transport_common::SessionHandle, SessionInfo> SessionInfoMap;
        SessionInfoMap m_SessionInfoMap;
        float                       m_WorstSendTime = 0;
        ValueHistory<float, 5>      m_WorstSendTimeHistory;
        bool                        m_DecoderQueueDepthExcess = false;
    };
}
