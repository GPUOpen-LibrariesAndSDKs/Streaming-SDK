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

#if defined(__ANDROID__)

#include "AudioPresenterAndroid.h"

#include "amf/public/common/TraceAdapter.h"

#include <android/log.h>
#include <algorithm>
#include <set>

static constexpr const wchar_t* AMF_FACILITY = L"AudioPresenterAndroid";

namespace ssdk::audio
{
    AudioPresenterAndroid::~AudioPresenterAndroid()
    {
        Terminate();
    }

    AMF_RESULT AudioPresenterAndroid::SetSampleRate(int64_t sampleRate)
    {
        AMF_RESULT result = AMF_NOT_SUPPORTED;
        std::set<int64_t> supportedSamplingRates = {
                8000, 11025, 16000, 22050,
                32000, 44100, 48000, 88200, 96000, 192000
        };
        if (supportedSamplingRates.contains(sampleRate))
        {
            m_Config.sample_rate = static_cast<int>(sampleRate);
            result = AMF_OK;
        }
        return result;
    }

    AMF_RESULT AudioPresenterAndroid::SetFormat(amf::AMF_AUDIO_FORMAT format)
    {
        AMF_RESULT result = AMF_OK;

        amf::AMFLock lock(&m_Guard);
        aaudio_format_t prevFormat = m_Config.sample_format;
        switch (format)
        {
        case amf::AMF_AUDIO_FORMAT::AMFAF_S16:
            m_Config.sample_format = AAUDIO_FORMAT_PCM_I16;
            break;
        case amf::AMF_AUDIO_FORMAT::AMFAF_S32:
            m_Config.sample_format = AAUDIO_FORMAT_PCM_I32;
            break;
        case amf::AMF_AUDIO_FORMAT::AMFAF_FLT:
            m_Config.sample_format = AAUDIO_FORMAT_PCM_FLOAT;
            break;
        default:
            result = AMF_NOT_SUPPORTED;
            m_Config.sample_format = AAUDIO_FORMAT_INVALID;
            AMFTraceError(AMF_FACILITY, L"Unsupported audio format %d", static_cast<int>(format));
            break;
        }
        if (result == AMF_OK && prevFormat != m_Config.sample_format)
        {
            Terminate();
            Init();
        }
        return result;
    }

    AMF_RESULT AudioPresenterAndroid::SetChannels(unsigned int numOfChannels)
    {
        AMF_RESULT result = AMF_OK;
        amf::AMFLock lock(&m_Guard);
        if (numOfChannels > 18) //  Ffmpeg and AAudio layouts match up to the first 18 layouts and then diverge. 18 channels is plenty for a streaming setup
        {
            result = AMF_NOT_SUPPORTED;
        }
        else
        {
            m_Config.channels = numOfChannels;
        }
        return result;
    }

    AMF_RESULT AudioPresenterAndroid::SetLayout(uint32_t layout)
    {
        AMF_RESULT result = AMF_OK;
        amf::AMFLock lock(&m_Guard);
        if ((static_cast<int>(layout) & (~0x2FFFF)) != 0)  //  Ffmpeg and AAudio layouts match up to the first 18 layouts and then diverge. 18 channels is plenty for a streaming setup
        {
            result = AMF_NOT_SUPPORTED;
        }
        else
        {
            m_Config.channel_layout = layout;
        }
        return result;
    }


