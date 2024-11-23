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

#include "Codec.h"
#include <algorithm>
#include <cctype>

namespace ssdk::transport_amd
{
    //  Codec: base class
    static const char* CODEC_ID = "Codec";
    bool CodecSerializer::Compare(const ssdk::CodecDescriptor& first, const ssdk::CodecDescriptor& second) noexcept
    {
        //  Compare both names ignoring case
        return first.GetName().length() == second.GetName().length() &&
            std::equal(first.GetName().begin(), first.GetName().end(), second.GetName().begin(),
                       [](char a, char b) { return std::tolower(a) == std::tolower(b); });
    }

    amf::JSONParser::Result CodecSerializer::FromJSON(ssdk::CodecDescriptor& descriptor, const amf::JSONParser::Node* node)
    {
        amf::JSONParser::Result result = amf::JSONParser::Result::INVALID_ARG;
        if (node != nullptr)
        {
            amf::JSONParser::Value::Ptr name(static_cast<amf::JSONParser::Value*>(node->GetElementByName(CODEC_ID)));
            if (name != nullptr)
            {
                descriptor.SetName(name->GetValue());
                result = amf::JSONParser::Result::OK;
            }
        }
        return result;
    }

    amf::JSONParser::Result CodecSerializer::ToJSON(const ssdk::CodecDescriptor& descriptor, amf::JSONParser* parser, amf::JSONParser::Node** node)
    {
        amf::JSONParser::Result result = amf::JSONParser::Result::INVALID_ARG;
        if (parser != nullptr && node != nullptr)
        {
            if ((result = parser->CreateNode(node)) == amf::JSONParser::Result::OK)
            {
                amf::JSONParser::Value::Ptr name;
                if ((result = parser->CreateValue(&name)) == amf::JSONParser::Result::OK)
                {
                    name->SetValue(descriptor.GetName());
                    result = (*node)->AddElement(CODEC_ID, name);
                }
            }
        }
        return result;
    }


    //  Video codec descriptor
    static const char* AUDIO_CODEC_RESOLUTION = "Resolution";
    static const char* AUDIO_CODEC_FRAMERATE = "Framerate";

    VideoCodec::VideoCodec(const std::string& name, const AMFSize& resolution, const AMFRate& frameRate) :
        VideoCodecDescriptor(name, resolution, frameRate)
    {
    }

    amf::JSONParser::Result VideoCodec::FromJSON(const amf::JSONParser::Node* node)
    {
        amf::JSONParser::Result result = CodecSerializer::FromJSON(*this, node);
        if (result == amf::JSONParser::Result::OK)
        {
            amf::GetSizeValue(node, AUDIO_CODEC_RESOLUTION, m_Resolution);
            amf::GetRateValue(node, AUDIO_CODEC_FRAMERATE, m_FrameRate);
        }
        return result;
    }

    amf::JSONParser::Result VideoCodec::ToJSON(amf::JSONParser* parser, amf::JSONParser::Node** node)
    {
        amf::JSONParser::Result result = CodecSerializer::ToJSON(*this, parser, node);
        if (result == amf::JSONParser::Result::OK)
        {
            if (m_Resolution != AMFConstructSize(0, 0))
            {
                amf::SetSizeValue(parser, *node, AUDIO_CODEC_RESOLUTION, m_Resolution);
            }
            if (m_FrameRate.num != 0)
            {
                amf::SetRateValue(parser, *node, AUDIO_CODEC_FRAMERATE, m_FrameRate);
            }
        }
        return result;
    }

    static bool AreResolutionsCompatible(const AMFSize& res1, const AMFSize& res2)
    {
        return (res1.width == 0 || res2.width == 0 || res1.width == res2.width) &&
            (res1.height == 0 || res2.height == 0 || res1.height == res2.height);
    }

    static bool AreFrameRatesCompatible(const AMFRate& rate1, const AMFRate& rate2)
    {
        return (rate1.num == 0 || rate2.num == 0 || rate1.num == rate2.num) &&
            (rate1.den == 0 || rate2.den == 0 || rate1.den == rate2.den);
    }

    bool VideoCodec::IsCompatibleWith(const VideoCodec& other) const noexcept
    {
        return Compare(*this, other) &&
            AreResolutionsCompatible(m_Resolution, other.GetResolution()) &&
            AreFrameRatesCompatible(m_FrameRate, other.GetFrameRate());
    }


    // Audio codec descriptor
    static const char* AUDIO_CODEC_SAMPLE_RATE = "SampleRate";
    static const char* AUDIO_CODEC_CHANNELS = "Channels";
    static const char* AUDIO_CODEC_LAYOUT = "Layout";
    AudioCodec::AudioCodec(const std::string& name, uint32_t samplingRate, uint32_t channels, uint32_t layout) :
        AudioCodecDescriptor(name, samplingRate, channels, layout)
    {
    }

    amf::JSONParser::Result AudioCodec::FromJSON(const amf::JSONParser::Node* node)
    {
        amf::JSONParser::Result result = CodecSerializer::FromJSON(*this, node);
        if (result == amf::JSONParser::Result::OK)
        {
            amf::GetUInt32Value(node, AUDIO_CODEC_SAMPLE_RATE, m_SamplingRate);
            amf::GetUInt32Value(node, AUDIO_CODEC_CHANNELS, m_Channels);
            amf::GetUInt32Value(node, AUDIO_CODEC_LAYOUT, m_Layout);
        }
        return result;
    }

    amf::JSONParser::Result AudioCodec::ToJSON(amf::JSONParser* parser, amf::JSONParser::Node** node)
    {
        amf::JSONParser::Result result = CodecSerializer::ToJSON(*this, parser, node);
        if (result == amf::JSONParser::Result::OK)
        {
            if (m_SamplingRate != 0)
            {
                amf::SetUInt32Value(parser, *node, AUDIO_CODEC_SAMPLE_RATE, m_SamplingRate);
            }
            if (m_Channels != 0)
            {
                amf::SetUInt32Value(parser, *node, AUDIO_CODEC_CHANNELS, m_Channels);
            }
            if (m_Layout != 0)
            {
                amf::SetUInt32Value(parser, *node, AUDIO_CODEC_LAYOUT, m_Layout);
            }
        }
        return result;
    }

    bool AudioCodec::IsCompatibleWith(const AudioCodec& other) const noexcept
    {
        return Compare(*this, other) &&
            (m_SamplingRate == 0 || other.GetSamplingRate() == 0 || m_SamplingRate == other.GetSamplingRate()) &&
            (m_Channels == 0 || other.GetChannels() == 0 || m_Channels == other.GetChannels()) &&
            (m_Layout == 0 || other.GetLayout() == 0 || m_Layout == other.GetLayout());
    }
}
