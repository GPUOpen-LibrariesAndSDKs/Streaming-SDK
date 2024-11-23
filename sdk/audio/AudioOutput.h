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

#include "encoders/AudioEncodeEngine.h"
#include "AudioTransmitterAdapter.h"

#include "amf/public/include/components/Component.h"
#include "amf/public/common/Thread.h"

#include <memory>
#include <set>

namespace ssdk::audio
{
    class AudioOutput
    {
    public:
        typedef std::shared_ptr<AudioOutput>    Ptr;

        constexpr static int32_t DEFAULT_SAMPLING_RATE = 44100;
        constexpr static int32_t DEFAULT_CHANNELS = 2;
        constexpr static int32_t DEFAULT_LAYOUT = amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_FRONT_LEFT | amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_FRONT_RIGHT;

    public:
        AudioOutput(TransmitterAdapter& transportAdapter, AudioEncodeEngine::Ptr encoder, amf::AMFContext* context);
        ~AudioOutput();

        AMF_RESULT Init(amf::AMF_AUDIO_FORMAT inputFormat, int32_t inputSamplingRate, int32_t inputChannels, int32_t inputLayout,
                        int32_t outputSamplingRate, int32_t outputChannels, int32_t outputLayout, int32_t bitrate);

        AMF_RESULT Terminate();

        AMF_RESULT SubmitInput(amf::AMFAudioBuffer* input);

        AMF_RESULT SetBitrate(int32_t bitrate);

    protected:
        AMF_RESULT InitializeEncoder(int32_t outputSamplingRate, int32_t outputChannels, int32_t outputLayout, int32_t bitrate);
        AMF_RESULT InitializeConverter(amf::AMF_AUDIO_FORMAT inputFormat, int32_t inputSamplingRate, int32_t inputChannels, int32_t inputLayout,
                                       amf::AMF_AUDIO_FORMAT outputFormat, int32_t outputSamplingRate, int32_t outputChannels, int32_t outputLayout);

        void SaveExtraData(amf::AMFBuffer* buffer);
        bool GetExtraData(amf::AMFBuffer** buffer, amf_pts& id);

        AMF_RESULT ConvertBuffer(amf::AMFAudioBuffer* inBuffer, amf::AMFAudioBuffer** outBuffer);
        AMF_RESULT EncodeAndSendBuffer(amf::AMFAudioBuffer* inBuffer);

        transport_common::Result SendBufferToAllClients(amf::AMFBuffer* inBuffer);

        typedef amf::AMFQueue<amf::AMFDataPtr> InputQueue;
        class Pump :
            public amf::AMFQueueThread<amf::AMFDataPtr, int>
        {
        public:
            typedef std::unique_ptr<Pump>  Ptr;

        public:
            Pump(AudioOutput& pipeline, InputQueue& inputQueue);
            virtual bool Process(amf_ulong& ulID, amf::AMFDataPtr& inData, int&);

        private:
            AudioOutput& m_Pipeline;
#if !defined (__APPLE__)
            amf_pts m_LastBufferPts = 0;
#endif
        };

    protected:
        mutable amf::AMFCriticalSection m_Guard;

        bool                        m_Initialized = false;

        TransmitterAdapter&      m_TransportAdapter;

        amf::AMFContextPtr          m_Context;
        amf::AMFComponentPtr        m_Converter;
        AudioEncodeEngine::Ptr      m_Encoder;
        Pump::Ptr                   m_Pump;
        InputQueue                  m_InputQueue;

        int32_t                     m_InputSamplingRate = DEFAULT_SAMPLING_RATE;
        int32_t                     m_OutputSamplingRate = DEFAULT_SAMPLING_RATE;
        int32_t                     m_InputChannels = DEFAULT_CHANNELS;
        int32_t                     m_OutputChannels = DEFAULT_CHANNELS;
        int32_t                     m_InputLayout = DEFAULT_LAYOUT;
        int32_t                     m_OutputLayout = DEFAULT_LAYOUT;
        amf::AMF_AUDIO_FORMAT       m_InputFormat = amf::AMF_AUDIO_FORMAT::AMFAF_UNKNOWN;
        amf::AMF_AUDIO_FORMAT       m_OutputFormat = amf::AMF_AUDIO_FORMAT::AMFAF_UNKNOWN;

        amf::AMFBufferPtr           m_ExtraData;
        transport_common::InitID    m_InitID = 0,
                                    m_LastRetrievedExtraDataID = 0;

        int64_t                     m_SequenceNumber = 0;

        bool                        m_Discontinuity = false;
        amf_pts                     m_ExpectedPts = 0;

    };
}