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

#include "amf/public/include/core/Platform.h"
#include <string>

namespace ssdk
{
    class CodecDescriptor
    {
    protected:
        CodecDescriptor() = default;
        CodecDescriptor(const std::string& name) : m_Name(name) {}

    public:
        inline const std::string& GetName() const noexcept { return m_Name; }
        inline void SetName(const std::string& name) noexcept { m_Name = name; }

    protected:
        std::string m_Name;
    };

    class VideoCodecDescriptor : public CodecDescriptor
    {
    public:
        VideoCodecDescriptor() = default;
        VideoCodecDescriptor(const std::string& name, const AMFSize& resolution, const AMFRate& frameRate) :
            CodecDescriptor(name), m_Resolution(resolution), m_FrameRate(frameRate) {}

        inline const AMFSize& GetResolution() const noexcept { return m_Resolution; }
        inline void SetResolution(const AMFSize& resolution) noexcept { m_Resolution = resolution; }

        inline const AMFRate& GetFrameRate() const noexcept { return m_FrameRate; }
        inline void SetFrameRate(const AMFRate& framerate) noexcept { m_FrameRate = framerate; }

    protected:
        AMFSize m_Resolution = AMFConstructSize(0, 0);
        AMFRate m_FrameRate = AMFConstructRate(0, 1);
    };

    class AudioCodecDescriptor : public CodecDescriptor
    {
    public:
        AudioCodecDescriptor() = default;
        AudioCodecDescriptor(const std::string& name, uint32_t samplingRate, uint32_t channels, uint32_t layout) :
            CodecDescriptor(name), m_SamplingRate(samplingRate), m_Channels(channels), m_Layout(layout) {}

        inline uint32_t GetSamplingRate() const noexcept { return m_SamplingRate; }
        inline void SetSamplingRate(uint32_t samplingRate) noexcept { m_SamplingRate = samplingRate; }

        inline uint32_t GetChannels() const noexcept { return m_Channels; }
        inline void SetChannels(uint32_t channels) noexcept { m_Channels = channels; }

        inline uint32_t GetLayout() const noexcept { return m_Layout; }
        inline void SetLayout(uint32_t layout) noexcept { m_Layout = layout; }

    protected:
        uint32_t m_SamplingRate = 44100;
        uint32_t m_Channels = 2;    //  Stereo
        uint32_t m_Layout = 3;      //  Left+Right
    };

}
