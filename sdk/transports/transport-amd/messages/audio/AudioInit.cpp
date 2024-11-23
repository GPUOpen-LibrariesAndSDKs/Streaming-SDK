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

#include "AudioInit.h"
#include "transports/transport-amd/Channels.h"

namespace ssdk::transport_amd
{
    static constexpr const char* TAG_SAMPLE_RATE = "SampleRate";
    static constexpr const char* TAG_CHANNELS = "Channels";
    static constexpr const char* TAG_LAYOUT = "ChannelLayout";
    static constexpr const char* TAG_FORMAT = "Format";
    static constexpr const char* TAG_CODEC_ID = "CodecID";
    static constexpr const char* TAG_AUDIOINIT_ID = "ID";
    static constexpr const char* TAG_STREAM_ID = "StreamID";

    static constexpr const char* DEFAULT_CODEC = "aac";

    AudioInit::AudioInit() :
        Message(uint8_t(AUDIO_OP_CODE::INIT))
    {
    }

    AudioInit::AudioInit(amf_pts id, const char* codecID, amf::AMF_AUDIO_FORMAT  format, int32_t sampleRate, int32_t channels, int32_t layout, transport_common::StreamID streamID) :
        Message(uint8_t(AUDIO_OP_CODE::INIT)),
        m_iSampleRate(sampleRate),
        m_iChannels(channels),
        m_iChannelLayout(layout),
        m_CodecID(codecID),
        m_iFormat(format),
        m_ID(id),
        m_StreamID(streamID)
    {
        amf::JSONParser::Ptr parser;
        CreateJSONParser(&parser);
        amf::JSONParser::Node::Ptr root;
        parser->CreateNode(&root);

        SetInt32Value(parser, root, TAG_SAMPLE_RATE, sampleRate);
        SetInt32Value(parser, root, TAG_CHANNELS, channels);
        SetInt32Value(parser, root, TAG_LAYOUT, layout);
        SetInt32Value(parser, root, TAG_FORMAT, format);
        SetStringValue(parser, root, TAG_CODEC_ID, codecID);
        SetInt64Value(parser, root, TAG_AUDIOINIT_ID, id);
        if (streamID != transport_common::DEFAULT_STREAM)
        {
            SetInt64Value(parser, root, TAG_STREAM_ID, m_StreamID);
        }

        m_Data += root->Stringify();
    }

    bool AudioInit::FromJSON(amf::JSONParser::Node* root)
    {
        GetInt32Value(root, TAG_SAMPLE_RATE, m_iSampleRate);
        GetInt32Value(root, TAG_CHANNELS, m_iChannels);
        GetInt32Value(root, TAG_LAYOUT, m_iChannelLayout);
        GetInt32Value(root, TAG_FORMAT, reinterpret_cast<int32_t&>(m_iFormat));
        GetInt64Value(root, TAG_AUDIOINIT_ID, m_ID);
        if (GetStringValue(root, TAG_CODEC_ID, m_CodecID) == false)
        {
            m_CodecID = DEFAULT_CODEC;
        }
        if (GetInt64Value(root, TAG_STREAM_ID, m_StreamID) == false)
        {
            m_StreamID = transport_common::DEFAULT_STREAM;
        }

        return true;
    }

    static constexpr const char* TAG_ACK = "Ack";

    AudioInitAck::AudioInitAck() :
        Message(uint8_t(AUDIO_OP_CODE::INIT_ACK))
    {
    }

    AudioInitAck::AudioInitAck(amf_pts id, bool ack, transport_common::StreamID streamID) :
        Message(uint8_t(AUDIO_OP_CODE::INIT_ACK)),
        m_ID(id),
        m_Ack(ack),
        m_StreamID(streamID)
    {
        amf::JSONParser::Ptr parser;
        CreateJSONParser(&parser);
        amf::JSONParser::Node::Ptr root;
        parser->CreateNode(&root);

        SetInt64Value(parser, root, TAG_AUDIOINIT_ID, m_ID);
        SetBoolValue(parser, root, TAG_ACK, m_Ack);
        if (streamID != transport_common::DEFAULT_STREAM)
        {
            SetInt64Value(parser, root, TAG_STREAM_ID, m_StreamID);
        }

        m_Data += root->Stringify();
    }

    bool AudioInitAck::FromJSON(amf::JSONParser::Node* root)
    {
        GetInt64Value(root, TAG_AUDIOINIT_ID, m_ID);
        GetBoolValue(root, TAG_ACK, m_Ack);
        if (GetInt64Value(root, TAG_STREAM_ID, m_StreamID) == false)
        {
            m_StreamID = transport_common::DEFAULT_STREAM;
        }

        return true;
    }

    AudioInitRequest::AudioInitRequest() :
        Message(uint8_t(AUDIO_OP_CODE::INIT_REQUEST))
    {
        amf::JSONParser::Ptr parser;
        CreateJSONParser(&parser);
        amf::JSONParser::Node::Ptr root;
        parser->CreateNode(&root);

        std::string jsonStr = root->Stringify();
        m_Data += jsonStr;
    }

    AudioInitRequest::AudioInitRequest(transport_common::StreamID streamID) :
        Message(uint8_t(AUDIO_OP_CODE::INIT_REQUEST)),
        m_StreamID(streamID)
    {
        amf::JSONParser::Ptr parser;
        CreateJSONParser(&parser);
        amf::JSONParser::Node::Ptr root;
        parser->CreateNode(&root);

        if (streamID != transport_common::DEFAULT_STREAM)
        {
            SetInt64Value(parser, root, TAG_STREAM_ID, m_StreamID);
        }
        m_Data += root->Stringify();
    }

    bool AudioInitRequest::FromJSON(amf::JSONParser::Node* root)
    {
        if (GetInt64Value(root, TAG_STREAM_ID, m_StreamID) == false)
        {
            m_StreamID = transport_common::DEFAULT_STREAM;
        }

        return true;
    }
}