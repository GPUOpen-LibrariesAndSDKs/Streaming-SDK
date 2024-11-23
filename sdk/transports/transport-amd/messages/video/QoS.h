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

#include "transports/transport-amd/messages/Message.h"
#include "transports/transport-common/Transport.h"

namespace ssdk::transport_amd
{
    class QoSData : public Message
    {
    public:
        QoSData();
        QoSData(amf_pts observationPeriod, uint32_t sampleCount, amf_pts actualLatency, amf_pts worstLatency, amf_pts requiredLatency,
                uint32_t glitchCount,amf_pts avgGlitch,amf_pts worstGlitch, transport_common::StreamID streamID = transport_common::DEFAULT_STREAM);

        virtual bool FromJSON(amf::JSONParser::Node* root) override;

        inline amf_pts GetObservationPeriod() const noexcept { return m_ObservationPeriod; }
        inline amf_pts GetActualLatency() const noexcept { return m_ActualLatency; }
        inline amf_pts GetWorstLatency() const noexcept { return m_WorstLatency; }
        inline amf_pts GetRequiredLatency() const noexcept { return m_RequiredLatency; }
        inline amf_pts GetWorstGlitch() const noexcept { return m_WorstGlitch; }
        inline amf_pts GetAvgGlitch() const noexcept { return m_AvgGlitch; }
        inline uint32_t GetGlitchCount() const noexcept { return m_GlitchCount; }
        inline uint32_t GetSampleCount() const noexcept { return m_SampleCount; }
        inline transport_common::StreamID GetStreamID() const noexcept { return m_streamID; }

    private:
        amf_pts m_ObservationPeriod = 0;
        amf_pts m_ActualLatency = 0;
        amf_pts m_WorstLatency = 0;
        amf_pts m_RequiredLatency = 0;
        amf_pts m_WorstGlitch = 0;
        amf_pts m_AvgGlitch = 0;
        uint32_t m_SampleCount = 0;
        uint32_t m_GlitchCount = 0;
        transport_common::StreamID m_streamID = transport_common::DEFAULT_STREAM;
    };
}
