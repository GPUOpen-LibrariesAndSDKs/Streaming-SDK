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
#include <string>

namespace ssdk::transport_amd
{
    class StartRequest : public Message
    {
    public:
        StartRequest();
        StartRequest(const std::string& displayModel, int32_t displayWidth, int32_t displayHeight, float framerate,
                    int64_t bitrate, float interpupillaryDistance, float aspectRatio, bool bSeparateEyeProcessing, const char* videoCodec,
                    bool bNonLinearScalingSupported, const char* audioCodec, int64_t audioChannels, int64_t audioChannelLayout, int32_t displayClientWidth,
                    int32_t displayClientHeight, transport_common::StreamID videoStreamID, transport_common::StreamID audioStreamID);

        virtual bool FromJSON(amf::JSONParser::Node* root) override;

        inline int32_t GetDisplayWidth() const noexcept { return m_DisplayWidth; }
        inline int32_t GetDisplayHeight() const noexcept { return m_DisplayHeight; }
        inline int64_t GetBitrate()  const noexcept { return m_Bitrate; }
        inline float GetFrameRate() const noexcept { return m_FrameRate; }
        inline float GetInterpupillaryDistance() const noexcept { return m_InterpupillaryDistance; }
        inline float GetAspectRatio() const noexcept { return m_AspectRatio; }
        inline const std::string& GetDisplayModel() const noexcept { return m_DisplayModel; }

        inline bool GetSeparateEyeProcessing() const noexcept { return m_bSeparateEyeProcessing; }

        inline const std::string& GetVideoCodec() const noexcept { return m_VideoCodec; }
        inline bool GetNonLinearScalingSupported() const noexcept { return m_bNonLinearScalingSupported; }

        inline int64_t GetAudioChannels() const noexcept { return m_AudioChannels; }
        inline int64_t GetAudioChannelLayout() const noexcept { return m_AudioChannelLayout; }
        inline const std::string& GetAudioCodec() const noexcept { return m_AudioCodec; }

        inline bool IsVideoInitAckRequired() const noexcept { return m_VideoAck; }
        inline bool IsAudioInitAckRequired() const noexcept { return m_AudioAck; }

        inline int32_t GetClientDisplayWidth() const noexcept { return m_ClientDisplayWidth; }
        inline int32_t GetClientDisplayHeight() const noexcept { return m_ClientDisplayHeight; }

        inline transport_common::StreamID GetVideoStreamID() const noexcept { return m_VideoStreamID; }
        inline transport_common::StreamID GetAudioStreamID() const noexcept { return m_AudioStreamID; }

    private:
        int32_t m_DisplayWidth = 0;
        int32_t m_DisplayHeight = 0;
        int64_t m_Bitrate = 0;
        float m_FrameRate = 0;
        float m_AspectRatio = 1;
        std::string m_DisplayModel;
        std::string m_VideoCodec;
        std::string m_AudioCodec;

        bool m_bSeparateEyeProcessing = true;
        float m_InterpupillaryDistance = 0.063f;
        bool m_bNonLinearScalingSupported = false;

        int64_t m_AudioChannels = 2;
        int64_t m_AudioChannelLayout = 3;

        bool    m_VideoAck = false;
        bool    m_AudioAck = false;

        int32_t m_ClientDisplayWidth = m_DisplayWidth;
        int32_t m_ClientDisplayHeight = m_DisplayHeight;

        transport_common::StreamID m_VideoStreamID = transport_common::DEFAULT_STREAM;
        transport_common::StreamID m_AudioStreamID = transport_common::DEFAULT_STREAM;
    };

    //---------------------------------------------------------------------------------------------
    class StopRequest : public Message
    {
    public:
        StopRequest();
        StopRequest(transport_common::StreamID videoStreamID, transport_common::StreamID audioStreamID);

        virtual bool FromJSON(amf::JSONParser::Node* root) override;
        inline transport_common::StreamID GetVideoStreamID() const noexcept { return m_VideoStreamID; }
        inline transport_common::StreamID GetAudioStreamID() const noexcept { return m_AudioStreamID; }

    private:
        transport_common::StreamID m_VideoStreamID = transport_common::DEFAULT_STREAM;
        transport_common::StreamID m_AudioStreamID = transport_common::DEFAULT_STREAM;
    };
}
