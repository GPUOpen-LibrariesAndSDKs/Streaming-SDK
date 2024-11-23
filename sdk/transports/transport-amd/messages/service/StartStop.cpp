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

#include "StartStop.h"
#include "transports/transport-amd/Channels.h"

namespace ssdk::transport_amd
{
    static constexpr const char* TAG_START_DISPLAY_MODEL = "DisplayModel";
    static constexpr const char* TAG_START_DISPLAY_WIDTH = "DisplayWidth";
    static constexpr const char* TAG_START_DISPLAY_HEIGHT = "DisplayHeight";
    static constexpr const char* TAG_START_CLIENT_DISPLAY_WIDTH = "ClientDisplayWidth";
    static constexpr const char* TAG_START_CLIENT_DISPLAY_HEIGHT = "ClientDisplayHeight";

    static constexpr const char* TAG_START_VIDEO_BITRATE = "Bitrate";
    static constexpr const char* TAG_START_VIDEO_FRAMERATE = "FrameRate";
    static constexpr const char* TAG_START_VIDEO_IPD = "InterpupillaryDistance";
    static constexpr const char* TAG_START_VIDEO_ASPECT = "AspectRatio";
    static constexpr const char* TAG_START_VIDEO_SEP_EYES = "SeparateEyeProcessing";
    static constexpr const char* TAG_START_VIDEO_CODEC = "VideoCodec";
    static constexpr const char* TAG_START_VIDEO_STREAM_ID = "VideoStreamID";
    static constexpr const char* TAG_START_VIDEO_INIT_ACK = "VideoInitAck";
    static constexpr const char* TAG_START_NON_LINEAR_SCALE = "NonLinearScalingSupported";

    static constexpr const char* TAG_START_AUDIO_CODEC = "AudioCodec";
    static constexpr const char* TAG_START_AUDIO_CHANNELS = "AudioChannels";
    static constexpr const char* TAG_START_AUDIO_LAYOUT = "AudioChannelLayout";
    static constexpr const char* TAG_START_AUDIO_STREAM_ID = "AudioStreamID";
    static constexpr const char* TAG_START_AUDIO_INIT_ACK = "AudioInitAck";



    StartRequest::StartRequest() : Message(uint8_t(SERVICE_OP_CODE::START))
    {
    }

    StartRequest::StartRequest(const std::string& displayModel, int32_t displayWidth, int32_t displayHeight, float framerate,
        int64_t bitrate, float interpupillaryDistance, float aspectRatio, bool bSeparateEyeProcessing, const char* videoCodec,
        bool bNonLinearScalingSupported, const char* audioCodec, int64_t audioChannels, int64_t audioChannelLayout, int32_t displayClientWidth,
        int32_t displayClientHeight, transport_common::StreamID videoStreamID, transport_common::StreamID audioStreamID) :
        Message(uint8_t(SERVICE_OP_CODE::START)),
        m_DisplayWidth(displayWidth),
        m_DisplayHeight(displayHeight),
        m_Bitrate(bitrate),
        m_FrameRate(framerate),
        m_AspectRatio(aspectRatio),
        m_DisplayModel(displayModel),
        m_bSeparateEyeProcessing(bSeparateEyeProcessing),
        m_InterpupillaryDistance(interpupillaryDistance),
        m_bNonLinearScalingSupported(bNonLinearScalingSupported),
        m_AudioChannels(audioChannels),
        m_AudioChannelLayout(audioChannelLayout),
        m_ClientDisplayWidth(displayClientWidth),
        m_ClientDisplayHeight(displayClientHeight),
        m_VideoStreamID(videoStreamID),
        m_AudioStreamID(audioStreamID)
    {
        if (videoCodec != nullptr)
        {
            m_VideoCodec = videoCodec;
        }
        if (audioCodec != nullptr)
        {
            m_AudioCodec = audioCodec;
        }

        amf::JSONParser::Ptr parser;
        CreateJSONParser(&parser);
        amf::JSONParser::Node::Ptr root;
        parser->CreateNode(&root);

        //  Video parameters
        if (m_DisplayModel.length() > 0)
        {
            SetStringValue(parser, root, TAG_START_DISPLAY_MODEL, m_DisplayModel.c_str());
        }
        SetInt32Value(parser, root, TAG_START_DISPLAY_WIDTH, m_DisplayWidth);
        SetInt32Value(parser, root, TAG_START_DISPLAY_HEIGHT, m_DisplayHeight);
        SetInt64Value(parser, root, TAG_START_VIDEO_BITRATE, m_Bitrate);
        SetFloatValue(parser, root, TAG_START_VIDEO_FRAMERATE, m_FrameRate);
        SetFloatValue(parser, root, TAG_START_VIDEO_ASPECT, m_AspectRatio);
        SetFloatValue(parser, root, TAG_START_VIDEO_IPD, m_InterpupillaryDistance);
        SetBoolValue(parser, root, TAG_START_VIDEO_SEP_EYES, m_bSeparateEyeProcessing);
        SetBoolValue(parser, root, TAG_START_NON_LINEAR_SCALE, m_bNonLinearScalingSupported);
        SetBoolValue(parser, root, TAG_START_VIDEO_INIT_ACK, true);
        if (m_VideoCodec.length() > 0)
        {
            SetStringValue(parser, root, TAG_START_VIDEO_CODEC, m_VideoCodec.c_str());
        }

        if (m_ClientDisplayWidth != 0)
        {
            SetInt32Value(parser, root, TAG_START_CLIENT_DISPLAY_WIDTH, m_ClientDisplayWidth);
        }
        if (m_ClientDisplayHeight != 0)
        {
            SetInt32Value(parser, root, TAG_START_CLIENT_DISPLAY_HEIGHT, m_ClientDisplayHeight);
        }

        //  Audio parameters
        SetInt64Value(parser, root, TAG_START_AUDIO_CHANNELS, m_AudioChannels);
        SetInt64Value(parser, root, TAG_START_AUDIO_LAYOUT, m_AudioChannelLayout);
        SetBoolValue(parser, root, TAG_START_AUDIO_INIT_ACK, false);
        if (m_AudioCodec.length() > 0)
        {
            SetStringValue(parser, root, TAG_START_AUDIO_CODEC, m_AudioCodec.c_str());
        }

        if (videoStreamID != transport_common::DEFAULT_STREAM)
        {
            SetInt64Value(parser, root, TAG_START_VIDEO_STREAM_ID, m_VideoStreamID);
        }

        if (audioStreamID != transport_common::DEFAULT_STREAM)
        {
            SetInt64Value(parser, root, TAG_START_AUDIO_STREAM_ID, m_AudioStreamID);
        }


        std::string jsonStr = root->Stringify();
        m_Data += jsonStr;
    }

