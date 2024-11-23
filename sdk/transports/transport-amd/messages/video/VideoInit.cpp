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

#include "VideoInit.h"
#include "transports/transport-amd/Channels.h"

namespace ssdk::transport_amd
{
    static constexpr const char* TAG_VIDEOINIT_WIDTH = "Width";
    static constexpr const char* TAG_VIDEOINIT_HEIGHT = "Height";
    static constexpr const char* TAG_VIDEOINIT_VIEWPORT = "Viewport";
    static constexpr const char* TAG_VIDEOINIT_CODECID = "CodecID";
    static constexpr const char* TAG_VIDEOINIT_NLSCALING = "NonLinearScaling";
    static constexpr const char* TAG_VIDEOINIT_BITDEPTH = "BitDepth";
    static constexpr const char* TAG_VIDEOINIT_ID = "ID";
    static constexpr const char* TAG_STREAM_ID = "StreamID";


    VideoInit::VideoInit() :
        Message(uint8_t(VIDEO_OP_CODE::INIT))
    {
    }

    VideoInit::VideoInit(amf_pts id, const char* codecID, int32_t width, int32_t height, AMFRect viewport, bool bNonLinearScaling, int32_t bitDepth, transport_common::StreamID streamID) :
        Message(uint8_t(VIDEO_OP_CODE::INIT)),
        m_ID(id),
        m_CodecID(codecID),
        m_iWidth(width),
        m_iHeight(height),
        m_Viewport(viewport),
        m_bNonLinearScaling(bNonLinearScaling),
        m_iBitDepth(bitDepth),
        m_streamID(streamID)
    {
        amf::JSONParser::Ptr parser;
        CreateJSONParser(&parser);
        amf::JSONParser::Node::Ptr root;
        parser->CreateNode(&root);

        SetInt32Value(parser, root, TAG_VIDEOINIT_WIDTH, m_iWidth);
        SetInt32Value(parser, root, TAG_VIDEOINIT_HEIGHT, m_iHeight);
        SetRectValue(parser, root, TAG_VIDEOINIT_VIEWPORT, m_Viewport);
        SetStringValue(parser, root, TAG_VIDEOINIT_CODECID, m_CodecID.c_str());
        SetBoolValue(parser, root, TAG_VIDEOINIT_NLSCALING, m_bNonLinearScaling);
        SetInt32Value(parser, root, TAG_VIDEOINIT_BITDEPTH, m_iBitDepth);
        SetInt64Value(parser, root, TAG_VIDEOINIT_ID, m_ID);
        if (streamID != transport_common::DEFAULT_STREAM)
        {
            SetInt64Value(parser, root, TAG_STREAM_ID, m_streamID);
        }

        m_Data += root->Stringify();
    }

    bool VideoInit::FromJSON(amf::JSONParser::Node* root)
    {
        GetInt32Value(root, TAG_VIDEOINIT_WIDTH, m_iWidth);
        GetInt32Value(root, TAG_VIDEOINIT_HEIGHT, m_iHeight);

        if (!GetRectValue(root, TAG_VIDEOINIT_VIEWPORT, m_Viewport))
        {
            m_Viewport = AMFConstructRect(0, 0, m_iWidth, m_iHeight);
        }

        GetStringValue(root, TAG_VIDEOINIT_CODECID, m_CodecID);
        GetBoolValue(root, TAG_VIDEOINIT_NLSCALING, m_bNonLinearScaling);
        GetInt32Value(root, TAG_VIDEOINIT_BITDEPTH, m_iBitDepth);
        GetInt64Value(root, TAG_VIDEOINIT_ID, m_ID);

        if (GetInt64Value(root, TAG_STREAM_ID, m_streamID) == false)
        {
            m_streamID = transport_common::DEFAULT_STREAM;
        }

        return true;
    }

    static constexpr const char* TAG_ACK = "Ack";

    VideoInitAck::VideoInitAck() :
        Message(uint8_t(VIDEO_OP_CODE::INIT_ACK)) { }

    VideoInitAck::VideoInitAck(amf_pts id, bool ack, transport_common::StreamID streamID) :
        Message(uint8_t(VIDEO_OP_CODE::INIT_ACK)),
        m_ID(id),
        m_Ack(ack),
        m_streamID(streamID)
    {
        amf::JSONParser::Ptr parser;
        CreateJSONParser(&parser);
        amf::JSONParser::Node::Ptr root;
        parser->CreateNode(&root);

        SetInt64Value(parser, root, TAG_VIDEOINIT_ID, m_ID);
        SetBoolValue(parser, root, TAG_ACK, m_Ack);

        if (streamID != transport_common::DEFAULT_STREAM)
        {
            SetInt64Value(parser, root, TAG_STREAM_ID, m_streamID);
        }

        m_Data += root->Stringify();
    }

    bool VideoInitAck::FromJSON(amf::JSONParser::Node* root)
    {
        GetInt64Value(root, TAG_VIDEOINIT_ID, m_ID);
        GetBoolValue(root, TAG_ACK, m_Ack);
        if (GetInt64Value(root, TAG_STREAM_ID, m_streamID) == false)
        {
            m_streamID = transport_common::DEFAULT_STREAM;
        }

        return true;
    }

    VideoInitRequest::VideoInitRequest() :
        Message(uint8_t(VIDEO_OP_CODE::INIT_REQUEST))
    {
        amf::JSONParser::Ptr parser;
        CreateJSONParser(&parser);
        amf::JSONParser::Node::Ptr root;
        parser->CreateNode(&root);

        std::string jsonStr = root->Stringify();
        m_Data += jsonStr;
    }

    VideoInitRequest::VideoInitRequest(transport_common::StreamID streamID) :
        Message(uint8_t(VIDEO_OP_CODE::INIT_REQUEST)),
        m_streamID(streamID)
    {
        amf::JSONParser::Ptr parser;
        CreateJSONParser(&parser);
        amf::JSONParser::Node::Ptr root;
        parser->CreateNode(&root);

        if (streamID != transport_common::DEFAULT_STREAM)
        {
            SetInt64Value(parser, root, TAG_STREAM_ID, m_streamID);
        }
        m_Data += root->Stringify();
    }

    bool VideoInitRequest::FromJSON(amf::JSONParser::Node* root)
    {
        if (GetInt64Value(root, TAG_STREAM_ID, m_streamID) == false)
        {
            m_streamID = transport_common::DEFAULT_STREAM;
        }

        return true;
    }

}