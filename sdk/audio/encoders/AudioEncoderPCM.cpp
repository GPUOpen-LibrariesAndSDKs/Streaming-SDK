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

#include "AudioEncoderPCM.h"
#include "amf/public/common/TraceAdapter.h"

static constexpr const wchar_t* AMF_FACILITY = L"AudioEncoderPCM.cpp";

namespace ssdk::audio
{
    AudioEncoderPCM::AudioEncoderPCM(amf::AMFContext* context) :
        AMFAudioEncoder(context, CODEC_OPUS_FFMPEG_ID)
    {
    }

    const char* AudioEncoderPCM::GetCodecName() const noexcept
    {
        return CODEC_OPUS;
    }

    const char* AudioEncoderPCM::GetCodecID() const noexcept
    {
        return CODEC_ID_PCM;
    }

    AMF_RESULT AudioEncoderPCM::Init(int32_t samplingRate, int32_t channels, int32_t layout, int32_t bitrate)
    {
        amf::AMFLock    lock(&m_Guard);
        AMF_RETURN_IF_FALSE(m_Initialized != true, AMF_ALREADY_INITIALIZED, L"Audio encoder PCM already initialized");
        AMF_RESULT result = AudioEncodeEngine::Init(channels, layout, samplingRate, bitrate);
        m_Draining = false;
        m_Queue.clear();
        m_Initialized = true;
        return result;
    }

    void AudioEncoderPCM::Terminate()
    {
        amf::AMFLock    lock(&m_Guard);
        m_Initialized = false;
    }

    AMF_RESULT AudioEncoderPCM::SubmitInput(amf::AMFAudioBuffer* input)
    {
        AMF_RESULT result = AMF_OK;
        amf::AMFLock    lock(&m_Guard);
        AMF_RETURN_IF_FALSE(m_Initialized == true, AMF_NOT_INITIALIZED, L"Encoder is not initialized");
        if (m_Draining == false)
        {
            m_Queue.push_back(input);
            result = AMF_OK;
        }
        else
        {
            result = AMF_WRONG_STATE;
        }
        return result;
    }

    AMF_RESULT AudioEncoderPCM::QueryOutput(amf::AMFBuffer** outputBuffer)
    {
        AMF_RETURN_IF_FALSE(m_Initialized == true, AMF_NOT_INITIALIZED, L"Encoder is not initialized");
        AMF_RETURN_IF_FALSE(outputBuffer != nullptr, AMF_INVALID_ARG, L"outputBuffer cannot be NULL");

        AMF_RESULT result = AMF_OK;
        amf::AMFLock    lock(&m_Guard);
        if (m_Queue.size() > 0)
        {
            amf::AMFAudioBufferPtr pData = m_Queue.front();
            m_Queue.pop_front();
            amf::AMFBufferPtr buf;
            m_Context->CreateBufferFromHostNative(pData->GetNative(), pData->GetSize(), &buf, nullptr);
            buf->SetPts(pData->GetPts());
            buf->SetDuration(pData->GetDuration());
            *outputBuffer = buf;
            (*outputBuffer)->Acquire();
            result = AMF_OK;
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

    AMF_RESULT AudioEncoderPCM::Flush()
    {
        amf::AMFLock    lock(&m_Guard);
        m_Queue.clear();
        return AMF_OK;
    }

    AMF_RESULT AudioEncoderPCM::Drain()
    {
        amf::AMFLock    lock(&m_Guard);
        m_Draining = true;
        return AMF_OK;
    }
}
