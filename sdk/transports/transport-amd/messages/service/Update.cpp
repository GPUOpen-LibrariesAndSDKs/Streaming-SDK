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

#include "Update.h"
#include "transports/transport-amd/Channels.h"


namespace ssdk::transport_amd
{
    static constexpr const char* TAG_UPDATE_BITRATE = "Bitrate";
    static constexpr const char* TAG_UPDATE_FRAMERATE = "FrameRate";
    static constexpr const char* TAG_UPDATE_RESOLUTION = "Resolution";

    static constexpr const char* TAG_STREAM_ID = "StreamID";

    UpdateRequest::UpdateRequest() :
        Message(uint8_t(SERVICE_OP_CODE::UPDATE))
    {
    }

    UpdateRequest::UpdateRequest(transport_common::StreamID streamID, float framerate) :
        Message(uint8_t(SERVICE_OP_CODE::UPDATE)),
        m_FrameRate(framerate),
        m_StreamID(streamID)
    {
        amf::JSONParser::Ptr parser;
        CreateJSONParser(&parser);
        amf::JSONParser::Node::Ptr root;
        parser->CreateNode(&root);

        SetFloatValue(parser, root, TAG_UPDATE_FRAMERATE, m_FrameRate);

        if (m_StreamID != transport_common::DEFAULT_STREAM)
        {
            SetInt64Value(parser, root, TAG_STREAM_ID, m_StreamID);
        }

        m_Data += root->Stringify();
    }

    UpdateRequest::UpdateRequest(transport_common::StreamID streamID, int64_t bitrate) :
        Message(uint8_t(SERVICE_OP_CODE::UPDATE)),
        m_Bitrate(bitrate),
        m_StreamID(streamID)
    {
        amf::JSONParser::Ptr parser;
        CreateJSONParser(&parser);
        amf::JSONParser::Node::Ptr root;
        parser->CreateNode(&root);

        SetInt64Value(parser, root, TAG_UPDATE_BITRATE, m_Bitrate);

        if (m_StreamID != transport_common::DEFAULT_STREAM)
        {
            SetInt64Value(parser, root, TAG_STREAM_ID, m_StreamID);
        }

        m_Data += root->Stringify();
    }

    UpdateRequest::UpdateRequest(transport_common::StreamID streamID, float framerate, const AMFSize& resolution, int64_t bitrate) :
        Message(uint8_t(SERVICE_OP_CODE::UPDATE)),
        m_FrameRate(framerate),
        m_Resolution(resolution),
        m_Bitrate(bitrate),
        m_StreamID(streamID)
    {
        amf::JSONParser::Ptr parser;
        CreateJSONParser(&parser);
        amf::JSONParser::Node::Ptr root;
        parser->CreateNode(&root);

        if (framerate != 0)
        {
            SetFloatValue(parser, root, TAG_UPDATE_FRAMERATE, framerate);
        }
        if (resolution.width > 0 && resolution.height > 0)
        {
            SetSizeValue(parser, root, TAG_UPDATE_RESOLUTION, resolution);
        }
        if (bitrate > 0)
        {
            SetInt64Value(parser, root, TAG_UPDATE_BITRATE, bitrate);
        }

        if (m_StreamID != transport_common::DEFAULT_STREAM)
        {
            SetInt64Value(parser, root, TAG_STREAM_ID, m_StreamID);
        }

        m_Data += root->Stringify();
    }

    bool UpdateRequest::FromJSON(amf::JSONParser::Node* root)
    {
        GetFloatValue(root, TAG_UPDATE_FRAMERATE, m_FrameRate);
        GetSizeValue(root, TAG_UPDATE_RESOLUTION, m_Resolution);
        GetInt64Value(root, TAG_UPDATE_BITRATE, m_Bitrate);

        if (GetInt64Value(root, TAG_STREAM_ID, m_StreamID) == false)
        {
            m_StreamID = transport_common::DEFAULT_STREAM;
        }

        return true;
    }

}
