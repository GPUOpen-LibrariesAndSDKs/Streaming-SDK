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

#include "decoders/AudioDecodeEngine.h"
#include "amf/public/include/core/Context.h"
#include "amf/public/common/Thread.h"

#include <deque>
#include <string>
#include <memory>


namespace ssdk::audio
{
    class AudioInput
    {
    public:
        typedef std::shared_ptr<AudioInput>   Ptr;

        class Consumer
        {
        public:
            virtual void OnAudioBuffer(amf::AMFAudioBuffer* buffer) = 0;
        };

        static constexpr const uint32_t DEFAULT_CHANNELS = 2;
        static constexpr const uint32_t DEFAULT_SAMPLING_RATE = 44100;
    public:
        AudioInput(amf::AMFContext* context, Consumer& sink);

        AMF_RESULT Init(const char* codecName, amf::AMF_AUDIO_FORMAT format, uint32_t channels, uint32_t layout, uint32_t samplingRate, amf::AMFBuffer* initBlock);
        AMF_RESULT Terminate();

        bool IsCodecSupported(const char* codecName) const;

        amf::AMF_AUDIO_FORMAT GetOutputFormat() const noexcept;

        uint32_t GetChannels() const noexcept;
        uint32_t GetLayout() const noexcept;
        uint32_t GetSamplingRate() const noexcept;

        AMF_RESULT SubmitInput(amf::AMFBuffer* input, bool discontinuity);

    private:
        static AMF_RESULT CreateDecoder(amf::AMFContext* context, const char* codecName, AudioDecodeEngine::Ptr& decoder);
        void PumpBuffers();

        typedef std::deque<amf::AMFBufferPtr>   InputQueue;

        class Pump : public amf::AMFThread
        {
        public:
            Pump(AudioInput& audioInput) : m_AudioInput(audioInput) {}

            virtual void Run() override;

        private:
            AudioInput& m_AudioInput;
        };


    protected:
        mutable amf::AMFCriticalSection m_Guard;
        amf::AMFContextPtr              m_Context;
        Consumer&                       m_Sink;
        AudioDecodeEngine::Ptr          m_Decoder;

        amf::AMF_AUDIO_FORMAT           m_Format = amf::AMF_AUDIO_FORMAT::AMFAF_UNKNOWN;
        uint32_t                        m_Channels = DEFAULT_CHANNELS; //  Stereo by default
        uint32_t                        m_Layout = amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_FRONT_LEFT | amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_BACK_RIGHT;
        uint32_t                        m_SamplingRate = DEFAULT_SAMPLING_RATE;

        mutable amf::AMFCriticalSection m_InputGuard;
        uint64_t                        m_InputBufCnt = 0;
        InputQueue                      m_InputQueue;

        mutable amf::AMFCriticalSection m_OutputGuard;
        uint64_t                        m_OutputBufCnt = 0;

        Pump                            m_Pump;
    };
}