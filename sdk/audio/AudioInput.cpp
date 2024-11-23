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

#include "AudioInput.h"
#include "AudioCodecs.h"
#include "decoders/AudioDecoderAAC.h"
#include "decoders/AudioDecoderOPUS.h"
#include "amf/public/common/TraceAdapter.h"

#include <sstream>

static constexpr const wchar_t* const AMF_FACILITY = L"ssdk::audio::AudioInput";
static constexpr const int32_t MAX_INPUT_QUEUE_DEPTH = 30;


namespace ssdk::audio
{
    AudioInput::AudioInput(amf::AMFContext* context, Consumer& sink) :
        m_Context(context),
        m_Sink(sink),
        m_Pump(*this)
    {
    }

    AMF_RESULT AudioInput::Init(const char* codecName, amf::AMF_AUDIO_FORMAT format, uint32_t channels, uint32_t layout, uint32_t samplingRate, amf::AMFBuffer* initBlock)
    {
        AMF_RESULT result = AMF_OK;
        amf::AMFLock m_Lock(&m_Guard);
        AMF_RETURN_IF_FALSE(m_Decoder == nullptr, AMF_ALREADY_INITIALIZED, L"Init: Already initialized, call Terminate() first to reinitialize");
        if ((result = CreateDecoder(m_Context, codecName, m_Decoder)) != AMF_OK)
        {
            AMFTraceError(AMF_FACILITY, L"Failed to create the %S video decoder, result=%s", codecName, amf::AMFGetResultText(result));
        }
        else
        {
            result = m_Decoder->Init(format, channels, layout, samplingRate, initBlock);
            if (result != AMF_OK)
            {
                AMFTraceError(AMF_FACILITY, L"Failed to initialize audio decoder: error=%s. codec: %S, format: %dx, channels %d, layout: %d, sampling rate: %d Hz",
                    amf::AMFGetResultText(result), codecName, format, channels, layout, samplingRate);
            }
            else
            {
                m_Format = format;
                m_Channels = channels;
                m_Layout = layout;
                m_SamplingRate = samplingRate;
                m_Pump.Start();
            }
        }
        return result;
    }

    AMF_RESULT AudioInput::CreateDecoder(amf::AMFContext* context, const char* codecName, AudioDecodeEngine::Ptr& decoder)
    {
        AMF_RESULT result = AMF_OK;
        std::string codecNameStr(codecName);
        std::stringstream aacNum, opusNum;
        aacNum << CODEC_AAC_FFMPEG_ID;
        opusNum << CODEC_OPUS_FFMPEG_ID;

        if (codecNameStr == CODEC_AAC || codecNameStr == aacNum.str())
        {
            decoder = AudioDecodeEngine::Ptr(new AudioDecoderAAC(context));
        }
        else if (codecNameStr == CODEC_OPUS || codecNameStr == opusNum.str())
        {
            decoder = AudioDecodeEngine::Ptr(new AudioDecoderOPUS(context));
        }
        else
        {
            result = AMF_NOT_SUPPORTED;
        }
        return result;
    }

    AMF_RESULT AudioInput::Terminate()
    {
        if (m_Pump.IsRunning() == true)
        {
            m_Pump.RequestStop();
            m_Pump.WaitForStop();
        }
        {
            amf::AMFLock m_Lock(&m_Guard);
            if (m_Decoder != nullptr)
            {
                m_Decoder->Terminate();
                m_Decoder = nullptr;
            }
            m_InputQueue.clear();
            m_Format = amf::AMF_AUDIO_FORMAT::AMFAF_UNKNOWN;
            m_Channels = DEFAULT_CHANNELS;
            m_Layout = amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_FRONT_LEFT | amf::AMF_AUDIO_CHANNEL_LAYOUT::AMFACL_SPEAKER_BACK_RIGHT;;
            m_SamplingRate = DEFAULT_SAMPLING_RATE;
            m_InputBufCnt = m_OutputBufCnt = 0;
        }
        return AMF_OK;
    }

    bool AudioInput::IsCodecSupported(const char* codecName) const
    {
        AudioDecodeEngine::Ptr decoder;
        return CreateDecoder(m_Context, codecName, decoder) == AMF_OK;
    }

    amf::AMF_AUDIO_FORMAT AudioInput::GetOutputFormat() const noexcept
    {
        amf::AMFLock m_Lock(&m_Guard);
        return m_Format;
    }

    uint32_t AudioInput::GetChannels() const noexcept
    {
        amf::AMFLock m_Lock(&m_Guard);
        return m_Channels;
    }

    uint32_t AudioInput::GetLayout() const noexcept
    {
        amf::AMFLock m_Lock(&m_Guard);
        return m_Layout;
    }

    uint32_t AudioInput::GetSamplingRate() const noexcept
    {
        amf::AMFLock m_Lock(&m_Guard);
        return m_SamplingRate;
    }

    AMF_RESULT AudioInput::SubmitInput(amf::AMFBuffer* input, bool discontinuity)
    {
        AMF_RESULT result = AMF_OK;

        amf::AMFLock m_Lock(&m_Guard);
        AMF_RETURN_IF_FALSE(m_Decoder != nullptr, AMF_NOT_INITIALIZED, L"Audio decoder has not be instantiated");
        AMF_RETURN_IF_FALSE(input != nullptr, AMF_INVALID_ARG, L"MonoscopicVideoInput::SubmitInput(): Video input cannot be NULL");
        amf::AMFLock m_LockInput(&m_InputGuard);
        if (m_InputQueue.size() < MAX_INPUT_QUEUE_DEPTH)
        {
            if (discontinuity == true)
            {
                input->SetProperty(ssdk::audio::AUDIO_DISCONTINUITY, discontinuity);
            }
            m_InputQueue.push_back(input);
            result = AMF_OK;
        }
        else
        {
            result = AMF_INPUT_FULL;
        }
        return result;
    }

    void AudioInput::PumpBuffers()
    {
        amf::AMFAudioBufferPtr outputBuffer;
        AudioDecodeEngine::Ptr decoder;
        {
            amf::AMFLock m_Lock(&m_Guard);
            decoder = m_Decoder;
        }
        if (decoder != nullptr)
        {
            {
                amf::AMFLock m_LockInput(&m_InputGuard);
                if (m_InputQueue.size() > 0)
                {
                    AMF_RESULT result = decoder->SubmitInput(m_InputQueue.front());
                    if (result == AMF_OK || result == AMF_NEED_MORE_INPUT)
                    {   //  Successfully submitted a frame to the decoder
                        m_InputQueue.pop_front();
                        ++m_InputBufCnt;
                    }
                }
            }
            if (decoder->QueryOutput(&outputBuffer) == AMF_OK && outputBuffer != nullptr)
            {
                amf::AMFLock m_LockOutput(&m_OutputGuard);
                ++m_OutputBufCnt;
            }
            else
            {
                size_t queueDepth;
                {
                    amf::AMFLock m_LockInput(&m_InputGuard);
                    queueDepth = m_InputQueue.size();
                }
                if (queueDepth == 0)
                {   //  No output has been produced yet and the input queue is empty - sleep for 1ms not to burn CPU cycles
                    amf_sleep(1);
                }
            }
        }
        if (outputBuffer != nullptr)
        {
            m_Sink.OnAudioBuffer(outputBuffer);
        }
    }

    void AudioInput::Pump::Run()
    {
        while (StopRequested() == false)
        {
            m_AudioInput.PumpBuffers();
        }
    }

}