    AMF_RESULT AudioPresenterAndroid::Init()
    {
        AMF_RESULT amfResult = AMF_OK;
        amf::AMFLock lock(&m_Guard);
        amf::AMFLock lockBuffer(&m_BufferGuard);

        if (m_Initialized) 
        {
            AMFTraceError(AMF_FACILITY, L"Already initialized");
            amfResult = AMF_ALREADY_INITIALIZED;
        }
        else
        {
            // Create AAudio stream builder
            AAudioStreamBuilder *builder = nullptr;
            aaudio_result_t result = AAudio_createStreamBuilder(&builder);
            if (result != AAUDIO_OK)
            {
                AMFTraceError(AMF_FACILITY, L"Failed to create stream builder: %s",
                              AAudio_convertResultToText(result));
                result = AMF_FAIL;
            }
            else
            {
                // Configure stream
                AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
                AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_SHARED);
                AAudioStreamBuilder_setPerformanceMode(builder, m_bLowLatency ? AAUDIO_PERFORMANCE_MODE_LOW_LATENCY : AAUDIO_PERFORMANCE_MODE_NONE);
                AAudioStreamBuilder_setSampleRate(builder, m_Config.sample_rate);
#if __ANDROID_API__ >= 32
                AAudioStreamBuilder_setChannelMask(builder, m_Config.channel_layout);
#else
                AAudioStreamBuilder_setChannelCount(builder, m_Config.channels);
#endif
                AAudioStreamBuilder_setFormat(builder, m_Config.sample_format);

                AAudioStreamBuilder_setDataCallback(builder, DataCallback, this);
                AAudioStreamBuilder_setErrorCallback(builder, ErrorCallback, this);

                // Open stream
                result = AAudioStreamBuilder_openStream(builder, &m_Stream);
                AAudioStreamBuilder_delete(builder);

                if (result != AAUDIO_OK)
                {
                    AMFTraceError(AMF_FACILITY, L"Failed to open stream: %S", AAudio_convertResultToText(result));
                    amfResult = AMF_FAIL;
                }
                else
                {
                    m_BufferCapacity = AAudioStream_getBufferCapacityInFrames(m_Stream);
                    m_CurrentBufferSize = AAudioStream_setBufferSizeInFrames(m_Stream, AAudioStream_getFramesPerBurst(m_Stream)/2);

                    AMFTraceInfo(AMF_FACILITY, L"Audio buffer is set to %d frames, capacity %d frames", m_CurrentBufferSize, m_BufferCapacity);

                    m_UnderrunCount = 0;
                    m_Initialized = true;

                    if ((result = Resume(0)) == AMF_OK)
                    {
                        m_FirstBuffer = true;
                    }
                    else
                    {
                        m_Initialized = false;
                        AMFTraceError(AMF_FACILITY, L"Failed to start Audio Presenter: %s", amf::AMFGetResultText(amfResult));
                    }
                }
            }
        }
        return amfResult;
    }

    AMF_RESULT AudioPresenterAndroid::Terminate()
    {
        AMF_RESULT result = AMF_OK;

        if (m_Initialized == false)
        {
            AMFTraceError(AMF_FACILITY, L"Not initialized");
            result = AMF_NOT_INITIALIZED;
        }
        else
        {
            amf::AMFLock lock(&m_Guard);
            if (m_Stream != nullptr)
            {
                Pause();
                Flush();
                aaudio_stream_state_t currentState = AAudioStream_getState(m_Stream);
                if (currentState != AAUDIO_STREAM_STATE_CLOSED)
                {
                    AAudioStream_close(m_Stream);
                    WaitForStateChange(AAUDIO_STREAM_STATE_CLOSED);
                }
                m_Stream = nullptr;
            }
            m_UnderrunCount = 0;
            m_FirstBuffer = true;
        }
        return result;
    }

    void AudioPresenterAndroid::Prime(int milliseconds)
    {
        m_PrimerBytesRemaining = m_Config.channels * GetBytesPerSample() * m_Config.sample_rate * milliseconds / 1000;
        AMFTraceDebug(AMF_FACILITY, L"Primed for %d ms: sampling rate %d Hz, %d channels, format %d, primer %lu bytes",
                      milliseconds, m_Config.sample_rate, m_Config.channels, m_Config.sample_format, m_PrimerBytesRemaining);

    }

    AMF_RESULT AudioPresenterAndroid::SubmitInput(amf::AMFData* buffer)
    {
        AMF_RESULT result = AMF_OK;
        amf::AMFLock lock(&m_Guard);
        amf::AMFLock lockBuffer(&m_BufferGuard);
        if (m_Initialized != true)
        {
            AMFTraceError(AMF_FACILITY, L"Not initialized");
            result = AMF_NOT_INITIALIZED;
        }
        else if (m_Playing != true)
        {
            result = AMF_WRONG_STATE;
        }
        else
        {
            amf::AMFAudioBufferPtr audioBuffer(buffer);

            if (m_FirstBuffer == true)
            {
                m_FirstBuffer = false;
            }

            amf_pts now = amf_high_precision_clock();
            if (m_PrevBufSubmissionTime != 0 && now - m_PrevBufSubmissionTime > m_PrevBufDuration)
            {
//                AMFTraceWarning(AMF_FACILITY, L"Audio submission is not frequent enough, late by %5.2f ms, duration %5.2f", float(now - m_PrevBufSubmissionTime - m_PrevBufDuration) / AMF_MILLISECOND, float(m_PrevBufDuration) / AMF_MILLISECOND);
            }

            m_InputQueue.push_back({.m_Buf = audioBuffer, .m_Ofs = 0});
            m_BytesInQueue += audioBuffer->GetSize();

            m_PrevBufSubmissionTime = now;
            m_PrevBufDuration = buffer->GetDuration();

            if (m_CurrentBufferSize < m_BufferCapacity)
            {
                int32_t underrunCount = AAudioStream_getXRunCount(m_Stream);
                if (underrunCount > m_UnderrunCount)
                {
                    // Try increasing the buffer size by one burst
                    m_CurrentBufferSize += AAudioStream_getFramesPerBurst(m_Stream);
                    m_CurrentBufferSize = AAudioStream_setBufferSizeInFrames(m_Stream, m_CurrentBufferSize);
                    AMFTraceWarning(AMF_FACILITY, L"Underrun count increased from %d to %d, increased playback buffer to %d frames", m_UnderrunCount, underrunCount, m_CurrentBufferSize);
                    m_UnderrunCount = underrunCount;
                }
            }

        }
        return result;
    }

    void AudioPresenterAndroid::WaitForStateChange(aaudio_stream_state_t targetState)
    {
        aaudio_result_t aaResult = AAUDIO_OK;
        aaudio_stream_state_t currentState = AAudioStream_getState(m_Stream);
        aaudio_stream_state_t inputState = currentState;
        while (aaResult == AAUDIO_OK && currentState != targetState)
        {
            aaResult = AAudioStream_waitForStateChange(m_Stream, inputState, &currentState, 1000000);
            inputState = currentState;
        }
    }

    AMF_RESULT AudioPresenterAndroid::Flush()
    {
        amf::AMFLock lock(&m_BufferGuard);
        m_InputQueue.clear();
        aaudio_stream_state_t currentState = AAudioStream_getState(m_Stream);
        if (currentState == AAUDIO_STREAM_STATE_PAUSED || currentState == AAUDIO_STREAM_STATE_OPEN || currentState == AAUDIO_STREAM_STATE_STOPPED)
        {
            AAudioStream_requestFlush(m_Stream);
            WaitForStateChange(AAUDIO_STREAM_STATE_FLUSHED);
        }
        else
        {
            AMFTraceWarning(AMF_FACILITY, L"AudioPresenterAndroid::Flush is called on a wrong state");
        }
        AMFTraceDebug(AMF_FACILITY, L"Flushed audio buffers");
        return AMF_OK;
    }

    AMF_RESULT AudioPresenterAndroid::Resume(amf_pts currentTime)
    {
        if (m_Initialized != true)
        {
            AMFTraceError(AMF_FACILITY, L"Audio presenter is not initialized");
            return AMF_NOT_INITIALIZED;
        }

        if (m_Playing)
        {
            return AMF_WRONG_STATE;
        }

        aaudio_result_t result = AAudioStream_requestStart(m_Stream);
        if (result != AAUDIO_OK)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to start stream: %S", AAudio_convertResultToText(result));
            return AMF_RESULT ::AMF_FAIL;
        }

        m_Playing = true;
        AMFTraceInfo(AMF_FACILITY, L"Started playback");

        return AMF_RESULT ::AMF_OK;
    }

    AMF_RESULT AudioPresenterAndroid::Pause()
    {
        AMF_RESULT result = AMF_OK;
        if (m_Playing != true)
        {
            result = AMF_WRONG_STATE;
            AMFTraceError(AMF_FACILITY, L"Audio presenter is not playing, cannot pause");
        }
        else
        {
            m_Playing = false;

            if (m_Stream)
            {
                aaudio_result_t aaResult = AAudioStream_requestStop(m_Stream);
                if (aaResult != AAUDIO_OK)
                {
                    AMFTraceError(AMF_FACILITY, L"Failed to stop audio stream: %S", AAudio_convertResultToText(aaResult));
                    result = AMF_FAIL;
                }
                else
                {
                    WaitForStateChange(AAUDIO_STREAM_STATE_STOPPED);
                    AMFTraceInfo(AMF_FACILITY, L"Stopped audio playback");
                }
            }
        }
        return result;
    }

    bool AudioPresenterAndroid::IsPlaying() const
    {
        return m_Playing;
    }

    void AudioPresenterAndroid::PlayRequestedSamples(uint8_t* buffer, int32_t num_frames)
    {
        amf::AMFLock lock(&m_BufferGuard);

        size_t bytesPerFrame = m_Config.channels * GetBytesPerSample();
        size_t totalBytesRequested = num_frames * bytesPerFrame;
        size_t bytesRemaining = totalBytesRequested;
        size_t outOfs = 0;

        // Step 1: Play primer (silence) if any remaining
        if (m_PrimerBytesRemaining > 0)
        {
            size_t bytesOfSilenceToPlay = std::min(m_PrimerBytesRemaining, bytesRemaining);
            std::fill_n(buffer + outOfs, bytesOfSilenceToPlay, 0);

            m_PrimerBytesRemaining -= bytesOfSilenceToPlay;
            bytesRemaining -= bytesOfSilenceToPlay;
            outOfs += bytesOfSilenceToPlay;

            AMFTraceInfo(AMF_FACILITY, L"Playing primer: %zu bytes", bytesOfSilenceToPlay);
        }

        // Step 2: Play audio from queue if primer is done and we need more data
        if (m_PrimerBytesRemaining == 0 && bytesRemaining > 0)
        {
            if (m_BytesInQueue > 0)
            {
                size_t bytesToPlayFromQueue = std::min(bytesRemaining, m_BytesInQueue);
                size_t bytesLeftToPlay = bytesToPlayFromQueue;

                while (bytesLeftToPlay > 0 && !m_InputQueue.empty())
                {
                    BufferDescriptor& curDesc = m_InputQueue.front();

                    const uint8_t* src = static_cast<const uint8_t*>(curDesc.m_Buf->GetNative()) +
                                         curDesc.m_Ofs;
                    size_t bytesLeftInCurBuffer = curDesc.m_Buf->GetSize() - curDesc.m_Ofs;
                    size_t bytesToCopy = std::min(bytesLeftInCurBuffer, bytesLeftToPlay);

                    std::copy(src, src + bytesToCopy, buffer + outOfs);

                    curDesc.m_Ofs += bytesToCopy;
                    outOfs += bytesToCopy;
                    bytesLeftToPlay -= bytesToCopy;
                    m_BytesInQueue -= bytesToCopy;

                    // Remove buffer if fully consumed
                    if (curDesc.m_Ofs >= curDesc.m_Buf->GetSize())
                    {
                        m_InputQueue.pop_front();
                    }
                }

                bytesRemaining -= bytesToPlayFromQueue;
            }

            // Step 3: Fill any remaining space with silence (underrun)
            if (bytesRemaining > 0)
            {
                std::fill_n(buffer + outOfs, bytesRemaining, 0);
//                AMFTraceWarning(AMF_FACILITY, L"UNDERRUN: Not enough data. Available: %zu, Needed: %zu, Queue size: %zu", m_BytesInQueue, bytesRemaining, m_InputQueue.size());
//                Prime(25);
            }
        }
    }


    aaudio_data_callback_result_t AudioPresenterAndroid::DataCallback(AAudioStream* /*stream*/, void* user_data, void* audio_data, int32_t num_frames)
    {
        static_cast<AudioPresenterAndroid*>(user_data)->PlayRequestedSamples(static_cast<uint8_t*>(audio_data), num_frames);
        return AAUDIO_CALLBACK_RESULT_CONTINUE;
    }

    void AudioPresenterAndroid::ErrorCallback(AAudioStream* /*stream*/, void* user_data, aaudio_result_t error)
    {

        AMFTraceError(AMF_FACILITY, L"AAudio error: %S", AAudio_convertResultToText(error));

        // Handle stream disconnection
        if (error == AAUDIO_ERROR_DISCONNECTED)
        {
            AudioPresenterAndroid* presenter = static_cast<AudioPresenterAndroid*>(user_data);
            presenter->m_Playing = false;
        }
    }

    int AudioPresenterAndroid::GetBytesPerSample() const
    {
        switch (m_Config.sample_format)
        {
        case AAUDIO_FORMAT_PCM_I16:
            return sizeof(int16_t);
        case AAUDIO_FORMAT_PCM_I24_PACKED:
            return sizeof(int16_t) + sizeof(int8_t);
        case AAUDIO_FORMAT_PCM_I32:
            return sizeof(int32_t);
        case AAUDIO_FORMAT_PCM_FLOAT:
            return sizeof(float_t);
        default:
            return 0;
        }
    }

    AMF_RESULT AudioPresenterAndroid::GetDescription(amf_int64& streamBitRate, amf_int64& streamSampleRate,
                                          amf_int64& streamChannels, amf_int64& streamFormat,
                                          amf_int64& streamLayout, amf_int64& streamBlockAlign) const
    {
        streamBitRate = m_Config.channels * m_Config.sample_rate * GetBytesPerSample() * 8;
        streamLayout = m_Config.channel_layout;
        streamSampleRate = m_Config.sample_rate;
        streamChannels = m_Config.channels;
        streamFormat = m_Config.sample_format;
        streamBlockAlign = GetBytesPerSample();
        return AMF_OK;
    }

}
#endif