    bool StartRequest::FromJSON(amf::JSONParser::Node* root)
    {
        GetStringValue(root, TAG_START_DISPLAY_MODEL, m_DisplayModel);

        GetInt32Value(root, TAG_START_DISPLAY_WIDTH, m_DisplayWidth);
        GetInt32Value(root, TAG_START_DISPLAY_HEIGHT, m_DisplayHeight);
        GetFloatValue(root, TAG_START_VIDEO_ASPECT, m_AspectRatio);

        GetInt64Value(root, TAG_START_VIDEO_BITRATE, m_Bitrate);
        GetFloatValue(root, TAG_START_VIDEO_FRAMERATE, m_FrameRate);
        GetStringValue(root, TAG_START_VIDEO_CODEC, m_VideoCodec);

        GetFloatValue(root, TAG_START_VIDEO_IPD, m_InterpupillaryDistance);
        GetBoolValue(root, TAG_START_VIDEO_SEP_EYES, m_bSeparateEyeProcessing);
        GetBoolValue(root, TAG_START_NON_LINEAR_SCALE, m_bNonLinearScalingSupported);

        GetInt64Value(root, TAG_START_AUDIO_CHANNELS, m_AudioChannels);
        GetInt64Value(root, TAG_START_AUDIO_LAYOUT, m_AudioChannelLayout);
        GetStringValue(root, TAG_START_AUDIO_CODEC, m_AudioCodec);

        GetBoolValue(root, TAG_START_VIDEO_INIT_ACK, m_VideoAck);
        GetBoolValue(root, TAG_START_AUDIO_INIT_ACK, m_AudioAck);

        if (GetInt32Value(root, TAG_START_CLIENT_DISPLAY_WIDTH, m_ClientDisplayWidth) == false)
        {
            m_ClientDisplayWidth = m_DisplayWidth;
        }
        if (GetInt32Value(root, TAG_START_CLIENT_DISPLAY_HEIGHT, m_ClientDisplayHeight) == false)
        {
            m_ClientDisplayHeight = m_DisplayHeight;
        }
        if (GetInt64Value(root, TAG_START_VIDEO_STREAM_ID, m_VideoStreamID) == false)
        {
            m_VideoStreamID = transport_common::DEFAULT_STREAM;
        }
        if (GetInt64Value(root, TAG_START_AUDIO_STREAM_ID, m_AudioStreamID) == false)
        {
            m_AudioStreamID = transport_common::DEFAULT_STREAM;
        }


        return true;
    }

    //---------------------------------------------------------------------------------------------
    StopRequest::StopRequest() : Message(uint8_t(SERVICE_OP_CODE::STOP))
    {
    }

    StopRequest::StopRequest(transport_common::StreamID videoStreamID, transport_common::StreamID audioStreamID) :
        Message(uint8_t(SERVICE_OP_CODE::STOP)),
        m_VideoStreamID(videoStreamID),
        m_AudioStreamID(audioStreamID)
    {
        amf::JSONParser::Ptr parser;
        CreateJSONParser(&parser);
        amf::JSONParser::Node::Ptr root;
        parser->CreateNode(&root);

        if (videoStreamID != transport_common::DEFAULT_STREAM)
        {
            SetInt64Value(parser, root, TAG_START_VIDEO_STREAM_ID, m_VideoStreamID);
        }

        if (audioStreamID != transport_common::DEFAULT_STREAM)
        {
            SetInt64Value(parser, root, TAG_START_AUDIO_STREAM_ID, m_AudioStreamID);
        }

        std::string jsonStr = root->Stringify();
        m_Data += jsonStr;
    }

    bool StopRequest::FromJSON(amf::JSONParser::Node* root)
    {
        if (GetInt64Value(root, TAG_START_VIDEO_STREAM_ID, m_VideoStreamID) == false)
        {
            m_VideoStreamID = transport_common::DEFAULT_STREAM;
        }
        if (GetInt64Value(root, TAG_START_AUDIO_STREAM_ID, m_AudioStreamID) == false)
        {
            m_AudioStreamID = transport_common::DEFAULT_STREAM;
        }

        return true;
    }


}
