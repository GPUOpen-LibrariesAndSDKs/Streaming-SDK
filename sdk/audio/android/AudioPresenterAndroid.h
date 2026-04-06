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

Copyright (c) Advanced Micro Devices, Inc. All rights reserved.

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
#if defined(__ANDROID__)

#include "amf/public/samples/CPPSamples/common/AudioPresenter.h"
#include "amf/public/include/core/AudioBuffer.h"
#include "amf/public/common/Thread.h"

#include <aaudio/AAudio.h>
#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstdint>
#include <memory>

namespace ssdk::audio
{
    class AudioPresenterAndroid :
            public AudioPresenter
    {
    public:
        typedef std::shared_ptr<AudioPresenterAndroid>  Ptr;

    public:
        AudioPresenterAndroid() = default;
        virtual ~AudioPresenterAndroid();

        // Initialize with specific audio configuration
        virtual AMF_RESULT Init() override;
        AMF_RESULT Terminate();

        virtual AMF_RESULT GetDescription(amf_int64& streamBitRate, amf_int64& streamSampleRate, amf_int64& streamChannels,
                                          amf_int64& streamFormat, amf_int64& streamLayout, amf_int64& streamBlockAlign) const override;

        AMF_RESULT SetSampleRate(int64_t sampleRate);
        AMF_RESULT SetFormat(amf::AMF_AUDIO_FORMAT format);
        AMF_RESULT SetChannels(unsigned int numOfChannels);
        AMF_RESULT SetLayout(uint32_t layout);

        // Submit audio data for playback
        virtual AMF_RESULT SubmitInput(amf::AMFData* buffer) override;

        // Flush pending audio buffers
        AMF_RESULT Flush() override;

        // Control playback
        virtual AMF_RESULT Resume(amf_pts currentTime) override;
        virtual AMF_RESULT Pause() override;
        bool IsPlaying() const;

    protected:
        // AAudio callback
        static aaudio_data_callback_result_t DataCallback(AAudioStream* stream, void* user_data, void* audio_data, int32_t num_frames);
        static void ErrorCallback(AAudioStream* stream, void* user_data, aaudio_result_t error);

        // Helper methods
        int GetBytesPerSample() const;

        void Prime(int milliseconds);

        void PlayRequestedSamples(uint8_t* buf, int32_t num_frames);

        void WaitForStateChange(aaudio_stream_state_t targetState);

    private:
        // AAudio stream
        AAudioStream* m_Stream = nullptr;

        struct AudioConfig
        {
            aaudio_format_t sample_format = AAUDIO_FORMAT_PCM_I16;
            int sample_rate = 44100;
            int channels = 2;
            aaudio_channel_mask_t channel_layout = AAUDIO_CHANNEL_STEREO;
        };

        struct BufferDescriptor
        {
            typedef std::deque<BufferDescriptor>    Queue;

            amf::AMFAudioBufferPtr  m_Buf;
            size_t                  m_Ofs = 0;
        };

        mutable amf::AMFCriticalSection m_Guard;
        mutable amf::AMFCriticalSection m_BufferGuard;

        BufferDescriptor::Queue m_InputQueue;
        size_t      m_BytesInQueue = 0;

        // Audio configuration
        AudioConfig m_Config;
        std::atomic<bool> m_Initialized = false;
        std::atomic<bool> m_Playing = false;

        // Primer
        size_t      m_PrimerBytesRemaining = 0;

        amf_pts     m_PrevBufSubmissionTime = 0;
        amf_pts     m_PrevBufDuration = 0;

        bool        m_FirstBuffer = true;

        int32_t     m_BufferCapacity = 0;
        int32_t     m_CurrentBufferSize = 0;
        int32_t     m_UnderrunCount = 0;
    };

}

#endif