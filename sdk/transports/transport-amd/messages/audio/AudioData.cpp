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

#include "AudioData.h"
#include "transports/transport-amd/Channels.h"

namespace ssdk::transport_amd
{
    static constexpr const char* TAG_AUDIO_TIMESTAMP = "PTS";
    static constexpr const char* TAG_AUDIO_DURATION = "Duration";
    static constexpr const char* TAG_AUDIO_PACKET_SIZE = "size";
    static constexpr const char* TAG_AUDIO_SEQUENCE_NUM = "idx";
    static constexpr const char* TAG_DISCONTINUITY = "discontinuity";
    static constexpr const char* TAG_STREAM_ID = "StreamID";

    AudioData::AudioData() :
        Message(uint8_t(AUDIO_OP_CODE::DATA))
    {
    }

    AudioData::AudioData(amf_pts pts, amf_pts duration, uint32_t size, int64_t sequenceNumber, bool discontinuity, transport_common::StreamID streamID) :
        Message(uint8_t(AUDIO_OP_CODE::DATA)),
        m_Pts(pts),
        m_Duration(duration),
        m_Size(size),
        m_SizePresent(true),
        m_bDiscontinuity(discontinuity),
        m_SequenceNumber(sequenceNumber),
        m_streamID(streamID)
    {
        amf::JSONParser::Ptr parser;
        CreateJSONParser(&parser);
        amf::JSONParser::Node::Ptr root;
        parser->CreateNode(&root);

        SetInt64Value(parser, root, TAG_AUDIO_TIMESTAMP, m_Pts);
        if (duration > 0)
        {
            SetInt64Value(parser, root, TAG_AUDIO_DURATION, duration);
        }
        SetUInt32Value(parser, root, TAG_AUDIO_PACKET_SIZE, m_Size);
        SetInt64Value(parser, root, TAG_AUDIO_SEQUENCE_NUM, m_SequenceNumber);

        if (m_bDiscontinuity == true)
        {
            SetBoolValue(parser, root, TAG_DISCONTINUITY, m_bDiscontinuity);
        }
        if (streamID != transport_common::DEFAULT_STREAM)
        {
            SetInt64Value(parser, root, TAG_STREAM_ID, m_streamID);
        }

        m_Data += root->Stringify();
    }

    bool AudioData::FromJSON(amf::JSONParser::Node* root)
    {
        GetInt64Value(root, TAG_AUDIO_TIMESTAMP, m_Pts);
        GetInt64Value(root, TAG_AUDIO_DURATION, m_Duration);
        GetInt64Value(root, TAG_AUDIO_SEQUENCE_NUM, m_SequenceNumber);
        m_SizePresent = GetUInt32Value(root, TAG_AUDIO_PACKET_SIZE, m_Size);
        GetBoolValue(root, TAG_DISCONTINUITY, m_bDiscontinuity);
        if (GetInt64Value(root, TAG_STREAM_ID, m_streamID) == false)
        {
            m_streamID = transport_common::DEFAULT_STREAM;
        }
        return true;
    }
}
