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
#include "transports/transport-amd/Channels.h"

namespace ssdk::transport_amd
{
    static constexpr const char* TAG_OBSERVATION_PERIOD = "ObservationPeriod";
    static constexpr const char* TAG_SAMPLE_COUNT = "SampleCount";
    static constexpr const char* TAG_REQUIRED_LATENCY = "RequiredLatency";
    static constexpr const char* TAG_WORST_LATENCY = "WorstLatency";
    static constexpr const char* TAG_LATENCY = "ActualLatency";
    static constexpr const char* TAG_GLITCH_COUNT = "GlitchCount";
    static constexpr const char* TAG_AVG_GLITCH = "AvgGlitch";
    static constexpr const char* TAG_WORST_GLITCH = "WorstGlitch";
    static constexpr const char* TAG_STREAM_ID = "StreamID";


    QoSData::QoSData() :
        Message(uint8_t(VIDEO_OP_CODE::QOS)),
        m_ActualLatency(0LL),
        m_RequiredLatency(0LL)
    {
    }

    QoSData::QoSData(amf_pts observationPeriod, uint32_t sampleCount, amf_pts actualLatency, amf_pts worstLatency, amf_pts requiredLatency,
                     uint32_t glitchCount, amf_pts avgGlitch, amf_pts worstGlitch, transport_common::StreamID streamID) :
        Message(uint8_t(VIDEO_OP_CODE::QOS)),
        m_ObservationPeriod(observationPeriod),
        m_SampleCount(sampleCount),
        m_ActualLatency(actualLatency),
        m_WorstLatency(worstLatency),
        m_RequiredLatency(requiredLatency),
        m_GlitchCount(glitchCount),
        m_WorstGlitch(worstGlitch),
        m_AvgGlitch(avgGlitch),
        m_streamID(streamID)
    {
        amf::JSONParser::Ptr parser;
        CreateJSONParser(&parser);
        amf::JSONParser::Node::Ptr root;
        parser->CreateNode(&root);

        SetInt64Value(parser, root, TAG_OBSERVATION_PERIOD, observationPeriod);
        SetInt64Value(parser, root, TAG_SAMPLE_COUNT, sampleCount);
        SetInt64Value(parser, root, TAG_REQUIRED_LATENCY, requiredLatency);
        SetInt64Value(parser, root, TAG_WORST_LATENCY, worstLatency);
        SetInt64Value(parser, root, TAG_LATENCY, actualLatency);
        SetInt64Value(parser, root, TAG_GLITCH_COUNT, glitchCount);
        SetInt64Value(parser, root, TAG_AVG_GLITCH, avgGlitch);
        SetInt64Value(parser, root, TAG_WORST_GLITCH, worstGlitch);
        if (streamID != transport_common::DEFAULT_STREAM)
        {
            SetInt64Value(parser, root, TAG_STREAM_ID, m_streamID);
        }

        m_Data += root->Stringify();
    }

    bool QoSData::FromJSON(amf::JSONParser::Node* root)
    {
        GetUInt32Value(root, TAG_SAMPLE_COUNT, m_SampleCount);
        GetInt64Value(root, TAG_OBSERVATION_PERIOD, m_ObservationPeriod);
        GetInt64Value(root, TAG_LATENCY, m_ActualLatency);
        GetInt64Value(root, TAG_WORST_LATENCY, m_WorstLatency);
        GetInt64Value(root, TAG_REQUIRED_LATENCY, m_RequiredLatency);
        GetUInt32Value(root, TAG_GLITCH_COUNT, m_GlitchCount);
        GetInt64Value(root, TAG_WORST_GLITCH, m_WorstGlitch);
        GetInt64Value(root, TAG_AVG_GLITCH, m_AvgGlitch);
        if (GetInt64Value(root, TAG_STREAM_ID, m_streamID) == false)
        {
            m_streamID = transport_common::DEFAULT_STREAM;
        }
        return true;
    }


}