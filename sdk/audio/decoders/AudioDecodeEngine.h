//
// Notice Regarding Standards.  AMD does not provide a license or sublicense to
// any Intellectual Property Rights relating to any standards, including but not
// limited to any audio and/or video codec technologies such as MPEG-2, MPEG-4;
// AVC/H.264; HEVC/H.265; AAC decode/FFMPEG; AAC encode/FFMPEG; VC-1; and MP3
// (collectively, the "Media Technologies"). For clarity, you will pay any
// royalties due for such third party technologies, which may include the Media
// Technologies that are owed as a result of AMD providing the Software to you.
//
// MIT license
//
//
// Copyright (c) 2018 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include "../AudioCodecs.h"
#include "amf/public/include/core/Context.h"
#include "amf/public/include/core/AudioBuffer.h"
#include "amf/public/include/components/Component.h"

#include <memory>

namespace ssdk::audio
{
    class AudioDecodeEngine
    {
    public:
        typedef std::shared_ptr<AudioDecodeEngine>  Ptr;

    protected:
        AudioDecodeEngine() = default;

    private:
        AudioDecodeEngine(const AudioDecodeEngine&) = delete;
        AudioDecodeEngine& operator=(const AudioDecodeEngine&) = delete;

    public:
        virtual ~AudioDecodeEngine() = default;

        virtual AMF_RESULT Init(amf::AMF_AUDIO_FORMAT format, int32_t channels, int32_t layout, int32_t samplingRate, amf::AMFBuffer* initBlock);
        virtual void Terminate() = 0;

        virtual AMF_RESULT SubmitInput(amf::AMFBuffer* input) = 0;
        virtual AMF_RESULT QueryOutput(amf::AMFAudioBuffer** outputBuffer) = 0;
        virtual AMF_RESULT Flush() = 0;
        virtual AMF_RESULT Drain() = 0;

        virtual const char* GetCodecName() const noexcept = 0;

        inline amf::AMF_AUDIO_FORMAT GetFormat() const noexcept { return m_Format; }
        inline int32_t GetChannels() const noexcept { return m_Channels; }
        inline int32_t GetLayout() const noexcept { return m_Layout; }
        inline int32_t GetSamplingRate() const noexcept { return m_SamplingRate; }

    protected:
        amf::AMF_AUDIO_FORMAT   m_Format = amf::AMF_AUDIO_FORMAT::AMFAF_UNKNOWN;
        int32_t                 m_Channels = 2; //  Stereo by default
        int32_t                 m_Layout = amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_FRONT_LEFT | amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_BACK_RIGHT;
        int32_t                 m_SamplingRate = 44100;
    };
}