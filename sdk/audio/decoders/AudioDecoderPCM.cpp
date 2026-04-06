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

#include "AudioDecoderPCM.h"
#include "amf/public/common/TraceAdapter.h"

static constexpr const wchar_t* AMF_FACILITY = L"AudioDecoderPCM.cpp";

namespace ssdk::audio
{
    AudioDecoderPCM::AudioDecoderPCM(amf::AMFContext* context) :
        AudioDecodeEngine(context, CODEC_PCM_FFMPEG_ID)
    {
    }

    AMF_RESULT AudioDecoderPCM::Init(amf::AMF_AUDIO_FORMAT format, int32_t channels, int32_t layout, int32_t samplingRate, amf::AMFBuffer* initBlock)
    {
        AMF_RESULT result = AMF_OK;

        amf::AMFLock    lock(&m_Guard);
        if (m_Initialized == true)
        {
            AMFTraceInfo(AMF_FACILITY, L"Audio decoder PCM already initialized");
        }
        else
        {
            result = AudioDecodeEngine::Init(format, channels, layout, samplingRate, initBlock);
            m_Draining = false;
            m_Queue.clear();
            m_Initialized = true;
        }
        return result;
    }

    void AudioDecoderPCM::Terminate()
    {
        amf::AMFLock    lock(&m_Guard);
        m_Draining = false;
        m_Queue.clear();
        m_Initialized = false;
    }

    AMF_RESULT AudioDecoderPCM::SubmitInput(amf::AMFBuffer* input)
    {
        AMF_RESULT result = AMF_OK;
        amf::AMFLock    lock(&m_Guard);
        AMF_RETURN_IF_FALSE(m_Initialized == true, AMF_NOT_INITIALIZED, L"Audio decoder PCM is not initialized");
        if (m_Draining == false)
        {
            m_Queue.push_back(input);
            result = AMF_OK;
        }
        else
        {
            result = AMF_WRONG_STATE;
        }
        return AMF_OK;
    }

    static int GetBytesPerSample(amf::AMF_AUDIO_FORMAT format)
    {
        int size = 0;

        switch (format)
        {
        case amf::AMFAF_U8:
        case amf::AMFAF_U8P:
            size = sizeof(uint8_t);
            break;
        case amf::AMFAF_S16:
        case amf::AMFAF_S16P:
            size = sizeof(int16_t);
            break;
        case amf::AMFAF_S32:
        case amf::AMFAF_S32P:
            size = sizeof(int32_t);
            break;
        case amf::AMFAF_FLT:
        case amf::AMFAF_FLTP:
            size = sizeof(float);
            break;
        case amf::AMFAF_DBL:
        case amf::AMFAF_DBLP:
            size = sizeof(double);
            break;
        default:
            break;
        }
        return size;
    }


    AMF_RESULT AudioDecoderPCM::QueryOutput(amf::AMFAudioBuffer** outputBuffer)
    {
        AMF_RETURN_IF_FALSE(m_Initialized == true, AMF_NOT_INITIALIZED, L"Audio decoder PCM is not initialized");
        AMF_RETURN_IF_FALSE(outputBuffer != nullptr, AMF_INVALID_ARG, L"outputBuffer cannot be NULL");

        AMF_RESULT result = AMF_OK;
        amf::AMFLock    lock(&m_Guard);
        if (m_Queue.size() > 0)
        {
            amf::AMFBufferPtr buf = m_Queue.front();
            int32_t samples = static_cast<int32_t>(buf->GetSize() / (GetChannels() * GetBytesPerSample(GetFormat())));
            amf::AMFAudioBufferPtr audioBuf;
            if ((result = m_Context->AllocAudioBuffer(amf::AMF_MEMORY_HOST, GetFormat(), samples, GetSamplingRate(), GetChannels(), &audioBuf)) == AMF_OK)
            {
                std::copy_n(static_cast<const uint8_t*>(buf->GetNative()), buf->GetSize(), static_cast<uint8_t*>(audioBuf->GetNative()));
                audioBuf->SetPts(buf->GetPts());
                audioBuf->SetDuration(buf->GetDuration());
                m_Queue.pop_front();
                *outputBuffer = audioBuf;
                (*outputBuffer)->Acquire();
            }
        }
        else
        {
            (*outputBuffer) = nullptr;
            if (m_Draining == true)
            {
                m_Draining = false;
                result = AMF_EOF;
            }
            else
            {
                result = AMF_REPEAT;
            }
        }
        return result;
    }

    AMF_RESULT AudioDecoderPCM::Flush()
    {
        amf::AMFLock    lock(&m_Guard);
        m_Queue.clear();
        return AMF_OK;
    }

    AMF_RESULT AudioDecoderPCM::Drain()
    {
        amf::AMFLock    lock(&m_Guard);
        m_Draining = true;
        return AMF_OK;
    }

    const char* AudioDecoderPCM::GetCodecName() const noexcept
    {
        return CODEC_PCM;
    }
}