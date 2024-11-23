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

#include "ComponentStats.h"

namespace ssdk::util
{
    void ComponentStats::InputSubmitted(amf_pts pts)
    {
        amf_pts now = amf_high_precision_clock();
        amf::AMFLock    lock(&m_Guard);
        amf_pts delta = now - m_LastInputTime;
        m_AvgInputRateAccum += delta;
        if (m_BestInputTime == 0 || m_BestInputTime > delta)
        {
            m_BestInputTime = delta;
        }
        if (m_WorstInputTime == 0 || m_WorstInputTime < delta)
        {
            m_WorstInputTime = delta;
        }
        m_LastInputTime = now;
        if (m_FirstInputTime == 0)
        {
            m_FirstInputTime = now;
        }
        ++m_InputCnt;
        m_LatencyMap[pts] = Element({.inputSubmittedTime = now});
    }

    void ComponentStats::OutputObtained(amf_pts pts)
    {
        amf_pts now = amf_high_precision_clock();
        amf::AMFLock    lock(&m_Guard);
        amf_pts delta = now - m_LastOutputTime;
        m_AvgOutputRateAccum += delta;
        if (m_BestOutputTime == 0 || m_BestOutputTime > delta)
        {
            m_BestOutputTime = delta;
        }
        if (m_WorstOutputTime == 0 || m_WorstOutputTime < delta)
        {
            m_WorstOutputTime = delta;
        }
        m_LastOutputTime = now;
        if (m_FirstOutputTime == 0)
        {
            m_FirstOutputTime = now;
        }
        ++m_OutputCnt;
        LatencyMap::iterator element = m_LatencyMap.find(pts);
        if (element != m_LatencyMap.end())
        {
            element->second.outputRetrievedTime = now;
        }
        else
        {
            //  In a general case the number of outputs may not be the same as the number of inputs, as in case of FRC or on some audio codecs like AAC.
            //  In such cases timestamps on outputs might not match input timestamps, so we look for the closest input timestamp that is less than the
            //  output timestamp and assume that the output with a higher timestamp was produced from this input. This might not be a 100% accurate, but
            //  this is the best estimate of latency for such components
            LatencyMap::iterator prev = m_LatencyMap.begin();
            for (LatencyMap::iterator it = m_LatencyMap.begin(); it != m_LatencyMap.end(); ++it)
            {
                if (it->first < pts)
                {
                    prev = it;
                }
                else
                {
                    break;
                }
            }
            if (prev != m_LatencyMap.end())
            {
                prev->second.outputRetrievedTime = now;
            }
        }
    }

    void ComponentStats::Reset()
    {
        amf_pts now = amf_high_precision_clock();
        amf::AMFLock    lock(&m_Guard);
        m_InputCnt = 0;
        m_LastInputTime = 0;
        m_AvgInputRateAccum = 0;
        m_FirstInputTime = 0;
        m_BestInputTime = 0;
        m_WorstInputTime = 0;

        m_OutputCnt = 0;
        m_LastOutputTime = 0;
        m_AvgOutputRateAccum = 0;
        m_FirstOutputTime = 0;
        m_BestOutputTime = 0;
        m_WorstOutputTime = 0;

        m_LatencyMap.clear();

        m_ResetTime = now;
    }

    void ComponentStats::GetBasicStatsSnapshot(BasicStatsSnapshot& snapshot) const
    {
        amf::AMFLock    lock(&m_Guard);
        amf_pts now = amf_high_precision_clock();
        snapshot.snapshotTime = now;

        snapshot.inputCnt = m_InputCnt;
        snapshot.avgInputRate = float(m_InputCnt) / (float(now - m_FirstInputTime) / float(AMF_SECOND));
        snapshot.lastInputTime = m_LastInputTime;

        snapshot.outputCnt = m_OutputCnt;
        snapshot.avgOutputRate = float(m_OutputCnt) / (float(now - m_FirstInputTime) / float(AMF_SECOND));
        snapshot.lastOutputTime = m_LastOutputTime;

        snapshot.inTransitCnt = m_InputCnt - m_OutputCnt;

        amf_pts latency = 0;
        size_t count = 0;
        for (LatencyMap::const_iterator it = m_LatencyMap.begin(); it != m_LatencyMap.end(); ++it)
        {
            if (it->second.outputRetrievedTime != 0)
            {
                latency += it->second.outputRetrievedTime - it->second.inputSubmittedTime;
                ++count;
            }
        }
        if (count > 0)
        {
            latency /= count;
        }
        snapshot.avgLatency = latency;
    }

    amf_pts ComponentStats::GetLastResetTime() const noexcept
    {
        amf::AMFLock    lock(&m_Guard);
        return m_ResetTime;
    }
}
