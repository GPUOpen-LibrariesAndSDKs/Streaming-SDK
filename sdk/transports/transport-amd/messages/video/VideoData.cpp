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

#include "VideoData.h"
#include "transports/transport-amd/Channels.h"

namespace ssdk::transport_amd
{
    static constexpr const char* TAG_PTS = "pts";
    static constexpr const char* TAG_PTS_SENSOR = "ptsSensor";
    static constexpr const char* TAG_PTS_SERVER_LAT = "ptsServerLat";
    static constexpr const char* TAG_PTS_ENCODER_LAT = "ptsEncoderLat";
    static constexpr const char* TAG_COMP_FRAME_SIZE = "cmpFrmSize";
    static constexpr const char* TAG_COMP_FRAME_TYPE = "frmType";
    static constexpr const char* TAG_COMP_ENCODED_FRAME_TYPE = "encType";
    static constexpr const char* TAG_PTS_SEND_DURATION = "ptsSend";
    static constexpr const char* TAG_PTS_FRAME_NUM = "frameNum";
    static constexpr const char* TAG_DISCONTINUITY = "discontinuity";
    static constexpr const char* TAG_STREAM_ID = "StreamID";

    VideoData::VideoData() :
        Message(uint8_t(VIDEO_OP_CODE::DATA))
    {
    }

    VideoData::VideoData(amf_pts pts, amf_pts originPts, amf_pts ptsServerLatency, amf_pts ptsEncoderLatency,
                         uint32_t compressedFrameSize, transport_common::VideoFrame::ViewType eViewType, transport_common::VideoFrame::SubframeType eSubframeType,
                         amf_pts ptsLastSendDuration, amf_uint64 uiFrameNum, bool discontinuity, transport_common::StreamID streamID) :
        Message(uint8_t(VIDEO_OP_CODE::DATA)),
        m_originPts(originPts),
        m_ptsServerLatency(ptsServerLatency),
        m_pts(pts),
        m_CompressedFrameSize(compressedFrameSize),
        m_eViewType(eViewType),
        m_eSubframeType(eSubframeType),
        m_ptsEncoderLatency(ptsEncoderLatency),
        m_ptsLastSendDuration(ptsLastSendDuration),
        m_uiFrameNum(uiFrameNum),
        m_bDiscontinuity(discontinuity),
        m_streamID(streamID)
    {
        amf::JSONParser::Ptr parser;
        CreateJSONParser(&parser);
        amf::JSONParser::Node::Ptr root;
        parser->CreateNode(&root);

        SetInt64Value(parser, root, TAG_PTS, pts);
        SetInt64Value(parser, root, TAG_PTS_SENSOR, originPts);
        SetInt64Value(parser, root, TAG_PTS_SERVER_LAT, ptsServerLatency);
        SetInt64Value(parser, root, TAG_PTS_ENCODER_LAT, m_ptsEncoderLatency);
        SetUInt32Value(parser, root, TAG_COMP_FRAME_SIZE, compressedFrameSize);
        SetUInt32Value(parser, root, TAG_COMP_FRAME_TYPE, static_cast<uint32_t>(m_eViewType));
        SetUInt32Value(parser, root, TAG_COMP_ENCODED_FRAME_TYPE, static_cast<uint32_t>(m_eSubframeType));
        SetInt64Value(parser, root, TAG_PTS_SEND_DURATION, m_ptsLastSendDuration);
        SetInt64Value(parser, root, TAG_PTS_FRAME_NUM, m_uiFrameNum);
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

    bool VideoData::FromJSON(amf::JSONParser::Node* root)
    {
        bool result = true;

        GetInt64Value(root, TAG_PTS_SENSOR, m_originPts);
        GetInt64Value(root, TAG_PTS_SERVER_LAT, m_ptsServerLatency);
        GetInt64Value(root, TAG_PTS_ENCODER_LAT, m_ptsEncoderLatency);
        GetInt64Value(root, TAG_PTS, m_pts);
        GetUInt32Value(root, TAG_COMP_FRAME_SIZE, m_CompressedFrameSize);

        uint32_t viewType = 0;
        if (GetUInt32Value(root, TAG_COMP_FRAME_TYPE, viewType) == true)
        {
            if (viewType <= static_cast<uint32_t>(transport_common::VideoFrame::ViewType::MONOSCOPIC))
            {
                m_eViewType = static_cast<transport_common::VideoFrame::ViewType>(viewType);
            }
            else
            {
                result = false;
            }
        }

        int32_t subframeType = 0;
        if (GetInt32Value(root, TAG_COMP_ENCODED_FRAME_TYPE, subframeType) == true)
        {
            if (subframeType >= -1 && subframeType <= static_cast<int32_t>(transport_common::VideoFrame::SubframeType::TRANSPARENCY))
            {
                m_eSubframeType = static_cast<transport_common::VideoFrame::SubframeType>(subframeType);
            }
            else
            {
                result = false;
            }

        }
        GetInt64Value(root, TAG_PTS_SEND_DURATION, m_ptsLastSendDuration);
        GetUInt64Value(root, TAG_PTS_FRAME_NUM, m_uiFrameNum);
        GetBoolValue(root, TAG_DISCONTINUITY, m_bDiscontinuity);
        if (GetInt64Value(root, TAG_STREAM_ID, m_streamID) == false)
        {
            m_streamID = transport_common::DEFAULT_STREAM;
        }
        return result;
    }


    VideoForceUpdate::VideoForceUpdate() :
        Message(uint8_t(VIDEO_OP_CODE::FORCE_UPDATE)),
        m_videoStreamID(transport_common::DEFAULT_STREAM)
    {
    }


    VideoForceUpdate::VideoForceUpdate(transport_common::StreamID videoStreamID) :
        Message(uint8_t(VIDEO_OP_CODE::FORCE_UPDATE)),
        m_videoStreamID(videoStreamID)
    {
        if (videoStreamID != transport_common::DEFAULT_STREAM)
        {
            amf::JSONParser::Ptr parser;
            CreateJSONParser(&parser);
            amf::JSONParser::Node::Ptr root;
            parser->CreateNode(&root);

            SetInt64Value(parser, root, TAG_STREAM_ID, m_videoStreamID);

            m_Data += root->Stringify();
        }
    }

    bool VideoForceUpdate::FromJSON(amf::JSONParser::Node* root)
    {
        if (GetInt64Value(root, TAG_STREAM_ID, m_videoStreamID) == false)
        {
            m_videoStreamID = transport_common::DEFAULT_STREAM;
        }

        return true;
    }

}
