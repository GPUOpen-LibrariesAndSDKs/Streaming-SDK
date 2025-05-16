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

#include "ClientStatsManager.h"

#include "amf/public/common/TraceAdapter.h"

static constexpr int64_t STATISTICS_SEND_PERIOD_SECONDS = 2;

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::util::ClientStatsManager";

namespace ssdk::util
{
    ClientStatsManager::ClientStatsManager()
    {
    }

    void ClientStatsManager::SetTransport(ssdk::transport_common::ClientTransport* transport)
    {
        amf::AMFLock lock(&m_Guard);
        m_Transport = transport;
    }

    void ClientStatsManager::Init()
    {
        Clear();
        m_LastFrameTime = amf_high_precision_clock();
    }

    void ClientStatsManager::Clear()
    {
        m_FrameDurationHistory.Clear();
        m_FullLatencyHistory.Clear();
        m_ClientLatencyHistory.Clear();
        m_ServerLatencyHistory.Clear();
        m_EncoderLatencyHistory.Clear();
        m_DecoderLatencyHistory.Clear();

        m_DecryptHistory.Clear();
        m_AVDesyncHistory.Clear();
        m_DecoderQueueTimes.clear();

        m_LastFrameTime = 0;
        m_LastSendStatsTime = 0;
    }

    void ClientStatsManager::IncrementDecoderQueueDepth(amf_pts pts)
    {
        amf_pts now = amf_high_precision_clock();;
        m_DecoderQueueTimes[pts] = now;
    };

    void ClientStatsManager::DecrementDecoderQueueDepth(amf_pts pts)
    {
        amf_pts ptsEnd = amf_high_precision_clock();;

        DecoderQueueTimes::const_iterator element = m_DecoderQueueTimes.find(pts);
        if (element != m_DecoderQueueTimes.end())
        {
            amf_pts ptsStart = element->second;
            m_DecoderLatencyHistory.Add((float)(ptsEnd - ptsStart) / AMF_MILLISECOND);
            m_DecoderQueueTimes.erase(element);
        }
    };


    void ClientStatsManager::UpdateFullLatencyAndFramerate(amf_pts fullLatencyPts)
    {
        amf_pts now = amf_high_precision_clock();
        float frameDuration = (float)(now - m_LastFrameTime) / AMF_SECOND;
        m_LastFrameTime = now;
        m_FrameDurationHistory.Add(frameDuration);
        float fullLatency = (float)(fullLatencyPts) / AMF_MILLISECOND;
        m_FullLatencyHistory.Add(fullLatency);
    }

    void ClientStatsManager::UpdateClientLatency(amf_pts clientLatencyPts)
    {
        float clientLatency = (float)(clientLatencyPts) / AMF_MILLISECOND;
        m_ClientLatencyHistory.Add(clientLatency);
    }

    void ClientStatsManager::UpdateServerLatency(amf_pts serverLatencyPts)
    {
        float serverLatency = (float)(serverLatencyPts) / AMF_MILLISECOND;
        m_ServerLatencyHistory.Add(serverLatency);
    }

    void ClientStatsManager::UpdateEncoderLatency(amf_pts encoderLatencyPts)
    {
        float encoderLatency = (float)(encoderLatencyPts) / AMF_MILLISECOND;
        m_EncoderLatencyHistory.Add(encoderLatency);
    }

    void ClientStatsManager::UpdateAudioStatistics(amf_pts AVDesync)
    {
        m_AVDesyncHistory.Add((float)(AVDesync) / AMF_MILLISECOND );
    }

    void ClientStatsManager::UpdateDecryptStatistics(amf_pts decrypt)
    {
        m_DecryptHistory.Add((float)(decrypt) / AMF_MILLISECOND );
    }

    void ClientStatsManager::SendStatistics()
    {
        amf_pts now = amf_high_precision_clock();;

        if (now - m_LastSendStatsTime > STATISTICS_SEND_PERIOD_SECONDS * AMF_SECOND)
        {
            ssdk::transport_common::ClientTransport* transport = nullptr;
            {
                amf::AMFLock lock(&m_Guard);
                transport = m_Transport;
            }

            if (transport != nullptr)
            {
                float fullLatency = m_FullLatencyHistory.GetAverageAndClear();
                float clientLatency = m_ClientLatencyHistory.GetAverageAndClear();
                float serverLatency = m_ServerLatencyHistory.GetAverageAndClear();
                float encoderLatency = m_EncoderLatencyHistory.GetAverageAndClear();
                float networkLatency = fullLatency - clientLatency - serverLatency;
                float decoderLatency = m_DecoderLatencyHistory.GetAverageAndClear();
                float decrypt = m_DecryptHistory.GetAverageAndClear();
                int32_t decoderQueueSize = static_cast<int32_t>(m_DecoderQueueTimes.size());
                float AVDesync = m_AVDesyncHistory.GetAverageAndClear();
                float frameDurationAvg = m_FrameDurationHistory.GetAverageAndClear();
                float framerate = 0;
                if (frameDurationAvg != 0)
                {
                    framerate = 1 / frameDurationAvg;
                }

                StatsSnapshotImpl statsSnapshotImpl(fullLatency, clientLatency, serverLatency, encoderLatency, networkLatency, decoderLatency, decrypt, decoderQueueSize, AVDesync, framerate);
                transport->SendStats(ssdk::transport_common::DEFAULT_STREAM, statsSnapshotImpl);
            }
            else
            {
                AMFTraceError(AMF_FACILITY, L"SendStatistics - transport is not initialized");
            }

            m_LastSendStatsTime = now;
        }
    }
}