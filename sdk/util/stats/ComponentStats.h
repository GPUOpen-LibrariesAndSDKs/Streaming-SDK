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

#include "amf/public/common/Thread.h"

#include <map>

namespace ssdk::util
{
    class ComponentStats
    {
    public:
        struct BasicStatsSnapshot
        {
            amf_pts     snapshotTime;

            size_t      inputCnt;
            amf_pts     lastInputTime;
            float       avgInputRate;

            size_t      outputCnt;
            amf_pts     lastOutputTime;
            float       avgOutputRate;

            amf_pts     avgLatency;

            size_t      inTransitCnt;
        };
    public:
        ComponentStats() = default;

        void InputSubmitted(amf_pts pts);
        void OutputObtained(amf_pts pts);

        void Reset();

        void GetBasicStatsSnapshot(BasicStatsSnapshot& snapshot) const;
        amf_pts GetLastResetTime() const noexcept;

    protected:
        mutable amf::AMFCriticalSection     m_Guard;
        size_t      m_InputCnt = 0;
        amf_pts     m_LastInputTime = 0;
        amf_pts     m_AvgInputRateAccum = 0;
        amf_pts     m_FirstInputTime = 0;
        amf_pts     m_BestInputTime = 0;
        amf_pts     m_WorstInputTime = 0;

        size_t      m_OutputCnt = 0;
        amf_pts     m_LastOutputTime = 0;
        amf_pts     m_AvgOutputRateAccum = 0;
        amf_pts     m_FirstOutputTime = 0;
        amf_pts     m_BestOutputTime = 0;
        amf_pts     m_WorstOutputTime = 0;

        amf_pts     m_ResetTime = 0;

        struct Element
        {
            amf_pts     inputSubmittedTime;
            amf_pts     outputRetrievedTime;
        };
        typedef std::map<amf_pts, Element>  LatencyMap;
        LatencyMap      m_LatencyMap;
    };
}
