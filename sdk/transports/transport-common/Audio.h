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

#include "amf/public/include/core/Context.h"
#include "amf/public/include/core/Buffer.h"
#include "amf/public/include/core/AudioBuffer.h"
#include "amf/public/include/core/PropertyStorage.h"

#include <string>

namespace ssdk::transport_common
{
    class AudioBuffer
    {
    protected:
        AudioBuffer() = default;
        AudioBuffer(amf::AMFBuffer* buffer, int64_t sequenceNumber, bool discontinuity = false);

    public:
        AMF_RESULT GetBuffer(amf::AMFBuffer** buf);
        inline int64_t GetSequenceNumber() const noexcept { return m_SequenceNumber; }
        inline bool IsDiscontinuity() const noexcept { return m_Discontinuity; }

        amf_pts GetPts() const noexcept;
        amf_pts GetDuration() const noexcept;

    private:
        amf::AMFBufferPtr   m_Buf;
        bool                m_Discontinuity = false;
        int64_t             m_SequenceNumber = 0;
    };

    class TransmittableAudioBuffer : public AudioBuffer
    {
    public:
        TransmittableAudioBuffer(amf::AMFBuffer* buffer, int64_t sequenceNumber, bool discontinuity = false);
    };

    class ReceivableAudioBuffer : public AudioBuffer
    {
    public:
        ReceivableAudioBuffer() = default;
        ReceivableAudioBuffer(amf::AMFBuffer* buffer, int64_t sequenceNumber, bool discontinuity);
    };

    class AudioStreamDescriptor
    {
    public:
        AudioStreamDescriptor() = default;
        AudioStreamDescriptor(const char* description, const char* codec, int32_t channels, int32_t layout, int32_t bitrate,
            int32_t bitsPerSample, amf::AMF_AUDIO_FORMAT format, int32_t samplingRate, const char* language = nullptr);

        inline const std::string& GetDescription() const noexcept { return m_Description; }
        inline void SetDescription(const std::string& desc) noexcept { m_Description = desc; }

        inline uint32_t GetSourceID() const noexcept { return m_SourceID; }
        inline void SetSourceID(uint32_t id) noexcept { m_SourceID = id; }

        inline const std::string& GetCodec() const noexcept { return m_Codec; }
        inline void SetCodec(const std::string& codec) noexcept { m_Codec = codec; }

        inline const std::string& GetLanguage() const noexcept { return m_Language; }
        inline void SetLanguage(const std::string& language) noexcept { m_Language = language; }

        inline int32_t GetBitrate() const noexcept { return m_Bitrate; }
        inline void SetBitrate(int32_t rate) noexcept { m_Bitrate = rate; }

        inline int32_t GetNumOfChannels() const noexcept { return m_Channels; }
        inline void SetNumOfChannels(int32_t channels) noexcept { m_Channels = channels; }

        inline int32_t GetLayout() const noexcept { return m_Layout; }
        inline void SetLayout(int32_t layout) noexcept { m_Layout = layout; }

        inline int32_t GetSamplingRate() const noexcept { return m_SamplingRate; }
        inline void SetSamplingRate(int32_t rate) noexcept { m_SamplingRate = rate; }

        inline int32_t GetBitsPerSample() const noexcept { return m_BitsPerSample; }
        inline void SetBitsPerSample(int32_t bits) noexcept { m_BitsPerSample = bits; }

    protected:
        std::string             m_Description;
        uint32_t                m_SourceID = 0;
        std::string             m_Codec;
        int32_t                 m_Bitrate = 0;
        int32_t                 m_Channels = 0;
        int32_t                 m_Layout = int32_t(amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_FRONT_LEFT) | int32_t(amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_FRONT_RIGHT);
        int32_t                 m_SamplingRate = 44100;
        int32_t                 m_BitsPerSample = 16;
        amf::AMF_AUDIO_FORMAT   m_Format = amf::AMFAF_UNKNOWN;
        std::string             m_Language;
    };

}