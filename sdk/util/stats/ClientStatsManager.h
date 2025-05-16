//
// Notice Regarding Standards.  AMD does not provide a license or sublicense to
// any Intellectual Property Rights relating to any standards, including but not
// limited to any audio and/or video codec technologies such as MPEG-2, MPEG-4;
// AVC/H.264; HEVC/H.265; AAC decode/FFMPEG; AAC encode/FFMPEG; VC-1; and MP3
// (collectively, the "Media Technologies"). For clarity, you will pay any
// royalties due for such third party technologies, which may include the Media
// Technologies that are owed as a result of AMD providing the Software to you.
//
// MIT license
//
//
// Copyright (c) 2018 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "transports/transport-common/ClientTransport.h"
#include "util/QoS/ValueHistory.h"
#include "amf/public/common/Thread.h"

#include <map>

namespace ssdk::util
{
    class ClientStatsManager
    {
    public:
        typedef std::shared_ptr<ClientStatsManager>    Ptr;

        class StatsSnapshotImpl : public ssdk::transport_common::ClientTransport::StatsSnapshot
        {
        public:
            StatsSnapshotImpl() = delete;
            StatsSnapshotImpl(float fullLatency, float clientLatency, float serverLatency, float encoderLatency, float networkLatency, float decoderLatency, float decryptLatency, int32_t decoderQueueDepth, float AVDesync, float framerate) :
                m_FullLatency(fullLatency),
                m_ClientLatency(clientLatency),
                m_ServerLatency(serverLatency),
                m_EncoderLatency(encoderLatency),
                m_NetworkLatency(networkLatency),
                m_DecoderLatency(decoderLatency),
                m_DecryptLatency(decryptLatency),
                m_DecoderQueueDepth(decoderQueueDepth),
                m_AVDesync(AVDesync),
                m_Framerate(framerate)
            {
            }
            virtual ~StatsSnapshotImpl() = default;
            float   GetFullLatency() const { return m_FullLatency; }
            float   GetClientLatency() const { return m_ClientLatency; }
            float   GetServerLatency() const { return m_ServerLatency; }
            float   GetEncoderLatency() const { return m_EncoderLatency; }
            float   GetNetworkLatency() const { return m_NetworkLatency; }
            float   GetDecoderLatency() const { return m_DecoderLatency; }
            float   GetDecryptLatency() const { return m_DecryptLatency; }
            int32_t GetDecoderQueueDepth() const { return m_DecoderQueueDepth; }
            float   GetAVDesync() const { return m_AVDesync; }
            float   GetFramerate() const { return m_Framerate; }
        protected:
            float   m_FullLatency = 0;
            float   m_ClientLatency = 0;
            float   m_ServerLatency = 0;
            float   m_EncoderLatency = 0;
            float   m_NetworkLatency = 0;
            float   m_DecoderLatency = 0;
            float   m_DecryptLatency = 0;
            int32_t m_DecoderQueueDepth = 0;
            float   m_AVDesync = 0;
            float   m_Framerate = 0;
        };

    public:
        ClientStatsManager();
        ~ClientStatsManager() {};
        ClientStatsManager(const ClientStatsManager&) = delete;
        ClientStatsManager& operator=(const ClientStatsManager&) = delete;
        void SetTransport(ssdk::transport_common::ClientTransport* transport);
        void Init();
        void Clear();
        
        void IncrementDecoderQueueDepth(amf_pts pts);
        void DecrementDecoderQueueDepth(amf_pts pts);
        void UpdateFullLatencyAndFramerate(amf_pts fullLatencyPts);
        void UpdateClientLatency(amf_pts clientLatencyPts);
        void UpdateServerLatency(amf_pts serverLatencyPts);
        void UpdateEncoderLatency(amf_pts encoderLatencyPts);
        void UpdateAudioStatistics(amf_pts AVDesync);
        void UpdateDecryptStatistics(amf_pts decrypt);
        void SendStatistics();

    private:
        mutable amf::AMFCriticalSection m_Guard;
        ssdk::transport_common::ClientTransport* m_Transport = nullptr;
        
        typedef ssdk::util::ValueAverage<float> FloatValueAverage;
        FloatValueAverage m_FrameDurationHistory;
        FloatValueAverage m_FullLatencyHistory;
        FloatValueAverage m_ClientLatencyHistory;
        FloatValueAverage m_ServerLatencyHistory;
        FloatValueAverage m_EncoderLatencyHistory;
        FloatValueAverage m_DecoderLatencyHistory;
        FloatValueAverage m_DecryptHistory;
        FloatValueAverage m_AVDesyncHistory;
        typedef std::map<amf_pts, amf_pts> DecoderQueueTimes;
        DecoderQueueTimes m_DecoderQueueTimes;

        amf_pts m_LastFrameTime = 0;
        amf_pts m_LastSendStatsTime = 0;

    };
}